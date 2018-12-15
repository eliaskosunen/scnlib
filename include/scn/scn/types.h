// Copyright 2017-2018 Elias Kosunen
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

#ifndef SCN_TYPES_H
#define SCN_TYPES_H

#include "core.h"
#include "util.h"

#include <vector>

#if SCN_HAS_CHARCONV
#include <charconv>
#endif

namespace scn {
    namespace detail {
        template <typename CharT>
        struct empty_parser {
            template <typename Context>
            expected<void, error> parse(Context& ctx)
            {
                if (*ctx.parse_context().begin() != CharT('{')) {
                    return make_unexpected(error::invalid_format_string);
                }
                ctx.parse_context().advance();
                return {};
            }
        };
    }  // namespace detail

    template <typename CharT>
    struct basic_value_scanner<CharT, CharT>
        : public detail::empty_parser<CharT> {
        template <typename Context>
        expected<void, error> scan(CharT& val, Context& ctx)
        {
            auto ch = ctx.stream().read_char();
            if (!ch) {
                return make_unexpected(ch.error());
            }
            val = ch.value();
            return {};
        }
    };

    template <typename CharT>
    struct basic_value_scanner<CharT, span<CharT>>
        : public detail::empty_parser<CharT> {
        template <typename Context>
        expected<void, error> scan(span<CharT> val, Context& ctx)
        {
            if (val.size() == 0) {
                return {};
            }

            std::vector<CharT> buf(static_cast<size_t>(val.size()));
            auto it = buf.begin();
            for (; it != buf.end(); ++it) {
                auto ch = ctx.stream().read_char();
                if (!ch) {
                    return make_unexpected(ch.error());
                }
                if (ctx.locale().is_space(ch.value())) {
                    break;
                }
                *it = ch.value();
            }
            std::copy(buf.begin(), buf.end(), val.begin());

            return {};
        }
    };

    template <typename CharT>
    struct basic_value_scanner<CharT, bool> {
        template <typename Context>
        expected<void, error> parse(Context& ctx)
        {
            // {}: no boolalpha, not localized
            // l: localized
            // a: boolalpha
            ctx.parse_context().advance();
            auto ch = *ctx.parse_context().begin();

            for (; ch; ctx.parse_context().advance(),
                       ch = *ctx.parse_context().begin()) {
                if (ch == CharT('}')) {
                    break;
                }
                else if (ch == CharT('l')) {
                    localized = true;
                }
                else if (ch == CharT('a')) {
                    boolalpha = true;
                }
                else {
                    return make_unexpected(error::invalid_format_string);
                }
            }

            if (localized && !boolalpha) {
                return make_unexpected(error::invalid_format_string);
            }
            if (ch != CharT('}')) {
                return make_unexpected(error::invalid_format_string);
            }
            return {};
        }

        template <typename Context>
        expected<void, error> scan(bool& val, Context& ctx)
        {
            if (boolalpha) {
#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif
                auto truename = ctx.locale().truename();
                auto falsename = ctx.locale().falsename();
                const auto max_len =
                    std::max(truename.size(), falsename.size());
                std::basic_string<CharT> buf(max_len, 0);
#if SCN_CLANG
#pragma clang diagnostic pop
#endif

                auto it = buf.begin();
                for (; it != buf.end(); ++it) {
                    auto ch = ctx.stream().read_char();
                    if (!ch) {
                        if (ch.error() == error::end_of_stream) {
                            break;
                        }
                        return make_unexpected(ch.error());
                    }
                    *it = ch.value();

                    if (std::equal(buf.begin(), it + 1, falsename.begin())) {
                        val = false;
                        return {};
                    }
                    if (std::equal(buf.begin(), it + 1, truename.begin())) {
                        val = true;
                        return {};
                    }
                }
            }
            else {
                auto tmp = ctx.stream().read_char();
                if (!tmp) {
                    return make_unexpected(tmp.error());
                }
                if (tmp == CharT('0')) {
                    val = false;
                    return {};
                }
                if (tmp == CharT('1')) {
                    val = true;
                    return {};
                }
            }

            return make_unexpected(error::invalid_scanned_value);
        }

        bool localized{false};
        bool boolalpha{false};
    };

