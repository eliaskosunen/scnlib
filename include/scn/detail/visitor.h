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
        template <typename CharT>
        using locale_ref = basic_locale_ref<CharT>&;

        template <typename CharT>
        struct propagate {
            SCN_CONSTEXPR either<scan_status> operator()(CharT) const noexcept
            {
                return scan_status::keep;
            }
        };
        template <typename CharT>
        struct until {
            SCN_CONSTEXPR either<scan_status> operator()(CharT ch) const
                noexcept
            {
                return ch == until_ch ? scan_status::end : scan_status::keep;
            }

            CharT until_ch;
        };
        template <typename CharT>
        struct until_one_of {
            either<scan_status> operator()(CharT ch)
            {
                if (std::find(until.begin(), until.end(), ch) != until.end()) {
                    return scan_status::end;
                }
                return scan_status::keep;
            }

            span<CharT> until;
        };
        template <typename CharT>
        struct until_space {
            either<scan_status> operator()(CharT ch)
            {
                if (locale.is_space(ch)) {
                    return scan_status::end;
                }
                return scan_status::keep;
            }

            locale_ref<CharT> locale;
        };

        template <typename CharT>
        struct until_and_skip_chars {
            either<scan_status> operator()(CharT ch)
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
            either<scan_status> operator()(CharT ch)
            {
                if (std::find(until.begin(), until.end(), ch) != until.end()) {
                    return scan_status::end;
                }
                if (std::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            span<CharT> until;
            span<CharT> skip;
        };
        template <typename CharT>
        struct until_space_and_skip_chars {
            either<scan_status> operator()(CharT ch)
            {
                if (locale.is_space(ch)) {
                    return scan_status::end;
                }
                if (std::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            locale_ref<CharT> locale;
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
        auto n = std::min(s.size(), stream.chars_to_read());
        s = s.first(n);
        auto ret = stream.read_sized(s);
        if (!ret) {
            return {s.begin(), ret};
        }
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
                return {it, ret.get_error()};
            }
            *it = ret.value();
        }
        return {it};
    }

    // read_into_with_buffer

    namespace detail {
        template <typename Stream,
                  typename Iterator,
                  typename CharT = typename Stream::char_type>
        result<size_t> read_into_with_buffer(Stream& stream,
                                             Iterator it,
                                             span<CharT> buffer)
        {
            size_t n = 0, size = 0;
            while (true) {
                n = std::min(stream.chars_to_read(), size_t{64});
                if (n == 0) {
                    break;
                }
                buffer = buffer.first(n);
                auto ret = read(stream, buffer);
                if (!ret) {
                    return {size, ret.get_error()};
                }
                it = std::copy(buffer.begin(), buffer.end(), it);
                size += n;
            }
            return {size};
        }
        template <typename Stream,
                  typename Iterator,
                  typename Sentinel,
                  typename CharT = typename Stream::char_type>
        result<Iterator> read_into_with_buffer(Stream& stream,
                                               Iterator it,
                                               Sentinel end,
                                               span<CharT> buffer)
        {
            auto n = 0;
            while (true) {
                n = std::min({static_cast<size_t>(std::distance(it, end)),
                              stream.chars_to_read(), size_t{64}});
                if (n == 0) {
                    break;
                }
                buffer = buffer.first(n);
                auto ret = read(stream, buffer);
                if (!ret) {
                    return {it, ret.get_error()};
                }
                it = std::copy(buffer.begin(), buffer.end(), it);
            }
            return {it};
        }
    }  // namespace detail

    // read_into

    template <typename Stream,
              typename Iterator,
              typename CharT = typename Stream::char_type,
              typename std::enable_if<is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<Iterator> read_into(Stream& s, Iterator it)
    {
        auto n = s.chars_to_read();
        size_t size = 0;
        if (n > 64) {
            std::array<CharT, 64> arr;
            auto ret = read(s, make_span(arr));
            if (!ret) {
                return {size, ret.get_error()};
            }
            it = std::copy(arr.begin(), arr.begin() + n, it);
            size += n;
            return detail::read_into_with_buffer(s, it, make_span(arr));
        }
        std::array<CharT, 64> arr;
        auto ret = read(s, make_span(arr.data(), n));
        if (!ret) {
            return {size, ret.get_error()};
        }
        std::copy(arr.begin(), arr.begin() + n, it);
        return size + n;
    }
    template <typename Stream,
              typename Iterator,
              typename CharT = typename Stream::char_type,
              typename std::enable_if<!is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<Iterator> read_into(Stream& s, Iterator it)
    {
        size_t n = 0;
        while (true) {
            auto ret = s.read_char();
            if (!ret) {
                return {n, ret.get_error()};
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
              typename CharT = typename Stream::char_type,
              typename std::enable_if<is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<Iterator> read_into(Stream& s, Iterator it, Sentinel end)
    {
        auto n = std::min(static_cast<size_t>(std::distance(it, end)),
                          s.chars_to_read());
        if (n > 64) {
            std::array<CharT, 64> arr;
            auto ret = read(s, make_span(arr));
            if (!ret) {
                return {it, ret.get_error()};
            }
            it = std::copy(arr.begin(), arr.begin() + n, it);
            return detail::read_into_with_buffer(s, it, end, make_span(arr));
        }
        std::array<CharT, 64> arr;
        auto ret = read(s, make_span(arr.data(), n));
        if (!ret) {
            return {it, ret.get_error()};
        }
        return {std::copy(arr.begin(), arr.begin() + n, it)};
    }
    template <typename Stream,
              typename Iterator,
              typename Sentinel,
              typename CharT = typename Stream::char_type,
              typename std::enable_if<!is_sized_stream<Stream>::value>::type* =
                  nullptr>
    result<Iterator> read_into(Stream& s, Iterator it, Sentinel end)
    {
        for (; it != end; ++it) {
            auto ret = s.read_char();
            if (!ret) {
                return {it, ret.get_error()};
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
        std::array<CharT, 64> arr;
        bool end = false;
        while (!end) {
            n = std::min(s.chars_to_read(), std::size_t{64});
            if (n == 0) {
                break;
            }

            auto ret = read(s, make_span(arr.data(), n));
            if (!ret) {
                return {size, ret.error()};
            }

            const auto arr_end = arr.begin() + n;
            for (auto arr_it = arr.begin(); arr_it != arr_end; ++arr_it) {
                auto r = p(*arr_it);
                if (!r) {
                    return {size, r.get_error()};
                }
                if (r.value() == scan_status::skip) {
                    continue;
                }
                if (r.value() == scan_status::end) {
                    if (keep_final) {
                        ++arr_it;
                    }
                    if (arr_it != arr_end) {
                        auto pb = s.putback_n(static_cast<size_t>(
                            std::distance(arr_it, arr_end)));
                        if (!pb) {
                            s._set_bad();
                            return {size, pb};
                        }
                    }
                    end = true;
                    break;
                }
                *it = *arr_it;
                ++it;
                ++size;
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
                if (ret.get_error() == error::end_of_stream) {
                    break;
                }
                return {n, ret.get_error()};
            }

            auto r = p(ret.value());
            if (!r) {
                return {n, r.get_error()};
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
        auto n = 0;
        std::array<CharT, 64> arr;
        while (true) {
            n = std::min({static_cast<size_t>(std::distance(it, end)),
                          s.chars_to_read(), size_t{64}});
            if (n == 0) {
                break;
            }

            auto ret = read(s, make_span(arr.data(), n));
            if (!ret) {
                return {it, ret.get_error()};
            }

            const auto arr_end = arr.begin() + n;
            for (auto arr_it = arr.begin(); arr_it != arr_end; ++arr_it) {
                auto r = p(*arr_it);
                if (!r) {
                    return {it, r.get_error()};
                }
                if (r.value() == scan_status::skip) {
                    continue;
                }
                if (r.value() == scan_status::end) {
                    if (keep_final) {
                        ++arr_it;
                    }
                    if (arr_it != arr_end) {
                        if (!s.putback_n(std::distance(arr_it, arr_end))) {
                            s._set_bad();
                            return {it, s.get_error()};
                        }
                    }
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
                return {it, ret.get_error()};
            }

            auto r = p(ret.value());
            if (!r) {
                return r.get_error();
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
        return s.putback_n(static_cast<size_t>(std::distance(begin, end)));
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
        struct char_scanner : public empty_parser<CharT> {
            template <typename Context>
            error scan(CharT& val, Context& ctx)
            {
                auto ch = ctx.stream().read_char();
                if (!ch) {
                    return ch.get_error();
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
                std::copy(buf.begin(), buf.end(), val.begin());

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

                bool allow_set = false;
                bool a = false, n = false;
                for (auto ch = pctx.next();
                     pctx && !pctx.check_arg_end(ctx.locale());
                     pctx.advance(), ch = pctx.next()) {
                    if (ch == detail::ascii_widen<CharT>('l')) {
                        localized = true;
                    }
                    if (ch == detail::ascii_widen<CharT>('l')) {
                        localized = true;
                    }
                    else if (ch == detail::ascii_widen<CharT>('a')) {
                        a = true;
                        allow_set = true;
                    }
                    else if (ch == detail::ascii_widen<CharT>('n')) {
                        n = true;
                        allow_set = true;
                    }
                    else {
                        return error(error::invalid_format_string,
                                     "Expected 'l', 'a' or argument end in "
                                     "format string");
                    }
                }
                if (allow_set) {
                    allow_alpha = a;
                    allow_num = n;
                }

                if (SCN_UNLIKELY(localized && !allow_alpha)) {
                    return error(error::invalid_format_string,
                                 "boolalpha-mode cannot be enabled with 'l' "
                                 "(localized) specifier with bool");
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
                    auto truename = ctx.locale().get_default().truename();
                    auto falsename = ctx.locale().get_default().falsename();
                    if (localized) {
                        truename = ctx.locale().truename();
                        falsename = ctx.locale().falsename();
                    }
                    const auto max_len =
                        std::max(truename.size(), falsename.size());
                    std::basic_string<CharT> buf;
                    buf.reserve(max_len);

                    auto i = read_into_if(
                        ctx.stream(), std::back_inserter(buf),
                        predicates::until_space<CharT>{ctx.locale()});
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
                        return tmp.get_error();
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

                if (pctx.check_arg_end(ctx.locale())) {
                    pctx.arg_end();
                    return {};
                }

                if (pctx.next() == detail::ascii_widen<CharT>('l')) {
                    localized = thousands_separator | decimal | digits;
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('n')) {
                    localized = thousands_separator | decimal;
                    ctx.parse_context().advance();
                }

                if (SCN_UNLIKELY(!pctx)) {
                    return error(error::invalid_format_string,
                                 "Unexpected format string end");
                }
                if (pctx.check_arg_end(ctx.locale())) {
                    pctx.arg_end();
                    return {};
                }

                if (pctx.next() == detail::ascii_widen<CharT>('d')) {
                    base = 10;
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('x')) {
                    base = 16;
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('o')) {
                    base = 8;
                    pctx.advance();
                }
                else if (pctx.next() == detail::ascii_widen<CharT>('b')) {
                    pctx.advance();
                    if (SCN_UNLIKELY(!pctx)) {
                        return error(error::invalid_format_string,
                                     "Unexpected format string end");
                    }
                    if (SCN_UNLIKELY(pctx.check_arg_end(ctx.locale()))) {
                        return error(error::invalid_format_string,
                                     "Unexpected argument end");
                    }

                    const auto zero = detail::ascii_widen<CharT>('0'),
                               nine = detail::ascii_widen<CharT>('9');
                    int tmp = 0;
                    if (pctx.next() < zero || pctx.next() > nine) {
                        return error(
                            error::invalid_format_string,
                            "Invalid character after 'b', expected digit");
                    }
                    tmp = pctx.next() - zero;
                    pctx.advance();

                    if (pctx.check_arg_end(ctx.locale())) {
                        base = tmp;
                        pctx.arg_end();
                        return {};
                    }
                    if (pctx.next() < zero || pctx.next() > nine) {
                        return error(
                            error::invalid_format_string,
                            "Invalid character after 'b', expected digit");
                    }
                    tmp *= 10;
                    tmp += pctx.next() - zero;
                    if (tmp < 1 || tmp > 36) {
                        return error(error::invalid_format_string,
                                     "Invalid base, must be between 1 and 36");
                    }
                    base = tmp;
                    pctx.advance();
                }
                else {
                    return error(error::invalid_format_string,
                                 "Expected argument end");
                }

                if (localized && (base != 0 && base != 10)) {
                    return error(
                        error::invalid_format_string,
                        "Localized integers can only be scanned in base 10");
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

                auto r =
                    read_into_if(ctx.stream(), std::back_inserter(buf),
                                 predicates::until_space<CharT>{ctx.locale()});
                if (!r) {
                    return r.error();
                }
                buf.erase(buf.begin() + r.value(), buf.end());
                buf.push_back(detail::ascii_widen<CharT>(0));

                T tmp = 0;
                size_t chars = 0;

                if ((localized & digits) != 0) {
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    std::basic_string<CharT> str(buf.data(), buf.size());
                    auto ret = ctx.locale().read_num(tmp, str);
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

                    if (!ret) {
                        return ret.get_error();
                    }

                    auto pb = putback_range(
                        ctx.stream(), buf.rbegin(),
                        buf.rend() - static_cast<std::ptrdiff_t>(ret.value()));
                    if (!pb) {
                        return pb;
                    }
                    val = tmp;
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
                auto e = do_read()(tmp, make_span(buf).as_const(), base,
                                   ctx.locale());
                if (!e) {
                    return e.get_error();
                }
                chars = e.value();
                buf.pop_back();  // pop null terminator

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
                decimal = 2,
                digits = 4
            };

            int base{0};
            uint8_t localized{0};

        private:
            static either<size_t> _read_sto(T& val,
                                            span<const CharT> buf,
                                            int base,
                                            const basic_locale_ref<CharT>& loc);
            static either<size_t> _read_strto(
                T& val,
                span<const CharT> buf,
                int base,
                const basic_locale_ref<CharT>& loc);
            static either<size_t> _read_from_chars(
                T& val,
                span<const CharT> buf,
                int base,
                const basic_locale_ref<CharT>& loc);
            static either<size_t> _read_custom(
                T& val,
                span<const CharT> buf,
                int base,
                const basic_locale_ref<CharT>& loc);

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

                bool point = false;
                auto r = read_into_if(
                    ctx.stream(), std::back_inserter(buf),
                    [&point, &ctx](CharT ch) -> either<scan_status> {
                        if (ctx.locale().is_space(ch)) {
                            return scan_status::end;
                        }
                        if (ch == ctx.locale().thousands_separator()) {
                            return scan_status::skip;
                        }
                        if (ch == ctx.locale().decimal_point()) {
                            if (point) {
                                return error(error::invalid_scanned_value,
                                             "Extra decimal separator found in "
                                             "parsing floating-point number");
                            }
                            point = true;
                        }
                        return scan_status::keep;
                    });
                if (!r) {
                    return r.error();
                }
                buf.erase(buf.begin() + r.value(), buf.end());
                buf.push_back(CharT{0});

                T tmp{};
                size_t chars = 0;

                if (localized) {
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    auto str = std::basic_string<CharT>(buf.data(), buf.size());
                    auto ret = ctx.locale().read_num(tmp, str);
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

                    if (!ret) {
                        return ret.get_error();
                    }
                    chars = ret.value();

                    auto pb = putback_range(
                        ctx.stream(), buf.rbegin(),
                        buf.rend() - static_cast<std::ptrdiff_t>(chars));
                    if (!pb) {
                        return pb;
                    }
                    val = tmp;
                }
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
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    SCN_UNREACHABLE;
                    SCN_GCC_POP
                };
                auto e =
                    do_read()(tmp, make_span(buf).as_const(), ctx.locale());
                if (!e) {
                    return e.get_error();
                }
                chars = e.value();
                buf.pop_back();

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
            static either<size_t> _read_sto(T& val,
                                            span<const CharT> buf,
                                            basic_locale_ref<CharT>& loc);
            static either<size_t> _read_strto(T& val,
                                              span<const CharT> buf,
                                              basic_locale_ref<CharT>& loc);
            static either<size_t> _read_from_chars(
                T& val,
                span<const CharT> buf,
                basic_locale_ref<CharT>& loc);
            static either<size_t> _read_custom(T& val,
                                               span<const CharT> buf,
                                               basic_locale_ref<CharT>& loc);
        };

        template <typename CharT>
        struct string_scanner : public empty_parser<CharT> {
        public:
            template <typename Context>
            error scan(std::basic_string<CharT>& val, Context& ctx)
            {
                val.clear();
                val.reserve(15);
                auto s =
                    read_into_if(ctx.stream(), std::back_inserter(val),
                                 predicates::until_space<CharT>{ctx.locale()});
                if (SCN_UNLIKELY(!s)) {
                    return s.error();
                }
                val.erase(s.value());
                if (SCN_UNLIKELY(val.empty())) {
                    return error(error::invalid_scanned_value,
                                 "Empty string parsed");
                }

                return {};
            }
        };
    }  // namespace detail

    /**
     * Skip any whitespace from the stream.
     * Next read_char() will return the first non-whitespace character of EOF.
     * \param ctx Stream and locale to use
     * error::end_of_stream if `false`
     */
    template <typename Context>
    error skip_stream_whitespace(Context& ctx) noexcept
    {
        while (true) {
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

            auto ch = ctx.stream().read_char();
            if (!ch) {
                return ch.get_error();
            }
            if (!ctx.locale().is_space(ch.value())) {
                auto pb = ctx.stream().putback(ch.value());
                if (!pb) {
                    return pb;
                }
                break;
            }

            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
        }
        return {};
    }

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
        template <typename T>
        auto visit(T& val, detail::priority_tag<0>) ->
            typename std::enable_if<std::is_integral<T>::value &&
                                        !std::is_same<T, char_type>::value,
                                    error>::type
        {
            detail::integer_scanner<char_type, T> s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        template <typename T>
        auto visit(T&, detail::priority_tag<0>) ->
            typename std::enable_if<std::is_integral<T>::value &&
                                        std::is_same<T, char_type>::value,
                                    error>::type
        {
            return error(error::invalid_operation,
                         "Cannot scan this type with this char_type");
        }
        template <typename T>
        auto visit(T& val, detail::priority_tag<1>) ->
            typename std::enable_if<std::is_floating_point<T>::value,
                                    error>::type
        {
            detail::float_scanner<char_type, T> s;
            auto err = parse(s);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
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
    result<int> visit(Context& ctx)
    {
        int args_read = 0;

        auto reterror = [&args_read](error e) -> result<int> {
            return result<int>(args_read, std::move(e));
        };

        auto& pctx = ctx.parse_context();
        auto arg_wrapped = ctx.next_arg();
        if (!arg_wrapped) {
            return reterror(arg_wrapped.get_error());
        }
        auto arg = arg_wrapped.value();

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
                    if (ret == error::end_of_stream && !arg) {
                        return {args_read};
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
                        return reterror(ret.get_error());
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
                arg_wrapped = ctx.next_arg();
                if (!arg_wrapped) {
                    return reterror(arg_wrapped.get_error());
                }
                arg = arg_wrapped.value();
                pctx.advance();
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
