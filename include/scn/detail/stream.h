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
#include "small_vector.h"
#include "string_view.h"

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wpadded")

namespace scn {
    SCN_BEGIN_NAMESPACE

    /**
     * \defgroup stream_concept Stream
     * \ingroup concepts
     * \{
     * \par
     * A `Stream` is something that the library can read values from.
     *
     * \par Description
     * A `Stream` contains a <i>stream source</i> or just <i>source</i> for
     * short inside of it, which is where the stream gets its input. The
     * <i>source</i> can be a buffer, a file, a socket, or whatever the `Stream`
     * happens to implement.
     *
     * \par
     * In addition, a `Stream` contains a <i>putback buffer</i>.
     * In practice, not all concrete stream types have one, but use the
     * underlying source to their advantage, to achieve the same effect, under
     * the as-if rule.
     *
     * \par
     * Every `Stream` has an associated <i>character type</i>, which must be
     * either `char` or `wchar_t`. This is the type of the charact//
     * exposition-onlyers that the external interface uses; the stream source
     * can use whatever character type it likes.
     *
     * \par
     * An example of a `Stream` is `scn::basic_cstdio_stream`.
     *
     * \par Valid expressions
     * A type `S` satisfies `Stream`, if
     *   - the type `S` satisfies `MoveConstructible`, `MoveAssignable` and
     * `Destructible`, and
     *   - lvalues of type `S` satisfy `Swappable`, and
     *
     * \par
     * given
     *   - `s`, an lvalue of type `S`, and
     *   - `ch`, a value of type `S::char_type`,
     *
     * \par
     * the following expressions must be valid and have their specified effects:
     *   - `S::char_type`: character type of `s`
     *   - `S::is_sized_stream::value -> bool`
     *   - `s.read_char() -> expected<S::char_type>`:
     *      Reads a character from `s`, or returns an error.
     *      <b>Precondition:</b> `s` must be <i>readable</i>
     *   - `s.putback(ch) -> error`:
     *      Puts a character into the <i>putback buffer</i> of `s`.
     *      <b>Postcondition:</b> If the operation was successful,
     *      `s` will be <i>readable</i>
     *   - `s.bad() -> bool`: Returns `true` if `s` is <i>bad</i>
     *   - `(bool)s`: Equivalent to: `!s.bad()`
     *   - `s.set_roll_back() -> error`: Sets the current state of `s`
     *      as the <i>recovery state</i>.
     *      <b>Precondition:</b> `s` must not be <i>bad</i>
     *   - `s.roll_back() -> error`: Resets the state of `s` into the
     *      <i>recovery state</i>.
     *      <b>Precondition:</b> `s` must not be <i>bad</i>
     *   - `s.rcount() -> std::size_t`: Returns the number of characters read,
     *      minus the size of the putback buffer, since the last call to
     *      `set_roll_back()` or `roll_back()`
     *
     * \par Notes
     * A `Stream` is <i>bad</i> if some non-recoverable error has occurred.
     *
     * \par
     * A `Stream` is <i>readable</i> if it is:
     *   - not <i>bad</i>, and
     *   - the previous call to `read_char()` did not return an error.
     *
     * \par
     * If the previous call to `read_char()` failed, either:
     *   - `putback()` must be called, or
     *   - the <i>source</i> must be modified (if supported)
     *
     * \par
     * for the stream to become <i>readable</i> again.
     *
     * \par
     * A call to `read_char()` first checks the top of the putback buffer,
     * popping that if there's any characters there, and only if there's none,
     * will it reach for the source.
     *
     * \par
     * A `Stream` has a <i>recovery state</i>, which is the state of the stream
     * at construction, or after any subsequent `set_roll_back()` call. This
     * state can be then rolled back to using `roll_back()`. This functionality
     * can be used for error recovery; if a higher-level operation fails, and
     * `set_roll_back()` was called before the operation, the stream can be
     * rolled back to the <i>recovery state</i> with `roll_back()`.
     *
     * \par
     * `is_sized_stream::value` is `true` if and only if the type also satisfies
     * `SizedStream`.
     *
     * \par
     * If `s.putback()` is called, and then the underlying <i>stream source</i>
     * is mutated, the behavior is undefined. Some concrete stream types may
     * relax this requirement.
     *
     * \par Exposition-only Concept
     * \code{.cpp}
     * // exposition-only
     * template <typename S>
     * concept Stream =
     *     std::Movable<S> && std::Destructible<S> &&
     *     requires(S& s, typename S::char_type ch, typename S::is_sized_stream)
     *     {
     *         { s.read_char() } -> expected<char_type>;
     *         { s.putback(ch) } -> error;
     *         { s.bad() } -> std::Boolean;
     *         { s } -> std::Boolean;
     *         { s.set_roll_back() } -> error;
     *         { s.roll_back() } -> error;
     *         { s.rcount() } -> std::size_t;
     *         { S::is_sized_stream::value } -> std::Boolean;
     *     };
     * \endcode
     * \}
     */

