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

#ifndef SCN_DETAIL_STREAM_H
#define SCN_DETAIL_STREAM_H

#include "result.h"
#include "string_view.h"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wpadded")

namespace scn {
    template <typename S>
    struct is_bulk_stream : S::is_bulk_stream {
    };

    struct stream_base {
        using is_bulk_stream = std::false_type;

        SCN_CONSTEXPR14 void _set_bad() noexcept
        {
            m_bad = true;
        }

        SCN_CONSTEXPR bool bad() const noexcept
        {
            return m_bad;
        }

        SCN_CONSTEXPR explicit operator bool() const noexcept
        {
            return !bad();
        }

    private:
        bool m_bad{false};
    };

    template <typename Char, typename Source, typename Enable = void>
    class basic_static_container_stream;

    template <typename Char, typename Container>
    class basic_static_container_stream<Char, Container> : public stream_base {
    public:
        using char_type = Char;
        using source_type = Container;
        using iterator = typename source_type::const_iterator;
        using is_bulk_stream = std::true_type;

        SCN_CONSTEXPR basic_static_container_stream(
            const source_type& s) noexcept
            : m_source(std::addressof(s)), m_begin(_begin(s)), m_next(begin())
        {
        }

        SCN_CONSTEXPR14 result<char_type> read_char() noexcept
        {
            if (m_next == end()) {
                return error(error::end_of_stream, "EOF");
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        SCN_CONSTEXPR14 error putback(char_type) noexcept
        {
            if (m_begin == m_next) {
                return error(
                    error::invalid_operation,
                    "Cannot putback to a stream that hasn't been read from");
            }
            --m_next;
            return {};
        }

        SCN_CONSTEXPR14 error read_bulk(span<char_type> s) noexcept
        {
            if (chars_to_read() < static_cast<size_t>(s.size())) {
                return error(error::end_of_stream, "EOF");
            }
            std::copy(m_next, m_next + s.size(), s.begin());
            m_next += s.size();
            return {};
        }

        SCN_CONSTEXPR14 error set_roll_back() noexcept
        {
            m_begin = m_next;
            return {};
        }
        SCN_CONSTEXPR14 error roll_back() noexcept
        {
            m_next = begin();
            return {};
        }

        size_t rcount() const noexcept
        {
            return static_cast<size_t>(std::distance(m_begin, m_next));
        }

        SCN_CONSTEXPR14 size_t chars_to_read() const noexcept
        {
            return static_cast<size_t>(std::distance(m_next, end()));
        }

        SCN_CONSTEXPR14 error skip(size_t n) noexcept
        {
            if (chars_to_read() < n) {
                m_next = end();
                return error(error::end_of_stream, "EOF");
            }
            m_next += static_cast<std::ptrdiff_t>(n);
            return {};
        }
        SCN_CONSTEXPR14 error skip_all() noexcept
        {
            m_next = end();
            return {};
        }

    private:
        SCN_CONSTEXPR iterator begin() const noexcept
        {
            return m_begin;
        }
        SCN_CONSTEXPR14 iterator end() const noexcept
        {
            using std::end;
            return end(*m_source);
        }

        static SCN_CONSTEXPR14 iterator _begin(const source_type& s) noexcept
        {
            using std::begin;
            return begin(s);
        }

        const source_type* m_source;
        iterator m_begin, m_next{};
    };

    namespace detail {
        template <typename CharT, size_t N>
        struct array_container_stream_adaptor {
            using type = const CharT (&)[N];

            using value_type = const CharT;
            using reference = const CharT&;
            using const_reference = reference;
            using pointer = const CharT*;
            using const_pointer = pointer;
            using iterator = pointer;
            using const_iterator = const_pointer;

            type array;
        };

        template <typename CharT,
                  size_t N,
                  typename T = array_container_stream_adaptor<CharT, N>>
        SCN_CONSTEXPR auto begin(const T& a) -> typename T::iterator
        {
            return std::begin(a.array);
        }
        template <typename CharT,
                  size_t N,
                  typename T = array_container_stream_adaptor<CharT, N>>
        SCN_CONSTEXPR auto end(const T& a) -> typename T::iterator
        {
            return std::end(a.array);
        }
    }  // namespace detail

    template <typename CharT>
    SCN_CONSTEXPR basic_static_container_stream<CharT, std::basic_string<CharT>>
    make_stream(const std::basic_string<CharT>& s) noexcept
    {
        return s;
    }
    template <typename CharT>
    SCN_CONSTEXPR basic_static_container_stream<CharT, std::vector<CharT>>
    make_stream(const std::vector<CharT>& s) noexcept
    {
        return s;
    }
    template <typename CharT, size_t N>
    SCN_CONSTEXPR basic_static_container_stream<CharT, std::array<CharT, N>>
    make_stream(const std::array<CharT, N>& s) noexcept
    {
        return s;
    }
    template <typename CharT, size_t N>
    SCN_CONSTEXPR basic_static_container_stream<
        CharT,
        detail::array_container_stream_adaptor<CharT, N>>
    make_stream(const CharT (&s)[N]) noexcept
    {
        return s;
    }

    template <typename Char>
    class basic_static_container_stream<Char, span<const Char>>
        : public stream_base {
    public:
        using char_type = Char;
        using source_type = span<const Char>;
        using iterator = typename source_type::const_iterator;
        using is_bulk_stream = std::true_type;

        SCN_CONSTEXPR basic_static_container_stream(source_type s) noexcept
            : m_source(s), m_begin(m_source.begin()), m_next(begin())
        {
        }

        SCN_CONSTEXPR14 result<char_type> read_char() noexcept
        {
            if (m_next == end()) {
                return error(error::end_of_stream, "EOF");
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        SCN_CONSTEXPR14 error putback(char_type) noexcept
        {
            if (m_begin == m_next) {
                return error(
                    error::invalid_operation,
                    "Cannot putback to a stream that hasn't been read from");
            }
            --m_next;
            return {};
        }

        SCN_CONSTEXPR14 error read_bulk(span<char_type> s) noexcept
        {
            if (chars_to_read() < s.size()) {
                return error(error::end_of_stream,
                             "Cannot complete read_bulk: EOF encountered");
            }
            std::copy(m_next, m_next + s.size(), s.begin());
            m_next += s.size();
            return {};
        }

        SCN_CONSTEXPR14 error set_roll_back() noexcept
        {
            m_begin = m_next;
            return {};
        }
        SCN_CONSTEXPR14 error roll_back() noexcept
        {
            m_next = begin();
            return {};
        }

        size_t rcount() const noexcept
        {
            return static_cast<size_t>(std::distance(m_begin, m_next));
        }

        SCN_CONSTEXPR14 size_t chars_to_read() const noexcept
        {
            return static_cast<size_t>(std::distance(m_next, end()));
        }

        SCN_CONSTEXPR14 error skip(size_t n) noexcept
        {
            if (chars_to_read() < n) {
                m_next = end();
                return error(error::end_of_stream, "EOF");
            }
            m_next += n;
            return {};
        }
        SCN_CONSTEXPR14 error skip_all() noexcept
        {
            m_next = end();
            return {};
        }

    private:
        SCN_CONSTEXPR iterator begin() const noexcept
        {
            return m_begin;
        }
        SCN_CONSTEXPR iterator end() const noexcept
        {
            return m_source.end();
        }

        source_type m_source;
        iterator m_begin{}, m_next{};
    };

    template <typename CharT>
    SCN_CONSTEXPR basic_static_container_stream<CharT, span<const CharT>>
    make_stream(span<const CharT> s) noexcept
    {
        return s;
    }

    template <typename Iterator>
    struct basic_bidirectional_iterator_stream : public stream_base {
        using char_type = typename std::iterator_traits<Iterator>::value_type;
        using is_bulk_stream = std::true_type;

        SCN_CONSTEXPR basic_bidirectional_iterator_stream(Iterator begin,
                                                          Iterator end) noexcept
            : m_begin(begin), m_end(end), m_next(begin)
        {
        }

        SCN_CONSTEXPR14 result<char_type> read_char() noexcept
        {
            if (m_next == m_end) {
                return error(error::end_of_stream, "EOF");
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        SCN_CONSTEXPR14 error putback(char_type) noexcept
        {
            if (m_begin == m_next) {
                return error(
                    error::invalid_operation,
                    "Cannot putback to a stream that hasn't been read from");
            }
            --m_next;
            return {};
        }

        SCN_CONSTEXPR14 error read_bulk(span<char_type> s) noexcept
        {
            if (chars_to_read() < static_cast<size_t>(s.size())) {
                return error(error::end_of_stream,
                             "Cannot complete read_bulk: EOF encountered");
            }
            std::copy(m_next, m_next + s.size(), s.begin());
            std::advance(m_next, s.size());
            return {};
        }

        SCN_CONSTEXPR14 error set_roll_back() noexcept
        {
            m_begin = m_next;
            return {};
        }
        SCN_CONSTEXPR14 error roll_back() noexcept
        {
            m_next = m_begin;
            return {};
        }

        size_t rcount() const noexcept
        {
            return static_cast<size_t>(std::distance(m_begin, m_next));
        }

        SCN_CONSTEXPR14 size_t chars_to_read() const noexcept
        {
            return static_cast<size_t>(std::distance(m_next, m_end));
        }

        SCN_CONSTEXPR14 error skip(size_t n) noexcept
        {
            if (chars_to_read() < n) {
                m_next = m_end;
                return error(error::end_of_stream, "EOF");
            }
            m_next += n;
            return {};
        }
        SCN_CONSTEXPR14 error skip_all() noexcept
        {
            m_next = m_end;
            return {};
        }

    private:
        Iterator m_begin, m_end, m_next;
    };

    template <typename Iterator>
    struct basic_forward_iterator_stream : public stream_base {
        using char_type = typename std::iterator_traits<Iterator>::value_type;
        using is_bulk_stream = std::true_type;

        SCN_CONSTEXPR basic_forward_iterator_stream(Iterator begin,
                                                    Iterator end) noexcept
            : m_begin(begin), m_end(end)
        {
        }

        result<char_type> read_char() noexcept
        {
            if (m_rollback.size() > 0) {
                auto top = m_rollback.back();
                m_rollback.pop_back();
                return top;
            }
            if (m_begin == m_end) {
                return error(error::end_of_stream, "EOF");
            }
            auto ch = *m_begin;
            ++m_begin;
            return ch;
        }
        error putback(char_type ch)
        {
            m_rollback.push_back(ch);
            return {};
        }

        SCN_CONSTEXPR14 error read_bulk(span<char_type> s) noexcept
        {
            if (chars_to_read() < s.size()) {
                return error(error::end_of_stream,
                             "Cannot complete read_bulk: EOF encountered");
            }
            std::copy(m_begin, m_begin + s.size(), s.begin());
            std::advance(m_begin, s.size());
            return {};
        }

        error set_roll_back() noexcept
        {
            m_rollback.clear();
            return {};
        }
        SCN_CONSTEXPR error roll_back() const noexcept
        {
            return {};
        }

        size_t rcount() const noexcept
        {
            return m_rollback.size();
        }

        SCN_CONSTEXPR14 size_t chars_to_read() const noexcept
        {
            return static_cast<size_t>(std::distance(m_begin, m_end));
        }

        SCN_CONSTEXPR14 error skip(size_t n) noexcept
        {
            if (chars_to_read() < n) {
                m_begin = m_end;
                return error(error::end_of_stream, "EOF");
            }
            m_begin += n;
            return {};
        }
        SCN_CONSTEXPR14 error skip_all() noexcept
        {
            m_begin = m_end;
            return {};
        }

    private:
        Iterator m_begin, m_end;
        detail::small_vector<char_type, 32> m_rollback;
    };

    namespace detail {
        template <typename Iterator>
        struct bidir_iterator_stream {
            using iterator = Iterator;
            using type = basic_bidirectional_iterator_stream<iterator>;

            static SCN_CONSTEXPR type make_stream(iterator b,
                                                  iterator e) noexcept
            {
                return {b, e};
            }
        };
        template <typename Iterator>
        struct fwd_iterator_stream {
            using iterator = Iterator;
            using type = basic_forward_iterator_stream<iterator>;

            static type make_stream(iterator b, iterator e) noexcept
            {
                return {b, e};
            }
        };

        template <typename Iterator, typename Tag>
        struct iterator_stream;
        template <typename Iterator>
        struct iterator_stream<Iterator, std::forward_iterator_tag>
            : public fwd_iterator_stream<Iterator> {
        };
        template <typename Iterator>
        struct iterator_stream<Iterator, std::bidirectional_iterator_tag>
            : public bidir_iterator_stream<Iterator> {
        };
        template <typename Iterator>
        struct iterator_stream<Iterator, std::random_access_iterator_tag>
            : public bidir_iterator_stream<Iterator> {
        };
    }  // namespace detail

    template <typename Iterator,
              typename StreamHelper = detail::iterator_stream<
                  Iterator,
                  typename std::iterator_traits<Iterator>::iterator_category>>
    typename StreamHelper::type make_stream(Iterator begin,
                                            Iterator end) noexcept
    {
        return StreamHelper::make_stream(begin, end);
    }

    template <typename CharT>
    struct basic_cstdio_stream;

    template <>
    struct basic_cstdio_stream<char> : public stream_base {
        using char_type = char;

        basic_cstdio_stream(FILE* f) noexcept : m_file(f) {}

        result<char_type> read_char()
        {
            auto ret = std::fgetc(m_file);
            if (ret == EOF) {
                if (std::ferror(m_file) != 0) {
                    return error(error::stream_source_error,
                                 std::strerror(errno));
                }
                if (std::feof(m_file) != 0) {
                    return error(error::end_of_stream, "EOF");
                }
                return error(error::unrecoverable_stream_source_error,
                             "Unknown error");
            }
            m_read.push_back(static_cast<char_type>(ret));
            return static_cast<char_type>(ret);
        }
        error putback(char_type ch) noexcept
        {
            assert(!m_read.empty());
            if (std::ungetc(ch, m_file) == EOF) {
                return error(error::unrecoverable_stream_source_error,
                             std::strerror(errno));
            }
            m_read.pop_back();
            return {};
        }

        error set_roll_back()
        {
            m_read.clear();
            return {};
        }
        error roll_back() noexcept
        {
            if (m_read.empty()) {
                return {};
            }
            for (auto it = m_read.rbegin(); it != m_read.rend(); ++it) {
                if (std::ungetc(*it, m_file) == EOF) {
                    return error(error::unrecoverable_stream_source_error,
                                 std::strerror(errno));
                }
            }
            m_read.clear();
            return {};
        }

        size_t rcount() const noexcept
        {
            return m_read.size();
        }

    private:
        FILE* m_file;
        std::string m_read{};
    };
    template <>
    struct basic_cstdio_stream<wchar_t> : public stream_base {
        using char_type = wchar_t;

        basic_cstdio_stream(FILE* f) noexcept : m_file(f) {}

        result<char_type> read_char()
        {
            auto ret = std::fgetwc(m_file);
            if (ret == WEOF) {
                if (std::ferror(m_file) != 0) {
                    return error(error::stream_source_error,
                                 std::strerror(errno));
                }
                if (std::feof(m_file) != 0) {
                    return error(error::end_of_stream, "EOF");
                }
                return error(error::unrecoverable_stream_source_error,
                             "Unknown error");
            }
            m_read.push_back(static_cast<char_type>(ret));
            return static_cast<char_type>(ret);
        }
        error putback(char_type ch) noexcept
        {
            assert(!m_read.empty());
            if (std::ungetwc(std::char_traits<char_type>::to_int_type(ch),
                             m_file) == WEOF) {
                return error(error::unrecoverable_stream_source_error,
                             std::strerror(errno));
            }
            m_read.pop_back();
            return {};
        }

        error set_roll_back() noexcept
        {
            m_read.clear();
            return {};
        }
        error roll_back() noexcept
        {
            if (m_read.empty()) {
                return {};
            }
            for (auto it = m_read.rbegin(); it != m_read.rend(); ++it) {
                if (std::ungetwc(std::char_traits<char_type>::to_int_type(*it),
                                 m_file) == WEOF) {
                    return error(error::unrecoverable_stream_source_error,
                                 std::strerror(errno));
                }
            }
            m_read.clear();
            return {};
        }

        size_t rcount() const noexcept
        {
            return m_read.size();
        }

    private:
        FILE* m_file;
        std::wstring m_read{};
    };

    template <typename CharT = char>
    basic_cstdio_stream<CharT> make_stream(FILE* s) noexcept
    {
        return s;
    }
    inline basic_cstdio_stream<char> make_narrow_stream(FILE* s) noexcept
    {
        return s;
    }
    inline basic_cstdio_stream<wchar_t> make_wide_stream(FILE* s) noexcept
    {
        return s;
    }
}  // namespace scn

SCN_CLANG_POP

#endif  // SCN_DETAIL_STREAM_H