    template <typename CharT, typename T>
    struct basic_value_scanner<
        CharT,
        T,
        typename std::enable_if<std::is_integral<T>::value &&
                                !std::is_same<T, CharT>::value &&
                                !std::is_same<T, bool>::value>::type> {
        template <typename Context>
        expected<void, error> parse(Context& ctx)
        {
            // {}: base detect, not localized
            // n: localized decimal/thousand separator
            // l: n + localized digits
            // d: decimal, o: octal, x: hex, b[1-36]: base
            ctx.parse_context().advance();
            auto ch = *ctx.parse_context().begin();

            if (ch == CharT('}')) {
                return {};
            }

            if (ch == CharT('l')) {
                localized = thousands_separator | decimal | digits;
                ctx.parse_context().advance();
            }
            else if (ch == CharT('n')) {
                localized = thousands_separator | decimal;
                ctx.parse_context().advance();
            }
            ch = *ctx.parse_context().begin();
            if (ch == CharT('}')) {
                return {};
            }

            if (ch == CharT('d')) {
                base = 10;
            }
            else if (ch == CharT('x')) {
                base = 16;
            }
            else if (ch == CharT('o')) {
                base = 8;
            }
            else if (ch == CharT('b')) {
                ctx.parse_context().advance();
                ch = *ctx.parse_context().begin();

                int tmp = 0;
                if (ch < CharT('0') || ch > CharT('9')) {
                    return make_unexpected(error::invalid_format_string);
                }
                tmp = ch - CharT('0');
                ctx.parse_context().advance();
                ch = *ctx.parse_context().begin();

                if (ch == CharT('}')) {
                    base = tmp;
                    return {};
                }
                if (ch < CharT('0') || ch > CharT('9')) {
                    return make_unexpected(error::invalid_format_string);
                }
                tmp *= 10;
                tmp += ch - CharT('0');
                if (tmp < 1 || tmp > 36) {
                    return make_unexpected(error::invalid_format_string);
                }
                base = tmp;
            }
            else {
                return make_unexpected(error::invalid_format_string);
            }

            if (localized && (base != 0 && base != 10)) {
                return make_unexpected(error::invalid_format_string);
            }
            return {};
        }

        template <typename Context>
        expected<void, error> scan(T& val, Context& ctx)
        {
            std::basic_string<CharT> buf{};
            buf.reserve(static_cast<size_t>(
                detail::max_digits<T>(base == 0 ? 8 : base)));

            while (true) {
                auto ch = ctx.stream().read_char();
                if (!ch) {
                    if (ch.error() == error::end_of_stream) {
                        break;
                    }
                    return make_unexpected(ch.error());
                }
                if (ctx.locale().is_space(ch.value())) {
                    break;
                }
#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif
                if (ctx.locale().thousands_separator() == ch.value()) {
                    continue;
                }
#if SCN_CLANG
#pragma clang diagnostic pop
#endif
                buf.push_back(ch.value());
            }

            T tmp = 0;
            size_t chars = 0;

            auto putback = [&]() -> expected<void, error> {
                for (auto it = buf.rbegin();
                     it !=
                     buf.rend() - static_cast<typename std::iterator_traits<
                                      decltype(it)>::difference_type>(chars);
                     ++it) {
                    auto ret = ctx.stream().putback(*it);
                    if (!ret) {
                        return ret;
                    }
                }
                return {};
            };

            if ((localized & digits) != 0) {
                auto ret = ctx.locale().read_num(tmp, buf);
                if (!ret) {
                    return make_unexpected(ret.error());
                }
                chars = ret.value();
                auto pb = putback();
                if (!pb) {
                    return pb;
                }
                val = tmp;
            }
#if SCN_HAS_CHARCONV
            auto begin = buf.data();
            auto end = begin + buf.size();
            auto result = std::from_chars(begin, end, tmp, base);
            if (result.ec == std::errc::result_out_of_range) {
                return make_unexpected(error::value_out_of_range);
            }
            if (result.ec == std::errc::invalid_argument) {
                return make_unexpected(error::invalid_scanned_value);
            }

            for (auto it = buf.rbegin(); it != result.ptr; ++it) {
                auto ret = ctx.stream().putback(*it);
                if (!ret) {
                    return ret;
                }
            }
            val = tmp;
#else
            try {
#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#if SCN_MSVC
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4127)  // conditional expression is constant
#endif
                if (std::is_unsigned<T>::value) {
                    if (buf.front() == CharT('-')) {
                        return make_unexpected(error::value_out_of_range);
                    }
                    if (std::is_same<T, unsigned long long>::value) {
                        tmp = std::stoull(buf, &chars, base);
                    }
                    else {
                        auto i = std::stoul(buf, &chars, base);
                        if (i > static_cast<unsigned long>(
                                    std::numeric_limits<T>::max())) {
                            return make_unexpected(error::value_out_of_range);
                        }
                        tmp = static_cast<T>(i);
                    }
                }
                else {
                    if (std::is_same<T, long long>::value) {
                        tmp = std::stoll(buf, &chars, base);
                    }
                    else if (std::is_same<T, long>::value) {
                        tmp = std::stol(buf, &chars, base);
                    }
                    else {
                        auto i = std::stoi(buf, &chars, base);
                        if (i >
                            static_cast<int>(std::numeric_limits<T>::max())) {
                            return make_unexpected(error::value_out_of_range);
                        }
                        if (i <
                            static_cast<int>(std::numeric_limits<T>::min())) {
                            return make_unexpected(error::value_out_of_range);
                        }
                        tmp = static_cast<T>(i);
                    }
                }
#if SCN_MSVC
#pragma warning(pop)
#endif
#if SCN_CLANG
#pragma clang diagnostic pop
#endif
            }
            catch (const std::invalid_argument&) {
                return make_unexpected(error::invalid_scanned_value);
            }
            catch (const std::out_of_range&) {
                return make_unexpected(error::value_out_of_range);
            }

            auto pb = putback();
            if (!pb) {
                return pb;
            }
            val = tmp;
#endif
            return {};
        }

