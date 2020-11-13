// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is a part of scnlib:
//     https://github.com/eliaskosunen/scnlib

#ifndef SCN_DETAIL_FILE_H
#define SCN_DETAIL_FILE_H

#include <cstdio>
#include <string>

#include "range.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        struct native_file_handle {
#if SCN_WINDOWS
            using handle_type = void*;
#else
            using handle_type = int;
#endif

            static native_file_handle invalid();

            handle_type handle;
        };

        class byte_mapped_file {
        public:
            using iterator = const char*;
            using sentinel = const char*;

            byte_mapped_file() = default;
            explicit byte_mapped_file(const char* filename);

            byte_mapped_file(const byte_mapped_file&) = delete;
            byte_mapped_file& operator=(const byte_mapped_file&) = delete;

            byte_mapped_file(byte_mapped_file&& o) noexcept
                : m_map(exchange(o.m_map, span<char>{})),
                  m_file(exchange(o.m_file, native_file_handle::invalid()))
            {
#if SCN_WINDOWS
                m_map_handle =
                    exchange(o.m_map_handle, native_file_handle::invalid());
#endif
                SCN_ENSURE(!o.valid());
                SCN_ENSURE(valid());
            }
            byte_mapped_file& operator=(byte_mapped_file&& o) noexcept
            {
                if (valid()) {
                    _destruct();
                }

                m_map = exchange(o.m_map, span<char>{});
                m_file = exchange(o.m_file, native_file_handle::invalid());
#if SCN_WINDOWS
                m_map_handle =
                    exchange(o.m_map_handle, native_file_handle::invalid());
#endif

                SCN_ENSURE(!o.valid());
                SCN_ENSURE(valid());
                return *this;
            }

            ~byte_mapped_file()
            {
                if (valid()) {
                    _destruct();
                }
            }

            SCN_NODISCARD bool valid() const
            {
                return m_file.handle != native_file_handle::invalid().handle;
            }

            SCN_NODISCARD iterator begin() const
            {
                return m_map.begin();
            }
            SCN_NODISCARD sentinel end() const
            {
                return m_map.end();
            }

        protected:
            void _destruct();

            span<char> m_map{};
            native_file_handle m_file{native_file_handle::invalid().handle};
#if SCN_WINDOWS
            native_file_handle m_map_handle{
                native_file_handle::invalid().handle};
#endif
        };
    }  // namespace detail

    /**
     * Memory-mapped file range.
     * Manages the lifetime of the mapping itself.
     */
    template <typename CharT>
    class basic_mapped_file : public detail::byte_mapped_file {
    public:
        using iterator = const CharT*;
        using sentinel = const CharT*;

        /// Constructs an empty mapping
        basic_mapped_file() = default;

        /// Constructs a mapping to a filename
        explicit basic_mapped_file(const char* f) : detail::byte_mapped_file{f}
        {
        }

        SCN_NODISCARD iterator begin() const noexcept
        {
            // embrace the UB
            return reinterpret_cast<iterator>(byte_mapped_file::begin());
        }
        SCN_NODISCARD sentinel end() const noexcept
        {
            return reinterpret_cast<sentinel>(byte_mapped_file::end());
        }

        SCN_NODISCARD iterator data() const noexcept
        {
            return begin();
        }
        SCN_NODISCARD size_t size() const noexcept
        {
            return m_map.size() / sizeof(CharT);
        }

        /// Mapping data
        span<const CharT> buffer() const
        {
            return {data(), size()};
        }

        detail::range_wrapper<basic_string_view<CharT>> wrap() const noexcept
        {
            return basic_string_view<CharT>{data(), size()};
        }
    };

    using mapped_file = basic_mapped_file<char>;
    using mapped_wfile = basic_mapped_file<wchar_t>;

    /**
     * Range mapping to a C FILE*.
     * Not copyable or reconstructible.
     */
    template <typename CharT>
    class basic_file {
    public:
        class iterator {
        public:
            using char_type = CharT;
            using value_type = expected<CharT>;
            using reference = value_type;
            using pointer = value_type*;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::bidirectional_iterator_tag;
            using file_type = basic_file<CharT>;

            iterator() = default;

            expected<CharT> operator*() const
            {
                SCN_EXPECT(m_file);

                if (m_file->m_buffer.empty()) {
                    // no chars have been read
                    return m_file->_read_single();
                }
                if (!m_last_error) {
                    // last read failed
                    return m_last_error;
                }
                return m_file->_get_char_at(m_current);
            }

            iterator& operator++()
            {
                SCN_EXPECT(m_file);
                ++m_current;
                return *this;
            }
            iterator operator++(int)
            {
                iterator tmp(*this);
                operator++();
                return tmp;
            }

            iterator& operator--()
            {
                SCN_EXPECT(m_file);
                SCN_EXPECT(m_current > 0);

                m_last_error = error{};
                --m_current;

                return *this;
            }
            iterator operator--(int)
            {
                iterator tmp(*this);
                operator--();
                return tmp;
            }

            bool operator==(const iterator& o) const
            {
                if (m_file && (m_file == o.m_file || !o.m_file)) {
                    if (m_file->_is_at_end(m_current) &&
                        m_last_error.code() != error::end_of_range) {
                        m_last_error = error{};
                        auto r = m_file->_read_single();
                        if (!r) {
                            m_last_error = r.error();
                            return !o.m_file || m_current == o.m_current ||
                                   o.m_last_error.code() == error::end_of_range;
                        }
                    }
                }

                // null file == null file
                if (!m_file && !o.m_file) {
                    return true;
                }
                // null file == eof file
                if (!m_file && o.m_file) {
                    // lhs null, rhs potentially eof
                    return o.m_last_error.code() == error::end_of_range;
                }
                // eof file == null file
                if (m_file && !o.m_file) {
                    // rhs null, lhs potentially eof
                    return m_last_error.code() == error::end_of_range;
                }
                // eof file == eof file
                if (m_last_error == o.m_last_error &&
                    m_last_error.code() == error::end_of_range) {
                    return true;
                }

                return m_file == o.m_file && m_current == o.m_current;
            }
            bool operator!=(const iterator& o) const
            {
                return !operator==(o);
            }

            bool operator<(const iterator& o) const
            {
                // any valid iterator is before eof and null
                if (!m_file) {
                    return !o.m_file;
                }
                if (!o.m_file) {
                    return !m_file;
                }
                SCN_EXPECT(m_file == o.m_file);
                return m_current < o.m_current;
            }
            bool operator>(const iterator& o) const
            {
                return o.operator<(*this);
            }
            bool operator<=(const iterator& o) const
            {
                return !operator>(o);
            }
            bool operator>=(const iterator& o) const
            {
                return !operator<(o);
            }

        private:
            friend class basic_file;

            iterator(const file_type& file, size_t i)
                : m_file{std::addressof(file)}, m_current{i}
            {
            }

            mutable error m_last_error{};
            const file_type* m_file{nullptr};
            mutable size_t m_current{0};
        };

        using sentinel = iterator;
        using char_type = CharT;

        /**
         * Construct an empty file.
         * Reading not possible: valid() is `false`
         */
        basic_file() = default;
        /**
         * Construct from a FILE*.
         * Must be a valid handle that can be read from.
         */
        basic_file(FILE* f) : m_file{f} {}

        basic_file(const basic_file&) = delete;
        basic_file& operator=(const basic_file&) = delete;

        basic_file(basic_file&& o)
            : m_buffer(detail::exchange(o.m_buffer, {})),
              m_file(detail::exchange(o.m_file, nullptr))
        {
        }
        basic_file& operator=(basic_file&& o)
        {
            if (valid()) {
                sync();
            }
            m_buffer = detail::exchange(o.m_buffer, {});
            m_file = detail::exchange(o.m_file, nullptr);
            return *this;
        }

        ~basic_file()
        {
            if (valid()) {
                _sync_all();
            }
        }

        /**
         * Get the FILE* for this range.
         * Only use this handle for reading sync() has been called and no
         * reading operations have taken place after that.
         *
         * \see sync
         */
        FILE* handle() const
        {
            return m_file;
        }

        /**
         * Reset the file handle.
         * Calls sync(), if necessary, before resetting.
         * @return The old handle
         */
        FILE* set_handle(FILE* f)
        {
            auto old = m_file;
            if (old) {
                sync();
            }
            m_file = f;
            return old;
        }

        /// Whether the file has been opened
        bool valid() const
        {
            return m_file != nullptr;
        }

        /**
         * Synchronizes this file with the underlying FILE*.
         * Invalidates all non-end iterators.
         * File must be open.
         *
         * Necessary for mixing-and-matching scnlib and <cstdio>:
         * \code{.cpp}
         * scn::scan(file, ...);
         * file.sync();
         * std::fscanf(file.handle(), ...);
         * \endcode
         *
         * Necessary for synchronizing result objects:
         * \code{.cpp}
         * auto result = scn::scan(file, ...);
         * // only result.range() can now be used for scanning
         * result = scn::scan(result.range(), ...);
         * // .sync() allows the original file to also be used
         * file.sync();
         * result = scn::scan(file, ...);
         * \endcode
         */
        void sync()
        {
            _sync_all();
            m_buffer.clear();
        }

        iterator begin() const noexcept
        {
            return {*this, 0};
        }
        sentinel end() const noexcept
        {
            return {};
        }

    private:
        friend class iterator;

        expected<CharT> _read_single() const;

        void _sync_all()
        {
            _sync_until(m_buffer.size());
        }
        void _sync_until(size_t pos);

        CharT _get_char_at(size_t i) const
        {
            SCN_EXPECT(valid());
            SCN_EXPECT(i < m_buffer.size());
            return m_buffer[i];
        }

        bool _is_at_end(size_t i) const
        {
            SCN_EXPECT(valid());
            return i >= m_buffer.size();
        }

        mutable std::basic_string<CharT> m_buffer{};
        FILE* m_file{nullptr};
    };

    using file = basic_file<char>;
    using wfile = basic_file<wchar_t>;

    template <>
    inline expected<char> file::_read_single() const
    {
        SCN_EXPECT(valid());
        int tmp = std::fgetc(m_file);
        if (tmp == EOF) {
            if (std::feof(m_file) != 0) {
                return error(error::end_of_range, "EOF");
            }
            if (std::ferror(m_file) != 0) {
                return error(error::source_error, "fgetc error");
            }
            return error(error::unrecoverable_source_error,
                         "Unknown fgetc error");
        }
        auto ch = static_cast<char>(tmp);
        m_buffer.push_back(ch);
        return ch;
    }
    template <>
    inline expected<wchar_t> wfile::_read_single() const
    {
        SCN_EXPECT(valid());
        wint_t tmp = std::fgetwc(m_file);
        if (tmp == WEOF) {
            if (std::feof(m_file) != 0) {
                return error(error::end_of_range, "EOF");
            }
            if (std::ferror(m_file) != 0) {
                return error(error::source_error, "fgetc error");
            }
            return error(error::unrecoverable_source_error,
                         "Unknown fgetc error");
        }
        auto ch = static_cast<wchar_t>(tmp);
        m_buffer.push_back(ch);
        return ch;
    }

    template <>
    inline void file::_sync_until(std::size_t pos)
    {
        for (auto it = m_buffer.rbegin();
             it != m_buffer.rend() - static_cast<std::ptrdiff_t>(pos); ++it) {
            std::ungetc(static_cast<unsigned char>(*it), m_file);
        }
    }
    template <>
    inline void wfile::_sync_until(std::size_t pos)
    {
        for (auto it = m_buffer.rbegin();
             it != m_buffer.rend() - static_cast<std::ptrdiff_t>(pos); ++it) {
            std::ungetwc(static_cast<wint_t>(*it), m_file);
        }
    }

    /**
     * A child class for basic_file, handling fopen, fclose, and lifetimes with
     * RAII.
     */
    template <typename CharT>
    class basic_owning_file : public basic_file<CharT> {
    public:
        using char_type = CharT;

        /// Open an empty file
        basic_owning_file() = default;
        /// Open a file, with fopen arguments
        basic_owning_file(const char* f, const char* mode)
            : basic_file<CharT>(std::fopen(f, mode))
        {
        }

        /// Steal ownership of a FILE*
        explicit basic_owning_file(FILE* f) : basic_file<CharT>(f) {}

        ~basic_owning_file()
        {
            if (is_open()) {
                close();
            }
        }

        /// fopen
        bool open(const char* f, const char* mode)
        {
            SCN_EXPECT(!is_open());
            auto h = std::fopen(f, mode);
            if (h) {
                this->set_handle(h);
                return true;
            }
            else {
                return false;
            }
        }
        /// Steal ownership
        bool open(FILE* f)
        {
            SCN_EXPECT(!is_open());
            this->set_handle(f);
            return true;
        }

        /// Close file
        void close()
        {
            SCN_EXPECT(is_open());
            std::fclose(this->handle());
        }

        /// Is the file open
        SCN_NODISCARD bool is_open() const
        {
            return this->valid();
        }
    };

    using owning_file = basic_owning_file<char>;
    using owning_wfile = basic_owning_file<wchar_t>;

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wexit-time-destructors")

    // Avoid documentation issues: without this, Doxygen will think
    // SCN_CLANG_PUSH is a part of the stdin_range declaration
    namespace dummy {
    }

    /**
     * Get a reference to the global stdin range
     */
    template <typename CharT>
    basic_file<CharT>& stdin_range()
    {
        static auto f = basic_file<CharT>{stdin};
        return f;
    }
    /**
     * Get a reference to the global `char`-oriented stdin range
     */
    inline file& cstdin()
    {
        return stdin_range<char>();
    }
    /**
     * Get a reference to the global `wchar_t`-oriented stdin range
     */
    inline wfile& wcstdin()
    {
        return stdin_range<wchar_t>();
    }
    SCN_CLANG_POP

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_FILE_CPP)
#include "file.cpp"
#endif

#endif  // SCN_DETAIL_FILE_H
