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

#ifndef SCN_DETAIL_READER_COMMON_H
#define SCN_DETAIL_READER_COMMON_H

#include "../locale.h"
#include "../range.h"

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
            return {error::end_of_range, "EOF"};
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
            return {error::end_of_range, "EOF"};
        }
        for (ranges::range_difference_t<WrappedRange> i = 0; i < n; ++i) {
            if (r.begin() == r.end()) {
                return {error::end_of_range, "EOF"};
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
            return {error::end_of_range, "EOF"};
        }
        for (ranges::range_difference_t<WrappedRange> i = 0; i < n; ++i) {
            if (r.begin() == r.end()) {
                return {error::end_of_range, "EOF"};
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
            return {error::end_of_range, "EOF"};
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
            return {error::end_of_range, "EOF"};
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
            return {error::end_of_range, "EOF"};
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
            return {error::end_of_range, "EOF"};
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
                return {error::unrecoverable_source_error, "Putback failed"};
            }
        }
        return {};
    }

    /// @}

    namespace detail {
        template <typename CharT>
        struct is_space_predicate {
            using char_type = CharT;
            using locale_type = basic_locale_ref<char_type>;

            SCN_CONSTEXPR14 is_space_predicate(const locale_type& l,
                                               bool localized,
                                               size_t width)
                : m_locale{nullptr},
                  m_width{width},
                  m_fn{get_fn(localized, width != 0)}
            {
                if (localized) {
                    l.prepare_localized();
                    m_locale = l.get_localized_unsafe();
                }
            }

            bool operator()(char_type ch)
            {
                SCN_EXPECT(m_fn);
                return m_fn(m_locale, ch, m_i, m_width);
            }

        private:
            using static_locale_type = typename locale_type::static_type;
            using custom_locale_type = typename locale_type::custom_type;
            const custom_locale_type* m_locale;
            size_t m_width{0}, m_i{0};

            constexpr static bool call(const custom_locale_type*,
                                       char_type ch,
                                       size_t&,
                                       size_t)
            {
                return static_locale_type::is_space(ch);
            }
            static bool localized_call(const custom_locale_type* locale,
                                       char_type ch,
                                       size_t&,
                                       size_t)
            {
                SCN_EXPECT(locale != nullptr);
                return locale->is_space(ch);
            }
            SCN_CONSTEXPR14 static bool call_counting(const custom_locale_type*,
                                                      char_type ch,
                                                      size_t& i,
                                                      size_t max)
            {
                if (i == max) {
                    return true;
                }
                ++i;
                return static_locale_type::is_space(ch);
            }
            static bool localized_call_counting(
                const custom_locale_type* locale,
                char_type ch,
                size_t& i,
                size_t max)
            {
                SCN_EXPECT(locale != nullptr);
                if (i == max) {
                    return true;
                }
                ++i;
                return locale->is_space(ch);
            }

            using fn_type = bool (*)(const custom_locale_type*,
                                     char_type,
                                     size_t&,
                                     size_t);
            fn_type m_fn{nullptr};

            static SCN_CONSTEXPR14 fn_type get_fn(bool localized, bool counting)
            {
                if (localized) {
                    return counting ? localized_call_counting : localized_call;
                }
                return counting ? call_counting : call;
            }
        };

        template <typename CharT>
        is_space_predicate<CharT> make_is_space_predicate(
            const basic_locale_ref<CharT>& locale,
            bool localized,
            size_t width = 0)
        {
            return {locale, localized, width};
        }
    }  // namespace detail

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
    error skip_range_whitespace(Context& ctx, bool localized) noexcept
    {
        auto is_space_pred =
            detail::make_is_space_predicate(ctx.locale(), localized);
        while (true) {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

            auto ch = read_char(ctx.range());
            if (SCN_UNLIKELY(!ch)) {
                return ch.error();
            }
            if (!is_space_pred(ch.value())) {
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
    error skip_range_whitespace(Context& ctx, bool localized) noexcept
    {
        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
        auto is_space_pred =
            detail::make_is_space_predicate(ctx.locale(), localized);
        const auto end = ctx.range().end();
        for (auto it = ctx.range().begin(); it != end; ++it) {
            if (!is_space_pred(*it)) {
                ctx.range().advance_to(it);
                return {};
            }
        }
        ctx.range().advance_to(end);
        return {};

        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
    }

    /// @}

    namespace detail {
        template <typename T>
        struct simple_integer_scanner {
            template <typename CharT>
            static expected<typename span<const CharT>::iterator>
            scan(span<const CharT> buf, T& val, int base = 10);
        };
    }  // namespace detail

    struct empty_parser : parser_base {
        template <typename ParseCtx>
        error parse(ParseCtx& pctx)
        {
            pctx.arg_begin();
            if (SCN_UNLIKELY(!pctx)) {
                return {error::invalid_format_string,
                        "Unexpected format string end"};
            }
            if (!pctx.check_arg_end()) {
                return {error::invalid_format_string, "Expected argument end"};
            }
            pctx.arg_end();
            return {};
        }
    };

    struct common_parser : parser_base {
        static constexpr bool support_align_and_fill()
        {
            return true;
        }

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
        error check_end(ParseCtx& pctx)
        {
            if (!pctx || pctx.check_arg_end()) {
                return {error::invalid_format_string,
                        "Unexpected end of format string argument"};
            }
            return {};
        }

        template <typename ParseCtx>
        error parse_common_flags(ParseCtx& pctx)
        {
            SCN_EXPECT(check_end(pctx));
            using char_type = typename ParseCtx::char_type;

            auto ch = pctx.next();
            auto next_char = [&]() -> error {
                pctx.advance();
                auto e = check_end(pctx);
                if (!e) {
                    return e;
                }
                ch = pctx.next();
                return {};
            };
            auto parse_number = [&](size_t& n) -> error {
                SCN_EXPECT(pctx.locale().get_static().is_digit(ch));

                auto it = pctx.begin();
                for (; it != pctx.end(); ++it) {
                    if (!pctx.locale().get_static().is_digit(*it)) {
                        break;
                    }
                }
                auto buf = make_span(pctx.begin(), it);

                auto s = detail::simple_integer_scanner<size_t>{};
                auto res = s.scan(buf.as_const(), n, 10);
                if (!res) {
                    return res.error();
                }

                for (it = pctx.begin(); it != res.value();
                     pctx.advance(), it = pctx.begin()) {}
                return {};
            };

            auto get_align_char = [&](char_type c) -> common_options_type {
                if (c == detail::ascii_widen<char_type>('<')) {
                    return aligned_left;
                }
                if (c == detail::ascii_widen<char_type>('>')) {
                    return aligned_right;
                }
                if (c == detail::ascii_widen<char_type>('^')) {
                    return aligned_center;
                }
                return common_options_none;
            };
            auto parse_align = [&](common_options_type align, char_type fill) {
                if (align != common_options_none) {
                    common_options |= align;
                }
                fill_char = static_cast<char32_t>(fill);
            };

            common_options_type align{};
            bool align_set = false;
            if (pctx.chars_left() > 1 &&
                ch != detail::ascii_widen<char_type>('[')) {
                const auto peek = pctx.peek();
                align = get_align_char(peek);
                if (align != common_options_none) {
                    parse_align(align, ch);

                    auto e = next_char();
                    SCN_ENSURE(e);
                    if (!next_char()) {
                        return {};
                    }
                    align_set = true;
                }
            }
            if (!align_set) {
                align = get_align_char(ch);
                if (align != common_options_none) {
                    parse_align(align, detail::ascii_widen<char_type>(' '));
                    if (!next_char()) {
                        return {};
                    }
                }
            }

            if (pctx.locale().get_static().is_digit(ch)) {
                common_options |= width_set;

                size_t w{};
                auto e = parse_number(w);
                if (!e) {
                    return e;
                }
                field_width = w;
                return {};
            }
            if (ch == detail::ascii_widen<char_type>('L')) {
                common_options |= localized;

                if (!next_char()) {
                    return {};
                }
            }

            return {};
        }

        template <typename ParseCtx>
        error parse_common_end(ParseCtx& pctx)
        {
            if (!pctx || !pctx.check_arg_end()) {
                return {error::invalid_format_string, "Expected argument end"};
            }

            pctx.arg_end();
            return {};
        }

        template <typename ParseCtx>
        static error null_type_cb(ParseCtx&, bool&)
        {
            return {};
        }

        template <typename ParseCtx,
                  typename F,
                  typename CharT = typename ParseCtx::char_type>
        error parse_common(ParseCtx& pctx,
                           span<const CharT> type_options,
                           span<bool> type_flags,
                           F&& type_cb)
        {
            SCN_EXPECT(type_options.size() == type_flags.size());

            auto e = parse_common_begin(pctx);
            if (!e) {
                return e;
            }

            if (!pctx) {
                return {error::invalid_format_string,
                        "Unexpected end of format string"};
            }
            if (pctx.check_arg_end()) {
                return {};
            }

            e = parse_common_flags(pctx);
            if (!e) {
                return e;
            }

            if (!pctx) {
                return {error::invalid_format_string,
                        "Unexpected end of format string"};
            }
            if (pctx.check_arg_end()) {
                return {};
            }

            for (auto ch = pctx.next(); pctx && !pctx.check_arg_end();
                 ch = pctx.next()) {
                bool parsed = false;
                for (std::size_t i = 0; i < type_options.size() && !parsed;
                     ++i) {
                    if (ch == type_options[i]) {
                        if (SCN_UNLIKELY(type_flags[i])) {
                            return {error::invalid_format_string,
                                    "Repeat flag in format string"};
                        }
                        type_flags[i] = true;
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

                e = type_cb(pctx, parsed);
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

        template <typename ParseCtx>
        error parse_default(ParseCtx& pctx)
        {
            return parse_common(pctx, {}, {}, null_type_cb<ParseCtx>);
        }

        constexpr bool is_aligned_left() const noexcept
        {
            return (common_options & aligned_left) != 0 ||
                   (common_options & aligned_center) != 0;
        }
        constexpr bool is_aligned_right() const noexcept
        {
            return (common_options & aligned_right) != 0 ||
                   (common_options & aligned_center) != 0;
        }
        template <typename CharT>
        constexpr CharT get_fill_char() const noexcept
        {
            return static_cast<CharT>(fill_char);
        }

        size_t field_width{0};
        char32_t fill_char{0};
        enum common_options_type : uint8_t {
            common_options_none = 0,
            localized = 1,       // 'L',
            aligned_left = 2,    // '<'
            aligned_right = 4,   // '>'
            aligned_center = 8,  // '^'
            width_set = 16,      // width
            common_options_all = 31,
        };
        uint8_t common_options{0};
    };

    struct common_parser_default : common_parser {
        template <typename ParseCtx>
        error parse(ParseCtx& pctx)
        {
            return parse_default(pctx);
        }
    };

    namespace detail {
        template <typename Context,
                  typename std::enable_if<
                      !Context::range_type::is_contiguous>::type* = nullptr>
        error scan_alignment(Context& ctx,
                             typename Context::char_type fill) noexcept
        {
            while (true) {
                SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

                auto ch = read_char(ctx.range());
                if (SCN_UNLIKELY(!ch)) {
                    return ch.error();
                }
                if (ch.value() != fill) {
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
        error scan_alignment(Context& ctx,
                             typename Context::char_type fill) noexcept
        {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            const auto end = ctx.range().end();
            for (auto it = ctx.range().begin(); it != end; ++it) {
                if (*it != fill) {
                    ctx.range().advance_to(it);
                    return {};
                }
            }
            ctx.range().advance_to(end);
            return {};

            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }

        template <typename Scanner, typename = void>
        struct scanner_supports_alignment : std::false_type {
        };
        template <typename Scanner>
        struct scanner_supports_alignment<
            Scanner,
            typename std::enable_if<Scanner::support_align_and_fill()>::type>
            : std::true_type {
        };

        template <typename Context, typename Scanner>
        error skip_alignment(Context& ctx,
                             Scanner& scanner,
                             bool left,
                             std::true_type)
        {
            if (left && !scanner.is_aligned_left()) {
                return {};
            }
            if (!left && !scanner.is_aligned_right()) {
                return {};
            }
            return scan_alignment(
                ctx,
                scanner.template get_fill_char<typename Context::char_type>());
        }
        template <typename Context, typename Scanner>
        error skip_alignment(Context&, Scanner&, bool, std::false_type)
        {
            return {};
        }

        template <typename Scanner,
                  typename T,
                  typename Context,
                  typename ParseCtx>
        error visitor_boilerplate(T& val, Context& ctx, ParseCtx& pctx)
        {
            Scanner scanner;

            auto err = pctx.parse(scanner);
            if (!err) {
                return err;
            }

            if (scanner.skip_preceding_whitespace()) {
                err = skip_range_whitespace(ctx, false);
                if (!err) {
                    return err;
                }
            }

            err = skip_alignment(ctx, scanner, false,
                                 scanner_supports_alignment<Scanner>{});
            if (!err) {
                return err;
            }

            err = scanner.scan(val, ctx);
            if (!err) {
                return err;
            }

            return skip_alignment(ctx, scanner, true,
                                  scanner_supports_alignment<Scanner>{});
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#endif
