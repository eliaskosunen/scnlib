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

#ifndef SCN_DETAIL_VISITOR_H
#define SCN_DETAIL_VISITOR_H

#include "context.h"
#include "stream.h"

#include <string>

namespace scn {
    SCN_BEGIN_NAMESPACE

    enum class scan_status { keep, skip, end };

    namespace predicates {
        template <typename Context>
        using locale_ref = typename Context::locale_type&;

        template <typename Predicate>
        struct predicate_skips : Predicate::does_skip {
        };

        template <typename CharT>
        struct propagate {
            using does_skip = std::false_type;
            SCN_CONSTEXPR expected<scan_status> operator()(CharT) const noexcept
            {
                return scan_status::keep;
            }
        };
        template <typename CharT>
        struct until {
            using does_skip = std::false_type;
            SCN_CONSTEXPR expected<scan_status> operator()(CharT ch) const
                noexcept
            {
                return ch == until_ch ? scan_status::end : scan_status::keep;
            }

            CharT until_ch;
        };
        template <typename CharT>
        struct until_one_of {
            using does_skip = std::false_type;
            expected<scan_status> operator()(CharT ch)
            {
                if (detail::find(until.begin(), until.end(), ch) !=
                    until.end()) {
                    return scan_status::end;
                }
                return scan_status::keep;
            }

            span<CharT> until;
        };
        template <typename CharT, typename Locale>
        struct until_space {
            using does_skip = std::false_type;
            expected<scan_status> operator()(CharT ch)
            {
                if (locale.is_space(ch)) {
                    return scan_status::end;
                }
                return scan_status::keep;
            }

            Locale& locale;
        };

