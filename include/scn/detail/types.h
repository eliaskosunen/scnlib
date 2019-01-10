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

#ifndef SCN_DETAIL_TYPES_H
#define SCN_DETAIL_TYPES_H

#include "context.h"
#include "core.h"
#include "util.h"

#include <algorithm>
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
            result<scan_status> operator()(Context&,
                                           typename Context::char_type)
            {
                return scan_status::keep;
            }
        };
        template <typename Context>
        struct until {
            result<scan_status> operator()(Context&,
                                           typename Context::char_type ch)
            {
                if (ch == until_ch) {
                    return scan_status::end;
                }
                return scan_status::keep;
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
    }  // namespace detail

    template <typename CharT>
    struct basic_value_scanner<CharT, CharT>
        : public detail::empty_parser<CharT> {
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
    struct basic_value_scanner<CharT, span<CharT>>
        : public detail::empty_parser<CharT> {
        template <typename Context>
        error scan(span<CharT> val, Context& ctx)
        {
            if (val.size() == 0) {
                return {};
            }

            std::vector<CharT> buf(static_cast<size_t>(val.size()));
            auto s = scan_chars_until(ctx, buf.begin(), buf.end(),
                                      predicates::propagate<Context>{});
            if (!s) {
                return s.get_error();
            }
            std::copy(buf.begin(), s.value(), val.begin());

            return {};
        }
    };

    template <typename CharT>
    struct basic_value_scanner<CharT, bool> {
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
                basic_string_view<CharT> truename{"true"};
                basic_string_view<CharT> falsename{"false"};
                if (localized) {
                    truename = ctx.locale().truename();
                    falsename = ctx.locale().falsename();
                }
                const auto max_len =
                    std::max(truename.size(), falsename.size());
                std::basic_string<CharT> buf(max_len, 0);

                auto it = scan_chars_until(ctx, buf.begin(), buf.end(),
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

    namespace detail {
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
    }  // namespace detail

    template <typename CharT, typename T>
    struct basic_value_scanner<
        CharT,
        T,
        typename std::enable_if<std::is_integral<T>::value &&
                                !std::is_same<T, CharT>::value &&
                                !std::is_same<T, bool>::value>::type> {
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
            buf.reserve(static_cast<size_t>(
                detail::max_digits<T>(base == 0 ? 8 : base)));

            auto r = scan_chars(ctx, std::back_inserter(buf),
                                predicates::until_space<Context>{}, true);
            if (!r) {
                return r.get_error();
            }

            T tmp = 0;
            size_t chars = 0;

            auto putback = [&chars, &ctx, &buf]() -> error {
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
                auto ret = detail::str_to_int<CharT, T>::get(buf, chars, base);
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
            buf.reserve(21);

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
    struct basic_value_scanner<CharT, std::basic_string<CharT>>
        : public detail::empty_parser<CharT> {
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

    template <typename Stream,
              typename Traits,
              typename Allocator,
              typename CharT = typename Stream::char_type>
    error getline(Stream& s, std::basic_string<CharT, Traits, Allocator>& str)
    {
        using context_type = basic_context<Stream>;
        auto f = string_view("{}");
        auto ctx = context_type(s, f);

        str.clear();
        auto res = scan_chars(
            ctx, std::back_inserter(str),
            predicates::until<context_type>{ctx.locale().widen('\n')});
        if (!res) {
            return res.get_error();
        }
        return {};
    }
    template <typename Stream,
              typename Traits,
              typename Allocator,
              typename CharT = typename Stream::char_type>
    error getline(Stream& s,
                  std::basic_string<CharT, Traits, Allocator>& str,
                  CharT until)
    {
        using context_type = basic_context<Stream>;
        auto f = string_view("{}");
        auto ctx = context_type(s, f);

        str.clear();
        auto res = scan_chars(ctx, std::back_inserter(str),
                              predicates::until<context_type>{until});
        if (!res) {
            return res.get_error();
        }
        return {};
    }

    namespace detail {
        template <typename CharT>
        struct ignore_iterator {
            using value_type = CharT;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::input_iterator_tag;

            ignore_iterator() = default;
            explicit ignore_iterator(difference_type n) : i(n) {}

            reference operator*() const
            {
                static CharT c(0);
                return c;
            }
            pointer operator->() const
            {
                return &operator*();
            }

            ignore_iterator& operator++()
            {
                ++i;
                return *this;
            }
            ignore_iterator operator++(int)
            {
                auto tmp(*this);
                operator++();
                return tmp;
            }

            void swap(ignore_iterator& other)
            {
                using std::swap;
                swap(i, other.i);
            }

            bool operator==(ignore_iterator& other)
            {
                return i == other.i;
            }
            bool operator!=(ignore_iterator& other)
            {
                return !(*this == other);
            }

            static ignore_iterator max()
            {
                return {std::numeric_limits<difference_type>::max()};
            }

            difference_type i{0};
        };

        template <typename CharT>
        void swap(ignore_iterator<CharT>& a, ignore_iterator<CharT>& b)
        {
            a.swap(b);
        }
    }  // namespace detail

    template <typename Stream, typename CharT = typename Stream::char_type>
    error ignore_all(Stream& s)
    {
        using context_type = basic_context<Stream>;
        auto f = string_view("{}");
        auto ctx = context_type(s, f);

        auto res = scan_chars(ctx, detail::ignore_iterator<CharT>{},
                              predicates::propagate<context_type>{});
        if (!res) {
            return res.get_error();
        }
        return {};
    }
    template <typename Stream, typename CharT = typename Stream::char_type>
    error ignore_until(Stream& s, CharT until)
    {
        using context_type = basic_context<Stream>;
        auto f = string_view("{}");
        auto ctx = context_type(s, f);

        auto res = scan_chars(ctx, detail::ignore_iterator<CharT>{},
                              predicates::until<context_type>{until});
        if (!res) {
            return res.get_error();
        }
        return {};
    }
    template <typename Stream, typename CharT = typename Stream::char_type>
    error ignore_n(Stream& s, std::ptrdiff_t count)
    {
        using context_type = basic_context<Stream>;
        auto f = string_view("{}");
        auto ctx = context_type(s, f);

        auto res = scan_chars_until(ctx, detail::ignore_iterator<CharT>{},
                                    detail::ignore_iterator<CharT>{count},
                                    predicates::propagate<context_type>{});
        if (!res) {
            return res.get_error();
        }
        return {};
    }
    template <typename Stream, typename CharT = typename Stream::char_type>
    error ignore_n_until(Stream& s, std::ptrdiff_t count, CharT until)
    {
        using context_type = basic_context<Stream>;
        auto f = string_view("{}");
        auto ctx = context_type(s, f);

        auto res = scan_chars_until(ctx, detail::ignore_iterator<CharT>{},
                                    detail::ignore_iterator<CharT>{count},
                                    predicates::until<context_type>{until});
        if (!res) {
            return res.get_error();
        }
        return {};
    }
}  // namespace scn

#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
// -Wundefined-func-template
#pragma clang diagnostic pop
#endif

#endif  // SCN_DETAIL_TYPES_H
