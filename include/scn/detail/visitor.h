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

#include "args.h"
#include "context.h"
#include "stream.h"

namespace scn {
    enum class scan_status { keep, skip, end };

    namespace predicates {
        template <typename Context>
        struct propagate {
            SCN_CONSTEXPR result<scan_status> operator()(
                Context&,
                typename Context::char_type) const noexcept
            {
                return scan_status::keep;
            }
        };
        template <typename Context>
        struct until {
            SCN_CONSTEXPR result<scan_status> operator()(
                Context&,
                typename Context::char_type ch) const noexcept
            {
                return ch == until_ch ? scan_status::end : scan_status::keep;
            }

            typename Context::char_type until_ch;
        };
        template <typename Context>
        struct until_one_of {
            result<scan_status> operator()(Context&,
                                           typename Context::char_type ch)
            {
                if (std::find(until.begin(), until.end(), ch) != until.end()) {
                    return scan_status::end;
                }
                return scan_status::keep;
            }

            span<typename Context::char_type> until;
        };
        template <typename Context>
        struct until_space {
            result<scan_status> operator()(Context& ctx,
                                           typename Context::char_type ch)
            {
                if (ctx.locale().is_space(ch)) {
                    return scan_status::end;
                }
                return scan_status::keep;
            }
        };

