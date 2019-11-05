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

#ifndef SCN_DETAIL_SCAN_H
#define SCN_DETAIL_SCAN_H

#include <vector>

#include "vscan.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Range, typename E = wrapped_error>
        struct scan_result_for_range {
            using type = scan_result<
                typename detail::range_wrapper_for_t<Range>::return_type,
                E>;
        };
        template <typename Range, typename E = wrapped_error>
        using scan_result_for_range_t =
            typename scan_result_for_range<Range, E>::type;
    }  // namespace detail

    /**
     * \defgroup scanning Scanning API
     * Core part of the public scanning API.
     *
     * Generally, the functions in this group take a range, a format string, and
     * a list of arguments. The arguments are parsed from the range based on the
     * information given in the format string.
     *
     * The range is described further here: \ref range.
     *
     * If the function takes a format string and a range, they must share
     * character types. Also, the format string must be convertible to
     * `basic_string_view<CharT>`, where `CharT` is that aforementioned
     * character type.
     *
     * The majority of the functions in this category return a
     * `scan_result<Range, result<ptrdiff_t>>`, which has the following member
     * functions:
     *  - `operator bool`: `true` when successful
     *  - `range() -> Range`
     *  - `error() -> error`
     */

    /// @{

    // scan

    /**
     * The most fundamental part of the scanning API.
     * Reads from the range in \c r according to the format string \c f.
     */
    template <typename Range, typename Format, typename... Args>
    auto scan(Range&& r, const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using range_type = detail::range_wrapper_for_t<Range>;
        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx, {args});
    }

    // scan localized

    /**
     * Read from the range in \c r using the locale in \c loc.
     * \c loc must be a std::locale.
     *
     * Use of this function is discouraged, due to the overhead involved with
     * locales. Note, that the other functions are completely locale-agnostic,
     * and aren't affected by changes to the global C locale.
     */
    template <typename Locale,
              typename Range,
              typename Format,
              typename... Args>
    auto scan_localized(const Locale& loc,
                        Range&& r,
                        const Format& f,
                        Args&... a) -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using range_type = detail::range_wrapper_for_t<Range>;
        using locale_type = basic_locale_ref<typename range_type::char_type>;
        using context_type = basic_context<range_type, locale_type>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)),
                                {std::addressof(loc)});
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx, {args});
    }

    // default format

    /**
     * Equivalent to \ref scan, but with a
     * format string with the appropriate amount of space-separated "{}"s for
     * the number of arguments. Because this function doesn't have to parse the
     * format string, performance is improved.
     *
     * \see scan
     */
    template <typename Range, typename... Args>
    auto scan(Range&& r, detail::default_t, Args&... a)
        -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using range_type = detail::range_wrapper_for_t<Range>;
        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_empty_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
        auto pctx = parse_context_type(static_cast<int>(sizeof...(Args)), ctx);
        return vscan(ctx, pctx, {args});
    }

    // value

    /**
     * Scans a single value with the default options, returning it instead of
     * using an output parameter.
     *
     * The parsed value is in `ret.value()`, if `ret == true`.
     *
     * \code{.cpp}
     * auto ret = scn::scan_value<int>("42");
     * if (ret) {
     *   // ret.value() == 42
     * }
     * \endcode
     */
    template <typename T, typename Range>
    auto scan_value(Range&& r)
        -> detail::scan_result_for_range_t<Range, expected<T>>
    {
        using range_type = detail::range_wrapper_for_t<Range>;
        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_empty_parse_context<typename context_type::locale_type>;

        T value;
        auto args = make_args<context_type, parse_context_type>(value);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)));

#if 0
        using char_type = typename context_type::char_type;
        auto e = skip_range_whitespace(ctx);
        if (!e) {
            ctx.range().reset_to_rollback_point();
            return {e, ctx.range().get_return()};
        }

        scanner<char_type, T> s{};
        e = s.scan(value, ctx);
        if (!e) {
            ctx.range().reset_to_rollback_point();
            return {e, ctx.range().get_return()};
        }
        return {std::move(value), ctx.range().get_return()};
#else
        auto pctx = parse_context_type(1, ctx);
        auto ret = vscan(ctx, pctx, {args});
        if (!ret) {
            return {ret.error(), ret.range()};
        }
        return {value, ret.range()};