        template <typename CharT>
        struct until_and_skip_chars {
            using does_skip = std::true_type;
            expected<scan_status> operator()(CharT ch)
            {
                if (ch == until) {
                    return scan_status::end;
                }
                if (std::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            CharT until;
            span<CharT> skip;
        };
        template <typename CharT>
        struct until_one_of_and_skip_chars {
            using does_skip = std::true_type;
            expected<scan_status> operator()(CharT ch)
            {
                if (detail::find(until.begin(), until.end(), ch) !=
                    until.end()) {
                    return scan_status::end;
                }
                if (detail::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            span<CharT> until;
            span<CharT> skip;
        };
        template <typename CharT, typename Locale>
        struct until_space_and_skip_chars {
            using does_skip = std::true_type;
            expected<scan_status> operator()(CharT ch)
            {
                if (locale.is_space(ch)) {
                    return scan_status::end;
                }
                if (detail::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            Locale& locale;
            span<CharT> skip;
        };
    }  // namespace predicates

    namespace pred = predicates;

    // read

    template <typename Stream,
              typename CharT = typename Stream::char_type,
              typename Span = span<CharT>,
              typename std::enable_if<is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<typename Span::iterator> read(Stream& stream, Span s)
    {
        auto n = detail::min(s.size(), stream.chars_to_read());
        s = s.first(n);
        stream.read_sized(s);
        return {s.end()};
    }
    template <typename Stream,
              typename CharT = typename Stream::char_type,
              typename Span = span<CharT>,
              typename std::enable_if<!is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<typename Span::iterator> read(Stream& stream, Span s)
    {
        auto it = s.begin();
        for (; it != s.end(); ++it) {
            auto ret = stream.read_char();
            if (!ret) {
                if (ret.error() == error::end_of_stream) {
                    return {it};
                }
                return {it, ret.error()};
            }
            *it = ret.value();
        }
        return {it};
    }

    // read_into_if

    template <typename Stream,
              typename Iterator,
              typename Predicate,
              typename CharT = typename Stream::char_type,
              typename std::enable_if<is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<size_t> read_into_if(Stream& s,
                                Iterator it,
                                Predicate&& p,
                                bool keep_final = false)
    {
        size_t n = 0, size = 0;
        detail::array<CharT, 64> arr;
        bool end = false;
        while (!end) {
            n = detail::min(s.chars_to_read(), std::size_t{64});
            if (n == 0) {
                break;
            }

            auto ret = read(s, make_span(arr.data(), n));
            if (!ret) {
                return {size, ret.error()};
            }

            const auto arr_end = arr.begin() + n;

            if (predicates::predicate_skips<Predicate>::value) {
                for (auto arr_it = arr.begin(); arr_it != arr_end; ++arr_it) {
                    auto r = p(*arr_it);
                    if (!r) {
                        return {size, r.error()};
                    }
                    if (r.value() == scan_status::skip) {
                        continue;
                    }
                    if (r.value() == scan_status::end) {
                        if (keep_final) {
                            ++arr_it;
                        }
                        if (arr_it != arr_end) {
                            s.putback_n(static_cast<size_t>(
                                std::distance(arr_it, arr_end)));
                        }
                        end = true;
                        break;
                    }
                    *it = *arr_it;
                    ++it;
                    ++size;
                }
            }
            else {
                auto arr_it = arr.begin();
                for (; arr_it != arr_end; ++arr_it) {
                    auto r = p(*arr_it);
                    if (!r) {
                        return {size, r.error()};
                    }
                    if (r.value() == scan_status::end) {
                        if (keep_final) {
                            ++arr_it;
                        }
                        if (arr_it != arr_end) {
                            s.putback_n(static_cast<size_t>(
                                std::distance(arr_it, arr_end)));
                        }
                        if (keep_final) {
                            --arr_it;
                        }
                        end = true;
                        break;
                    }
                }
                const auto chars =
                    static_cast<size_t>(std::distance(arr.begin(), arr_it));
                size += chars;
                std::copy(arr.begin(), arr_it, it);
            }
        }
        return {size};
    }
    template <typename Stream,
              typename Iterator,
              typename Predicate,
              typename CharT = typename Stream::char_type,
              typename std::enable_if<!is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<size_t> read_into_if(Stream& s,
                                Iterator it,
                                Predicate&& p,
                                bool keep_final = false)
    {
        size_t n = 0;
        while (true) {
            auto ret = s.read_char();
            if (!ret) {
                if (ret.error() == error::end_of_stream) {
                    break;
                }
                return {n, ret.error()};
            }

            auto r = p(ret.value());
            if (!r) {
                return {n, r.error()};
            }
            if (r.value() == scan_status::skip) {
                continue;
            }
            if (r.value() == scan_status::end) {
                if (keep_final) {
                    *it = ret.value();
                    ++it;
                }
                break;
            }
            *it = ret.value();
            ++it;
            ++n;
        }
        return {n};
    }

    template <typename Stream,
              typename Iterator,
              typename Sentinel,
              typename Predicate,
              typename CharT = typename Stream::char_type,
              typename std::enable_if<is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<Iterator> read_into_if(Stream& s,
                                  Iterator it,
                                  Sentinel end,
                                  Predicate&& p,
                                  bool keep_final = false)
    {
        size_t n = 0;
        detail::array<CharT, 64> arr;
        bool done = false;
        while (!done) {
            n = detail::min({static_cast<size_t>(std::distance(it, end)),
                             s.chars_to_read(), size_t{64}});
            if (n == 0) {
                break;
            }

            auto ret = read(s, make_span(arr.data(), n));
            if (!ret) {
                return {it, ret.error()};
            }

            auto arr_it = arr.begin();
            const auto arr_end = arr.begin() + n;
            for (; arr_it != arr_end; ++arr_it) {
                auto r = p(*arr_it);
                if (!r) {
                    return {it, r.error()};
                }
                if (r.value() == scan_status::skip) {
                    continue;
                }
                if (r.value() == scan_status::end) {
                    if (keep_final) {
                        ++arr_it;
                    }
                    if (arr_it != arr_end) {
                        s.putback_n(static_cast<size_t>(
                            std::distance(arr_it, arr_end)));
                    }
                    done = true;
                    break;
                }
                *it = *arr_it;
                ++it;
            }
        }
        return {it};
    }
    template <typename Stream,
              typename Iterator,
              typename Sentinel,
              typename Predicate,
              typename CharT = typename Stream::char_type,
              typename std::enable_if<!is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<Iterator> read_into_if(Stream& s,
                                  Iterator it,
                                  Sentinel end,
                                  Predicate&& p,
                                  bool keep_final = false)
    {
        while (it != end) {
            auto ret = s.read_char();
            if (!ret) {
                if (ret.error() == error::end_of_stream) {
                    break;
                }
                return {it, ret.error()};
            }

            auto r = p(ret.value());
            if (!r) {
                return {it, r.error()};
            }
            if (r.value() == scan_status::skip) {
                continue;
            }
            if (r.value() == scan_status::end) {
                if (keep_final) {
                    *it = ret.value();
                    ++it;
                }
                break;
            }
            *it = ret.value();
            ++it;
        }
        return {it};
    }

    // read_into_until_space

    namespace detail {
        template <typename Stream,
                  typename Locale,
                  typename Iterator,
                  typename = void>
        struct read_into_until_space_impl {
            using char_type = typename Stream::char_type;
            static result<size_t> f(Stream& s,
                                    Locale& l,
                                    Iterator it,
                                    bool keep_final)
            {
                return read_into_if(
                    s, it, pred::until_space<char_type, Locale>{l}, keep_final);
            }
        };

        template <typename Stream, typename Iterator>
        struct read_into_until_space_impl<
            Stream,
            basic_default_locale_ref<char>,
            Iterator,
            typename std::enable_if<
                std::is_same<typename Stream::char_type, char>::value &&
                is_sized_stream<Stream>::value>::type> {
            using char_type = char;
            using locale_type = basic_default_locale_ref<char>;

            static result<size_t> f(Stream& s,
                                    locale_type& loc,
                                    Iterator it,
                                    bool keep_final)
            {
                size_t i = 0;
                char ch{};
                for (; s.chars_to_read() != 0; ++i) {
                    s.read_sized(make_span(&ch, 1));
                    if (loc.is_space(ch)) {
                        if (keep_final) {
                            ++i;
                            *it = ch;
                            ++it;
                        }
                        else {
                            s.putback_n(1);
                        }
                        break;
                    }
                    *it = ch;
                    ++it;
                }
                return {i};
            }
        };
    }  // namespace detail

    template <typename Stream, typename Locale, typename Iterator>
    result<size_t> read_into_until_space(Stream& s,
                                         Locale& l,
                                         Iterator it,
                                         bool keep_final = false)
    {
        return detail::read_into_until_space_impl<Stream, Locale, Iterator>::f(
            s, l, it, keep_final);
    }

    // putback_range

    template <typename Stream,
              typename Iterator,
              typename std::enable_if<!is_sized_stream<Stream>::value>::type* =
                  nullptr>
    error putback_range(Stream& s, Iterator begin, Iterator end)
    {
        for (; begin != end; ++begin) {
            auto ret = s.putback(*begin);
            if (!ret) {
                return ret;
            }
        }
        return {};
    }
    template <typename Stream,
              typename Iterator,
              typename std::enable_if<is_sized_stream<Stream>::value>::type* =
                  nullptr>
    error putback_range(Stream& s, Iterator begin, Iterator end)
    {
        s.putback_n(static_cast<size_t>(std::distance(begin, end)));
        return {};
    }

    template <typename CharT>
    struct empty_parser {
        template <typename Context>
        error parse(Context& ctx)
        {
            auto& pctx = ctx.parse_context();
            pctx.arg_begin();
            if (SCN_UNLIKELY(!pctx)) {
                return error(error::invalid_format_string,
                             "Unexpected format string end");
            }
            if (!pctx.check_arg_end(ctx.locale())) {
                return error(error::invalid_format_string,
                             "Expected argument end");
            }
            pctx.arg_end();
            return {};
        }
    };

    namespace detail {
        template <typename CharT>
        struct char_scanner {
            template <typename Context>
            error parse(Context& ctx)
            {
                auto& pctx = ctx.parse_context();
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }
                if (pctx.next() == detail::ascii_widen<CharT>('c')) {
                    pctx.advance();
                }
                if (!pctx.check_arg_end(ctx.locale())) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(CharT& val, Context& ctx)
            {
                auto ch = ctx.stream().read_char();
                if (!ch) {
                    return ch.error();
                }
                val = ch.value();
                return {};
            }
        };
        template <typename CharT>
        struct buffer_scanner : public empty_parser<CharT> {
            template <typename Context>
            error scan(span<CharT>& val, Context& ctx)
            {
                if (val.size() == 0) {
                    return {};
                }

                detail::small_vector<CharT, 64> buf(
                    static_cast<size_t>(val.size()));
                auto span = scn::make_span(buf);
                auto s = read(ctx.stream(), span);
                if (!s) {
                    return s.error();
                }
                std::memcpy(val.begin(), buf.begin(), buf.size());

                return {};
            }
        };

        template <typename CharT>
        struct bool_scanner {
            template <typename Context>
            error parse(Context& ctx)
            {
                // {}: no boolalpha, not localized
                // l: localized
                // a: boolalpha
                auto& pctx = ctx.parse_context();
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }

                bool a = false, n = false;
                for (auto ch = pctx.next();
                     pctx && !pctx.check_arg_end(ctx.locale());
                     pctx.advance(), ch = pctx.next()) {
                    if (ch == detail::ascii_widen<CharT>('l')) {
                        localized = true;
                    }
                    else if (ch == detail::ascii_widen<CharT>('a')) {
                        a = true;
                    }
                    else if (ch == detail::ascii_widen<CharT>('n')) {
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

                if (pctx.next() == detail::ascii_widen<CharT>('b')) {
                    pctx.advance();
                }
                if (!pctx.check_arg_end(ctx.locale())) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(bool& val, Context& ctx)
            {
                if (allow_alpha) {
                    auto truename = detail::locale_defaults<CharT>::truename();
                    auto falsename =
                        detail::locale_defaults<CharT>::falsename();
                    if (localized) {
                        truename = ctx.locale().truename();
                        falsename = ctx.locale().falsename();
                    }
                    const auto max_len =
                        detail::max(truename.size(), falsename.size());
                    std::basic_string<CharT> buf;
                    buf.reserve(max_len);

                    auto i = read_into_until_space(ctx.stream(), ctx.locale(),
                                                   std::back_inserter(buf));
#if 0
                    auto i = read_into_if(
                        ctx.stream(), std::back_inserter(buf),
                        predicates::until_space<CharT,
                                                typename Context::locale_type>{
                            ctx.locale()});
#endif
                    if (!i) {
                        return i.error();
                    }
                    buf.erase(i.value());

                    bool found = false;
                    size_t chars = 0;

                    if (buf.size() >= falsename.size()) {
                        if (std::equal(falsename.begin(), falsename.end(),
                                       buf.begin())) {
                            val = false;
                            found = true;
                            chars = falsename.size();
                        }
                    }
                    if (!found && buf.size() >= truename.size()) {
                        if (std::equal(truename.begin(), truename.end(),
                                       buf.begin())) {
                            val = true;
                            found = true;
                            chars = truename.size();
                        }
                    }
                    if (found) {
                        return putback_range(
                            ctx.stream(), buf.rbegin(),
                            buf.rend() - static_cast<std::ptrdiff_t>(chars));
                    }
                    else {
                        auto pb = putback_range(ctx.stream(), buf.rbegin(),
                                                buf.rend());
                        if (!pb) {
                            return pb;
                        }
                    }
                }

                if (allow_num) {
                    auto tmp = ctx.stream().read_char();
                    if (!tmp) {
                        return tmp.error();
                    }
                    if (tmp.value() == detail::ascii_widen<CharT>('0')) {
                        val = false;
                        return {};
                    }
                    if (tmp.value() == detail::ascii_widen<CharT>('1')) {
                        val = true;
                        return {};
                    }
                    auto pb = ctx.stream().putback(tmp.value());
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

        namespace sto {
            template <typename CharT, typename T>
            struct str_to_int;
        }  // namespace sto

        namespace strto {
            template <typename CharT, typename T>
            struct str_to_int;
        }  // namespace strto

        template <typename CharT, typename T>
        struct float_scanner;

        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wpadded")

        template <typename CharT, typename T>
        class integer_scanner {
        public:
            template <typename Context>
            error parse(Context& ctx)
            {
                // {}: base detect, not localized
                // n: localized decimal/thousand separator
                // l: n + localized digits
                // d: decimal, o: octal, x: hex, b[1-36]: base
                auto& pctx = ctx.parse_context();
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }

                bool base_set = false;
                bool loc_set = false;
                for (auto ch = pctx.next();
                     pctx && !pctx.check_arg_end(ctx.locale());
                     pctx.advance(), ch = pctx.next()) {
                    if (!base_set) {
                        if (ch == detail::ascii_widen<CharT>('d')) {
                            base = 10;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<CharT>('x')) {
                            base = 16;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<CharT>('o')) {
                            base = 8;
                            base_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<CharT>('i')) {
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
                        else if (ch == detail::ascii_widen<CharT>('u')) {
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
                        else if (ch == detail::ascii_widen<CharT>('b')) {
                            pctx.advance();
                            if (SCN_UNLIKELY(!pctx)) {
                                return error(error::invalid_format_string,
                                             "Unexpected format string end");
                            }
                            if (SCN_UNLIKELY(
                                    pctx.check_arg_end(ctx.locale()))) {
                                return error(error::invalid_format_string,
                                             "Unexpected argument end");
                            }
                            ch = pctx.next();

                            const auto zero = detail::ascii_widen<CharT>('0'),
                                       nine = detail::ascii_widen<CharT>('9');
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

                            if (pctx.check_arg_end(ctx.locale())) {
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
                        if (ch == detail::ascii_widen<CharT>('l')) {
                            localized = thousands_separator | digits;
                            loc_set = true;
                            continue;
                        }
                        else if (ch == detail::ascii_widen<CharT>('n')) {
                            localized = thousands_separator;
                            loc_set = true;
                            continue;
                        }
                    }

                    if (!have_thsep) {
                        if (ch == detail::ascii_widen<CharT>('\'')) {
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
                if (have_thsep && ctx.int_method() != method::custom) {
                    return error(error::invalid_format_string,
                                 "Thousand separator scanning is only "
                                 "supported with custom parsing method");
                }
                if (!pctx.check_arg_end(ctx.locale())) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                detail::small_vector<CharT, 32> buf;

                auto r = read_into_until_space(ctx.stream(), ctx.locale(),
                                               std::back_inserter(buf));
                if (!r) {
                    return r.error();
                }
                buf.erase(buf.begin() + r.value(), buf.end());

                T tmp = 0;
                size_t chars = 0;

                if ((localized & digits) != 0) {
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    std::basic_string<CharT> str(buf.data(), buf.size());
                    auto ret = ctx.locale().read_num(tmp, str);
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

                    if (!ret) {
                        return ret.error();
                    }

                    auto pb = putback_range(
                        ctx.stream(), buf.rbegin(),
                        buf.rend() - static_cast<std::ptrdiff_t>(ret.value()));
                    if (!pb) {
                        return pb;
                    }
                    val = tmp;
                    return {};
                }

                SCN_MSVC_PUSH
                SCN_MSVC_IGNORE(4244)
                SCN_MSVC_IGNORE(4127)  // conditional expression is constant

                if (std::is_unsigned<T>::value) {
                    if (buf.front() == detail::ascii_widen<CharT>('-')) {
                        return error(error::value_out_of_range,
                                     "Unexpected sign '-' when scanning an "
                                     "unsigned integer");
                    }
                }

                SCN_MSVC_POP

                auto do_read = [&]() {
                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wswitch-default")
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    switch (ctx.int_method()) {
                        case method::sto:
                            return &_read_sto;
                        case method::from_chars:
                            return &_read_from_chars;
                        case method::strto:
                            return &_read_strto;
                        case method::custom:
                            return &_read_custom;
                    }
                    SCN_UNREACHABLE;
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    SCN_GCC_POP
                };
                auto e = do_read()(
                    tmp, make_span(buf).as_const(), base,
                    have_thsep ? ctx.locale().thousands_separator() : 0);
                if (!e) {
                    return e.error();
                }
                chars = e.value();

                auto pb = putback_range(
                    ctx.stream(), buf.rbegin(),
                    buf.rend() - static_cast<std::ptrdiff_t>(chars));
                if (!pb) {
                    return pb;
                }
                val = tmp;

                return {};
            }

            enum localized_type : uint8_t {
                thousands_separator = 1,
                digits = 2
            };

            int base{0};
            uint8_t localized{0};
            bool have_thsep{false};

        private:
            static expected<size_t> _read_sto(T& val,
                                              span<const CharT> buf,
                                              int base,
                                              CharT thsep);
            static expected<size_t> _read_strto(T& val,
                                                span<const CharT> buf,
                                                int base,
                                                CharT thsep);
            static expected<size_t> _read_from_chars(T& val,
                                                     span<const CharT> buf,
                                                     int base,
                                                     CharT thsep);
            static expected<size_t> _read_custom(T& val,
                                                 span<const CharT> buf,
                                                 int base,
                                                 CharT thsep);

            friend struct float_scanner<CharT, float>;
            friend struct float_scanner<CharT, double>;
            friend struct float_scanner<CharT, long double>;
        };

        SCN_CLANG_POP

        namespace sto {
            template <typename CharT, typename T>
            struct str_to_float;
        }  // namespace sto

        namespace strto {
            template <typename CharT, typename T>
            struct str_to_float;
        }  // namespace strto

        template <typename CharT, typename T>
        struct float_scanner {
            template <typename Context>
            error parse(Context& ctx)
            {
                // {}: not localized
                // l: localized
                auto& pctx = ctx.parse_context();
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }

                if (pctx.check_arg_end(ctx.locale())) {
                    pctx.arg_end();
                    return {};
                }

                if (pctx.next() == detail::ascii_widen<CharT>('l')) {
                    localized = true;
                    pctx.advance();
                }

                if (pctx.check_arg_end(ctx.locale())) {
                    pctx.arg_end();
                    return {};
                }

                if (pctx.next() == detail::ascii_widen<CharT>('a')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('A')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('e')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('E')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('f')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('F')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('g')) {
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('G')) {
                    pctx.advance();
                }

                if (!pctx.check_arg_end(ctx.locale())) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }
            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                detail::small_vector<CharT, 32> buf{};

                auto r = read_into_until_space(ctx.stream(), ctx.locale(),
                                               std::back_inserter(buf));
                if (!r) {
                    return r.error();
                }
                buf.erase(buf.begin() + r.value(), buf.end());

                T tmp{};
                size_t chars = 0;

                if (localized) {
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    auto str = std::basic_string<CharT>(buf.data(), buf.size());
                    auto ret = ctx.locale().read_num(tmp, str);
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

                    if (!ret) {
                        return ret.error();
                    }
                    chars = ret.value();

                    auto pb = putback_range(
                        ctx.stream(), buf.rbegin(),
                        buf.rend() - static_cast<std::ptrdiff_t>(chars));
                    if (!pb) {
                        return pb;
                    }
                    val = tmp;
                    return {};
                }

                auto do_read = [&]() {
                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wswitch-default")
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    switch (ctx.float_method()) {
                        case method::sto:
                            return &_read_sto;
                        case method::from_chars:
                            return &_read_from_chars;
                        case method::strto:
                            return &_read_strto;
                        case method::custom:
                            return &_read_custom;
                    }
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    SCN_UNREACHABLE;
                    SCN_GCC_POP
                };
                auto e = do_read()(tmp, make_span(buf).as_const());
                if (!e) {
                    return e.error();
                }
                chars = e.value();

                auto pb = putback_range(
                    ctx.stream(), buf.rbegin(),
                    buf.rend() - static_cast<std::ptrdiff_t>(chars));
                if (!pb) {
                    return pb;
                }
                val = tmp;

                return {};
            }

            bool localized{false};

        private:
            static expected<size_t> _read_sto(T& val, span<const CharT> buf);
            static expected<size_t> _read_strto(T& val, span<const CharT> buf);
            static expected<size_t> _read_from_chars(T& val,
                                                     span<const CharT> buf);
            static expected<size_t> _read_custom(T& val, span<const CharT> buf);
        };

        template <typename CharT>
        struct string_scanner {
        public:
            template <typename Context>
            error parse(Context& ctx)
            {
                auto& pctx = ctx.parse_context();
                pctx.arg_begin();
                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }
                if (pctx.next() == detail::ascii_widen<CharT>('s')) {
                    pctx.advance();
                }
                if (!pctx.check_arg_end(ctx.locale())) {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }
                pctx.arg_end();
                return {};
            }

            template <typename Context>
            error scan(std::basic_string<CharT>& val, Context& ctx)
            {
                std::basic_string<CharT> tmp;
                /* tmp.reserve(15); */
                auto s = read_into_until_space(ctx.stream(), ctx.locale(),
                                               std::back_inserter(tmp));
#if 0
                auto s = read_into_if(
                    ctx.stream(), std::back_inserter(tmp),
                    predicates::until_space<CharT,
                                            typename Context::locale_type>{
                        ctx.locale()});
#endif
                if (SCN_UNLIKELY(!s)) {
                    return s.error();
                }
                tmp.erase(s.value());
                if (SCN_UNLIKELY(tmp.empty())) {
                    return error(error::invalid_scanned_value,
                                 "Empty string parsed");
                }
                val = std::move(tmp);

                return {};
            }
        };
    }  // namespace detail

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")
    template <typename CharT>
    struct scanner<CharT, CharT> : public detail::char_scanner<CharT> {
    };
    template <typename CharT>
    struct scanner<CharT, span<CharT>> : public detail::buffer_scanner<CharT> {
    };
    template <typename CharT>
    struct scanner<CharT, bool> : public detail::bool_scanner<CharT> {
    };
    template <typename CharT>
    struct scanner<CharT, short>
        : public detail::integer_scanner<CharT, short> {
    };
    template <typename CharT>
    struct scanner<CharT, int> : public detail::integer_scanner<CharT, int> {
    };
    template <typename CharT>
    struct scanner<CharT, long> : public detail::integer_scanner<CharT, long> {
    };
    template <typename CharT>
    struct scanner<CharT, long long>
        : public detail::integer_scanner<CharT, long long> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned short>
        : public detail::integer_scanner<CharT, unsigned short> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned int>
        : public detail::integer_scanner<CharT, unsigned int> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned long>
        : public detail::integer_scanner<CharT, unsigned long> {
    };
    template <typename CharT>
    struct scanner<CharT, unsigned long long>
        : public detail::integer_scanner<CharT, unsigned long long> {
    };
    template <typename CharT>
    struct scanner<CharT, float> : public detail::float_scanner<CharT, float> {
    };
    template <typename CharT>
    struct scanner<CharT, double>
        : public detail::float_scanner<CharT, double> {
    };
    template <typename CharT>
    struct scanner<CharT, long double>
        : public detail::float_scanner<CharT, long double> {
    };
    template <typename CharT>
    struct scanner<CharT, std::basic_string<CharT>>
        : public detail::string_scanner<CharT> {
    };
    template <typename CharT>
    struct scanner<CharT, detail::monostate>;
    SCN_CLANG_POP

    template <typename Context>
    error skip_stream_whitespace(Context& ctx) noexcept
    {
        while (true) {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

            auto ch = ctx.stream().read_char();
            if (SCN_UNLIKELY(!ch)) {
                return ch.error();
            }
            if (!ctx.locale().is_space(ch.value())) {
                auto pb = ctx.stream().putback(ch.value());
                if (SCN_UNLIKELY(!pb)) {
                    return pb;
                }
                break;
            }

            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }
        return {};
    }
#if 0
    template <typename Context,
              typename std::enable_if<is_sized_stream<
                  typename Context::stream_type>::value>::type* = nullptr>
    error skip_stream_whitespace(Context& ctx) noexcept
    {
        using char_type = typename Context::char_type;
        detail::array<char_type, 8> buf;
        bool end = false;
        while (!end) {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

            const auto n =
                detail::min(ctx.stream().chars_to_read(), buf.size());
            if (n == 0) {
                return error(error::end_of_stream, "EOF");
            }
            auto s = make_span(buf.data(), n);
            ctx.stream().read_sized(s);

            for (auto it = s.begin(); it != s.end(); ++it) {
                if (!ctx.locale().is_space(*it)) {
                    ctx.stream().putback_n(
                        static_cast<size_t>(std::distance(it, s.end())));
                    end = true;
                    break;
                }
            }

            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }
        return {};
    }
#endif

    template <typename Context>
    class basic_visitor {
    public:
        using context_type = Context;
        using char_type = typename Context::char_type;

        basic_visitor(Context& ctx) : m_ctx(std::addressof(ctx)) {}

        template <typename T>
        auto operator()(T&& val) -> error
        {
            return visit(std::forward<T>(val), detail::priority_tag<1>{});
        }

    private:
        auto visit(char_type& val, detail::priority_tag<1>) -> error
        {
            detail::char_scanner<char_type> s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(span<char_type>& val, detail::priority_tag<1>) -> error
        {
            detail::buffer_scanner<char_type> s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(bool& val, detail::priority_tag<1>) -> error
        {
            detail::bool_scanner<char_type> s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }

#define SCN_VISIT_INT(T)                         \
    error visit(T& val, detail::priority_tag<0>) \
    {                                            \
        detail::integer_scanner<char_type, T> s; \
        auto err = parse(s);                     \
        if (!err) {                              \
            return err;                          \
        }                                        \
        return s.scan(val, *m_ctx);              \
    }
        SCN_VISIT_INT(short)
        SCN_VISIT_INT(int)
        SCN_VISIT_INT(long)
        SCN_VISIT_INT(long long)
        SCN_VISIT_INT(unsigned short)
        SCN_VISIT_INT(unsigned int)
        SCN_VISIT_INT(unsigned long)
        SCN_VISIT_INT(unsigned long long)
#undef SCN_VISIT_INT

#define SCN_VISIT_FLOAT(T)                       \
    error visit(T& val, detail::priority_tag<1>) \
    {                                            \
        detail::float_scanner<char_type, T> s;   \
        auto err = parse(s);                     \
        if (!err) {                              \
            return err;                          \
        }                                        \
        return s.scan(val, *m_ctx);              \
    }
        SCN_VISIT_FLOAT(float)
        SCN_VISIT_FLOAT(double)
        SCN_VISIT_FLOAT(long double)
#undef SCN_VISIT_FLOAT

        auto visit(std::basic_string<char_type>& val, detail::priority_tag<1>)
            -> error
        {
            detail::string_scanner<char_type> s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(typename Context::arg_type::handle val,
                   detail::priority_tag<1>) -> error
        {
            return val.scan(*m_ctx);
        }
        auto visit(detail::monostate, detail::priority_tag<0>) -> error
        {
            return error(error::invalid_operation, "Cannot scan a monostate");
        }

        template <typename Scanner>
        error parse(Scanner& s)
        {
            return m_ctx->parse_context().parse(s, *m_ctx);
        }

        Context* m_ctx;
    };

    template <typename Context>
    scan_result visit(Context& ctx)
    {
        int args_read = 0;

        auto reterror = [&args_read](error e) -> scan_result {
            return scan_result(args_read, std::move(e));
        };

        auto& pctx = ctx.parse_context();
        auto arg = typename Context::arg_type();

        {
            auto ret = skip_stream_whitespace(ctx);
            if (!ret) {
                return reterror(ret);
            }
        }

        while (pctx) {
            if (pctx.should_skip_ws(ctx.locale())) {
                // Skip whitespace from format string and from stream
                // EOF is not an error
                auto ret = skip_stream_whitespace(ctx);
                if (SCN_UNLIKELY(!ret)) {
                    if (ret == error::end_of_stream) {
                        break;
                    }
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        return reterror(rb);
                    }
                    return reterror(ret);
                }
                // Don't advance pctx, should_skip_ws() does it for us
                continue;
            }

            // Non-brace character, or
            // Brace followed by another brace, meaning a literal '{'
            if (pctx.should_read_literal(ctx.locale())) {
                if (SCN_UNLIKELY(!pctx)) {
                    return reterror(error(error::invalid_format_string,
                                          "Unexpected end of format string"));
                }
                // Check for any non-specifier {foo} characters
                auto ret = ctx.stream().read_char();
                SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                if (!ret || !pctx.check_literal(ret.value())) {
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        // Failed rollback
                        return reterror(rb);
                    }
                    if (!ret) {
                        // Failed read
                        return reterror(ret.error());
                    }

                    // Mismatching characters in scan string and stream
                    return reterror(
                        error(error::invalid_scanned_value,
                              "Expected character from format string not "
                              "found in the stream"));
                }
                // Bump pctx to next char
                pctx.advance();
            }
            else {
                // Scan argument
                auto id_wrapped = pctx.parse_arg_id(ctx.locale());
                if (!id_wrapped) {
                    return reterror(id_wrapped.error());
                }
                auto id = id_wrapped.value();
                auto arg_wrapped = [&]() -> expected<typename Context::arg_type>
                {
                    if (id.empty()) {
                        return ctx.next_arg();
                    }
                    if (ctx.locale().is_digit(id.front())) {
                        size_t tmp = 0;
                        for (auto ch : id) {
                            tmp =
                                tmp * 10 +
                                static_cast<size_t>(
                                    ch - detail::ascii_widen<
                                             typename Context::char_type>('0'));
                        }
                        return ctx.arg(tmp);
                    }
                    return ctx.arg(id);
                }
                ();
                if (!arg_wrapped) {
                    return reterror(arg_wrapped.error());
                }
                arg = arg_wrapped.value();
                if (!pctx) {
                    return reterror(error(error::invalid_format_string,
                                          "Unexpected end of format argument"));
                }
                if (!arg) {
                    // Mismatch between number of args and {}s
                    return reterror(
                        error(error::invalid_format_string,
                              "Mismatch between number of arguments and "
                              "'{}' in the format string"));
                }
                auto ret = visit_arg<Context>(basic_visitor<Context>(ctx), arg);
                if (!ret) {
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        return reterror(rb);
                    }
                    return reterror(ret);
                }
                // Handle next arg and bump pctx
                ++args_read;
                pctx.arg_handled();
                if (pctx) {
                    pctx.advance();
                }
            }
        }
        if (pctx) {
            // Format string not exhausted
            return reterror(error(error::invalid_format_string,
                                  "Format string not exhausted"));
        }
        auto srb = ctx.stream().set_roll_back();
        if (!srb) {
            return reterror(srb);
        }
        return {args_read};
    }

    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_VISITOR_CPP)
#include "visitor.cpp"
#endif

#endif  // SCN_DETAIL_VISITOR_H
