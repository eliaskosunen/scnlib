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

#include <string>  // for stoi, stol, stoll

namespace scn {
    SCN_BEGIN_NAMESPACE

    enum class scan_status { keep, skip, end };

    namespace predicates {
        template <typename Context>
        struct propagate {
            SCN_CONSTEXPR either<scan_status> operator()(
                Context&,
                typename Context::char_type) const noexcept
            {
                return scan_status::keep;
            }
        };
        template <typename Context>
        struct until {
            SCN_CONSTEXPR either<scan_status> operator()(
                Context&,
                typename Context::char_type ch) const noexcept
            {
                return ch == until_ch ? scan_status::end : scan_status::keep;
            }

            typename Context::char_type until_ch;
        };
        template <typename Context>
        struct until_one_of {
            either<scan_status> operator()(Context&,
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
            either<scan_status> operator()(Context& ctx,
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
            either<scan_status> operator()(Context&,
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
            either<scan_status> operator()(Context&,
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
            either<scan_status> operator()(Context& ctx,
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
    either<Iterator> scan_chars(Context& ctx,
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
    either<Iterator> scan_chars_until(Context& ctx,
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
    either<Iterator> propagate_chars_until(Context& ctx,
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
    either<Iterator> sized_propagate_chars_until(Context& ctx, Span s)
    {
        auto e = ctx.stream().read_sized(s);
        if (e != error::end_of_stream) {
            return s.end();
        }
        return propagate_chars_until(ctx, s.begin(), s.end());
    }

    template <typename Context,
              typename Char = typename Context::char_type,
              typename Span = span<Char>,
              typename Iterator = typename Span::iterator>
    auto propagate_chars_until(Context& ctx, Span s) -> typename std::enable_if<
        is_sized_stream<typename Context::stream_type>::value,
        either<Iterator>>::type
    {
        return sized_propagate_chars_until(ctx, s);
    }
    template <typename Context,
              typename Char = typename Context::char_type,
              typename Span = span<Char>,
              typename Iterator = typename Span::iterator>
    auto propagate_chars_until(Context& ctx, Span s) -> typename std::enable_if<
        !is_sized_stream<typename Context::stream_type>::value,
        either<Iterator>>::type
    {
        return propagate_chars_until(ctx, s.begin(), s.end());
    }

    template <typename CharT>
    struct empty_parser {
        template <typename Context>
        error parse(Context& ctx)
        {
            if (*ctx.parse_context().begin() != ctx.locale().widen('{')) {
                return error(error::invalid_format_string, "Expected '}'");
            }
            ctx.parse_context().advance();
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
                        return error(error::invalid_format_string,
                                     "Expected '}', 'l' or 'a'");
                    }
                }

                if (localized && !boolalpha) {
                    return error(
                        error::invalid_format_string,
                        "'l' and 'a' cannot be used simultaneously with bool");
                }
                if (ch != ctx.locale().widen('}')) {
                    return error(error::invalid_format_string, "Expected '}'");
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

                return error(error::invalid_scanned_value,
                             "Couldn't scan bool");
            }

            bool localized{false};
            bool boolalpha{false};
        };

        namespace sto {
            template <typename CharT, typename T>
            struct str_to_int;
        }  // namespace sto

        namespace strto {
            template <typename CharT, typename T>
            struct str_to_int;
        }  // namespace strto

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
                        return error(
                            error::invalid_format_string,
                            "Invalid character after 'b', expected digit");
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
                        return error(
                            error::invalid_format_string,
                            "Invalid character after 'b', expected digit");
                    }
                    tmp *= 10;
                    tmp += ch - ctx.locale().widen('0');
                    if (tmp < 1 || tmp > 36) {
                        return error(error::invalid_format_string,
                                     "Invalid base, must be between 1 and 36");
                    }
                    base = tmp;
                    ch = *ctx.parse_context().advance();
                }
                else {
                    return error(error::invalid_format_string, "Expected '}'");
                }

                if (localized && (base != 0 && base != 10)) {
                    return error(
                        error::invalid_format_string,
                        "Localized integers can only be scanned in base 10");
                }
                if (ch != ctx.locale().widen('}')) {
                    return error(error::invalid_format_string, "Expected '}'");
                }
                return {};
            }

            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                std::basic_string<CharT> buf{};
                buf.reserve(15 / sizeof(CharT));

                auto r = scan_chars(ctx, std::back_inserter(buf),
                                    predicates::until_space<Context>{}, true);
                if (!r) {
                    return r.get_error();
                }

                T tmp = 0;
                size_t chars = 0;

                auto putback = [&ctx, &buf](size_t n) -> error {
                    for (auto it = buf.rbegin();
                         it !=
                         buf.rend() - static_cast<typename std::iterator_traits<
                                          decltype(it)>::difference_type>(n);
                         ++it) {
                        auto ret = ctx.stream().putback(*it);
                        if (!ret) {
                            return ret;
                        }
                    }
                    return {};
                };

                if ((localized & digits) != 0) {
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    auto ret = ctx.locale().read_num(tmp, buf);
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

                    if (!ret) {
                        return ret.get_error();
                    }
                    auto pb = putback(ret.value());
                    if (!pb) {
                        return pb;
                    }
                    val = tmp;
                }

                SCN_MSVC_PUSH
                SCN_MSVC_IGNORE(4244)
                SCN_MSVC_IGNORE(4127)  // conditional expression is constant

                if (std::is_unsigned<T>::value) {
                    if (buf.front() == ctx.locale().widen('-')) {
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
                    }
                    SCN_UNREACHABLE;
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    SCN_GCC_POP
                };
                auto e = do_read()(tmp, buf, base);
                if (!e) {
                    return e.get_error();
                }
                chars = e.value();

                auto pb = putback(chars);
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
                                            const std::basic_string<CharT>& buf,
                                            int base);
            static either<size_t>
            _read_strto(T& val, const std::basic_string<CharT>& buf, int base);
            static either<size_t> _read_from_chars(
                T& val,
                const std::basic_string<CharT>& buf,
                int base);
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

                if (ch != ctx.locale().widen('}')) {
                    return error(error::invalid_format_string, "Expected '}'");
                }
                return {};
            }
            template <typename Context>
            error scan(T& val, Context& ctx)
            {
                std::basic_string<CharT> buf{};
                buf.reserve(15);

                bool point = false;
                auto r = scan_chars(
                    ctx, std::back_inserter(buf),
                    [&point](Context& c, CharT ch) -> either<scan_status> {
                        if (c.locale().is_space(ch)) {
                            return scan_status::end;
                        }
                        if (ch == c.locale().thousands_separator()) {
                            return scan_status::skip;
                        }
                        if (ch == c.locale().decimal_point()) {
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
                    return r.get_error();
                }

                T tmp{};
                size_t chars = 0;

                auto putback = [&buf, &ctx](size_t n) -> error {
                    for (auto it = buf.rbegin();
                         it !=
                         buf.rend() - static_cast<typename std::iterator_traits<
                                          decltype(it)>::difference_type>(n);
                         ++it) {
                        auto ret = ctx.stream().putback(*it);
                        if (!ret) {
                            return ret;
                        }
                    }
                    return {};
                };

                if (localized) {
                    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
                    auto ret = ctx.locale().read_num(tmp, buf);
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

                    if (!ret) {
                        return ret.get_error();
                    }
                    chars = ret.value();
                    auto pb = putback(chars);
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
                    }
                    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
                    SCN_UNREACHABLE;
                    SCN_GCC_POP
                };
                auto e = do_read()(tmp, buf);
                if (!e) {
                    return e.get_error();
                }
                chars = e.value();

                auto pb = putback(chars);
                if (!pb) {
                    return pb;
                }
                val = tmp;

                return {};
            }

            bool localized{false};

        private:
            static either<size_t> _read_sto(
                T& val,
                const std::basic_string<CharT>& buf);
            static either<size_t> _read_strto(
                T& val,
                const std::basic_string<CharT>& buf);
            static either<size_t> _read_from_chars(
                T& val,
                const std::basic_string<CharT>& buf);
        };

        template <typename CharT>
        struct string_scanner : public empty_parser<CharT> {
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
                    return error(error::invalid_scanned_value,
                                 "Empty string parsed");
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
            return visit(std::forward<T>(val), detail::priority_tag<1>{});
        }

    private:
        auto visit(char_type& val, detail::priority_tag<1>) -> error
        {
            detail::char_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(span<char_type>& val, detail::priority_tag<1>) -> error
        {
            detail::buffer_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(bool& val, detail::priority_tag<1>) -> error
        {
            detail::bool_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
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
            auto err = s.parse(*m_ctx);
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
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(std::basic_string<char_type>& val, detail::priority_tag<1>)
            -> error
        {
            detail::string_scanner<char_type> s;
            auto err = s.parse(*m_ctx);
            if (!err) {
                return err;
            }
            return s.scan(val, *m_ctx);
        }
        auto visit(typename basic_arg<Context>::handle val,
                   detail::priority_tag<1>) -> error
        {
            return val.scan(*m_ctx);
        }
        auto visit(detail::monostate, detail::priority_tag<0>) -> error
        {
            return error(error::invalid_operation, "Cannot scan a monostate");
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

        for (auto it = pctx.begin(); it != pctx.end(); it = pctx.begin()) {
            if (ctx.locale().is_space(*it)) {
                // Skip whitespace from format string and from stream
                // EOF is not an error
                auto ret = parse_whitespace(ctx);
                if (!ret) {
                    if (ret == error::end_of_stream && !arg) {
                        return {args_read};
                    }
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        return reterror(rb);
                    }
                    return reterror(ret);
                }
                // Don't advance pctx, parse_whitespace() does it for us
                continue;
            }

            // Non-brace character, or
            // Brace followed by another brace, meaning a literal '{'
            bool literal_brace = *it == ctx.locale().widen('{') &&
                                 it + 1 != pctx.end() &&
                                 *(it + 1) == ctx.locale().widen('{');
            if (*it != ctx.locale().widen('{') || literal_brace) {
                if (literal_brace || *it == ctx.locale().widen('}')) {
                    pctx.advance();
                    ++it;
                }

                // Check for any non-specifier {foo} characters
                auto ret = ctx.stream().read_char();
                if (!ret || ret.value() != *it) {
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
                auto ret = visit_arg(basic_visitor<Context>(ctx), arg);
                if (!ret) {
                    auto rb = ctx.stream().roll_back();
                    if (!rb) {
                        return reterror(rb);
                    }
                    return reterror(ret);
                }
                // Handle next arg and bump pctx
                ++args_read;
                arg_wrapped = ctx.next_arg();
                if (!arg_wrapped) {
                    return reterror(arg_wrapped.get_error());
                }
                arg = arg_wrapped.value();
                pctx.advance();
            }
        }
        if (pctx.begin() != pctx.end()) {
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
