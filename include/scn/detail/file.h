// Copyright 2017-2019 Elias Kosunen
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

            static constexpr native_file_handle invalid()
            {
#if SCN_WINDOWS
                return {nullptr};
#else
                return {-1};
#endif
            }

            handle_type handle;
        };

        class byte_mapped_file {
        public:
            using iterator = const char*;
            using sentinel = const char*;

            byte_mapped_file() = default;
            byte_mapped_file(const char* filename);

            byte_mapped_file(const byte_mapped_file&) = delete;
            byte_mapped_file& operator=(const byte_mapped_file&) = delete;

            byte_mapped_file(byte_mapped_file&& o) noexcept
                : m_file(o.m_file), m_map(o.m_map)
            {
                o.m_file = native_file_handle::invalid();
                o.m_map = span<char>{};

                SCN_ENSURE(!o.valid());
                SCN_ENSURE(valid());
            }
            byte_mapped_file& operator=(byte_mapped_file&& o) noexcept
            {
                if (valid()) {
                    _destruct();
                }
                m_file = o.m_file;
                m_map = o.m_map;

                o.m_file = native_file_handle::invalid();
                o.m_map = span<char>{};

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

        private:
            void _destruct();

            native_file_handle m_file{native_file_handle::invalid().handle};
            span<char> m_map{};
        };
    }  // namespace detail

    template <typename CharT>
    class basic_mapped_file : public detail::byte_mapped_file {
    public:
        using iterator = const CharT*;
        using sentinel = const CharT*;

        using byte_mapped_file::byte_mapped_file;

        // embrace the UB
        SCN_NODISCARD iterator begin() const
        {
            return reinterpret_cast<iterator>(byte_mapped_file::begin());
        }
        SCN_NODISCARD sentinel end() const
        {
            return reinterpret_cast<sentinel>(byte_mapped_file::end());
        }
    };

    using mapped_file = basic_mapped_file<char>;
    using wmapped_file = basic_mapped_file<wchar_t>;

    template <typename CharT>
    class basic_file_view;

    template <typename CharT>
    class basic_file {
    public:
        using char_type = CharT;
        using view_type = basic_file_view<CharT>;

        basic_file() = default;
        explicit basic_file(FILE* f) : m_file(f) {}

        basic_file(const basic_file&) = delete;
        basic_file& operator=(const basic_file&) = delete;

        basic_file(basic_file&& f) noexcept
            : m_file(detail::exchange(f.m_file, nullptr)),
              m_lock_counter(detail::exchange(f.m_lock_counter, size_t{0})),
              m_buffer(detail::exchange(f.m_buffer, {}))
        {
            SCN_EXPECT(!f.is_locked());
        }
        basic_file& operator=(basic_file&& f) noexcept
        {
            SCN_EXPECT(!is_locked());
            SCN_EXPECT(!f.is_locked());
            m_file = detail::exchange(f.m_file, nullptr);
            m_buffer = detail::exchange(f.m_buffer, {});
            // m_lock_counter and f.m_lock_counter guaranteed to be 0
            return *this;
        }

        ~basic_file()
        {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            _sync(m_buffer.size());
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }

        FILE* handle() const
        {
            SCN_EXPECT(!is_locked());
            return m_file;
        }

        bool valid() const
        {
            return m_file;
        }

        FILE* set_handle(FILE* n)
        {
            SCN_EXPECT(!is_locked());
            _sync(m_buffer.size());
            m_buffer.clear();
            return detail::exchange(m_file, n);
        }

        view_type lock();

        bool is_locked() const
        {
            return m_lock_counter > 0;
        }

    private:
        friend class basic_file_view<CharT>;

        void _release_lock(size_t pos)
        {
            SCN_EXPECT(is_locked());
            --m_lock_counter;
            if (!is_locked()) {
                _sync(pos);
                m_buffer.clear();
            }
        }

        bool _is_end(size_t n) const
        {
            SCN_EXPECT(valid());
            return m_buffer.size() == n;
        }

        bool _should_read(size_t n) const
        {
            return _is_end(n);
        }
        CharT _get_char_at(size_t n) const
        {
            SCN_EXPECT(valid());
            SCN_EXPECT(n < m_buffer.size());
            return m_buffer[n];
        }

        expected<CharT> _read() const;

        void _sync(size_t pos);

        FILE* m_file{nullptr};
        size_t m_lock_counter{0};
        mutable std::basic_string<CharT> m_buffer{};
    };

    using file = basic_file<char>;
    using wfile = basic_file<wchar_t>;

    template <>
    inline expected<char> file::_read() const
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
    inline expected<wchar_t> wfile::_read() const
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

    template <typename CharT>
    class basic_owning_file : public basic_file<CharT> {
    public:
        using char_type = CharT;

        basic_owning_file() = default;
        basic_owning_file(const char* f, const char* mode)
            : basic_file<CharT>(std::fopen(f, mode))
        {
        }

        ~basic_owning_file()
        {
            if (is_open()) {
                close();
            }
        }

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
        void close()
        {
            SCN_EXPECT(is_open());
            std::fclose(this->handle());
        }

        SCN_NODISCARD bool is_open() const
        {
            return this->valid();
        }
    };

    using owning_file = basic_owning_file<char>;
    using wowning_file = basic_owning_file<wchar_t>;

    template <typename CharT>
    class basic_file_view {
    public:
        class iterator {
        public:
            using value_type = expected<CharT>;
            using reference = value_type;
            using pointer = value_type*;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::bidirectional_iterator_tag;

            iterator() = default;

            expected<CharT> operator*() const
            {
                SCN_EXPECT(m_file);
                if (m_file->_should_read(m_current)) {
                    auto r = m_file->_read();
                    if (!r) {
                        if (r.error().code() == error::end_of_range &&
                            !m_file->_is_end(m_current)) {
                            ++m_current;
                            SCN_ENSURE(m_file->_is_end(m_current));
                        }
                    }
                    return r;
                }
                return m_file->_get_char_at(m_current);
            }

            iterator& operator++()
            {
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
                SCN_EXPECT(m_current > 0);
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
                if (!m_file && !o.m_file) {
                    return true;
                }
                if (!m_file && o.m_file) {
                    // lhs null, rhs potentially eof
                    return o.m_file->_is_end(o.m_current);
                }
                if (!m_file && o.m_file) {
                    // rhs null, lhs potentially eof
                    return m_file->_is_end(m_current);
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
            friend class basic_file_view;

            // eww
            iterator(const basic_file_view* v, size_t c)
                : m_file(const_cast<basic_file_view*>(v)->m_file), m_current(c)
            {
            }

            basic_file<CharT>* m_file{};
            mutable size_t m_current{};  // so yucky
        };

        basic_file_view() = default;

        basic_file_view(iterator b, iterator)
        {
            if (b.m_file) {
                m_file = b.m_file;
                m_begin = b.m_current;
                ++m_file->m_lock_counter;
            }
        }

        basic_file_view(const basic_file_view& o)
            : m_file(o.m_file), m_begin(o.m_begin)
        {
            if (m_file) {
                ++m_file->m_lock_counter;
            }
        }
        basic_file_view& operator=(const basic_file_view& o)
        {
            if (m_file) {
                m_file->_release_lock(m_begin);
            }
            m_file = o.m_file;
            m_begin = o.m_begin;
            ++m_file->m_lock_counter;
        }

        basic_file_view(basic_file_view&& o) noexcept
            : m_file(o.m_file), m_begin(o.m_begin)
        {
            o.m_file = nullptr;
        }
        basic_file_view& operator=(basic_file_view&& o)
        {
            if (m_file) {
                m_file->_release_lock(m_begin);
            }
            m_file = o.m_file;
            m_begin = o.m_begin;
            o.m_file = nullptr;
            return *this;
        }

        ~basic_file_view()
        {
            if (m_file) {
                m_file->_release_lock(m_begin);
            }
        }

        SCN_NODISCARD bool is_valid() const
        {
            return m_file;
        }

        void release()
        {
            SCN_EXPECT(m_file);
            m_file->_release_lock(m_begin);
            m_file = nullptr;
        }

        iterator begin() noexcept
        {
            return {this, m_begin};
        }
        iterator end() noexcept
        {
            return {};
        }

        iterator begin() const noexcept
        {
            return {this, m_begin};
        }
        iterator end() const noexcept
        {
            return {};
        }

    private:
        friend class basic_file<CharT>;
        friend class iterator;

        explicit basic_file_view(basic_file<CharT>& f)
            : m_file(std::addressof(f))
        {
        }

        basic_file<CharT>* m_file{nullptr};
        size_t m_begin{0};
    };

    using file_view = basic_file_view<char>;
    using wfile_view = basic_file_view<wchar_t>;

    template <typename CharT>
    auto basic_file<CharT>::lock() -> view_type
    {
        SCN_EXPECT(!is_locked());
        m_lock_counter = 1;
        return view_type{*this};
    }

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wexit-time-destructors")
    template <typename CharT>
    basic_file<CharT>& stdin_range()
    {
        static auto f = basic_file<CharT>{stdin};
        return f;
    }
    inline file& cstdin()
    {
        return stdin_range<char>();
    }
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
