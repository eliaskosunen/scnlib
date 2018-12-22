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

#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif

namespace scn {
    enum class scan_status { keep, skip, end };

    namespace predicates {
        template <typename Context>
        struct propagate {
            expected<scan_status, error> operator()(typename Context::char_type)
            {
                return scan_status::keep;
            }
        };
        template <typename Context>
        struct until_space {
            expected<scan_status, error> operator()(
                typename Context::char_type ch)
            {
                if (ctx.locale().is_space(ch)) {
                    return scan_status::end;
                }
                return scan_status::keep;
            }

            Context& ctx;
        };
        template <typename Context>
        struct until_space_and_skip_chars {
            expected<scan_status, error> operator()(
                typename Context::char_type ch)
            {
                if (ctx.locale().is_space(ch)) {
                    return scan_status::end;
                }
                if (std::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            Context& ctx;
            span<typename Context::char_type> skip;
        };
    }  // namespace predicates

    template <typename Context, typename Iterator, typename Predicate>
    expected<Iterator, error> scan_chars(Context& ctx,
                                         Iterator begin,
                                         Predicate&& p)
    {
        while (true) {
            auto ret = ctx.stream().read_char();
            if (!ret) {
                if (ret.error() == error::end_of_stream) {
                    return begin;
                }
                return make_unexpected(ret.error());
            }

            auto r = p(ret.value());
            if (!r) {
                return make_unexpected(r.error());
            }
            if (r.value() == scan_status::skip) {
                continue;
            }
            if (r.value() == scan_status::end) {
                break;
            }
            *begin = ret.value();
            ++begin;
        }
        return begin;
    }
    template <typename Context,
              typename Iterator,
              typename EndIterator,
              typename Predicate>
    expected<Iterator, error> scan_chars_until(Context& ctx,
                                               Iterator begin,
                                               EndIterator end,
                                               Predicate&& p)
    {
        while (begin != end) {
            auto ret = ctx.stream().read_char();
            if (!ret) {
                if (ret.error() == error::end_of_stream) {
                    return begin;
                }
                return make_unexpected(ret.error());
            }

            auto r = p(ret.value());
            if (!r) {
                return make_unexpected(r.error());
            }
            if (r.value() == scan_status::skip) {
                continue;
            }
            if (r.value() == scan_status::end) {
                break;
            }
            *begin = ret.value();
            ++begin;
        }
        return begin;
    }
    namespace detail {
        template <typename CharT>
        struct empty_parser {
            template <typename Context>
            expected<void, error> parse(Context& ctx)
            {
                if (*ctx.parse_context().begin() != ctx.locale().widen('{')) {
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
            auto s = scan_chars_until(ctx, buf.begin(), buf.end(),
                                      predicates::propagate<Context>{});
            if (!s) {
                return make_unexpected(s.error());
            }
            std::copy(buf.begin(), s.value(), val.begin());

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
                if (ch == ctx.locale().widen('}')) {
                    break;
                }
                else if (ch == ctx.locale().widen('l')) {
                    localized = true;
                }
                else if (ch == ctx.locale().widen('a')) {
                    boolalpha = true;
                }
                else {
                    return make_unexpected(error::invalid_format_string);
                }
            }

            if (localized && !boolalpha) {
                return make_unexpected(error::invalid_format_string);
            }
            if (ch != ctx.locale().widen('}')) {
                return make_unexpected(error::invalid_format_string);
            }
            return {};
        }

        template <typename Context>
        expected<void, error> scan(bool& val, Context& ctx)
        {
            if (boolalpha) {
                const auto truename = ctx.locale().truename();
                const auto falsename = ctx.locale().falsename();
                const auto max_len =
                    std::max(truename.size(), falsename.size());
                std::basic_string<CharT> buf(max_len, 0);

                auto it =
                    scan_chars_until(ctx, buf.begin(), buf.end(),
                                     predicates::until_space<Context>{ctx});
                if (!it) {
                    return make_unexpected(it.error());
                }
                buf.erase(it.value());

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
                    for (auto i = buf.rbegin();
                         i !=
                         buf.rend() - static_cast<typename std::iterator_traits<
                                          decltype(i)>::difference_type>(chars);
                         ++i) {
                        auto ret = ctx.stream().putback(*i);
                        if (!ret) {
                            return ret;
                        }
                    }
                    return {};
                }
            }
            else {
                auto tmp = ctx.stream().read_char();
                if (!tmp) {
                    return make_unexpected(tmp.error());
                }
                if (tmp == ctx.locale().widen('0')) {
                    val = false;
                    return {};
                }
                if (tmp == ctx.locale().widen('1')) {
                    val = true;
                    return {};
                }
            }

            return make_unexpected(error::invalid_scanned_value);
        }

        bool localized{false};
        bool boolalpha{false};
    };

    namespace detail {
        template <typename CharT, typename T>
        struct str_to_int;