        enum localized_type {
            thousands_separator = 1,
            decimal = 2,
            digits = 4
        };

        int base{0};
        int localized{0};
    };

    template <typename CharT, typename T>
    struct basic_value_scanner<
        CharT,
        T,
        typename std::enable_if<std::is_floating_point<T>::value>::type> {
        template <typename Context>
        expected<void, error> parse(Context& ctx)
        {
            // {}: not localized
            // l: localized
            ctx.parse_context().advance();
            auto ch = *ctx.parse_context().begin();

            if (ch == CharT('}')) {
                return {};
            }

            if (ch == CharT('l')) {
                localized = true;
                ctx.parse_context().advance();
                ch = *ctx.parse_context().begin();
            }
            return make_unexpected(error::invalid_format_string);
        }
        template <typename Context>
        expected<void, error> scan(T& val, Context& ctx)
        {
            std::basic_string<CharT> buf{};
            buf.reserve(21);

            bool point = false;
            while (true) {
                auto tmp = ctx.stream().read_char();
                if (!tmp) {
                    if (tmp.error() == error::end_of_stream) {
                        break;
                    }
                    return make_unexpected(tmp.error());
                }
                if (ctx.locale().is_space(tmp.value())) {
                    break;
                }
                if (tmp.value() == ctx.locale().thousands_separator()) {
                    continue;
                }
                if (tmp.value() == ctx.locale().decimal_point()) {
                    if (point) {
                        return make_unexpected(error::invalid_scanned_value);
                    }
                    point = true;
                    buf.push_back(tmp.value());
                    continue;
                }
                buf.push_back(tmp.value());
            }

            T tmp{};
            size_t chars = 0;

            auto putback = [&]() -> expected<void, error> {
                for (auto it = buf.rbegin();
                     it !=
                     buf.rend() - static_cast<typename std::iterator_traits<
                                      decltype(it)>::difference_type>(chars);
                     ++it) {
                    auto ret = ctx.stream().putback(*it);
                    if (!ret) {
                        return ret;
                    }
                }
                return {};
            };

            if (localized) {
                auto ret = ctx.locale().read_num(tmp, buf);
                if (!ret) {
                    return make_unexpected(ret.error());
                }
                chars = ret.value();
                auto pb = putback();
                if (!pb) {
                    return pb;
                }
                val = tmp;
            }
#if SCN_HAS_CHARCONV
            auto begin = buf.data();
            auto end = begin + buf.size();
            auto result = std::from_chars(begin, end, tmp);
            if (result.ec == std::errc::result_out_of_range) {
                return make_unexpected(error::value_out_of_range);
            }
            if (result.ec == std::errc::invalid_argument) {
                return make_unexpected(error::invalid_scanned_value);
            }

            for (auto it = buf.rbegin(); it != result.ptr; ++it) {
                auto ret = ctx.stream().putback(*it);
                if (!ret) {
                    return ret;
                }
            }
            val = tmp;
#else
            try {
#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
#if SCN_MSVC
#pragma warning(push)
#pragma warning(disable : 4244)  // narrowing conversion
#endif
                if (std::is_same<T, float>::value) {
                    tmp = std::stof(buf, &chars);
                }
                else if (std::is_same<T, double>::value) {
                    tmp = std::stod(buf, &chars);
                }
                else {
                    tmp = std::stold(buf, &chars);
                }
#if SCN_MSVC
#pragma warning(pop)
#endif
#if SCN_CLANG
#pragma clang diagnostic pop
#endif
            }
            catch (const std::invalid_argument&) {
                return make_unexpected(error::invalid_scanned_value);
            }
            catch (const std::out_of_range&) {
                return make_unexpected(error::value_out_of_range);
            }

            auto pb = putback();
            if (!pb) {
                return pb;
            }
            val = tmp;
#endif

            return {};
        }

        bool localized{false};
    };
}  // namespace scn

#endif  // SCN_TYPES_H
