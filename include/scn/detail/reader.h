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

#ifndef SCN_DETAIL_READER_H
#define SCN_DETAIL_READER_H

#include "args.h"
#include "locale.h"
#include "range.h"
#include "result.h"
#include "small_vector.h"
#include "span.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    // read_char

    /// @{
    /**
     * Reads a single character from the range.
     * If `r.begin() == r.end()`, returns EOF.
     * Dereferences the begin iterator, wrapping it in an `expected` if
     * necessary.
     * If the reading was successful, and `advance` is `true`, the range is
     * advanced by a single character.
     */
    template <typename WrappedRange,
              typename std::enable_if<WrappedRange::is_direct>::type* = nullptr>
    expected<ranges::range_value_t<WrappedRange>> read_char(WrappedRange& r,
                                                            bool advance = true)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        auto ch = *r.begin();
        if (advance) {
            r.advance();
        }
        return {ch};
    }
    template <
        typename WrappedRange,
        typename std::enable_if<!WrappedRange::is_direct>::type* = nullptr>
    ranges::range_value_t<WrappedRange> read_char(WrappedRange& r,
                                                  bool advance = true)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        auto ch = *r.begin();
        if (advance && ch) {
            r.advance();
        }
        return ch;
    }
    /// @}

    // read_zero_copy

    /// @{
    /**
     * Reads up to `n` characters from `r`, and returns a `span` into the range.
     * If `r.begin() == r.end()`, returns EOF.
     * If the range does not satisfy `contiguous_range`, returns an empty
     * `span`.
     *
     * Let `count` be `min(r.size(), n)`.
     * Returns a span pointing to `r.data()` with the length `count`.
     * Advances the range by `count` characters.
     */
    template <
        typename WrappedRange,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_zero_copy(WrappedRange& r, ranges::range_difference_t<WrappedRange> n)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        const auto n_to_read = detail::min(r.size(), n);
        auto s = make_span(r.data(), static_cast<size_t>(n_to_read)).as_const();
        r.advance(n_to_read);
        return s;
    }
    template <
        typename WrappedRange,
        typename std::enable_if<!WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_zero_copy(WrappedRange& r, ranges::range_difference_t<WrappedRange>)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        return span<const typename detail::extract_char_type<
            typename WrappedRange::iterator>::type>{};
    }
    /// @}

    // read_all_zero_copy

    /// @{
    /**
     * Reads every character from `r`, and returns a `span` into the range.
     * If `r.begin() == r.end()`, returns EOF.
     * If the range does not satisfy `contiguous_range`, returns an empty
     * `span`.
     */
    template <
        typename WrappedRange,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_all_zero_copy(WrappedRange& r)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        auto s = make_span(r.data(), static_cast<size_t>(r.size())).as_const();
        r.advance(r.size());
        return s;
    }
    template <
        typename WrappedRange,
        typename std::enable_if<!WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_all_zero_copy(WrappedRange& r)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        return span<const typename detail::extract_char_type<
            typename WrappedRange::iterator>::type>{};
    }
    /// @}

    // read_into

    /// @{
    /**
     * Reads `n` characters from `r` into `it`.
     * If `r.begin() == r.end()` in the beginning or before advancing `n`
     * characters, returns EOF. If `r` can't be advanced by `n` characters, the
     * range is advanced by an indeterminate amout. If successful, the range is
     * advanced by `n` characters.
     */
    template <
        typename WrappedRange,
        typename OutputIterator,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    error read_into(WrappedRange& r,
                    OutputIterator& it,
                    ranges::range_difference_t<WrappedRange> n)
    {
        auto s = read_zero_copy(r, n);
        if (!s) {
            return s.error();
        }
        if (s.value().ssize() != n) {
            return error(error::end_of_range, "EOF");
        }
        it = std::copy(s.value().begin(), s.value().end(), it);
        return {};
    }
    template <typename WrappedRange,
              typename OutputIterator,
              typename std::enable_if<!WrappedRange::is_contiguous &&
                                      WrappedRange::is_direct>::type* = nullptr>
    error read_into(WrappedRange& r,
                    OutputIterator& it,
                    ranges::range_difference_t<WrappedRange> n)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        for (ranges::range_difference_t<WrappedRange> i = 0; i < n; ++i) {
            if (r.begin() == r.end()) {
                return error(error::end_of_range, "EOF");
            }
            *it = *r.begin();
            ++it;
            r.advance();
        }
        return {};
    }
    template <
        typename WrappedRange,
        typename OutputIterator,
        typename std::enable_if<!WrappedRange::is_contiguous &&
                                !WrappedRange::is_direct>::type* = nullptr>
    error read_into(WrappedRange& r,
                    OutputIterator& it,
                    ranges::range_difference_t<WrappedRange> n)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        for (ranges::range_difference_t<WrappedRange> i = 0; i < n; ++i) {
            if (r.begin() == r.end()) {
                return error(error::end_of_range, "EOF");
            }
            auto tmp = *r.begin();
            if (!tmp) {
                return tmp.error();
            }
            *it = tmp.value();
            r.advance();
        }
        return {};
    }
    /// @}

    // read_until_space_zero_copy

    /// @{
    /**
     * Reads characters from `r` until a space is found (as determined by
     * `is_space`), and returns a `span` into the range.
     * If `r.begin() == r.end()`, returns EOF.
     * If the range does not satisfy `contiguous_range`,
     * returns an empty `span`.
     *
     * \param is_space Predicate taking a character and returning a `bool`.
     *                 `true` means, that the given character is a space.
     * \param keep_final_space Whether the final found space character is
     *                         included in the returned span,
     *                         and is advanced past.
     */
    template <
        typename WrappedRange,
        typename Predicate,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_until_space_zero_copy(WrappedRange& r,
                               Predicate is_space,
                               bool keep_final_space)
    {
        auto it = r.begin();
        const auto end = r.end();
        if (it == end) {
            return error(error::end_of_range, "EOF");
        }
        for (; it != end; ++it) {
            if (is_space(*it)) {
                auto b = r.begin();
                r.advance_to(it);
                if (keep_final_space) {
                    ++it;
                    r.advance();
                }
                return {{&*b, &*it}};
            }
        }
        auto b = r.begin();
        r.advance_to(r.end());
        return {{&*b, &*(r.end() - 1) + 1}};
    }
    template <
        typename WrappedRange,
        typename Predicate,
        typename std::enable_if<!WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_until_space_zero_copy(WrappedRange& r, Predicate, bool)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        return span<const typename detail::extract_char_type<
            typename WrappedRange::iterator>::type>{};
    }
    /// @}

    // read_until_space

    /// @{

    /**
     * Reads characters from `r` until a space is found (as determined by
     * `is_space`) and writes them into `out`.
     * If `r.begin() == r.end()`, returns EOF.
     *
     * \param is_space Predicate taking a character and returning a `bool`.
     *                 `true` means, that the given character is a space.
     * \param keep_final_space Whether the final found space character is
     *                         written into `out` and is advanced past.
     */
    template <
        typename WrappedRange,
        typename OutputIterator,
        typename Predicate,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    error read_until_space(WrappedRange& r,
                           OutputIterator& out,
                           Predicate is_space,
                           bool keep_final_space)
    {
        auto s = read_until_space_zero_copy(r, is_space, keep_final_space);
        if (!s) {
            return s.error();
        }
        out = std::copy(s.value().begin(), s.value().end(), out);
        return {};
    }
    template <typename WrappedRange,
              typename OutputIterator,
              typename Predicate,
              typename std::enable_if<!WrappedRange::is_contiguous &&
                                      WrappedRange::is_direct>::type* = nullptr>
    error read_until_space(WrappedRange& r,
                           OutputIterator& out,
                           Predicate is_space,
                           bool keep_final_space)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        while (r.begin() != r.end()) {
            const auto ch = *r.begin();
            if (is_space(ch)) {
                if (keep_final_space) {
                    *out = ch;
                    ++out;
                }
                return {};
            }
            *out = ch;
            ++out;
            r.advance();
        }
        return {};
    }
    template <
        typename WrappedRange,
        typename OutputIterator,
        typename Predicate,
        typename std::enable_if<!WrappedRange::is_contiguous &&
                                !WrappedRange::is_direct>::type* = nullptr>
    error read_until_space(WrappedRange& r,
                           OutputIterator& out,
                           Predicate is_space,
                           bool keep_final_space)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        while (r.begin() != r.end()) {
            auto tmp = *r.begin();
            if (!tmp) {
                return tmp.error();
            }
            auto ch = tmp.value();
            if (is_space(ch)) {
                if (keep_final_space) {
                    *out = ch;
                    ++out;
                }
                return {};
            }
            *out = ch;
            ++out;

            r.advance();
        }
        return {};
    }

    /// @}

    // read_until_space_ranged

    /// @{

    /**
     * Reads characters from `r` until a space is found (as determined by
     * `is_space`), or `out` reaches `end`, and writes them into `out`.
     * If `r.begin() == r.end()`, returns EOF.
     *
     * \param is_space Predicate taking a character and returning a `bool`.
     *                 `true` means, that the given character is a space.
     * \param keep_final_space Whether the final found space character is
     *                         written into `out` and is advanced past.
     */
    template <typename WrappedRange,
              typename OutputIterator,
              typename Sentinel,
              typename Predicate,
              typename std::enable_if<WrappedRange::is_direct>::type* = nullptr>
    error read_until_space_ranged(WrappedRange& r,
                                  OutputIterator& out,
                                  Sentinel end,
                                  Predicate is_space,
                                  bool keep_final_space)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        for (auto it = r.begin(); it != r.end() && out != end;
             ++it, (void)r.advance()) {
            const auto ch = *it;
            if (is_space(ch)) {
                if (keep_final_space) {
                    *out = ch;
                    ++out;
                }
                return {};
            }
            *out = ch;
            ++out;
        }
        return {};
    }
    template <
        typename WrappedRange,
        typename OutputIterator,
        typename Sentinel,
        typename Predicate,
        typename std::enable_if<!WrappedRange::is_direct>::type* = nullptr>
    error read_until_space_ranged(WrappedRange& r,
                                  OutputIterator& out,
                                  Sentinel end,
                                  Predicate is_space,
                                  bool keep_final_space)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        for (auto it = r.begin(); it != r.end() && out != end;
             ++it, (void)r.advance()) {
            auto tmp = *it;
            if (!tmp) {
                return tmp.error();
            }
            auto ch = tmp.value();
            if (is_space(ch)) {
                if (keep_final_space) {
                    *out = ch;
                    ++out;
                }
                return {};
            }
            *out = ch;
            ++out;
        }
        return {};
    }

    /// @}

    // putback_n

    /// @{

    /**
     * Puts back `n` characters into `r` as if by repeatedly calling
     * `r.advance(-1)` .
     */
    template <
        typename WrappedRange,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    error putback_n(WrappedRange& r, ranges::range_difference_t<WrappedRange> n)
    {
        SCN_EXPECT(n <= ranges::distance(r.begin_underlying(), r.begin()));
        r.advance(-n);
        return {};
    }
    template <
        typename WrappedRange,
        typename std::enable_if<!WrappedRange::is_contiguous>::type* = nullptr>
    error putback_n(WrappedRange& r, ranges::range_difference_t<WrappedRange> n)
    {
        for (ranges::range_difference_t<WrappedRange> i = 0; i < n; ++i) {
            r.advance(-1);
            if (r.begin() == r.end()) {
                return error(error::unrecoverable_source_error,
                             "Putback failed");
            }
        }
        return {};
    }

    /// @}

    struct empty_parser {
        template <typename ParseCtx>
        error parse(ParseCtx& pctx)
        {
            pctx.arg_begin();
            if (SCN_UNLIKELY(!pctx)) {
                return error(error::invalid_format_string,
                             "Unexpected format string end");
            }
            if (!pctx.check_arg_end()) {
                return error(error::invalid_format_string,
                             "Expected argument end");
            }
            pctx.arg_end();
            return {};
        }
    };

    namespace detail {
        struct char_scanner {
            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                using char_type = typename ParseCtx::char_type;

                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }
                if (pctx.next() == detail::ascii_widen<char_type>('c')) {
                    pctx.advance();
                }
                if (!pctx.check_arg_end()) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(typename Context::char_type& val, Context& ctx)
            {
                auto ch = read_char(ctx.range());
                if (!ch) {
                    return ch.error();
                }
                val = ch.value();
                return {};
            }
        };

        struct buffer_scanner : public empty_parser {
            template <typename Context>
            error scan(span<typename Context::char_type>& val, Context& ctx)
            {
                using char_type = typename Context::char_type;

                if (val.size() == 0) {
                    return {};
                }

                auto s = read_zero_copy(ctx.range(), val.ssize());
                if (!s) {
                    return s.error();
                }
                if (s.value().size() != 0) {
                    if (s.value().size() != val.size()) {
                        return error(error::end_of_range, "EOF");
                    }
                    std::memcpy(val.begin(), s.value().begin(),
                                s.value().size() * sizeof(char_type));
                    return {};
                }

                small_vector<char_type, 32> buf(val.size());
                auto it = buf.begin();
                auto e = read_into(ctx.range(), it, val.ssize());
                if (!e) {
                    return e;
                }
                buf.erase(it);
                std::memcpy(val.begin(), buf.begin(),
                            buf.size() * sizeof(char_type));
                return {};
            }
        };

        struct bool_scanner {
            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                // {}: no boolalpha, not localized
                // l: localized
                // a: boolalpha
                using char_type = typename ParseCtx::char_type;

                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }

                bool a = false, n = false;
                for (auto ch = pctx.next(); pctx && !pctx.check_arg_end();
                     pctx.advance(), ch = pctx.next()) {
                    if (ch == detail::ascii_widen<char_type>('l')) {
                        localized = true;
                    }
                    else if (ch == detail::ascii_widen<char_type>('a')) {
                        a = true;
                    }
                    else if (ch == detail::ascii_widen<char_type>('n')) {
                        n = true;
                    }
                    else {
                        break;
                    }
                }
                if (a || n) {
                    allow_alpha = a;
                    allow_num = n;
                }

                if (SCN_UNLIKELY(localized && !allow_alpha)) {
                    return error(error::invalid_format_string,
                                 "boolalpha-mode cannot be disabled with 'l' "
                                 "(localized) specifier with bool");
                }

                if (pctx.next() == detail::ascii_widen<char_type>('b')) {
                    pctx.advance();
                }
                if (!pctx.check_arg_end()) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(bool& val, Context& ctx)
            {
                using char_type = typename Context::char_type;

                if (allow_alpha) {
                    auto truename = locale_defaults<char_type>::truename();
                    auto falsename = locale_defaults<char_type>::falsename();
                    if (localized) {
                        truename = ctx.locale().truename();
                        falsename = ctx.locale().falsename();
                    }
                    const auto max_len =
                        detail::max(truename.size(), falsename.size());
                    std::basic_string<char_type> buf;
                    buf.reserve(max_len);

                    auto tmp_it = std::back_inserter(buf);
                    auto e = read_until_space(
                        ctx.range(), tmp_it,
                        [&ctx](char_type ch) {
                            return ctx.locale().is_space(ch);
                        },
                        false);
                    if (!e) {
                        return e;
                    }

                    bool found = false;
                    if (buf.size() >= falsename.size()) {
                        if (std::equal(falsename.begin(), falsename.end(),
                                       buf.begin())) {
                            val = false;
                            found = true;
                        }
                    }
                    if (!found && buf.size() >= truename.size()) {
                        if (std::equal(truename.begin(), truename.end(),
                                       buf.begin())) {
                            val = true;
                            found = true;
                        }
                    }
                    if (found) {
                        return {};
                    }
                    else {
                        auto pb =
                            putback_n(ctx.range(),
                                      static_cast<std::ptrdiff_t>(buf.size()));
                        if (!pb) {
                            return pb;
                        }
                    }
                }

                if (allow_num) {
                    auto tmp = read_char(ctx.range());
                    if (!tmp) {
                        return tmp.error();
                    }
                    if (tmp.value() == detail::ascii_widen<char_type>('0')) {
                        val = false;
                        return {};
                    }
                    if (tmp.value() == detail::ascii_widen<char_type>('1')) {
                        val = true;
                        return {};
                    }
                    auto pb = putback_n(ctx.range(), 1);
                    if (!pb) {
                        return pb;
                    }
                }

                return error(error::invalid_scanned_value,
                             "Couldn't scan bool");
            }

            bool localized{false};
            bool allow_alpha{true};
            bool allow_num{true};
        };

        template <typename T>
        struct integer_scanner {
            static_assert(std::is_integral<T>::value, "");

            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                // {}: base detect, not localized
                // n: localized decimal/thousand separator
                // l: n + localized digits
                // d: decimal, o: octal, x: hex, b[1-36]: base
                using char_type = typename ParseCtx::char_type;
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }

                bool base_set = false;
                bool loc_set = false;
                for (auto ch = pctx.next(); pctx && !pctx.check_arg_end();
                     pctx.advance(), ch = pctx.next()) {
                    if (!base_set) {
                        if (ch == detail::ascii_widen<char_type>('d')) {
                            base = 10;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<char_type>('x')) {
                            base = 16;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<char_type>('o')) {
                            base = 8;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<char_type>('i')) {
                            if (std::is_unsigned<T>::value) {
                                return error(
                                    error::invalid_format_string,
                                    "'i' format specifier expects signed "
                                    "integer argument");
                            }
                            base = 0;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<char_type>('u')) {
                            if (std::is_signed<T>::value) {
                                return error(
                                    error::invalid_format_string,
                                    "'u' format specifier expects unsigned "
                                    "integer argument");
                            }
                            base = 0;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<char_type>('b')) {
                            pctx.advance();
                            if (SCN_UNLIKELY(!pctx)) {
                                return error(error::invalid_format_string,
                                             "Unexpected format string end");
                            }
                            if (SCN_UNLIKELY(pctx.check_arg_end())) {
                                return error(error::invalid_format_string,
                                             "Unexpected argument end");
                            }
                            ch = pctx.next();

                            const auto zero =
                                           detail::ascii_widen<char_type>('0'),
                                       nine =
                                           detail::ascii_widen<char_type>('9');
                            int tmp = 0;
                            if (ch < zero || ch > nine) {
                                return error(error::invalid_format_string,
                                             "Invalid character after 'b', "
                                             "expected digit");
                            }
                            tmp = pctx.next() - zero;
                            if (tmp < 1) {
                                return error(
                                    error::invalid_format_string,
                                    "Invalid base, must be between 1 and 36");
                            }

                            pctx.advance();
                            ch = pctx.next();

                            if (pctx.check_arg_end()) {
                                base = tmp;
                                break;
                            }
                            if (ch < zero || ch > nine) {
                                return error(error::invalid_format_string,
                                             "Invalid character after 'b', "
                                             "expected digit");
                            }
                            tmp *= 10;
                            tmp += ch - zero;
                            if (tmp < 1 || tmp > 36) {
                                return error(
                                    error::invalid_format_string,
                                    "Invalid base, must be between 1 and 36");
                            }
                            base = tmp;
                            base_set = true;
                            continue;
                        }
                    }

                    if (!loc_set) {
                        if (ch == detail::ascii_widen<char_type>('l')) {
                            localized = thousands_separator | digits;
                            loc_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<char_type>('n')) {
                            localized = thousands_separator;
                            loc_set = true;
                            continue;
                        }
                    }

                    if (!have_thsep) {
                        if (ch == detail::ascii_widen<char_type>('\'')) {
                            have_thsep = true;
                            continue;
                        }
                    }

                    return error(error::invalid_format_string,
                                 "Unexpected character in format string");
                }

                if (localized && (base != 0 && base != 10)) {
                    return error(
                        error::invalid_format_string,
                        "Localized integers can only be scanned in base 10");
                }
                if (!pctx.check_arg_end()) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                using char_type = typename Context::char_type;
                auto do_parse_int = [&](span<const char_type> s) -> error {
                    T tmp = 0;
                    expected<std::ptrdiff_t> ret{0};
                    if (SCN_UNLIKELY((localized & digits) != 0)) {
                        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                        std::basic_string<char_type> str(s.data(), s.size());
                        ret = ctx.locale().read_num(tmp, str);
                        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    }
                    else {
                        ret = _parse_int(tmp, s,
                                         ctx.locale().thousands_separator());
                    }

                    if (!ret) {
                        return ret.error();
                    }
                    if (ret.value() != s.ssize()) {
                        auto pb =
                            putback_n(ctx.range(), s.ssize() - ret.value());
                        if (!pb) {
                            return pb;
                        }
                    }
                    val = tmp;
                    return {};
                };

                auto is_space_pred = [&ctx](char_type ch) {
                    return ctx.locale().is_space(ch);
                };

                if (Context::range_type::is_contiguous) {
                    auto s = read_all_zero_copy(ctx.range());
                    if (!s) {
                        return s.error();
                    }
                    return do_parse_int(s.value());
                }

                small_vector<char_type, 32> buf;
                auto outputit = std::back_inserter(buf);
                auto e = read_until_space(ctx.range(), outputit, is_space_pred,
                                          false);
                if (!e && buf.empty()) {
                    return e;
                }

                return do_parse_int(make_span(buf).as_const());
            }

            enum localized_type : uint8_t {
                thousands_separator = 1,
                digits = 2
            };

            int base{0};
            uint8_t localized{0};
            bool have_thsep{false};

            template <typename CharT>
            expected<std::ptrdiff_t> _parse_int(T& val,
                                                span<const CharT> s,
                                                CharT thsep)
            {
                SCN_MSVC_PUSH
                SCN_MSVC_IGNORE(4244)
                SCN_MSVC_IGNORE(4127)  // conditional expression is constant

                if (std::is_unsigned<T>::value) {
                    if (s[0] == detail::ascii_widen<CharT>('-')) {
                        return error(error::value_out_of_range,
                                     "Unexpected sign '-' when scanning an "
                                     "unsigned integer");
                    }
                }

                SCN_MSVC_POP

                T tmp = 0;
                bool minus_sign = false;
                auto it = s.begin();

                if (s[0] == ascii_widen<CharT>('-') ||
                    s[0] == ascii_widen<CharT>('+')) {
                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wsign-conversion")
                    minus_sign = s[0] == ascii_widen<CharT>('-');
                    ++it;
                    SCN_GCC_POP
                }
                if (SCN_UNLIKELY(it == s.end())) {
                    return error(error::invalid_scanned_value,
                                 "Expected number after sign");
                }

                if (*it == ascii_widen<CharT>('0')) {
                    ++it;
                    if (it == s.end()) {
                        val = 0;
                        return ranges::distance(s.begin(), it);
                    }
                    if (*it == ascii_widen<CharT>('x') ||
                        *it == ascii_widen<CharT>('X')) {
                        if (SCN_UNLIKELY(base != 0 && base != 16)) {
                            val = 0;
                            return ranges::distance(s.begin(), it);
                        }
                        ++it;
                        if (SCN_UNLIKELY(it == s.end())) {
                            --it;
                            val = 0;
                            return ranges::distance(s.begin(), it);
                        }
                        if (base == 0) {
                            base = 16;
                        }
                    }
                    else if (base == 0) {
                        base = 8;
                    }
                }
                if (base == 0) {
                    base = 10;
                }

                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                SCN_GCC_IGNORE("-Wsign-conversion")
                SCN_GCC_IGNORE("-Wsign-compare")

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wconversion")
                SCN_CLANG_IGNORE("-Wsign-conversion")
                SCN_CLANG_IGNORE("-Wsign-compare")

                SCN_ASSUME(base > 0);

                auto r = _read_int(tmp, minus_sign,
                                   make_span(it, s.end()).as_const(), thsep);
                if (!r) {
                    return r.error();
                }
                it = r.value();
                if (s.begin() == it) {
                    return error(error::invalid_scanned_value,
                                 "custom::read_int");
                }
                val = tmp;
                return ranges::distance(s.begin(), it);

                SCN_CLANG_POP
                SCN_GCC_POP
            }

            template <typename CharT>
            expected<typename span<const CharT>::iterator> _read_int(
                T& val,
                bool minus_sign,
                span<const CharT> buf,
                CharT thsep) const
            {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                SCN_GCC_IGNORE("-Wsign-conversion")
                SCN_GCC_IGNORE("-Wsign-compare")

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wconversion")
                SCN_CLANG_IGNORE("-Wsign-conversion")
                SCN_CLANG_IGNORE("-Wsign-compare")

                SCN_MSVC_PUSH
                SCN_MSVC_IGNORE(4018)  // > signed/unsigned mismatch
                SCN_MSVC_IGNORE(4389)  // == signed/unsigned mismatch
                SCN_MSVC_IGNORE(4244)  // lossy conversion

                using utype = typename std::make_unsigned<T>::type;

                const auto ubase = static_cast<utype>(base);
                SCN_ASSUME(ubase > 0);

                constexpr auto uint_max = static_cast<utype>(-1);
                constexpr auto int_max = static_cast<utype>(uint_max >> 1);
                constexpr auto abs_int_min = static_cast<utype>(int_max + 1);

                const auto cut = div(
                    [&]() -> utype {
                        if (std::is_signed<T>::value) {
                            if (minus_sign) {
                                return abs_int_min;
                            }
                            return int_max;
                        }
                        return uint_max;
                    }(),
                    ubase);
                const auto cutoff = cut.first;
                const auto cutlim = cut.second;

                auto it = buf.begin();
                const auto end = buf.end();
                if (SCN_UNLIKELY(have_thsep)) {
                    for (; it != end; ++it) {
                        if (*it == thsep) {
                            continue;
                        }

                        const auto digit = _char_to_int(*it);
                        if (digit >= ubase) {
                            break;
                        }
                        if (SCN_UNLIKELY(val > cutoff ||
                                         (val == cutoff && digit > cutlim))) {
                            if (!minus_sign) {
                                return error(error::value_out_of_range,
                                             "Out of range: integer overflow");
                            }
                            return error(error::value_out_of_range,
                                         "Out of range: integer underflow");
                        }
                        val = val * ubase + digit;
                    }
                }
                else {
                    for (; it != end; ++it) {
                        const auto digit = _char_to_int(*it);
                        if (digit >= ubase) {
                            break;
                        }
                        if (SCN_UNLIKELY(val > cutoff ||
                                         (val == cutoff && digit > cutlim))) {
                            if (!minus_sign) {
                                return error(error::value_out_of_range,
                                             "Out of range: integer overflow");
                            }
                            return error(error::value_out_of_range,
                                         "Out of range: integer underflow");
                        }
                        val = val * ubase + digit;
                    }
                }
                val = val * (minus_sign ? -1 : 1);
                return it;

                SCN_MSVC_POP
                SCN_CLANG_POP
                SCN_GCC_POP
            }

            unsigned char _char_to_int(char ch) const
            {
                static constexpr unsigned char digits_arr[] = {
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
                    255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,
                    17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
                    29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255,
                    255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
                    21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
                    33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255};
                return digits_arr[static_cast<unsigned char>(ch)];
            }
            unsigned char _char_to_int(wchar_t ch) const
            {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                if (ch >= std::numeric_limits<char>::min() &&
                    ch <= std::numeric_limits<char>::max()) {
                    return _char_to_int(static_cast<char>(ch));
                }
                return 255;
                SCN_GCC_POP
            }
        };

        template <typename T>
        struct float_scanner {
            static_assert(std::is_floating_point<T>::value, "");

            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                // {}: not localized
                // l: localized
                using char_type = typename ParseCtx::char_type;
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }

                if (pctx.check_arg_end()) {
                    pctx.arg_end();
                    return {};
                }

                if (pctx.next() == detail::ascii_widen<char_type>('l')) {
                    localized = true;
                    pctx.advance();
                }

                if (pctx.check_arg_end()) {
                    pctx.arg_end();
                    return {};
                }

                if (pctx.next() == detail::ascii_widen<char_type>('a')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<char_type>('A')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<char_type>('e')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<char_type>('E')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<char_type>('f')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<char_type>('F')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<char_type>('g')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<char_type>('G')) {
                    pctx.advance();
                }

                if (!pctx.check_arg_end()) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                using char_type = typename Context::char_type;
                auto do_parse_float = [&](span<const char_type> s) -> error {
                    T tmp = 0;
                    expected<std::ptrdiff_t> ret{0};
                    if (SCN_UNLIKELY(localized)) {
                        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                        std::basic_string<char_type> str(s.data(), s.size());
                        ret = ctx.locale().read_num(tmp, str);
                        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    }
                    else {
                        ret = _read_float(tmp, s);
                    }

                    if (!ret) {
                        return ret.error();
                    }
                    if (ret.value() != s.ssize()) {
                        auto pb =
                            putback_n(ctx.range(), s.ssize() - ret.value());
                        if (!pb) {
                            return pb;
                        }
                    }
                    val = tmp;
                    return {};
                };

                auto is_space_pred = [&ctx](char_type ch) {
                    return ctx.locale().is_space(ch);
                };

                if (Context::range_type::is_contiguous) {
                    auto s = read_until_space_zero_copy(ctx.range(),
                                                        is_space_pred, false);
                    if (!s) {
                        return s.error();
                    }
                    return do_parse_float(s.value());
                }

                small_vector<char_type, 32> buf;
                auto outputit = std::back_inserter(buf);
                auto e = read_until_space(ctx.range(), outputit, is_space_pred,
                                          false);
                if (!e && buf.empty()) {
                    return e;
                }

                return do_parse_float(make_span(buf).as_const());
            }

            bool localized{false};

            template <typename CharT>
            expected<std::ptrdiff_t> _read_float(T& val, span<const CharT> s)
            {
                size_t chars{};
                std::basic_string<CharT> str(s.data(), s.size());
                SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                auto ret = _read_float_impl(str.data(), chars);
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                if (!ret) {
                    return ret.error();
                }
                val = ret.value();
                return static_cast<std::ptrdiff_t>(chars);
            }

            template <typename CharT>
            expected<T> _read_float_impl(const CharT* str, size_t& chars);
        };

        struct string_scanner {
            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                using char_type = typename ParseCtx::char_type;
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }
                if (pctx.next() == detail::ascii_widen<char_type>('s')) {
                    pctx.advance();
                }
                if (!pctx.check_arg_end()) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context, typename Allocator>
            error scan(
                std::basic_string<typename Context::char_type,
                                  std::char_traits<typename Context::char_type>,
                                  Allocator>& val,
                Context& ctx)
            {
                using char_type = typename Context::char_type;
                using string_type =
                    std::basic_string<char_type, std::char_traits<char_type>,
                                      Allocator>;

                auto is_space_pred = [&ctx](char_type ch) {
                    return ctx.locale().is_space(ch);
                };

                if (Context::range_type::is_contiguous) {
                    auto s = read_until_space_zero_copy(ctx.range(),
                                                        is_space_pred, false);
                    if (!s) {
                        return s.error();
                    }
                    val.assign(s.value().data(), s.value().size());
                    return {};
                }

                string_type tmp(val.get_allocator());
                auto outputit = std::back_inserter(tmp);
                auto ret = read_until_space(ctx.range(), outputit,
                                            is_space_pred, false);
                if (SCN_UNLIKELY(!ret)) {
                    return ret;
                }
                if (SCN_UNLIKELY(tmp.empty())) {
                    return error(error::invalid_scanned_value,
                                 "Empty string parsed");
                }
                val = std::move(tmp);

                return {};
            }
        };

        struct string_view_scanner : string_scanner {
        public:
            template <typename Context>
            error scan(basic_string_view<typename Context::char_type>& val,
                       Context& ctx)
            {
                using char_type = typename Context::char_type;
                auto is_space_pred = [&ctx](char_type ch) {
                    return ctx.locale().is_space(ch);
                };
                if (!Context::range_type::is_contiguous) {
                    return error(error::invalid_operation,
                                 "Cannot read a string_view from a "
                                 "non-contiguous_range");
                }
                auto s = read_until_space_zero_copy(ctx.range(), is_space_pred,
                                                    false);
                if (!s) {
                    return s.error();
                }
                val = basic_string_view<char_type>(s.value().data(),
                                                   s.value().size());
                return {};
            }
        };