        template <typename Context>
        struct until_and_skip_chars {
            result<scan_status> operator()(Context&,
                                           typename Context::char_type ch)
            {
                if (ch == until) {
                    return scan_status::end;
                }
                if (std::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            typename Context::char_type until;
            span<typename Context::char_type> skip;
        };
        template <typename Context>
        struct until_one_of_and_skip_chars {
            result<scan_status> operator()(Context&,
                                           typename Context::char_type ch)
            {
                if (std::find(until.begin(), until.end(), ch) != until.end()) {
                    return scan_status::end;
                }
                if (std::find(skip.begin(), skip.end(), ch) != skip.end()) {
                    return scan_status::skip;
                }
                return scan_status::keep;
            }

            span<typename Context::char_type> until;
            span<typename Context::char_type> skip;
        };
        template <typename Context>
        struct until_space_and_skip_chars {
            result<scan_status> operator()(Context& ctx,
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

            span<typename Context::char_type> skip;
        };
    }  // namespace predicates

    namespace pred = predicates;

    template <typename Context, typename Iterator, typename Predicate>
    result<Iterator> scan_chars(Context& ctx,
                                Iterator begin,
                                Predicate&& p,
                                bool keep_final = false)
    {
        while (true) {
            auto ret = ctx.stream().read_char();
            if (!ret) {
                if (ret.get_error() == error::end_of_stream) {
                    return begin;
                }
                return ret.get_error();
            }

            auto r = p(ctx, ret.value());
            if (!r) {
                return r.get_error();
            }
            if (r.value() == scan_status::skip) {
                continue;
            }
            if (r.value() == scan_status::end) {
                if (keep_final) {
                    *begin = ret.value();
                }
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
    result<Iterator> scan_chars_until(Context& ctx,
                                      Iterator begin,
                                      EndIterator end,
                                      Predicate&& p,
                                      bool keep_final = false)
    {
        while (begin != end) {
            auto ret = ctx.stream().read_char();
            if (!ret) {
                if (ret.get_error() == error::end_of_stream) {
                    return begin;
                }
                return ret.get_error();
            }

            auto r = p(ctx, ret.value());
            if (!r) {
                return r.get_error();
            }
            if (r.value() == scan_status::skip) {
                continue;
            }
            if (r.value() == scan_status::end) {
                if (keep_final) {
                    *begin = ret.value();
                }
                break;
            }
            *begin = ret.value();
            ++begin;
        }
        return begin;
    }

    template <typename Context, typename Iterator, typename EndIterator>
    result<Iterator> propagate_chars_until(Context& ctx,
                                           Iterator begin,
                                           EndIterator end)
    {
        for (; begin != end; ++begin) {
            auto ret = ctx.stream().read_char();
            if (!ret) {
                if (ret.get_error() == error::end_of_stream) {
                    return begin;
                }
                return ret.get_error();
            }

            *begin = ret.value();
        }
        return begin;
    }

    template <typename Context,
              typename Char = typename Context::char_type,
              typename Span = span<Char>,
              typename Iterator = typename Span::iterator>
    result<Iterator> bulk_propagate_chars_until(Context& ctx, Span s)
    {
        auto e = ctx.stream().read_bulk(s);
        if (e != error::code::end_of_stream) {
            return s.end();
        }
        return propagate_chars_until(ctx, s.begin(), s.end());
    }

    template <typename Context,
              typename Char = typename Context::char_type,
              typename Span = span<Char>,
              typename Iterator = typename Span::iterator>
    auto propagate_chars_until(Context& ctx, Span s) -> typename std::enable_if<
        is_bulk_stream<typename Context::stream_type>::value,
        result<Iterator>>::type
    {
        return bulk_propagate_chars_until(ctx, s);
    }
    template <typename Context,
              typename Char = typename Context::char_type,
              typename Span = span<Char>,
              typename Iterator = typename Span::iterator>
    auto propagate_chars_until(Context& ctx, Span s) -> typename std::enable_if<
        !is_bulk_stream<typename Context::stream_type>::value,
        result<Iterator>>::type
    {
        return propagate_chars_until(ctx, s.begin(), s.end());
    }

    namespace detail {
        template <typename CharT>
        struct empty_parser {
            template <typename Context>
            error parse(Context& ctx)
            {
                if (*ctx.parse_context().begin() != ctx.locale().widen('{')) {
                    return error::invalid_format_string;
                }
                ctx.parse_context().advance();
                return {};
            }
        };

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

                std::basic_string<CharT> buf(static_cast<size_t>(val.size()),
                                             0);
                auto span = scn::make_span(buf);
                auto s = propagate_chars_until(ctx, span);
                if (!s) {
                    return s.get_error();
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
                ctx.parse_context().advance();
                auto ch = *ctx.parse_context().begin();

                for (; ch; ctx.parse_context().advance(),
                           ch = *ctx.parse_context().begin()) {
                    if (ch == ctx.locale().widen('}')) {
                        break;
                    }
                    if (ch == ctx.locale().widen('l')) {
                        localized = true;
                    }
                    else if (ch == ctx.locale().widen('a')) {
                        boolalpha = true;
                    }
                    else {
                        return error::invalid_format_string;
                    }
                }

                if (localized && !boolalpha) {
                    return error::invalid_format_string;
                }
                if (ch != ctx.locale().widen('}')) {
                    return error::invalid_format_string;
                }
                return {};
            }

            template <typename Context>
            error scan(bool& val, Context& ctx)
            {
                if (boolalpha) {
                    auto truename = ctx.locale().get_default().truename();
                    auto falsename = ctx.locale().get_default().falsename();
                    if (localized) {
                        truename = ctx.locale().truename();
                        falsename = ctx.locale().falsename();
                    }
                    const auto max_len =
                        std::max(truename.size(), falsename.size());
                    std::basic_string<CharT> buf(max_len, 0);

                    auto it =
                        scan_chars_until(ctx, buf.begin(), buf.end(),
                                         predicates::until_space<Context>{});
                    if (!it) {
                        return it.get_error();
                    }
                    if (it.value() != buf.end()) {
                        buf.erase(it.value());
                    }

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
                             i != buf.rend() -
                                      static_cast<typename std::iterator_traits<
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
                        return tmp.get_error();
                    }
                    if (tmp.value() == ctx.locale().widen('0')) {
                        val = false;
                        return {};
                    }
                    if (tmp.value() == ctx.locale().widen('1')) {
                        val = true;
                        return {};
                    }
                }

                return error::invalid_scanned_value;
            }

            bool localized{false};
            bool boolalpha{false};
        };

        template <typename CharT, typename T>
        struct str_to_int;

        template <typename CharT>
        struct str_to_int<CharT, long long> {
            static result<long long> get(const std::basic_string<CharT>& str,
                                         size_t& chars,
                                         int base)
            {
                return std::stoll(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, long> {
            static result<long> get(const std::basic_string<CharT>& str,
                                    size_t& chars,
                                    int base)
            {
                return std::stol(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, int> {
            static result<int> get(const std::basic_string<CharT>& str,
                                   size_t& chars,
                                   int base)
            {
                return std::stoi(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, short> {
            static result<short> get(const std::basic_string<CharT>& str,
                                     size_t& chars,
                                     int base)
            {
                auto i = std::stoi(str, &chars, base);
                if (i > static_cast<int>(std::numeric_limits<short>::max())) {
                    return make_error(error::value_out_of_range);
                }
                if (i < static_cast<int>(std::numeric_limits<short>::min())) {
                    return make_error(error::value_out_of_range);
                }
                return static_cast<short>(i);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned long long> {
            static result<unsigned long long>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                return std::stoull(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned long> {
            static result<unsigned long>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                return std::stoul(str, &chars, base);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned int> {
            static result<unsigned int> get(const std::basic_string<CharT>& str,
                                            size_t& chars,
                                            int base)
            {
                auto i = std::stoul(str, &chars, base);
                if (i > static_cast<unsigned long>(
                            std::numeric_limits<unsigned int>::max())) {
                    return make_error(error::value_out_of_range);
                }
                return static_cast<unsigned int>(i);
            }
        };
        template <typename CharT>
        struct str_to_int<CharT, unsigned short> {
            static result<unsigned short>
            get(const std::basic_string<CharT>& str, size_t& chars, int base)
            {
                auto i = std::stoul(str, &chars, base);
                if (i > static_cast<unsigned long>(
                            std::numeric_limits<unsigned short>::max())) {
                    return make_error(error::value_out_of_range);
                }
                return static_cast<unsigned short>(i);
            }
        };

        template <typename CharT, typename T>
        struct integer_scanner {
            template <typename Context>
            error parse(Context& ctx)
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
                    ch = *ctx.parse_context().advance();
                }
                else if (ch == ctx.locale().widen('x')) {
                    base = 16;
                    ch = *ctx.parse_context().advance();
                }
                else if (ch == ctx.locale().widen('o')) {
                    base = 8;
                    ch = *ctx.parse_context().advance();
                }
                else if (ch == ctx.locale().widen('b')) {
                    ctx.parse_context().advance();
                    ch = *ctx.parse_context().begin();

                    int tmp = 0;
                    if (ch < ctx.locale().widen('0') ||
                        ch > ctx.locale().widen('9')) {
                        return error::invalid_format_string;
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
                        return error::invalid_format_string;
                    }
                    tmp *= 10;
                    tmp += ch - ctx.locale().widen('0');
                    if (tmp < 1 || tmp > 36) {
                        return error::invalid_format_string;
                    }
                    base = tmp;
                    ch = *ctx.parse_context().advance();
                }
                else {
                    return error::invalid_format_string;
                }

                if (localized && (base != 0 && base != 10)) {
                    return error::invalid_format_string;
                }
                if (ch != ctx.locale().widen('}')) {
                    return error::invalid_format_string;
                }
                return {};
            }

            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                std::basic_string<CharT> buf{};
                buf.reserve(15);

                auto r = scan_chars(ctx, std::back_inserter(buf),
                                    predicates::until_space<Context>{}, true);
                if (!r) {
                    return r.get_error();
                }

                T tmp = 0;
                size_t chars = 0;

                auto putback = [&chars, &ctx, &buf]() -> error {
                    for (auto it = buf.rbegin();
                         it != buf.rend() -
                                   static_cast<typename std::iterator_traits<
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
                        return ret.get_error();
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
                    return error::value_out_of_range;
                }
                if (result.ec == std::errc::invalid_argument) {
                    return error::invalid_scanned_value;
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
                            return error::value_out_of_range;
                        }
                    }
                    auto ret =
                        detail::str_to_int<CharT, T>::get(buf, chars, base);
                    if (!ret) {
                        return ret.get_error();
                    }
                    tmp = ret.value();
#if SCN_MSVC
#pragma warning(pop)
#endif
                }
                catch (const std::invalid_argument&) {
                    return error::invalid_scanned_value;
                }
                catch (const std::out_of_range&) {
                    return error::value_out_of_range;
                }

                auto pb = putback();
                if (!pb) {
                    return pb;
                }
                val = tmp;
#endif
                return {};
            }

            enum localized_type : uint8_t {
                thousands_separator = 1,
                decimal = 2,
                digits = 4
            };

            int base{0};
            uint8_t localized{0};
        };

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

        template <typename CharT, typename T>
        struct float_scanner {
            template <typename Context>
            error parse(Context& ctx)
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
                return error::invalid_format_string;
            }
            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                std::basic_string<CharT> buf{};
                buf.reserve(15);

                bool point = false;
                auto r = scan_chars(
                    ctx, std::back_inserter(buf),
                    [&point](Context& c, CharT ch) -> result<scan_status> {
                        if (c.locale().is_space(ch)) {
                            return scan_status::end;
                        }
                        if (ch == c.locale().thousands_separator()) {
                            return scan_status::skip;
                        }
                        if (ch == c.locale().decimal_point()) {
                            if (point) {
                                return make_error(error::invalid_scanned_value);
                            }
                            point = true;
                        }
                        return scan_status::keep;
                    });
                if (!r) {
                    return r.get_error();
                }

                T tmp{};
                size_t chars = 0;

                auto putback = [&chars, &buf, &ctx]() -> error {
                    for (auto it = buf.rbegin();
                         it != buf.rend() -
                                   static_cast<typename std::iterator_traits<
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
                        return ret.get_error();
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
                    return error::value_out_of_range;
                }
                if (result.ec == std::errc::invalid_argument) {
                    return error::invalid_scanned_value;
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
                    return error::invalid_scanned_value;
                }
                catch (const std::out_of_range&) {
                    return error::value_out_of_range;
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

        template <typename CharT>
        struct string_scanner : public detail::empty_parser<CharT> {
        public:
            template <typename Context>
            error scan(std::basic_string<CharT>& val, Context& ctx)
            {
                {
                    auto err = skip_stream_whitespace(ctx);
                    if (!err) {
                        return err;
                    }
                }

                val.clear();
                auto s = scan_chars(ctx, std::back_inserter(val),
                                    predicates::until_space<Context>{});
                if (!s) {
                    return s.get_error();
                }
                if (val.empty()) {
                    return error::invalid_scanned_value;
                }

                return {};
            }
        };
    }  // namespace detail

    template <typename Context>
    class basic_visitor {
    public:
        using context_type = Context;
        using char_type = typename Context::char_type;

        basic_visitor(Context& ctx) : m_ctx(std::addressof(ctx)) {}

        template <typename T>
        auto operator()(T&& val) -> error
        {
            return visit(std::forward<T>(val), priority_tag<1>{});
        }

    private:
        auto visit(char_type& val, priority_tag<1>) -> error
        {
            detail::char_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(span<char_type>& val, priority_tag<1>) -> error
        {
            detail::buffer_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(bool& val, priority_tag<1>) -> error
        {
            detail::bool_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        template <typename T>
        auto visit(T& val, priority_tag<0>) ->
            typename std::enable_if<std::is_integral<T>::value &&
                                        sizeof(T) >= sizeof(char_type),
                                    error>::type
        {
            detail::integer_scanner<char_type, T> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        template <typename T>
        auto visit(T&, priority_tag<0>) ->
            typename std::enable_if<std::is_integral<T>::value &&
                                        sizeof(T) < sizeof(char_type),
                                    error>::type
        {
            return error::invalid_operation;
        }
        template <typename T>
        auto visit(T& val, priority_tag<1>) ->
            typename std::enable_if<std::is_floating_point<T>::value,
                                    error>::type
        {
            detail::float_scanner<char_type, T> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(std::basic_string<char_type>& val, priority_tag<1>) -> error
        {
            detail::string_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(typename basic_arg<Context>::handle val, priority_tag<1>)
            -> error
        {
            return val.scan(*m_ctx);
        }
        auto visit(detail::monostate, priority_tag<0>) -> error
        {
            return error::invalid_argument;
        }

        Context* m_ctx;
    };

    template <typename Context>
    error visit(Context& ctx)
    {
        auto& pctx = ctx.parse_context();
        auto arg_wrapped = ctx.next_arg();
        if (!arg_wrapped) {
            return arg_wrapped.get_error();
        }
        auto arg = arg_wrapped.value();

        {
            auto ret = skip_stream_whitespace(ctx);
            if (!ret) {
                return ret;
            }
        }

        for (auto it = pctx.begin(); it != pctx.end(); it = pctx.begin()) {
            if (ctx.locale().is_space(*it)) {
                // Skip whitespace from format string and from stream
                // EOF is not an error
                auto ret = parse_whitespace(ctx);
                if (!ret) {
                    if (ret == error::end_of_stream && !arg) {
                        return {};
                    }
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        return rb;
                    }
                    return ret;
                }
                // Don't advance pctx, parse_whitespace() does it for us
            }
            else if (*it != ctx.locale().widen('{')) {
                // Check for any non-specifier {foo} characters
                auto ret = ctx.stream().read_char();
                if (!ret || ret.value() != *it) {
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        // Failed rollback
                        return rb;
                    }
                    if (!ret) {
                        // Failed read
                        return ret.get_error();
                    }

                    // Mismatching characters in scan string and stream
                    return error::invalid_scanned_value;
                }
                // Bump pctx to next char
                pctx.advance();
            }
            else {
                // Scan argument
                if (!arg) {
                    // Mismatch between number of args and {}s
                    return error::invalid_format_string;
                }
                auto ret = visit_arg(basic_visitor<Context>(ctx), arg);
                if (!ret) {
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        return rb;
                    }
                    return ret;
                }
                // Handle next arg and bump pctx
                arg_wrapped = ctx.next_arg();
                if (!arg_wrapped) {
                    return arg_wrapped.get_error();
                }
                arg = arg_wrapped.value();
                pctx.advance();
            }
        }
        if (pctx.begin() != pctx.end()) {
            // Format string not exhausted
            return error::invalid_format_string;
        }
        auto srb = ctx.stream().set_roll_back();
        if (!srb) {
            return srb;
        }
        return {};
    }
}  // namespace scn

#endif  // SCN_DETAIL_VISITOR_H
