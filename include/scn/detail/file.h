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
            explicit byte_mapped_file(const char* filename);

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

        protected:
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
        SCN_NODISCARD iterator begin() const noexcept
        {
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

        detail::range_wrapper<basic_string_view<CharT>> wrap() const
        {
            return basic_string_view<CharT>{data(), size()};
        }
    };

    using mapped_file = basic_mapped_file<char>;
    using wmapped_file = basic_mapped_file<wchar_t>;

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
                if (m_file->_is_at_end(m_current)) {
                    auto r = m_file->_read_single();
                    if (!r) {
                        if (r.error().code() == error::end_of_range &&
                            !m_file->_is_at_end(m_current)) {
                            ++m_current;
                            SCN_ENSURE(m_file->_is_at_end(m_current));
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
                    return o.m_file->_is_at_end(o.m_current);
                }
                if (!m_file && o.m_file) {
                    // rhs null, lhs potentially eof
                    return m_file->_is_at_end(m_current);
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

            iterator(const file_type& file) : m_file{std::addressof(file)} {}

            const file_type* m_file{nullptr};
            mutable size_t m_current{0};
        };

        using sentinel = iterator;
        using char_type = CharT;

        basic_file() = default;
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

        bool valid() const
        {
            return m_file != nullptr;
        }

        void sync()
        {
            _sync_all();
            m_buffer.clear();
        }

        iterator begin() const noexcept
        {
            return {*this};
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
            return i == m_buffer.size();
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

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wexit-time-destructors")
    template <typename CharT>
    basic_file<CharT>& stdin_range()
    {
        static thread_local auto f = basic_file<CharT>{stdin};
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
