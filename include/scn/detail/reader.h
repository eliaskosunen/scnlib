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

    template <typename WrappedRange,
              typename std::enable_if<WrappedRange::is_direct>::type* = nullptr>
    expected<detail::ranges::range_value_t<WrappedRange>> read_char(
        WrappedRange& r)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        auto ch = *r.begin();
        ++r.begin();
        return {ch};
    }
    template <
        typename WrappedRange,
        typename std::enable_if<!WrappedRange::is_direct>::type* = nullptr>
    detail::ranges::range_value_t<WrappedRange> read_char(WrappedRange& r)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        auto ch = *r.begin();
        ++r.begin();
        return ch;
    }

    // read_zero_copy

    template <
        typename WrappedRange,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_zero_copy(WrappedRange& r,
                   detail::ranges::range_difference_t<WrappedRange> n)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        const auto n_to_read = detail::min(r.size(), n);
        auto s = make_span(r.data(), static_cast<size_t>(n_to_read)).as_const();
        detail::ranges::advance(r.begin(), n_to_read);
        return s;
    }
    template <
        typename WrappedRange,
        typename std::enable_if<!WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_zero_copy(WrappedRange&,
                   detail::ranges::range_difference_t<WrappedRange>)
    {
        return span<const typename detail::extract_char_type<
            typename WrappedRange::iterator>::type>{};
    }

    // read_into

    template <
        typename WrappedRange,
        typename OutputIterator,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    error read_into(WrappedRange& r,
                    OutputIterator& it,
                    detail::ranges::range_difference_t<WrappedRange> n)
    {
        auto s = read_zero_copy(r, n);
        if (!s) {
            return s.error();
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
                    detail::ranges::range_difference_t<WrappedRange> n)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        for (detail::ranges::range_difference_t<WrappedRange> i = 0; i < n;
             ++i) {
            if (r.begin() == r.end()) {
                return error(error::end_of_range, "EOF");
            }
            *it = *r.begin();
            ++it;
            ++r.begin();
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
                    detail::ranges::range_difference_t<WrappedRange> n)
    {
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        for (detail::ranges::range_difference_t<WrappedRange> i = 0; i < n;
             ++i) {
            if (r.begin() == r.end()) {
                return error(error::end_of_range, "EOF");
            }
            auto tmp = *r.begin();
            if (!tmp) {
                return tmp.error();
            }
            *it = tmp.value();
            ++r.begin();
        }
        return {};
    }

    // read_until_space_zero_copy

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
        if (r.begin() == r.end()) {
            return error(error::end_of_range, "EOF");
        }
        for (auto it = r.begin(); it != r.end(); ++it) {
            if (is_space(*it)) {
                auto b = r.begin();
                r.begin() = it;
                if (keep_final_space) {
                    ++it;
                    ++r.begin();
                }
                return {{&*b, &*it}};
            }
        }
        auto b = r.begin();
        r.begin() = r.end();
        return {{&*b, &*(r.end() - 1) + 1}};
    }
    template <
        typename WrappedRange,
        typename Predicate,
        typename std::enable_if<!WrappedRange::is_contiguous>::type* = nullptr>
    expected<span<const typename detail::extract_char_type<
        typename WrappedRange::iterator>::type>>
    read_until_space_zero_copy(WrappedRange&, Predicate, bool)
    {
        return span<const typename detail::extract_char_type<
            typename WrappedRange::iterator>::type>{};
    }

    // read_until_space

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
        for (auto& it = r.begin(); it != r.end(); ++it) {
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
        for (auto& it = r.begin(); it != r.end(); ++it) {
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

    // read_until_space_ranged

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
        for (auto& it = r.begin(); it != r.end() && out != end; ++it) {
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
        for (auto& it = r.begin(); it != r.end() && out != end; ++it) {
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

    // putback_n

    template <
        typename WrappedRange,
        typename std::enable_if<WrappedRange::is_contiguous>::type* = nullptr>
    error putback_n(WrappedRange& r,
                    detail::ranges::range_difference_t<WrappedRange> n)
    {
        SCN_EXPECT(n <=
                   detail::ranges::distance(r.begin_underlying(), r.begin()));
        std::advance(r.begin(), -n);
        return {};
    }
    template <
        typename WrappedRange,
        typename std::enable_if<!WrappedRange::is_contiguous>::type* = nullptr>
    error putback_n(WrappedRange& r,
                    detail::ranges::range_difference_t<WrappedRange> n)
    {
        for (detail::ranges::range_difference_t<WrappedRange> i = 0; i < n;
             ++i) {
            --r.begin();
            if (r.begin() == r.end()) {
                return error(error::unrecoverable_source_error,
                             "Putback failed");
            }
        }
        return {};
    }

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

                small_vector<char_type, 64> buf(val.size());
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
                {
                    auto s = read_until_space_zero_copy(ctx.range(),
                                                        is_space_pred, false);
                    if (!s) {
                        return s.error();
                    }
                    if (s.value().size() != 0) {
                        return do_parse_int(s.value());
                    }
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
                T sign = 1;
                auto it = s.begin();

                if (s[0] == ascii_widen<CharT>('-') ||
                    s[0] == ascii_widen<CharT>('+')) {
                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wsign-conversion")
                    sign = 1 - 2 * (s[0] == ascii_widen<CharT>('-'));
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
                SCN_ASSUME(sign != 0);

                if (std::is_signed<T>::value) {
                    auto r = _read_signed(
                        tmp, sign, make_span(it, s.end()).as_const(), thsep);
                    if (!r) {
                        return r.error();
                    }
                    it = r.value();
                    if (s.begin() == it) {
                        return error(error::invalid_scanned_value,
                                     "custom::read_signed");
                    }
                    val = tmp;
                    return ranges::distance(s.begin(), it);
                }
                auto r = _read_unsigned(tmp, make_span(it, s.end()).as_const(),
                                        thsep);
                if (!r) {
                    return r.error();
                }
                it = r.value();
                if (s.begin() == it) {
                    return error(error::invalid_scanned_value,
                                 "custom::read_unsigned");
                }
                val = tmp;
                return ranges::distance(s.begin(), it);

                SCN_CLANG_POP
                SCN_GCC_POP
            }

            template <typename CharT>
            expected<typename span<const CharT>::iterator> _read_signed(
                T& val,
                T sign,
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

                SCN_ASSUME(sign != 0);

                using utype = typename std::make_unsigned<T>::type;

                SCN_MSVC_PUSH
                SCN_MSVC_IGNORE(4146)  // unary minus applied to unsigned
                utype cutoff_tmp = sign == -1
                                       ? -static_cast<unsigned long long>(
                                             std::numeric_limits<T>::min())
                                       : std::numeric_limits<T>::max();
                SCN_MSVC_POP

                const auto cutlim = detail::ascii_widen<CharT>(
                    cutoff_tmp % static_cast<utype>(base) + 48);  // 48 is '0'
                const utype cutoff = cutoff_tmp / base;

                auto it = buf.begin();
                for (; it != buf.end(); ++it) {
                    if (SCN_LIKELY(is_base_digit(*it, base))) {
                        if (SCN_UNLIKELY(val > cutoff ||
                                         (val == cutoff && *it > cutlim))) {
                            if (sign == 1) {
                                return error(error::value_out_of_range,
                                             "Out of range: integer overflow");
                            }
                            return error(error::value_out_of_range,
                                         "Out of range: integer underflow");
                        }
                        else {
                            val = val * base + _char_to_int(*it);
                        }
                    }
                    else {
                        if (have_thsep && *it == thsep) {
                            continue;
                        }
                        break;
                    }
                }
                val = val * sign;
                return it;

                SCN_MSVC_POP
                SCN_CLANG_POP
                SCN_GCC_POP
            }

            template <typename CharT>
            expected<typename span<const CharT>::iterator>
            _read_unsigned(T& val, span<const CharT> buf, CharT thsep) const
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

                const T cutoff = std::numeric_limits<T>::max() / base;
                const auto cutlim = detail::ascii_widen<CharT>(
                    std::numeric_limits<T>::max() % base + '0');

                auto it = buf.begin();
                for (; it != buf.end(); ++it) {
                    if (SCN_LIKELY(is_base_digit(*it, base))) {
                        if (SCN_UNLIKELY(val > cutoff ||
                                         (val == cutoff && *it > cutlim))) {
                            return error(error::value_out_of_range,
                                         "Out of range: integer overflow");
                        }
                        else {
                            val = val * base + _char_to_int(*it);
                        }
                    }
                    else {
                        if (have_thsep && *it == thsep) {
                            continue;
                        }
                        break;
                    }
                }
                return it;

                SCN_MSVC_POP
                SCN_CLANG_POP
                SCN_GCC_POP
            }

            template <typename CharT>
            T _char_to_int(CharT ch) const
            {
                static_assert(std::is_same<CharT, char>::value ||
                                  std::is_same<CharT, wchar_t>::value,
                              "");
                if (base <= 10) {
                    return static_cast<T>(ch - ascii_widen<CharT>('0'));
                }
                if (ch <= ascii_widen<CharT>('9')) {
                    return static_cast<T>(ch - ascii_widen<CharT>('0'));
                }
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                if (ch >= ascii_widen<CharT>('a') &&
                    ch <= ascii_widen<CharT>('z')) {
                    return 10 + static_cast<T>(ch - ascii_widen<CharT>('a'));
                }
                return 10 + static_cast<T>(ch - ascii_widen<CharT>('A'));
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
                {
                    auto s = read_until_space_zero_copy(ctx.range(),
                                                        is_space_pred, false);
                    if (!s) {
                        return s.error();
                    }
                    if (s.value().size() != 0) {
                        return do_parse_float(s.value());
                    }
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

            template <typename Context>
            error scan(std::basic_string<typename Context::char_type>& val,
                       Context& ctx)
            {
                using char_type = typename Context::char_type;

                auto is_space_pred = [&ctx](char_type ch) {
                    return ctx.locale().is_space(ch);
                };
                auto s = read_until_space_zero_copy(ctx.range(), is_space_pred,
                                                    false);
                if (!s) {
                    return s.error();
                }
                if (s.value().size() != 0) {
                    val = std::basic_string<char_type>{s.value().data(),
                                                       s.value().size()};
                }
                else {
                    std::basic_string<char_type> tmp;
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
                }

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
                auto s = read_until_space_zero_copy(ctx.range(), is_space_pred,
                                                    false);
                if (!s) {
                    return s.error();
                }
                if (s.value().size() == 0) {
                    // TODO: Compile-time error?
                    return error(error::invalid_operation,
                                 "Cannot read a string_view from a "
                                 "non-contiguous_range");
                }
                val = basic_string_view<char_type>(s.value().data(),
                                                   s.value().size());
                return {};
            }
        };
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
    template <typename CharT>
    struct scanner<CharT, std::basic_string<CharT>>
        : public detail::string_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, basic_string_view<CharT>>
        : public detail::string_view_scanner {
    };
    template <typename CharT>
    struct scanner<CharT, detail::monostate>;

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

        for (auto it = ctx.range().begin(); it != ctx.range().end(); ++it) {
            if (!ctx.locale().is_space(*it)) {
                ctx.range().begin() = it;
                return {};
            }
        }
        ctx.range().begin() = ctx.range().end();
        return {};

        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
    }

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_READER_CPP)
#include "reader.cpp"
#endif

#endif  // SCN_DETAIL_READER_H