    /**
     * \defgroup sized_stream_concept SizedStream
     * \ingroup concepts
     *
     * \par
     * `SizedStream` is a refinement of \ref stream_concept.
     * The size (number of characters) in a `SizedStream` <i>source</i> shall
     * not change after construction.
     *
     * \par
     * An example of a `SizedStream` is `scn::basic_static_container_stream`.
     *
     * \par Valid expressions
     * A type `S` satisfies `SizedStream`, if
     *   - the type `S` satisfies \ref stream_concept, and
     *
     * \par
     * given
     *   - `s`, an lvalue of type `S`, and
     *   - `ch`, a value of type `S::char_type`, and
     *   - `sz`, a value of type `std::size_t`, and
     *   - `buf`, a value of type `span<S::char_type>`,
     *
     * \par
     * the following expressions must be valid and have their specified effects:
     *   - `s.read_sized(buf) -> void`: Fills `buf` with characters from `s`.
     *      <b>Precondition:</b> `s` must be <i>readable</i> and
     *      `s.chars_to_read()` must be greater or equal to `buf.size()`.
     *   - `s.putback_n(sz) -> void`:
     *      Puts back the last `sz` characters read into `s`.
     *      <b>Precondition:</b> `s.rcount() >= sz` must be `true`.
     *      <b>Postcondition:</b> `s` will be <i>readable</i> for `sz`
     *      characters.
     *   - `s.chars_to_read() -> std::size_t`:
     *      Returns the number of characters `s` has available to read.
     *   - `s.skip(sz) -> void`: Skips `sz` characters.
     *      <b>Precondition:</b> `s` must be readable for `sz` characters.
     *   - `s.skip_all() -> void`: Skips to the end of `s`.
     *      <b>Postcondition:</b> `s` is not <i>readable</i>.
     *
     * \par Notes
     * For the definitions of <i>stream source</i> and <i>readable</i>, see
     * \ref stream_concept.
     *
     * \par Exposition-only Concept
     * \code{.cpp}
     * // exposition-only
     * template <typename S>
     * concept SizedStream =
     *    Stream<S> &&
     *    requires(S& s, typename S::char_type ch, std::size_t sz,
     *             span<typename S::char_type> buf)
     *    {
     *       { s.read_sized(buf) } -> void;
     *       { s.putback_n(sz) } -> void;
     *       { s.chars_to_read() } -> std::size_t;
     *       { s.skip(sz) } -> void;
     *       { s.skip_all() } -> void;
     *    }
     * \endcode
     */

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

    template <typename Char>
    class basic_null_stream : public stream_base {
    public:
        using char_type = Char;