#if SCN_HAS_STRING_VIEW
        struct std_string_view_scanner : string_view_scanner {
            template <typename Context>
            error scan(std::basic_string_view<typename Context::char_type>& val,
                       Context& ctx)
            {
                using char_type = typename Context::char_type;
                auto sv =
                    ::scn::basic_string_view<char_type>(val.data(), val.size());
                auto e = string_view_scanner::scan(sv, ctx);
                if (e) {
                    val =
                        std::basic_string_view<char_type>(sv.data(), sv.size());
                }
                return e;
            }
        };
#endif
    }  // namespace detail

    template <typename CharT>
    struct scanner<CharT, CharT> : public detail::char_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, span<CharT>> : public detail::buffer_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, bool> : public detail::bool_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, short> : public detail::integer_scanner<short> {
    };
    template <typename CharT>
    struct scanner<CharT, int> : public detail::integer_scanner<int> {
    };
    template <typename CharT>
    struct scanner<CharT, long> : public detail::integer_scanner<long> {
    };
    template <typename CharT>
    struct scanner<CharT, long long>
        : public detail::integer_scanner<long long> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned short>
        : public detail::integer_scanner<unsigned short> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned int>
        : public detail::integer_scanner<unsigned int> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned long>
        : public detail::integer_scanner<unsigned long> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned long long>
        : public detail::integer_scanner<unsigned long long> {
    };
    template <typename CharT>
    struct scanner<CharT, float> : public detail::float_scanner<float> {
    };
    template <typename CharT>
    struct scanner<CharT, double> : public detail::float_scanner<double> {
    };
    template <typename CharT>
    struct scanner<CharT, long double>
        : public detail::float_scanner<long double> {
    };
    template <typename CharT, typename Allocator>
    struct scanner<CharT,
                   std::basic_string<CharT, std::char_traits<CharT>, Allocator>>
        : public detail::string_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, basic_string_view<CharT>>
        : public detail::string_view_scanner {
    };