        template <typename CharT>
        struct str_to_int<CharT, long long> {
            static expected<long long, error>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                return std::stoll(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, long> {
            static expected<long, error>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                return std::stol(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, int> {
            static expected<int, error> get(const std::basic_string<CharT>& str,
                                            size_t& chars,
                                            int base)
            {
                return std::stoi(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, short> {
            static expected<short, error>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                auto i = std::stoi(str, &chars, base);
                if (i > static_cast<int>(std::numeric_limits<short>::max())) {
                    return make_unexpected(error::value_out_of_range);
                }
                if (i < static_cast<int>(std::numeric_limits<short>::min())) {
                    return make_unexpected(error::value_out_of_range);
                }
                return static_cast<short>(i);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned long long> {
            static expected<unsigned long long, error>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                return std::stoull(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned long> {
            static expected<unsigned long, error>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                return std::stoul(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned int> {
            static expected<unsigned int, error>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                auto i = std::stoul(str, &chars, base);
                if (i > static_cast<unsigned long>(
                            std::numeric_limits<unsigned int>::max())) {
                    return make_unexpected(error::value_out_of_range);
                }
                return static_cast<unsigned int>(i);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned short> {
            static expected<unsigned short, error>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                auto i = std::stoul(str, &chars, base);
                if (i > static_cast<unsigned long>(
                            std::numeric_limits<unsigned short>::max())) {
                    return make_unexpected(error::value_out_of_range);
                }
                return static_cast<unsigned short>(i);
            }
        };
    }  // namespace detail

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

            if (ch == ctx.locale().widen('}')) {
                return {};
            }

            if (ch == ctx.locale().widen('l')) {
                localized = thousands_separator | decimal | digits;
                ctx.parse_context().advance();
            }
            else if (ch == ctx.locale().widen('n')) {
                localized = thousands_separator | decimal;
                ctx.parse_context().advance();
            }
            ch = *ctx.parse_context().begin();
            if (ch == ctx.locale().widen('}')) {
                return {};
            }

            if (ch == ctx.locale().widen('d')) {
                base = 10;
            }
            else if (ch == ctx.locale().widen('x')) {
                base = 16;
            }
            else if (ch == ctx.locale().widen('o')) {
                base = 8;
            }
            else if (ch == ctx.locale().widen('b')) {
                ctx.parse_context().advance();
                ch = *ctx.parse_context().begin();

                int tmp = 0;
                if (ch < ctx.locale().widen('0') ||
                    ch > ctx.locale().widen('9')) {
                    return make_unexpected(error::invalid_format_string);
                }
                tmp = ch - ctx.locale().widen('0');
                ctx.parse_context().advance();
                ch = *ctx.parse_context().begin();

                if (ch == ctx.locale().widen('}')) {
                    base = tmp;
                    return {};
                }
                if (ch < ctx.locale().widen('0') ||
                    ch > ctx.locale().widen('9')) {
                    return make_unexpected(error::invalid_format_string);
                }
                tmp *= 10;
                tmp += ch - ctx.locale().widen('0');
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

            auto thousands_sep = ctx.locale().thousands_separator();
            auto thsep_span = span<CharT>(&thousands_sep, 1);
            auto r = scan_chars(ctx, std::back_inserter(buf),
                                predicates::until_space_and_skip_chars<Context>{
                                    ctx, thsep_span});
            if (!r) {
                return make_unexpected(r.error());
            }

            T tmp = 0;
            size_t chars = 0;

            auto putback = [&chars, &ctx, &buf]() -> expected<void, error> {
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
#if SCN_MSVC
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4127)  // conditional expression is constant
#endif
                if (std::is_unsigned<T>::value) {
                    if (buf.front() == ctx.locale().widen('-')) {
                        return make_unexpected(error::value_out_of_range);
                    }
                }
                auto ret = detail::str_to_int<CharT, T>::get(buf, chars, base);
                if (!ret) {
                    return make_unexpected(ret.error());
                }
                tmp = ret.value();
#if SCN_MSVC
#pragma warning(pop)
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

    namespace detail {
        template <typename CharT, typename T>
        struct str_to_float;

        template <typename CharT>
        struct str_to_float<CharT, float> {
            static float get(const std::basic_string<CharT>& str, size_t& chars)
            {
                return std::stof(str, &chars);
            }
        };
        template <typename CharT>
        struct str_to_float<CharT, double> {
            static double get(const std::basic_string<CharT>& str,
                              size_t& chars)
            {
                return std::stod(str, &chars);
            }
        };
        template <typename CharT>
        struct str_to_float<CharT, long double> {
            static long double get(const std::basic_string<CharT>& str,
                                   size_t& chars)
            {
                return std::stold(str, &chars);
            }
        };
    }  // namespace detail

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

            if (ch == ctx.locale().widen('}')) {
                return {};
            }

            if (ch == ctx.locale().widen('l')) {
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
            auto r = scan_chars(
                ctx, std::back_inserter(buf),
                [&ctx, &point](CharT ch) -> expected<scan_status, error> {
                    if (ctx.locale().is_space(ch)) {
                        return scan_status::end;
                    }
                    if (ch == ctx.locale().thousands_separator()) {
                        return scan_status::skip;
                    }
                    if (ch == ctx.locale().decimal_point()) {
                        if (point) {
                            return make_unexpected(
                                error::invalid_scanned_value);
                        }
                        point = true;
                    }
                    return scan_status::keep;
                });
            if (!r) {
                return make_unexpected(r.error());
            }

            T tmp{};
            size_t chars = 0;

            auto putback = [&chars, &buf, &ctx]() -> expected<void, error> {
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
                tmp = detail::str_to_float<CharT, T>::get(buf, chars);
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

#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
// -Wundefined-func-template
#pragma clang diagnostic pop
#endif

#endif  // SCN_TYPES_H

