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

#pragma once

#include <scn/detail/args.h>
#include <scn/detail/parse_context.h>

#include <limits>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        enum class align_type : unsigned char {
            none,
            left,   // '<'
            right,  // '>'
            center  // '^'
        };

        enum class presentation_type : unsigned char {
            none,
            int_binary,            // 'b'
            int_arbitrary_base,    // 'Bnn'
            int_decimal,           // 'd'
            int_generic,           // 'i'
            int_unsigned_decimal,  // 'u'
            int_octal,             // 'o'
            int_hex,               // 'x'
            float_hex,             // 'a', 'A'
            float_scientific,      // 'e', 'E'
            float_fixed,           // 'f', 'F'
            float_general,         // 'g', 'G'
            string,                // 's'
            string_set,            // '[...]'
            character,             // 'c'
            unicode_character,     // 'U'
            pointer,               // 'p'
        };

        template <typename CharT>
        struct basic_format_specs {
            int width{0};
            std::basic_string_view<CharT> fill{default_fill()};
            presentation_type type{presentation_type::none};
            std::basic_string_view<CharT> set_string{};
            unsigned arbitrary_base : 6;
            align_type align : 2;
            bool localized : 1;
            bool thsep : 1;
            unsigned char _reserved : 6;

            constexpr basic_format_specs()
                : arbitrary_base{0},
                  align{align_type::none},
                  localized{false},
                  thsep{false},
                  _reserved{0}
            {
            }

            SCN_NODISCARD constexpr int get_base(int default_base) const
            {
                SCN_GCC_COMPAT_PUSH
                SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")
                switch (type) {
                    case presentation_type::none:
                    case presentation_type::int_generic:
                        return default_base;
                    case presentation_type::int_arbitrary_base:
                        return arbitrary_base;

                    case presentation_type::int_binary:
                        return 2;
                    case presentation_type::int_octal:
                        return 8;
                    case presentation_type::int_decimal:
                    case presentation_type::int_unsigned_decimal:
                        return 10;
                    case presentation_type::int_hex:
                        return 16;

                    default:
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                }
                SCN_GCC_COMPAT_POP
            }

            static constexpr std::basic_string_view<CharT> default_fill()
            {
                if constexpr (std::is_same_v<CharT, char>) {
                    return " ";
                }
                else {
                    return L" ";
                }
            }
        };

        template <typename CharT>
        struct specs_setter {
        public:
            explicit constexpr specs_setter(basic_format_specs<CharT>& specs)
                : m_specs(specs)
            {
            }

            constexpr void on_align(align_type align)
            {
                m_specs.align = align;
            }
            constexpr void on_fill(std::basic_string_view<CharT> fill)
            {
                m_specs.fill = fill;
            }
            constexpr void on_localized()
            {
                m_specs.localized = true;
            }

            constexpr void on_width(int width)
            {
                m_specs.width = width;
            }

            constexpr void on_type(presentation_type type)
            {
                m_specs.type = type;
            }
            constexpr void on_character_set_type(
                std::basic_string_view<CharT> fmt)
            {
                m_specs.set_string = fmt;
                on_type(presentation_type::string_set);
            }

            constexpr void on_thsep()
            {
                m_specs.thsep = true;
            }

            // Intentionally not constexpr
            void on_error(const char* msg)
            {
                m_error = scan_error{scan_error::invalid_format_string, msg};
            }
            void on_error(scan_error err)
            {
                m_error = err;
            }

            constexpr explicit operator bool() const
            {
                return static_cast<bool>(m_error);
            }

            constexpr scan_error get_error() const
            {
                return m_error;
            }

        protected:
            basic_format_specs<CharT>& m_specs;
            scan_error m_error;
        };

        template <typename CharT, typename IDHandler>
        constexpr const CharT* parse_arg_id(const CharT* begin,
                                            const CharT* end,
                                            IDHandler&& handler)
        {
            SCN_EXPECT(begin != end);
            if (*begin != '}' && *begin != ':') {
                handler.on_error("Argument IDs in format strings unsupported");
                return end;
            }

            // TODO: do_parse_arg_id, fmt/core.h:2410
            handler();
            return begin;
        }

        template <typename CharT>
        constexpr presentation_type parse_presentation_type(CharT type)
        {
            switch (type) {
                case 'b':
                    return presentation_type::int_binary;
                case 'B':
                    return presentation_type::int_arbitrary_base;
                case 'd':
                    return presentation_type::int_decimal;
                case 'i':
                    return presentation_type::int_generic;
                case 'u':
                    return presentation_type::int_unsigned_decimal;
                case 'o':
                    return presentation_type::int_octal;
                case 'x':
                    return presentation_type::int_hex;
                case 'a':
                case 'A':
                    return presentation_type::float_hex;
                case 'e':
                case 'E':
                    return presentation_type::float_scientific;
                case 'f':
                case 'F':
                    return presentation_type::float_fixed;
                case 'g':
                case 'G':
                    return presentation_type::float_general;
                case 's':
                    return presentation_type::string;
                case 'c':
                    return presentation_type::character;
                case 'U':
                    return presentation_type::unicode_character;
                case 'p':
                    return presentation_type::pointer;
                case '[':
                    // Should be handled by parse_presentation_set
                    SCN_EXPECT(false);
                    SCN_UNREACHABLE;
                default:
                    return presentation_type::none;
            }
        }

        template <typename CharT>
        constexpr bool is_ascii_letter(CharT ch)
        {
            return (ch >= CharT{'a'} && ch <= CharT{'z'}) ||
                   (ch >= CharT{'A'} && ch <= CharT{'Z'});
        }

        template <typename CharT>
        constexpr int code_point_length(const CharT* begin, const CharT* end)
        {
            SCN_EXPECT(begin != end);
            if constexpr (sizeof(CharT) != 1) {
                return 1;
            }
            else {
                const auto lengths =
                    "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\2\2\2\2\3"
                    "\3\4";
                const int len =
                    lengths[static_cast<unsigned char>(*begin) >> 3];
                return len;
            }
        }

        template <typename CharT, typename Handler>
        constexpr const CharT* parse_align(const CharT* begin,
                                           const CharT* end,
                                           Handler&& handler)
        {
            SCN_EXPECT(begin != end);

            auto check_align = [](wchar_t ch) {
                switch (ch) {
                    case L'<':
                        return align_type::left;
                    case L'>':
                        return align_type::right;
                    case L'^':
                        return align_type::center;
                    default:
                        return align_type::none;
                }
            };

            auto potential_fill_len = code_point_length(begin, end);
            if (potential_fill_len == 0 ||
                std::distance(begin, end) < potential_fill_len) {
                handler.on_error("Invalid encoding in fill character");
                return begin;
            }

            auto potential_align_on_fill =
                check_align(static_cast<wchar_t>(*begin));

            auto potential_fill = std::basic_string_view<CharT>{
                begin, static_cast<size_t>(potential_fill_len)};
            const auto begin_before_fill = begin;
            begin += potential_fill_len;

            if (begin == end) {
                return begin_before_fill;
            }

            auto potential_align_after_fill =
                check_align(static_cast<wchar_t>(*begin));
            const auto begin_after_fill = begin;
            ++begin;

            if (potential_fill_len == 1) {
                if (potential_fill[0] == '{') {
                    handler.on_error(
                        "Invalid fill character '{' in format string");
                    return begin;
                }
                if (potential_fill[0] == '[') {
                    return begin_before_fill;
                }
            }

            if (potential_align_after_fill == align_type::none) {
                if (potential_align_on_fill != align_type::none) {
                    handler.on_align(potential_align_on_fill);
                    return begin_after_fill;
                }
                return begin_before_fill;
            }

            handler.on_fill(potential_fill);
            handler.on_align(potential_align_after_fill);
            return begin;
        }

        template <typename CharT>
        constexpr int parse_simple_int(const CharT*& begin, const CharT* end)
        {
            SCN_EXPECT(begin != end);
            SCN_EXPECT(*begin >= '0' && *begin <= '9');

            unsigned value = 0, prev = 0;
            auto p = begin;
            do {
                prev = value;
                value = value * 10 + static_cast<unsigned>(*p - '0');
                ++p;
            } while (p != end && *p >= '0' && *p <= '9');
            auto num_digits = p - begin;
            begin = p;
            if (num_digits <= std::numeric_limits<int>::digits10) {
                return static_cast<int>(value);
            }
            const auto max =
                static_cast<unsigned>((std::numeric_limits<int>::max)());
            return num_digits == std::numeric_limits<int>::digits10 + 1 &&
                           prev * 10ull + unsigned(p[-1] - '0') <= max
                       ? static_cast<int>(value)
                       : -1;
        }

        template <typename CharT, typename Handler>
        constexpr const CharT* parse_width(const CharT* begin,
                                           const CharT* end,
                                           Handler&& handler)
        {
            SCN_EXPECT(begin != end);

            if (*begin >= CharT{'0'} && *begin <= CharT{'9'}) {
                int width = parse_simple_int(begin, end);
                if (width != -1) {
                    handler.on_width(width);
                }
                else {
                    handler.on_error("Invalid field width");
                    return begin;
                }
            }
            else if (*begin == CharT{'{'}) {
                // TODO: dynamic width
            }
            return begin;
        }

        template <typename CharT, typename SpecHandler>
        constexpr std::basic_string_view<CharT> parse_presentation_set(
            const CharT*& begin,
            const CharT* end,
            SpecHandler&& handler)
        {
            // Actually parsed later, in character_set_reader
            SCN_EXPECT(*begin == CharT{'['});

            auto start = begin;

            if (++begin == end) {
                handler.on_error(
                    "Unexpected end of [character set] specifier in format "
                    "string");
                return {};
            }
            if (*begin == CharT{'^'}) {
                ++begin;
                if (*begin == CharT{']'}) {
                    ++begin;
                }
            }
            else if (*begin == CharT{']'}) {
                return make_string_view_from_iterators<CharT>(start, ++begin);
            }

            for (; begin != end; ++begin) {
                if (*begin == CharT{']'}) {
                    return make_string_view_from_iterators<CharT>(start,
                                                                  ++begin);
                }
            }

            handler.on_error(
                "Invalid [character set] specifier in format string");
            return {};
        }

        template <typename CharT, typename SpecHandler>
        constexpr const CharT* parse_format_specs(const CharT* begin,
                                                  const CharT* end,
                                                  SpecHandler&& handler)
        {
            auto do_presentation = [&]() -> const CharT* {
                if (*begin == CharT{'['}) {
                    auto set = parse_presentation_set(begin, end, handler);
                    if (set.empty()) {
                        handler.on_error(
                            "Invalid (empty) [character set] specifier in "
                            "format string");
                        return begin;
                    }
                    handler.on_character_set_type(set);
                    return begin;
                }
                presentation_type type = parse_presentation_type(*begin++);
                if (type == presentation_type::none) {
                    handler.on_error("Invalid type specifier in format string");
                    return begin;
                }
                handler.on_type(type);
                return begin;
            };

            if (end - begin > 1 && *(begin + 1) == CharT{'}'} &&
                is_ascii_letter(*begin) && *begin != CharT{'L'} &&
                *begin != CharT{'n'}) {
                return do_presentation();
            }

            if (begin == end) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            begin = parse_align(begin, end, handler);
            if (begin == end) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            begin = parse_width(begin, end, handler);
            if (begin == end) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            if (*begin == CharT{'L'}) {
                handler.on_localized();
                ++begin;
            }
            if (begin == end) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            if (*begin == CharT{'\''}) {
                handler.on_thsep();
                ++begin;
            }
            if (begin == end) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            if (begin != end && *begin != CharT{'}'}) {
                do_presentation();
            }
            if (begin == end) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            return begin;
        }

        template <typename CharT, typename Handler>
        constexpr const CharT* parse_replacement_field(const CharT* begin,
                                                       const CharT* end,
                                                       Handler& handler)
        {
            struct id_adapter {
                constexpr void operator()()
                {
                    arg_id = handler.on_arg_id();
                }
                constexpr void operator()(std::size_t id)
                {
                    arg_id = handler.on_arg_id(id);
                }

                constexpr void on_error(const char* msg)
                {
                    handler.on_error(msg);
                }

                Handler& handler;
                std::size_t arg_id;
            };

            ++begin;
            if (begin == end) {
                handler.on_error("Unexpected end of replacement field");
                return begin;
            }

            if (*begin == CharT{'}'}) {
                handler.on_replacement_field(handler.on_arg_id(), begin);
            }
            else if (*begin == CharT{'{'}) {
                handler.on_literal_text(begin, begin + 1);
            }
            else {
                auto adapter = id_adapter{handler, 0};
                begin = parse_arg_id(begin, end, adapter);

                if (begin == end) {
                    handler.on_error("Missing '}' in format string");
                    return begin;
                }

                if (*begin == CharT{'}'}) {
                    handler.on_replacement_field(adapter.arg_id, begin);
                }
                else if (*begin == CharT{':'}) {
                    begin =
                        handler.on_format_specs(adapter.arg_id, begin + 1, end);
                    if (begin == end || *begin != '}') {
                        handler.on_error("Unknown format specifier");
                        return begin;
                    }
                }
                else {
                    handler.on_error("Missing '}' in format string");
                    return begin;
                }
            }
            return begin + 1;
        }

        template <bool IsConstexpr, typename CharT, typename Handler>
        constexpr void parse_format_string_impl(
            std::basic_string_view<CharT> format,
            Handler&& handler)
        {
            if (format.size() < 32) {
                // Small size -> use a simple loop instead of memchr
                auto begin = format.begin();
                auto it = begin;

                while (it != format.end()) {
                    const auto ch = *it++;
                    if (ch == CharT{'{'}) {
                        handler.on_literal_text(begin, it - 1);

                        begin = it = parse_replacement_field(
                            it - 1, format.end(), handler);
                        if (!handler) {
                            return;
                        }
                    }
                    else if (ch == CharT{'}'}) {
                        if (it == format.end() || *it != CharT{'}'}) {
                            handler.on_error("Unmatched '}' in format string");
                            return;
                        }

                        handler.on_literal_text(begin, it);
                        begin = ++it;
                    }
                }

                handler.on_literal_text(begin, format.end());
                return;
            }

            const auto reader = [&handler](const CharT* begin,
                                           const CharT* end) {
                if (begin == end) {
                    return;
                }

                while (true) {
                    auto p = find<IsConstexpr>(begin, end, CharT{'}'});
                    if (p == end) {
                        handler.on_literal_text(begin, end);
                        return;
                    }

                    ++p;
                    if (p == end || *p != CharT{'}'}) {
                        handler.on_error("Unmatched '}' in format string");
                        return;
                    }

                    handler.on_literal_text(begin, p);
                    if (!handler) {
                        return;
                    }
                    begin = p + 1;
                }
            };

            auto begin = format.begin();
            while (begin != format.end()) {
                if (*begin != CharT{'{'}) {
                    reader(begin, format.end());
                    return;
                }

                auto p = find<IsConstexpr>(begin + 1, format.end(), CharT{'{'});
                if (p == format.end()) {
                    reader(begin, format.end());
                    return;
                }

                reader(begin, p);
                if (!handler) {
                    return;
                }

                begin = parse_replacement_field(p, format.end(), handler);
                if (!handler) {
                    return;
                }
            }
        }

        template <bool IsConstexpr, typename CharT, typename Handler>
        constexpr scan_error parse_format_string(
            std::basic_string_view<CharT> format,
            Handler&& handler)
        {
            parse_format_string_impl<IsConstexpr>(format, handler);
            handler.check_args_exhausted();
            return handler.get_error();
        }

        enum class arg_type_category {
            none,
            integer,
            unsigned_integer,
            floating,
            string,
            pointer,
            boolean,
            character,
            custom
        };

        constexpr arg_type_category get_category_for_arg_type(arg_type type)
        {
            switch (type) {
                case arg_type::none_type:
                    return arg_type_category::none;

                case arg_type::schar_type:
                case arg_type::short_type:
                case arg_type::int_type:
                case arg_type::long_type:
                case arg_type::llong_type:
                    return arg_type_category::integer;

                case arg_type::uchar_type:
                case arg_type::ushort_type:
                case arg_type::uint_type:
                case arg_type::ulong_type:
                case arg_type::ullong_type:
                    return arg_type_category::unsigned_integer;

                case arg_type::pointer_type:
                    return arg_type_category::pointer;
                case arg_type::bool_type:
                    return arg_type_category::boolean;
                case arg_type::narrow_character_type:
                case arg_type::wide_character_type:
                case arg_type::code_point_type:
                    return arg_type_category::character;

                case arg_type::float_type:
                case arg_type::double_type:
                case arg_type::ldouble_type:
                    return arg_type_category::floating;

                case arg_type::narrow_string_view_type:
                case arg_type::narrow_string_type:
                case arg_type::wide_string_view_type:
                case arg_type::wide_string_type:
                    return arg_type_category::string;

                case arg_type::custom_type:
                    return arg_type_category::custom;

                    SCN_CLANG_PUSH
                    SCN_CLANG_IGNORE("-Wcovered-switch-default")
                default:
                    SCN_ENSURE(false);
                    SCN_UNREACHABLE;
                    SCN_CLANG_POP
            }

            SCN_ENSURE(false);
            SCN_UNREACHABLE;
        }

        template <typename Handler>
        class specs_checker : public Handler {
        public:
            template <typename H>
            constexpr specs_checker(H&& handler, arg_type type)
                : Handler(SCN_FWD(handler)), m_arg_type(type)
            {
                SCN_EXPECT(m_arg_type != arg_type::custom_type);
            }

            constexpr void on_localized()
            {
                const auto cat = get_category_for_arg_type(m_arg_type);
                if (cat != arg_type_category::integer &&
                    cat != arg_type_category::unsigned_integer &&
                    cat != arg_type_category::floating &&
                    cat != arg_type_category::boolean) {
                    return this->on_error(
                        "'L' specifier can only be used with arguments of "
                        "integer, floating-point or boolean types");
                }

                Handler::on_localized();
            }
            constexpr void on_thsep()
            {
                const auto cat = get_category_for_arg_type(m_arg_type);
                if (cat != arg_type_category::integer &&
                    cat != arg_type_category::unsigned_integer &&
                    cat != arg_type_category::floating) {
                    return this->on_error(
                        "' specifier (for a thousands separator) can only "
                        "be "
                        "used with arguments of integer or floating-point "
                        "types");
                }

                Handler::on_thsep();
            }

        private:
            arg_type m_arg_type;
        };

        template <typename CharT, typename Handler>
        constexpr void check_disallow_thsep(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            if (specs.thsep) {
                return handler.on_error(
                    "' specifier not allowed for this type");
            }
        }

        template <typename CharT, typename Handler>
        constexpr void check_int_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            if (specs.type > presentation_type::int_hex) {
                return handler.on_error(
                    "Invalid type specifier for integer type");
            }
            if (specs.localized) {
                if (specs.type == presentation_type::int_binary) {
                    return handler.on_error(
                        "'b' specifier not supported for localized integers");
                }
                if (specs.type == presentation_type::int_arbitrary_base) {
                    return handler.on_error(
                        "Arbitrary bases not supported for localized integers");
                }
            }
        }

        template <typename CharT, typename Handler>
        constexpr void check_char_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            check_disallow_thsep(specs, handler);
            if (specs.type != presentation_type::none &&
                specs.type != presentation_type::character &&
                specs.type != presentation_type::unicode_character) {
                return handler.on_error(
                    "Invalid type specifier for character type");
            }
        }

        template <typename CharT, typename Handler>
        constexpr void check_float_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            if (specs.type != presentation_type::none &&
                (specs.type < presentation_type::float_hex ||
                 specs.type > presentation_type::float_general)) {
                return handler.on_error(
                    "Invalid type specifier for float type");
            }
        }

        template <typename CharT, typename Handler>
        constexpr void check_string_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            check_disallow_thsep(specs, handler);
            if (specs.type == presentation_type::none ||
                specs.type == presentation_type::string ||
                specs.type == presentation_type::string_set) {
                return;
            }
            if (specs.type == presentation_type::character ||
                specs.type == presentation_type::unicode_character) {
                if (specs.width == 0) {
                    return handler.on_error(
                        "'c' and 'U' type specifiers for strings requires a "
                        "width to be specified");
                }
                return;
            }
            handler.on_error("Invalid type specifier for string");
        }

        template <typename CharT, typename Handler>
        constexpr void check_pointer_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            check_disallow_thsep(specs, handler);
            if (specs.type != presentation_type::none &&
                specs.type != presentation_type::pointer) {
                return handler.on_error("Invalid type specifier for pointer");
            }
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
