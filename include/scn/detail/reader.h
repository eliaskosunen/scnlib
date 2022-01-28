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
        auto chars_to_read = detail::min(n, r.size());
        const bool incomplete = chars_to_read != n;
        auto s = read_zero_copy(r, chars_to_read);
        if (!s) {
            return s.error();
        }
        it = std::copy(s.value().begin(), s.value().end(), it);
        if (incomplete) {
            return error(error::end_of_range, "EOF");
        }
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

    struct common_parser {
        template <typename ParseCtx>
        error parse_common_begin(ParseCtx& pctx)
        {
            pctx.arg_begin();
            if (SCN_UNLIKELY(!pctx)) {
                return {error::invalid_format_string,
                        "Unexpected format string end"};
            }
            return {};
        }

        template <typename ParseCtx>
        error parse_common_each(ParseCtx& pctx, bool& parsed)
        {
            using char_type = typename ParseCtx::char_type;
            auto ch = pctx.next();
            parsed = false;

            if ((common_options & localized) == 0) {
                if (ch == detail::ascii_widen<char_type>('L')) {
                    if (SCN_UNLIKELY((common_options & localized) != 0)) {
                        return {error::invalid_format_string,
                                "Repeat 'L' flag in format string"};
                    }
                    common_options |= localized;
                    parsed = true;
                    pctx.advance();
                }
            }

            return {};
        }

        template <typename ParseCtx>
        error parse_common_end(ParseCtx& pctx)
        {
            if (!pctx.check_arg_end()) {
                return {error::invalid_format_string, "Expected argument end"};
            }

            pctx.arg_end();
            return {};
        }

        template <typename ParseCtx>
        static error null_each(ParseCtx&, bool&)
        {
            return {};
        }

        template <typename ParseCtx,
                  typename F,
                  typename CharT = typename ParseCtx::char_type>
        error parse_common(ParseCtx& pctx,
                           span<const CharT> options,
                           span<bool> flags,
                           F&& each)
        {
            SCN_EXPECT(options.size() == flags.size());

            auto e = parse_common_begin(pctx);
            if (!e) {
                return e;
            }

            for (auto ch = pctx.next(); pctx && !pctx.check_arg_end();
                 ch = pctx.next()) {
                bool parsed = false;
                for (std::size_t i = 0; i < options.size() && !parsed; ++i) {
                    if (ch == options[i]) {
                        if (SCN_UNLIKELY(flags[i])) {
                            return {error::invalid_format_string,
                                    "Repeat flag in format string"};
                        }
                        flags[i] = true;
                        parsed = true;
                    }
                }
                if (parsed) {
                    pctx.advance();
                    if (!pctx || pctx.check_arg_end()) {
                        break;
                    }
                    continue;
                }

                e = each(pctx, parsed);
                if (!e) {
                    return e;
                }
                if (parsed) {
                    if (!pctx || pctx.check_arg_end()) {
                        break;
                    }
                    continue;
                }
                ch = pctx.next();

                e = parse_common_each(pctx, parsed);
                if (!e) {
                    return e;
                }
                if (!parsed) {
                    return {error::invalid_format_string,
                            "Invalid character in format string"};
                }
                if (!pctx || pctx.check_arg_end()) {
                    break;
                }
            }

            return parse_common_end(pctx);
        }

        enum common_options_type {
            localized = 1,  // 'L'
        };
        uint8_t common_options{0};
    };

    namespace detail {
        template <typename T>
        struct integer_scanner;

        struct char_scanner : common_parser {
            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                using char_type = typename ParseCtx::char_type;

                auto c_flag = detail::ascii_widen<char_type>('c');
                bool c_set{};
                return parse_common(pctx, span<const char_type>{&c_flag, 1},
                                    span<bool>{&c_set, 1}, null_each<ParseCtx>);
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

        struct buffer_scanner : public ::scn::empty_parser {
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
                        return {error::end_of_range, "EOF"};
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
                std::memcpy(val.begin(), buf.begin(),
                            buf.size() * sizeof(char_type));
                return {};
            }
        };

        template <typename T>
        struct integer_scanner : common_parser {
            static_assert(std::is_integral<T>::value, "");

            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                using char_type = typename ParseCtx::char_type;

                int custom_base = 0;
                auto each = [&](ParseCtx& p, bool& parsed) -> error {
                    parsed = false;
                    auto ch = pctx.next();

                    if (ch == detail::ascii_widen<char_type>('B')) {
                        // Custom base
                        p.advance();
                        if (SCN_UNLIKELY(!p)) {
                            return {error::invalid_format_string,
                                    "Unexpected format string end"};
                        }
                        if (SCN_UNLIKELY(p.check_arg_end())) {
                            return {error::invalid_format_string,
                                    "Unexpected argument end"};
                        }
                        ch = p.next();

                        const auto zero = detail::ascii_widen<char_type>('0'),
                                   nine = detail::ascii_widen<char_type>('9');
                        int tmp = 0;
                        if (ch < zero || ch > nine) {
                            return {error::invalid_format_string,
                                    "Invalid character after 'B', "
                                    "expected digit"};
                        }
                        tmp = p.next() - zero;
                        if (tmp < 1) {
                            return {error::invalid_format_string,
                                    "Invalid base, must be between 2 and 36"};
                        }

                        p.advance();
                        ch = p.next();

                        if (p.check_arg_end()) {
                            custom_base = static_cast<uint8_t>(tmp);
                            parsed = true;
                            return {};
                        }
                        if (ch < zero || ch > nine) {
                            return {error::invalid_format_string,
                                    "Invalid character after 'B', "
                                    "expected digit"};
                        }
                        tmp *= 10;
                        tmp += ch - zero;
                        if (tmp < 2 || tmp > 36) {
                            return {error::invalid_format_string,
                                    "Invalid base, must be between 2 and 36"};
                        }
                        custom_base = static_cast<uint8_t>(tmp);
                        parsed = true;
                        pctx.advance();
                        return {};
                    }

                    return {};
                };

                array<char_type, 8> options{{// decimal
                                             ascii_widen<char_type>('d'),
                                             // binary
                                             ascii_widen<char_type>('b'),
                                             // octal
                                             ascii_widen<char_type>('o'),
                                             // hex
                                             ascii_widen<char_type>('x'),
                                             // detect base
                                             ascii_widen<char_type>('i'),
                                             // unsigned decimal
                                             ascii_widen<char_type>('u'),
                                             // localized digits
                                             ascii_widen<char_type>('n'),
                                             // thsep
                                             ascii_widen<char_type>('\'')}};
                bool flags[8] = {false};

                auto e = parse_common(
                    pctx, span<const char_type>{options.begin(), options.end()},
                    span<bool>{flags, 8}, each);
                if (!e) {
                    return e;
                }

                int base_flags_set = int(flags[0]) + int(flags[1]) +
                                     int(flags[2]) + int(flags[3]) +
                                     int(flags[4]) + int(flags[5]) +
                                     int(custom_base != 0);
                if (SCN_UNLIKELY(base_flags_set > 1)) {
                    return {error::invalid_format_string,
                            "Up to one base flags ('d', 'i', 'u', 'b', 'o', "
                            "'x', 'B') allowed"};
                }
                else if (base_flags_set == 0) {
                    // Default to 'd'
                    base = 10;
                }
                else if (custom_base != 0) {
                    // B__
                    base = static_cast<uint8_t>(custom_base);
                }
                else if (flags[0]) {
                    // 'd' flag
                    base = 10;
                }
                else if (flags[1]) {
                    // 'b' flag
                    base = 2;
                    format_options |= allow_base_prefix;
                }
                else if (flags[2]) {
                    // 'o' flag
                    base = 8;
                    format_options |= allow_base_prefix;
                }
                else if (flags[3]) {
                    // 'x' flag
                    base = 16;
                    format_options |= allow_base_prefix;
                }
                else if (flags[4]) {
                    // 'i' flag
                    base = 0;
                }
                else if (flags[5]) {
                    // 'u' flag
                    base = 10;
                    format_options |= only_unsigned;
                }

                // n set, implies L
                if (flags[6]) {
                    common_options |= localized;
                    format_options |= localized_digits;
                }
                if ((format_options & localized_digits) != 0 &&
                    (base != 0 && base != 10 && base != 8 && base != 16)) {
                    return {error::invalid_format_string,
                            "Localized integers can only be scanned in "
                            "bases 8, 10 and 16"};
                }

                // thsep flag
                if (flags[7]) {
                    format_options |= allow_thsep;
                }

                return {};
            }

            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                using char_type = typename Context::char_type;
                auto do_parse_int = [&](span<const char_type> s) -> error {
                    T tmp = 0;
                    expected<std::ptrdiff_t> ret{0};
                    if (SCN_UNLIKELY((format_options & localized_digits) !=
                                     0)) {
                        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                        int b{base};
                        auto r = parse_base_prefix<char_type>(s, b);
                        if (!r) {
                            return r.error();
                        }
                        if (b == -1) {
                            // -1 means we read a '0'
                            tmp = 0;
                            return {};
                        }
                        if (b != 10 && base != b && base != 0) {
                            return error(error::invalid_scanned_value,
                                         "Invalid base prefix");
                        }
                        if (base == 0) {
                            base = static_cast<uint8_t>(b);
                        }
                        if (base != 8 && base != 10 && base != 16) {
                            return error(error::invalid_scanned_value,
                                         "Localized values have to be in base "
                                         "8, 10 or 16");
                        }

                        auto it = r.value();
                        std::basic_string<char_type> str(to_address(it),
                                                         s.size());
                        ret = ctx.locale().read_num(tmp, str,
                                                    static_cast<int>(base));

                        if (tmp < T{0} &&
                            (format_options & only_unsigned) != 0) {
                            return error(
                                error::invalid_scanned_value,
                                "Parsed negative value when type was 'u'");
                        }
                        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    }
                    else {
                        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                        ret = _parse_int(tmp, s);
                        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
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

                std::basic_string<char_type> buf{};
                span<const char_type> bufspan{};
                auto e = _read_source(
                    ctx, buf, bufspan,
                    std::integral_constant<
                        bool, Context::range_type::is_contiguous>{});
                if (!e) {
                    return e;
                }

                return do_parse_int(bufspan.as_const());
            }

            enum format_options_type : uint8_t {
                // "n" option -> localized digits and digit grouping
                localized_digits = 1,
                // "'" option -> accept thsep
                // if "L" use locale, default=','
                allow_thsep = 2,
                // "u" option -> don't allow sign
                only_unsigned = 4,
                // Allow base prefix (e.g. 0B and 0x)
                allow_base_prefix = 8
            };
            uint8_t format_options{0};

            // 0 = detect base
            // Otherwise [2,36]
            uint8_t base{0};

            template <typename Context, typename Buf, typename CharT>
            error _read_source(Context& ctx,
                               Buf& buf,
                               span<const CharT>& s,
                               std::false_type)
            {
                auto do_read = [&](Buf& b) -> error {
                    auto outputit = std::back_inserter(b);
                    if ((common_options & localized) == 0) {
                        const auto& loc = ctx.locale().as_locale_ref();
                        auto e = read_until_space(
                            ctx.range(), outputit,
                            [&](const CharT& ch) { return loc.is_space(ch); },
                            false);
                        if (!e && b.empty()) {
                            return e;
                        }
                        return {};
                    }
                    auto e = read_until_space(
                        ctx.range(), outputit,
                        [&](const CharT& ch) {
                            return ctx.locale().is_space(ch);
                        },
                        false);
                    if (!e && b.empty()) {
                        return e;
                    }
                    return {};
                };

                if (SCN_LIKELY((format_options & allow_thsep) == 0)) {
                    auto e = do_read(buf);
                    if (!e) {
                        return e;
                    }
                    s = make_span(buf.data(), buf.size()).as_const();
                    return {};
                }

                Buf tmp;
                auto e = do_read(tmp);
                if (!e) {
                    return e;
                }
                auto thsep =
                    ((common_options & localized) != 0)
                        ? ctx.locale().thousands_separator()
                        : locale_defaults<CharT>::thousands_separator();

                auto it = tmp.begin();
                for (; it != tmp.end(); ++it) {
                    if (*it == thsep) {
                        for (auto it2 = it; ++it2 != tmp.end();) {
                            *it++ = SCN_MOVE(*it2);
                        }
                        break;
                    }
                }

                auto n =
                    static_cast<std::size_t>(std::distance(tmp.begin(), it));
                if (n == 0) {
                    return {error::invalid_scanned_value,
                            "Only a thousands separator found"};
                }

                buf = SCN_MOVE(tmp);
                s = make_span(buf.data(), n).as_const();
                return {};
            }

            template <typename Context, typename Buf, typename CharT>
            error _read_source(Context& ctx,
                               Buf& buf,
                               span<const CharT>& s,
                               std::true_type)
            {
                if (SCN_UNLIKELY((format_options & allow_thsep) != 0)) {
                    return _read_source(ctx, buf, s, std::false_type{});
                }
                auto ret = read_all_zero_copy(ctx.range());
                if (!ret) {
                    return ret.error();
                }
                s = ret.value();
                return {};
            }

            template <typename CharT>
            expected<typename span<const CharT>::iterator> parse_base_prefix(
                span<const CharT> s,
                int& b) const;

            template <typename CharT>
            expected<std::ptrdiff_t> _parse_int(T& val, span<const CharT> s);

            template <typename CharT>
            expected<typename span<const CharT>::iterator> _parse_int_impl(
                T& val,
                bool minus_sign,
                span<const CharT> buf) const;

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
        struct float_scanner : common_parser {
            static_assert(std::is_floating_point<T>::value, "");

            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                using char_type = typename ParseCtx::char_type;

                array<char_type, 10> options{
                    {// hex
                     ascii_widen<char_type>('a'), ascii_widen<char_type>('A'),
                     // scientific
                     ascii_widen<char_type>('e'), ascii_widen<char_type>('E'),
                     // fixed
                     ascii_widen<char_type>('f'), ascii_widen<char_type>('F'),
                     // general
                     ascii_widen<char_type>('g'), ascii_widen<char_type>('G'),
                     // localized digits
                     ascii_widen<char_type>('n'),
                     // thsep
                     ascii_widen<char_type>('\'')}};
                bool flags[10] = {false};

                auto e = parse_common(
                    pctx, span<const char_type>{options.begin(), options.end()},
                    span<bool>{flags, 10}, null_each<ParseCtx>);
                if (!e) {
                    return e;
                }

                if (flags[0] && flags[1]) {
                    return {error::invalid_format_string,
                            "Can't have both 'a' and 'A' flags with floats"};
                }
                if (flags[2] && flags[3]) {
                    return {error::invalid_format_string,
                            "Can't have both 'e' and 'E' flags with floats"};
                }
                if (flags[4] && flags[5]) {
                    return {error::invalid_format_string,
                            "Can't have both 'f' and 'F' flags with floats"};
                }
                if (flags[6] && flags[7]) {
                    return {error::invalid_format_string,
                            "Can't have both 'g' and 'G' flags with floats"};
                }

                bool set_hex = flags[0] || flags[1];
                bool set_scientific = flags[2] || flags[3];
                bool set_fixed = flags[4] || flags[5];
                bool set_general = flags[6] || flags[7];
                if (set_general && set_fixed) {
                    return {error::invalid_format_string,
                            "General float already implies fixed"};
                }
                if (set_general && set_scientific) {
                    return {error::invalid_format_string,
                            "General float already implies scientific"};
                }

                format_options = 0;
                if (set_hex) {
                    format_options |= allow_hex;
                }
                if (set_scientific) {
                    format_options |= allow_scientific;
                }
                if (set_fixed) {
                    format_options |= allow_fixed;
                }
                if (set_general) {
                    format_options |= allow_fixed | allow_scientific;
                }
                if (format_options == 0) {
                    format_options |=
                        allow_fixed | allow_scientific | allow_hex;
                }

                // 'n'
                if (flags[8]) {
                    common_options |= localized;
                    format_options |= localized_digits;
                }

                // thsep
                if (flags[9]) {
                    format_options |= allow_thsep;
                }

                return {};
            }

            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                using char_type = typename Context::char_type;
                auto do_parse_float = [&](span<const char_type> s) -> error {
                    T tmp = 0;
                    expected<std::ptrdiff_t> ret{0};
                    if (SCN_UNLIKELY((format_options & localized_digits) != 0 ||
                                     ((common_options & localized) != 0 &&
                                      (format_options & allow_hex) != 0))) {
                        // 'n' OR ('L' AND 'a')
                        // because none of our parsers support BOTH hexfloats
                        // and custom (localized) decimal points
                        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                        std::basic_string<char_type> str(s.data(), s.size());
                        ret =
                            ctx.locale().as_locale_ref().read_num(tmp, str, 0);
                        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    }
                    else {
                        ret = _read_float(
                            tmp, s,
                            (common_options & localized) != 0
                                ? ctx.locale().as_locale_ref().decimal_point()
                                : locale_defaults<char_type>::decimal_point());
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

            enum format_options_type {
                allow_hex = 1,
                allow_scientific = 2,
                allow_fixed = 4,
                localized_digits = 8,
                allow_thsep = 16
            };
            uint8_t format_options{allow_hex | allow_scientific | allow_fixed};

            template <typename CharT>
            expected<std::ptrdiff_t> _read_float(T& val,
                                                 span<const CharT> s,
                                                 CharT locale_decimal_point)
            {
                size_t chars{};
                std::basic_string<CharT> str(s.data(), s.size());
                SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                auto ret =
                    _read_float_impl(str.data(), chars, locale_decimal_point);
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                if (!ret) {
                    return ret.error();
                }
                val = ret.value();
                return static_cast<std::ptrdiff_t>(chars);
            }

            template <typename CharT>
            expected<T> _read_float_impl(const CharT* str,
                                         size_t& chars,
                                         CharT locale_decimal_point);
        };

        struct bool_scanner : common_parser {
            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                using char_type = typename ParseCtx::char_type;

                array<char_type, 3> options{{
                    // Only strings
                    ascii_widen<char_type>('s'),
                    // Only ints
                    ascii_widen<char_type>('i'),
                    // Localized digits
                    ascii_widen<char_type>('n'),
                }};
                bool flags[3] = {false};
                auto e = parse_common(
                    pctx, span<const char_type>{options.begin(), options.end()},
                    span<bool>{flags, 3}, null_each<ParseCtx>);

                if (!e) {
                    return e;
                }

                format_options = 0;
                // default ('s' + 'i')
                if (!flags[0] && !flags[1]) {
                    format_options |= allow_string | allow_int;
                }
                // 's'
                if (flags[0]) {
                    format_options |= allow_string;
                }
                // 'i'
                if (flags[1]) {
                    format_options |= allow_int;
                }
                // 'n'
                if (flags[2]) {
                    format_options |= localized_digits;
                    // 'n' implies 'L'
                    common_options |= localized;
                }
                return {};
            }

            template <typename Context>
            error scan(bool& val, Context& ctx)
            {
                using char_type = typename Context::char_type;

                if ((format_options & allow_string) != 0) {
                    auto truename = locale_defaults<char_type>::truename();
                    auto falsename = locale_defaults<char_type>::falsename();
                    if ((common_options & localized) != 0) {
                        truename = ctx.locale().as_locale_ref().truename();
                        falsename = ctx.locale().as_locale_ref().falsename();
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

                if ((format_options & allow_int) != 0) {
                    if ((format_options & localized_digits) != 0) {
                        int i{};
                        auto s = integer_scanner<int>{};
                        s.common_options = integer_scanner<int>::localized;
                        s.format_options =
                            integer_scanner<int>::only_unsigned |
                            integer_scanner<int>::localized_digits;
                        auto e = s.scan(i, ctx);
                        if (!e) {
                            return e;
                        }
                        if (SCN_UNLIKELY(i != 0 && i != 1)) {
                            return {
                                error::invalid_scanned_value,
                                "Scanned integral boolean not equal to 0 or 1"};
                        }
                        else if (i == 0) {
                            val = false;
                        }
                        else {
                            val = true;
                        }
                        return {};
                    }

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

                return {error::invalid_scanned_value, "Couldn't scan bool"};
            }

            enum format_options_type {
                // 's' option
                allow_string = 1,
                // 'i' option
                allow_int = 2,
                // 'n' option
                localized_digits = 4
            };
            uint8_t format_options{allow_string | allow_int};
        };

        struct string_scanner : common_parser {
            template <typename ParseCtx>
            error parse(ParseCtx& pctx)
            {
                using char_type = typename ParseCtx::char_type;

                auto c_flag = detail::ascii_widen<char_type>('s');
                bool c_set{};
                return parse_common(pctx, span<const char_type>{&c_flag, 1},
                                    span<bool>{&c_set, 1}, null_each<ParseCtx>);
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
                    return {error::invalid_scanned_value,
                            "Empty string parsed"};
                }
                val = SCN_MOVE(tmp);

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
                    return {error::invalid_operation,
                            "Cannot read a string_view from a "
                            "non-contiguous_range"};
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
     * Reads from the range in `ctx` as if by repeatedly calling
     * `read_char()`, until a non-space character is found (as determined by
     * `ctx.locale()`), or EOF is reached. That non-space character is then
     * put back into the range.
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

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && \
    !defined(SCN_READER_FLOAT_CPP)
#include "reader_float.cpp"
#endif
#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_READER_INT_CPP)
#include "reader_int.cpp"
#endif

#endif  // SCN_DETAIL_READER_H