#if SCN_HAS_STRING_VIEW
    template <typename CharT>
    struct scanner<CharT, std::basic_string_view<CharT>>
        : public detail::std_string_view_scanner {
    };
#endif
    template <typename CharT>
    struct scanner<CharT, detail::monostate>;

    /// @{

    /**
     * Reads from the range in `ctx` as if by repeatedly calling `read_char()`,
     * until a non-space character is found (as determined by `ctx.locale()`),
     * or EOF is reached. That non-space character is then put back into the
     * range.
     */
    template <typename Context,
              typename std::enable_if<
                  !Context::range_type::is_contiguous>::type* = nullptr>
    error skip_range_whitespace(Context& ctx) noexcept
    {
        while (true) {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

            auto ch = read_char(ctx.range());
            if (SCN_UNLIKELY(!ch)) {
                return ch.error();
            }
            if (!ctx.locale().is_space(ch.value())) {
                auto pb = putback_n(ctx.range(), 1);
                if (SCN_UNLIKELY(!pb)) {
                    return pb;
                }
                break;
            }

            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }
        return {};
    }
    template <typename Context,
              typename std::enable_if<
                  Context::range_type::is_contiguous>::type* = nullptr>
    error skip_range_whitespace(Context& ctx) noexcept
    {
        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

        const auto end = ctx.range().end();
        for (auto it = ctx.range().begin(); it != end; ++it) {
            if (!ctx.locale().is_space(*it)) {
                ctx.range().advance_to(it);
                return {};
            }
        }
        ctx.range().advance_to(end);
        return {};

        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
    }

    /// @}

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_READER_CPP)
#include "reader.cpp"
#endif

#endif  // SCN_DETAIL_READER_H
