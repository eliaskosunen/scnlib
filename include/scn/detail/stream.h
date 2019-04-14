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

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#if SCN_STL_OVERLOADS
#include <array>
#include <vector>
#endif

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wpadded")

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename S>
    struct is_sized_stream : S::is_sized_stream {
    };

    struct stream_base {
        using is_sized_stream = std::false_type;

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

    namespace detail {
        template <typename CharT>
        class erased_stream_base {
        public:
            using char_type = CharT;

            erased_stream_base(const erased_stream_base&) = delete;
            erased_stream_base& operator=(const erased_stream_base&) = delete;
            erased_stream_base(erased_stream_base&&) = default;
            erased_stream_base& operator=(erased_stream_base&&) = default;
            virtual ~erased_stream_base() = default;

            virtual either<char_type> read_char() = 0;
            virtual error putback(char_type) = 0;

            virtual error set_roll_back() = 0;
            virtual error roll_back() = 0;

        protected:
            erased_stream_base() = default;
        };

        template <typename CharT>
        class erased_sized_stream_base {
        public:
            using char_type = CharT;

            erased_sized_stream_base(const erased_sized_stream_base&) = delete;
            erased_sized_stream_base& operator=(
                const erased_sized_stream_base&) = delete;
            erased_sized_stream_base(erased_sized_stream_base&&) = default;
            erased_sized_stream_base& operator=(erased_sized_stream_base&&) =
                default;
            virtual ~erased_sized_stream_base() = default;

            virtual error read_sized(span<CharT> s) = 0;

            virtual size_t chars_to_read() const = 0;

            virtual error skip(size_t n) = 0;
            virtual error skip_all() = 0;

        protected:
            erased_sized_stream_base() = default;
        };

        template <typename Stream>
        class erased_stream_impl
            : public erased_stream_base<typename Stream::char_type> {
            using base = erased_stream_base<typename Stream::char_type>;

        public:
            using char_type = typename base::char_type;

            erased_stream_impl(Stream s) : m_stream(std::move(s)) {}

            either<char_type> read_char() override
            {
                return m_stream.read_char();
            }
            error putback(char_type ch) override
            {
                return m_stream.putback(ch);
            }

            error set_roll_back() override
            {
                return m_stream.set_roll_back();
            }
            error roll_back() override
            {
                return m_stream.roll_back();
            }

            Stream& get()
            {
                return m_stream;
            }
            const Stream& get() const
            {
                return m_stream;
            }

        private:
            Stream m_stream;
        };

        template <typename Stream>
        class erased_sized_stream_impl
            : public erased_sized_stream_base<typename Stream::char_type> {
            using base = erased_sized_stream_base<typename Stream::char_type>;

        public:
            using char_type = typename base::char_type;

            erased_sized_stream_impl(Stream& s) : m_stream(std::addressof(s)) {}

            error read_sized(span<char_type> s) override
            {
                return m_stream->read_sized(s);
            }

            size_t chars_to_read() const override
            {
                return m_stream->chars_to_read();
            }

            error skip(size_t n) override
            {
                return m_stream->skip(n);
            }
            error skip_all() override
            {
                return m_stream->skip_all();
            }

            Stream& get()
            {
                return *m_stream;
            }
            const Stream& get() const
            {
                return *m_stream;
            }

        private:
            Stream* m_stream;
        };
    }  // namespace detail

    template <typename CharT>
    class erased_stream : public stream_base {
    public:
        using char_type = CharT;
        template <typename Stream>
        erased_stream(Stream s)
            : m_stream(
                  detail::make_unique<detail::erased_stream_impl<Stream>>(std::move(s)))
        {
        }

        either<char_type> read_char()
        {
            return m_stream->read_char();
        }
        error putback(char_type ch)
        {
            return m_stream->putback(ch);
        }

        error set_roll_back()
        {
            return m_stream->set_roll_back();
        }
        error roll_back()
        {
            return m_stream->roll_back();
        }

        detail::erased_stream_base<CharT>& get()
        {
            return *m_stream;
        }
        const detail::erased_stream_base<CharT>& get() const
        {
            return *m_stream;
        }

        template <typename Stream>
        detail::erased_stream_impl<Stream>& get_as()
        {
            return static_cast<detail::erased_stream_impl<Stream>&>(*m_stream);
        }
        template <typename Stream>
        const detail::erased_stream_impl<Stream>& get_as() const
        {
            return static_cast<const detail::erased_stream_impl<Stream>&>(*m_stream);
        }

    private:
        detail::unique_ptr<detail::erased_stream_base<CharT>> m_stream;
    };

    template <typename CharT>
    class erased_sized_stream : public erased_stream<CharT> {
        using base = erased_stream<CharT>;

    public:
        using char_type = CharT;
        using is_sized_stream = std::true_type;

        template <typename Stream>
        erased_sized_stream(Stream s)
            : base(std::move(s)),
              m_stream(detail::make_unique<detail::erased_sized_stream_impl<Stream>>(
                  base::template get_as<Stream>().get()))
        {
        }

        error read_sized(span<char_type> s)
        {
            return m_stream->read_sized(s);
        }

        size_t chars_to_read() const
        {
            return m_stream->chars_to_read();
        }

        error skip(size_t n)
        {
            return m_stream->skip(n);
        }
        error skip_all()
        {
            return m_stream->skip_all();
        }

        detail::erased_sized_stream_base<CharT>& get_sized()
        {
            return *m_stream;
        }
        const detail::erased_sized_stream_base<CharT>& get_sized() const
        {
            return *m_stream;
        }

    private:
        detail::unique_ptr<detail::erased_sized_stream_base<CharT>> m_stream;
    };

    template <typename CharT>
    erased_stream<CharT> make_stream(erased_stream<CharT>& s) = delete;
    template <typename CharT>
    erased_sized_stream<CharT> make_stream(erased_sized_stream<CharT>& s) =
        delete;

    template <typename Char>
    class basic_null_stream : public stream_base {
    public:
        using char_type = Char;

        SCN_CONSTEXPR14 either<char_type> read_char() noexcept
        {
            ++m_read;
            return error(error::end_of_stream, "Null stream EOF");
        }
        SCN_CONSTEXPR14 error putback(char_type) noexcept
        {
            --m_read;
            return {};
        }

        SCN_CONSTEXPR14 error set_roll_back() noexcept
        {
            m_read = 0;
            return {};
        }
        SCN_CONSTEXPR14 error roll_back() noexcept
        {
            m_read = 0;
            return {};
        }

        SCN_CONSTEXPR size_t rcount() const noexcept
        {
            return m_read;
        }

    private:
        size_t m_read{0};
    };
    template <typename CharT>
    erased_stream<CharT> make_null_stream() noexcept
    {
        auto s = basic_null_stream<CharT>{};
        return {s};
    }

    template <typename Char, typename Source, typename Enable = void>
    class basic_static_container_stream;

    template <typename Char, typename Container>
    class basic_static_container_stream<Char, Container> : public stream_base {
    public:
        using char_type = Char;
        using source_type = Container;
        using iterator = typename source_type::const_iterator;
        using is_sized_stream = std::true_type;

        SCN_CONSTEXPR basic_static_container_stream(
            const source_type& s) noexcept
            : m_source(std::addressof(s)), m_begin(_begin(s)), m_next(begin())
        {
        }

        SCN_CONSTEXPR14 either<char_type> read_char() noexcept
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

        SCN_CONSTEXPR14 error read_sized(span<char_type> s) noexcept
        {
            if (chars_to_read() < static_cast<size_t>(s.size())) {
                return error(error::end_of_stream, "EOF");
            }
            std::copy(m_next, m_next + s.ssize(), s.begin());
            m_next += s.ssize();
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

#if SCN_STL_OVERLOADS
    template <typename CharT>
    erased_sized_stream<CharT> make_stream(const std::basic_string<CharT>& str)
    {
        auto s =
            basic_static_container_stream<CharT, std::basic_string<CharT>>(str);
        return {s};
    }
    template <typename CharT>
    erased_sized_stream<CharT> make_stream(const std::vector<CharT>& vec)
    {
        auto s = basic_static_container_stream<CharT, std::vector<CharT>>(vec);
        return {s};
    }
    template <typename CharT, size_t N>
    erased_sized_stream<CharT> make_stream(const std::array<CharT, N>& arr)
    {
        auto s =
            basic_static_container_stream<CharT, std::array<CharT, N>>(arr);
        return {s};
    }
#endif
    template <typename CharT, size_t N>
    erased_sized_stream<CharT> make_stream(const CharT (&arr)[N])
    {
        auto s = basic_static_container_stream<
            CharT, detail::array_container_stream_adaptor<CharT, N>>(arr);
        return {s};
    }

    template <typename Char>
    class basic_static_container_stream<Char, span<const Char>>
        : public stream_base {
    public:
        using char_type = Char;
        using source_type = span<const Char>;
        using iterator = typename source_type::const_iterator;
        using is_sized_stream = std::true_type;

        SCN_CONSTEXPR basic_static_container_stream(source_type s) noexcept
            : m_source(s), m_begin(m_source.begin()), m_next(begin())
        {
        }

        SCN_CONSTEXPR14 either<char_type> read_char() noexcept
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

        SCN_CONSTEXPR14 error read_sized(span<char_type> s) noexcept
        {
            if (chars_to_read() < s.size()) {
                return error(error::end_of_stream,
                             "Cannot complete read_sized: EOF encountered");
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
    erased_sized_stream<CharT> make_stream(span<const CharT> s) noexcept
    {
        auto stream =
            basic_static_container_stream<CharT, span<const CharT>>(s);
        return {stream};
    }

    template <typename Iterator>
    struct basic_bidirectional_iterator_stream : public stream_base {
        using char_type = typename std::iterator_traits<Iterator>::value_type;
        using is_sized_stream = std::true_type;

        SCN_CONSTEXPR basic_bidirectional_iterator_stream(Iterator begin,
                                                          Iterator end) noexcept
            : m_begin(begin), m_end(end), m_next(begin)
        {
        }

        SCN_CONSTEXPR14 either<char_type> read_char() noexcept
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

        SCN_CONSTEXPR14 error read_sized(span<char_type> s) noexcept
        {
            if (chars_to_read() < static_cast<size_t>(s.size())) {
                return error(error::end_of_stream,
                             "Cannot complete read_sized: EOF encountered");
            }
            std::copy(m_next, m_next + s.ssize(), s.begin());
            std::advance(m_next, s.ssize());
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
            m_next += static_cast<std::ptrdiff_t>(n);
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
        using is_sized_stream = std::true_type;

        SCN_CONSTEXPR basic_forward_iterator_stream(Iterator begin,
                                                    Iterator end) noexcept
            : m_begin(begin), m_end(end)
        {
        }

        either<char_type> read_char() noexcept
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

        SCN_CONSTEXPR14 error read_sized(span<char_type> s) noexcept
        {
            if (chars_to_read() < s.size()) {
                return error(error::end_of_stream,
                             "Cannot complete read_sized: EOF encountered");
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

    template <typename Iterator>
    erased_sized_stream<typename std::iterator_traits<Iterator>::value_type>
    make_stream(Iterator begin, Iterator end)
    {
        using StreamHelper = detail::iterator_stream<
            Iterator,
            typename std::iterator_traits<Iterator>::iterator_category>;
        auto s = StreamHelper::make_stream(begin, end);
        return {s};
    }

    template <typename CharT>
    struct basic_cstdio_stream;

    template <>
    struct basic_cstdio_stream<char> : public stream_base {
        using char_type = char;

        basic_cstdio_stream(FILE* f) noexcept : m_file(f) {}

        either<char_type> read_char()
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

        either<char_type> read_char()
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
    erased_stream<CharT> make_stream(FILE* f) noexcept
    {
        auto s = basic_cstdio_stream<CharT>(f);
        return {s};
    }
    inline erased_stream<char> make_narrow_stream(FILE* f) noexcept
    {
        return make_stream<char>(f);
    }
    inline erased_stream<wchar_t> make_wide_stream(FILE* f) noexcept
    {
        return make_stream<wchar_t>(f);
    }

    SCN_END_NAMESPACE
}  // namespace scn

SCN_CLANG_POP

#endif  // SCN_DETAIL_STREAM_H