        SCN_CONSTEXPR14 expected<char_type> read_char() noexcept
        {
            ++m_read;
            return error(error::end_of_stream, "Null stream EOF");
        }
        SCN_CONSTEXPR14 error putback(char_type) noexcept
        {
            SCN_EXPECT(m_read != 0);
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
    basic_null_stream<CharT> make_null_stream() noexcept
    {
        return basic_null_stream<CharT>{};
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

        SCN_CONSTEXPR14 expected<char_type> read_char() noexcept
        {
            if (SCN_UNLIKELY(m_next == end())) {
                return error(error::end_of_stream, "EOF");
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        SCN_CONSTEXPR14 error putback(char_type) noexcept
        {
            SCN_EXPECT(m_begin != m_next);
            --m_next;
            return {};
        }

        SCN_CONSTEXPR14 void read_sized(span<char_type> s) noexcept
        {
            SCN_EXPECT(chars_to_read() >= static_cast<size_t>(s.size()));
            std::copy(m_next, m_next + s.ssize(), s.begin());
            m_next += s.ssize();
        }

        SCN_CONSTEXPR14 void putback_n(size_t n) noexcept
        {
            SCN_EXPECT(rcount() >= n);
            m_next -= static_cast<std::ptrdiff_t>(n);
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

        SCN_CONSTEXPR14 void skip(size_t n) noexcept
        {
            SCN_EXPECT(chars_to_read() >= n);
            m_next += static_cast<std::ptrdiff_t>(n);
        }
        SCN_CONSTEXPR14 void skip_all() noexcept
        {
            m_next = end();
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

    template <typename ContiguousContainer,
              typename = decltype(std::declval<ContiguousContainer&>().data(),
                                  void())>
    basic_static_container_stream<typename ContiguousContainer::value_type,
                                  ContiguousContainer>
    make_stream(const ContiguousContainer& c)
    {
        return {c};
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

        SCN_CONSTEXPR14 expected<char_type> read_char() noexcept
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
            SCN_EXPECT(m_begin != m_next);
            --m_next;
            return {};
        }

        error read_sized(span<char_type> s) noexcept
        {
            SCN_EXPECT(chars_to_read() >= s.size());
            std::memcpy(s.begin(), m_next, s.size() * sizeof(char_type));
            /* std::copy(m_next, m_next + s.size(), s.begin()); */
            m_next += s.size();
            return {};
        }  // namespace scn

        error putback_n(size_t n) noexcept
        {
            SCN_EXPECT(rcount() >= n);
            m_next -= n;
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

        SCN_CONSTEXPR14 void skip(size_t n) noexcept
        {
            SCN_EXPECT(chars_to_read() >= n);
            m_next += n;
        }
        SCN_CONSTEXPR14 void skip_all() noexcept
        {
            m_next = end();
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
    basic_static_container_stream<CharT, span<const CharT>> make_stream(
        span<const CharT> s) noexcept
    {
        return {s};
    }

    template <typename CharT, size_t N>
    basic_static_container_stream<CharT, span<const CharT>> make_stream(
        const CharT (&arr)[N])
    {
        return {{arr, N - 1}};
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

        SCN_CONSTEXPR14 expected<char_type> read_char() noexcept
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
            SCN_EXPECT(m_begin != m_next);
            --m_next;
            return {};
        }

        SCN_CONSTEXPR14 error read_sized(span<char_type> s) noexcept
        {
            SCN_EXPECT(chars_to_read() >= static_cast<size_t>(s.size()));
            std::copy(m_next, m_next + s.ssize(), s.begin());
            std::advance(m_next, s.ssize());
            return {};
        }

        error putback_n(size_t n) noexcept
        {
            SCN_EXPECT(rcount() >= n);
            m_next -= static_cast<std::ptrdiff_t>(n);
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

        SCN_CONSTEXPR14 void skip(size_t n) noexcept
        {
            SCN_EXPECT(chars_to_read() >= n);
            m_next += static_cast<std::ptrdiff_t>(n);
        }
        SCN_CONSTEXPR14 void skip_all() noexcept
        {
            m_next = m_end;
        }

    private:
        Iterator m_begin, m_end, m_next;
    };

    template <typename Iterator>
    struct basic_forward_iterator_stream : public stream_base {
        using char_type = typename std::iterator_traits<Iterator>::value_type;

        SCN_CONSTEXPR basic_forward_iterator_stream(Iterator begin,
                                                    Iterator end) noexcept
            : m_begin(begin), m_end(end), m_read{}, m_read_it(m_read.begin())
        {
        }

        expected<char_type> read_char() noexcept
        {
            if (m_read_it != m_read.end()) {
                auto ch = *m_read_it;
                ++m_read_it;
                return ch;
            }
            if (m_begin == m_end) {
                return error(error::end_of_stream, "EOF");
            }
            auto ch = *m_begin;
            ++m_begin;
            m_read.push_back(ch);
            m_read_it = m_read.end();
            return ch;
        }
        error putback(char_type)
        {
            SCN_EXPECT(m_read_it != m_read.begin());
            --m_read_it;
            return {};
        }

        error set_roll_back() noexcept
        {
            m_read.clear();
            m_read_it = m_read.end();
            return {};
        }
        error roll_back() noexcept
        {
            m_read_it = m_read.begin();
            return {};
        }

        size_t rcount() const noexcept
        {
            return m_read.size();
        }

    private:
        using buffer_type = detail::small_vector<char_type, 32>;

        Iterator m_begin, m_end;
        buffer_type m_read;
        typename buffer_type::iterator m_read_it;
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
    typename StreamHelper::type make_stream(Iterator begin, Iterator end)
    {
        return StreamHelper::make_stream(begin, end);
    }

    template <typename CharT>
    struct basic_cstdio_stream;

    template <>
    struct basic_cstdio_stream<char> : public stream_base {
        using char_type = char;

        basic_cstdio_stream(FILE* f) noexcept : m_file(f) {}

        expected<char_type> read_char();
        error putback(char_type ch) noexcept;

        error set_roll_back()
        {
            m_read.clear();
            return {};
        }
        error roll_back() noexcept;

        size_t rcount() const noexcept
        {
            return m_read.size();
        }

    private:
        FILE* m_file;
        detail::small_vector<char, 32> m_read{};
    };
    template <>
    struct basic_cstdio_stream<wchar_t> : public stream_base {
        using char_type = wchar_t;

        basic_cstdio_stream(FILE* f) noexcept : m_file(f) {}

        expected<char_type> read_char();
        error putback(char_type ch) noexcept;

        error set_roll_back() noexcept
        {
            m_read.clear();
            return {};
        }
        error roll_back() noexcept;

        size_t rcount() const noexcept
        {
            return m_read.size();
        }

    private:
        FILE* m_file;
        detail::small_vector<wchar_t, 32> m_read{};
    };

    template <typename CharT = char>
    basic_cstdio_stream<CharT> make_stream(FILE* f) noexcept
    {
        return {f};
    }
    inline basic_cstdio_stream<char> make_narrow_stream(FILE* f) noexcept
    {
        return {f};
    }
    inline basic_cstdio_stream<wchar_t> make_wide_stream(FILE* f) noexcept
    {
        return {f};
    }

    SCN_END_NAMESPACE
}  // namespace scn

SCN_CLANG_POP

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_STREAM_CPP)
#include "stream.cpp"
#endif

#endif  // SCN_DETAIL_STREAM_H