#endif
    }

    // scanf

    /**
     * Otherwise equivalent to \ref scan, except it uses scanf-like format
     * string syntax, instead of the Python-like default one.
     */
    template <typename Range, typename Format, typename... Args>
    auto scanf(Range&& r, const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<detail::range_wrapper_for_t<Range>>;
        using parse_context_type =
            basic_scanf_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)));
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx, {args});
    }

    // input

    /**
     * Otherwise equivalent to \ref scan, expect reads from `stdin`.
     * Character type is determined by the format string.
     *
     * Does not sync with the rest `<cstdio>` (like
     * `std::ios_base::sync_with_stdio(false)`). To use `<cstdio>` (like `fread`
     * or `fgets`) with this function, call `cstdin().sync()` or
     * `wcstdin().sync()` before calling `<cstdio>`.
     */
    template <typename Format,
              typename... Args,
              typename CharT = detail::ranges::range_value_t<Format>>
    auto input(const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<decltype(stdin_range<CharT>())>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<
            detail::range_wrapper_for_t<decltype(stdin_range<CharT>())>>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(stdin_range<CharT>()));
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx, {args});
    }

    // prompt

    /**
     * Equivalent to \ref input, except writes what's in `p` to `stdout`.
     *
     * \code{.cpp}
     * int i{};
     * scn::prompt("What's your favorite number? ", "{}", i);
     * // Equivalent to:
     * //   std::fputs("What's your favorite number? ", stdout);
     * //   scn::input("{}", i);
     * \endcode
     */
    template <typename Format,
              typename... Args,
              typename CharT = detail::ranges::range_value_t<Format>>
    auto prompt(const CharT* p, const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<decltype(stdin_range<CharT>())>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");
        SCN_EXPECT(p != nullptr);

        std::fputs(p, stdout);

        using context_type = basic_context<
            detail::range_wrapper_for_t<decltype(stdin_range<CharT>())>>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(stdin_range<CharT>()));
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx, {args});
    }

    /**
     * Parses an integer into `val` in base `base` from `str`.
     * Returns a pointer past the last character read, or an error.
     * `str` can't be empty, and cannot have:
     *   - preceding whitespace
     *   - preceding `"0x"` or `"0"` (base is determined by the `base`
     *     parameter)
     *   - `+` sign (`-` is fine)
     */
    template <typename T, typename CharT>
    expected<const CharT*> parse_integer(basic_string_view<CharT> str,
                                         T& val,
                                         int base = 10)
    {
        SCN_EXPECT(!str.empty());
        auto s = scanner<CharT, T>{base};
        bool minus_sign = false;
        if (str[0] == detail::ascii_widen<CharT>('-')) {
            minus_sign = true;
        }
        auto ret = s._read_int(val, minus_sign,
                               make_span(str.data(), str.size()).as_const(),
                               detail::ascii_widen<CharT>('\0'));
        if (!ret) {
            return ret.error();
        }
        return {ret.value()};
    }

    // scanning api
    /// @}

    /**
     * \defgroup scanning_operations Higher-level scanning operations
     *
     * Functions in this category return a `scan_result<Range, error>`.
     * It has the following member functions:
     *  - `operator bool`: `true` when successful
     *  - `range() -> Range`
     *  - `error() -> error`
     */

    // getline

    namespace detail {
        template <typename WrappedRange, typename String, typename CharT>
        auto getline_impl(WrappedRange& r, String& str, CharT until)
            -> detail::scan_result_for_range_t<WrappedRange, wrapped_error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            auto s = read_until_space_zero_copy(r, until_pred, true);
            if (!s) {
                return {std::move(s.error()), r.get_return()};
            }
            if (s.value().size() != 0) {
                auto size = s.value().size();
                if (until_pred(s.value()[size - 1])) {
                    --size;
                }
                str.clear();
                str.resize(size);
                std::copy(s.value().begin(), s.value().begin() + size,
                          str.begin());
                return {{}, r.get_return()};
            }

            String tmp;
            auto out = std::back_inserter(tmp);
            auto e = read_until_space(r, out, until_pred, true);
            if (!e) {
                return {std::move(e), r.get_return()};
            }
            if (until_pred(tmp.back())) {
                tmp.pop_back();
            }
            str = std::move(tmp);
            return {{}, r.get_return()};
        }
        template <typename WrappedRange, typename CharT>
        auto getline_impl(WrappedRange& r,
                          basic_string_view<CharT>& str,
                          CharT until)
            -> detail::scan_result_for_range_t<WrappedRange, wrapped_error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            auto s = read_until_space_zero_copy(r, until_pred, true);
            if (!s) {
                return {std::move(s.error()), r.get_return()};
            }
            if (s.value().size() != 0) {
                auto size = s.value().size();
                if (until_pred(s.value()[size - 1])) {
                    --size;
                }
                str = basic_string_view<CharT>{s.value().data(), size};
                return {{}, r.get_return()};
            }
            // TODO: Compile-time error?
            return {
                error(
                    error::invalid_operation,
                    "Cannot getline a string_view from a non-contiguous range"),
                r.get_return()};
        }
    }  // namespace detail

    /**
     * \ingroup scanning_operations
     * Read the range in \c r into \c str until \c until is found.
     *
     * \c r and \c str must share character types, which must be \c CharT.
     *
     * If `str` is convertible to a `basic_string_view`:
     *  - And if `r` is a `contiguous_range`:
     *    - `str` is set to point inside `r` with the appropriate length
     *  - if not, returns an error
     *
     * Otherwise, clears `str` by calling `str.clear()`, and then reads the
     * range into `str` as if by repeatedly calling \c str.push_back.
     * `str.reserve` is also required to be present.
     */
    template <typename Range, typename String, typename CharT>
    auto getline(Range&& r, String& str, CharT until)
        -> decltype(detail::getline_impl(
            std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
            str,
            until))
    {
        auto wrapped = detail::wrap(std::forward<Range>(r));
        return getline_impl(wrapped, str, until);
    }

    /**
     * \ingroup scanning_operations
     * Equivalent to \ref getline with the last parameter set to <tt>'\\n'</tt>
     * with the appropriate character type.
     *
     * In other words, reads `r` into `str` until <tt>'\\n'</tt> is found.
     *
     * The character type is determined by `r`.
     */
    template <typename Range,
              typename String,
              typename CharT =
                  typename detail::extract_char_type<detail::ranges::iterator_t<
                      detail::range_wrapper_for_t<Range>>>::type>
    auto getline(Range&& r, String& str) -> decltype(
        getline(std::forward<Range>(r), str, detail::ascii_widen<CharT>('\n')))
    {
        return getline(std::forward<Range>(r), str,
                       detail::ascii_widen<CharT>('\n'));
    }

    // ignore

    namespace detail {
        template <typename CharT>
        struct ignore_iterator {
            using value_type = CharT;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::output_iterator_tag;

            constexpr ignore_iterator() = default;

            constexpr const ignore_iterator& operator=(CharT) const noexcept
            {
                return *this;
            }

            constexpr const ignore_iterator& operator*() const noexcept
            {
                return *this;
            }
            constexpr const ignore_iterator& operator++() const noexcept
            {
                return *this;
            }
        };

        template <typename CharT>
        struct ignore_iterator_n {
            using value_type = CharT;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::output_iterator_tag;

            ignore_iterator_n() = default;
            ignore_iterator_n(difference_type n) : i(n) {}

            constexpr const ignore_iterator_n& operator=(CharT) const noexcept
            {
                return *this;
            }

            constexpr const ignore_iterator_n& operator*() const noexcept
            {
                return *this;
            }

            SCN_CONSTEXPR14 ignore_iterator_n& operator++() noexcept
            {
                ++i;
                return *this;
            }

            constexpr bool operator==(const ignore_iterator_n& o) const noexcept
            {
                return i == o.i;
            }
            constexpr bool operator!=(const ignore_iterator_n& o) const noexcept
            {
                return !(*this == o);
            }

            difference_type i{0};
        };

        template <typename WrappedRange,
                  typename CharT = typename detail::extract_char_type<
                      detail::range_wrapper_for_t<
                          typename WrappedRange::iterator>>::type>
        auto ignore_until_impl(WrappedRange& r, CharT until)
            -> scan_result<WrappedRange, wrapped_error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            ignore_iterator<CharT> it{};
            auto e = read_until_space(r, it, until_pred, false);
            if (!e) {
                return {std::move(e), r.get_return()};
            }
            return {{}, r.get_return()};
        }

        template <typename WrappedRange,
                  typename CharT = typename detail::extract_char_type<
                      detail::range_wrapper_for_t<
                          typename WrappedRange::iterator>>::type>
        auto ignore_until_n_impl(WrappedRange& r,
                                 ranges::range_difference_t<WrappedRange> n,
                                 CharT until)
            -> scan_result<WrappedRange, wrapped_error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            ignore_iterator_n<CharT> begin{}, end{n};
            auto e = read_until_space_ranged(r, begin, end, until_pred, false);
            if (!e) {
                return {std::move(e), r.get_return()};
            }
            return {{}, r.get_return()};
        }
    }  // namespace detail

    /**
     * \ingroup scanning_operations
     *
     * Advances the beginning of \c r until \c until is found.
     * The character type of \c r must be \c CharT.
     */
    template <typename Range, typename CharT>
    auto ignore_until(Range&& r, CharT until)
        -> decltype(detail::ignore_until_impl(
            std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
            until))
    {
        auto wrapped = detail::wrap(std::forward<Range>(r));
        auto ret = detail::ignore_until_impl(wrapped, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
    }

    /**
     * \ingroup scanning_operations
     *
     * Advances the beginning of \c r until \c until is found, or the beginning
     * has been advanced \c n times. The character type of \c r must be \c
     * CharT.
     */
    template <typename Range, typename CharT>
    auto ignore_until_n(Range&& r,
                        detail::ranges::range_difference_t<Range> n,
                        CharT until)
        -> decltype(detail::ignore_until_n_impl(
            std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
            n,
            until))
    {
        auto wrapped = detail::wrap(std::forward<Range>(r));
        auto ret = detail::ignore_until_n_impl(wrapped, n, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
    }

    /**
     * \ingroup scanning_operations
     *
     * Reads values repeatedly from `r` and writes them into `c`.
     * The values read are of type `Container::value_type`, and they are written
     * into `c` using `c.push_back`. The values must be separated by whitespace,
     * and by a separator character `separator`, if specified. If `separator ==
     * 0`, no separator character is expected.
     */
    template <typename Range, typename Container, typename CharT>
    auto scan_list(Range&& r, Container& c, CharT separator)
        -> detail::scan_result_for_range_t<Range, wrapped_error>
    {
        using value_type = typename Container::value_type;
        using range_type = detail::range_wrapper_for_t<Range>;
        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_empty_parse_context<typename context_type::locale_type>;

        value_type value;
        auto args = make_args<context_type, parse_context_type>(value);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)));

        while (true) {
            auto pctx = parse_context_type(1, ctx);
            auto ret = vscan(ctx, pctx, {args});
            if (!ret) {
                if (ret.error() == error::end_of_range) {
                    break;
                }
                return {ret.error(), ctx.range().get_return()};
            }
            c.push_back(std::move(value));

            if (separator != 0) {
                auto sep_ret = read_char(ctx.range());
                if (!sep_ret) {
                    if (sep_ret.error() == scn::error::end_of_range) {
                        break;
                    }
                    return {sep_ret.error(), ctx.range().get_return()};
                }
                if (sep_ret.value() == separator) {
                    continue;
                }
                else {
                    return {error(error::invalid_scanned_value,
                                  "Invalid separator character"),
                            ctx.range().get_return()};
                }
            }
        }
        return {{}, ctx.range().get_return()};
    }

    /**
     * \defgroup convenience_scan_types Convenience scannable types
     * This category has types and factory functions, that can be passed as
     * arguments to `scn::scan` (or alike), providing various functionality.
     */

    template <typename T>
    struct discard_type {
        discard_type() = default;
    };

    /**
     * \ingroup convenience_scan_types
     *
     * Scans an instance of `T`, but doesn't store it anywhere.
     * Uses `scn::temp` internally, so the user doesn't have to bother.
     *
     * \code{.cpp}
     * int i{};
     * // 123 is discarded, 456 is read into `i`
     * auto ret = scn::scan("123 456", "{} {}",
     *     discard<T>(), i);
     * // ret == true
     * // ret.value() == 2
     * // i == 456
     * \endcode
     */
    template <typename T>
    discard_type<T>& discard()
    {
        return temp(discard_type<T>{})();
    }

    template <typename CharT, typename T>
    struct scanner<CharT, discard_type<T>> : public scanner<CharT, T> {
        template <typename Context>
        error scan(discard_type<T>&, Context& ctx)
        {
            T tmp;
            return scanner<CharT, T>::scan(tmp, ctx);
        }
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_SCAN_H
