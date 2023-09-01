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
#include <scn/detail/unicode.h>

#include <limits>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        enum class align_type {
            none = 0,
            left = 1,   // '<'
            right = 2,  // '>'
            center = 3  // '^'
        };

        enum class presentation_type {
            none,
            int_binary,            // 'b', 'B'
            int_decimal,           // 'd'
            int_generic,           // 'i'
            int_unsigned_decimal,  // 'u'
            int_octal,             // 'o'
            int_hex,               // 'x', 'X'
            int_arbitrary_base,    // 'rnn', 'Rnn' (R for radix)
            float_hex,             // 'a', 'A'
            float_scientific,      // 'e', 'E'
            float_fixed,           // 'f', 'F'
            float_general,         // 'g', 'G'
            string,                // 's'
            string_set,            // '[...]'
            character,             // 'c'
            escaped_character,     // '?'
            pointer,               // 'p'
        };

        enum class character_set_specifier {
            none = 0,

            space_literal = 1,       // ' '
            underscore_literal = 2,  // '_'

            space = 4,     // :space:
            blank = 8,     // :blank:
            punct = 16,    // :punct:
            upper = 32,    // :upper:
            lower = 64,    // :lower:
            alpha = 128,   // :alpha:
            digit = 256,   // :digit:
            xdigit = 512,  // :xdigit:
            cntrl = 1024,  // :cntrl:
            last = 2048,

            has_inverted_flag = 4096,  // '^'
            has_nonascii_literals = 8192,

            alnum = alpha | digit,          // :alnum:
            graph = alnum | punct,          // :graph:
            print = graph | space_literal,  // :print:

            letters = alpha,                                // \l
            inverted_letters = ~letters,                    // \L
            alnum_underscore = alnum | underscore_literal,  // \w
            inverted_alnum_underscore = ~alnum_underscore,  // \W
            whitespace = space,                             // \s
            inverted_whitespace = ~whitespace,              // \S
            numbers = digit,                                // \d
            inverted_numbers = ~numbers                     // \D
        };

        constexpr character_set_specifier operator|(character_set_specifier a,
                                                    character_set_specifier b)
        {
            return static_cast<character_set_specifier>(
                static_cast<unsigned>(a) | static_cast<unsigned>(b));
        }
        constexpr character_set_specifier operator&(character_set_specifier a,
                                                    character_set_specifier b)
        {
            return static_cast<character_set_specifier>(
                static_cast<unsigned>(a) & static_cast<unsigned>(b));
        }
        constexpr character_set_specifier& operator|=(
            character_set_specifier& a,
            character_set_specifier b)
        {
            return a = a | b;
        }

        template <typename CharT>
        struct basic_format_specs {
            int width{0};
            std::basic_string_view<CharT> fill{default_fill()};
            presentation_type type{presentation_type::none};
            character_set_specifier charset_specifiers{
                character_set_specifier::none};
            std::array<uint8_t, 128 / 8> charset_literals{0};
            std::basic_string_view<CharT> charset_string{};
            unsigned arbitrary_base : 6;
            unsigned align : 2;
            bool localized : 1;
            bool thsep : 1;

            constexpr basic_format_specs()
                : arbitrary_base{0},
                  align{static_cast<unsigned>(align_type::none)},
                  localized{false},
                  thsep{false}
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
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wconversion")
                m_specs.align = static_cast<unsigned>(align);
                SCN_GCC_POP
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

            constexpr void on_charset_specifier(character_set_specifier spec)
            {
                m_specs.charset_specifiers |= spec;
            }

            constexpr void on_charset_single(code_point cp)
            {
                const auto cp_value = static_cast<unsigned>(cp);
                if (SCN_LIKELY(cp_value <= 127)) {
                    m_specs.charset_literals[cp_value / 8] |=
                        static_cast<unsigned char>(1ul << (cp_value % 8));
                }
                else {
                    m_specs.charset_specifiers |=
                        character_set_specifier::has_nonascii_literals;
                }
            }

            constexpr void on_charset_range(code_point begin, code_point end)
            {
                const auto begin_value = static_cast<unsigned>(begin);
                const auto end_value = static_cast<unsigned>(end);
                SCN_EXPECT(begin_value < end_value);

                if (SCN_LIKELY(end_value <= 127)) {
                    // No need to bit-twiddle with a mask, because with the
                    // SCN_ASSUME, -O3 will optimize this to a single operation
                    SCN_ASSUME(begin_value < end_value);
                    for (auto v = begin_value; v != end_value; ++v) {
                        m_specs.charset_literals[v / 8] |=
                            static_cast<unsigned char>(1ul << (v % 8));
                    }
                }
                else {
                    m_specs.charset_specifiers |=
                        character_set_specifier::has_nonascii_literals;
                }
            }

            constexpr void on_character_set_string(
                std::basic_string_view<CharT> fmt)
            {
                m_specs.charset_string = fmt;
                on_type(presentation_type::string_set);
            }

            constexpr void on_thsep()
            {
                m_specs.thsep = true;
            }

            // Intentionally not constexpr
            void on_error(const char* msg)
            {
                SCN_UNLIKELY_ATTR
                m_error = scan_error{scan_error::invalid_format_string, msg};
            }
            void on_error(scan_error err)
            {
                SCN_LIKELY(err);
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
            if (SCN_LIKELY(num_digits <= std::numeric_limits<int>::digits10)) {
                SCN_LIKELY_ATTR
                return static_cast<int>(value);
            }
            const auto max =
                static_cast<unsigned>((std::numeric_limits<int>::max)());
            return num_digits == std::numeric_limits<int>::digits10 + 1 &&
                           prev * 10ull + unsigned(p[-1] - '0') <= max
                       ? static_cast<int>(value)
                       : -1;
        }

        template <typename CharT, typename IDHandler>
        constexpr const CharT* do_parse_arg_id(const CharT* begin,
                                               const CharT* end,
                                               IDHandler&& handler)
        {
            SCN_EXPECT(begin != end);

            CharT c = *begin;
            if (c < CharT{'0'} || c > CharT{'9'}) {
                handler.on_error("Invalid argument ID");
                return begin;
            }

            int idx = 0;
            if (c != CharT{'0'}) {
                idx = parse_simple_int(begin, end);
            }
            else {
                ++begin;
            }

            if (begin == end ||
                (*begin != CharT{'}'} && *begin != CharT{':'})) {
                handler.on_error("Invalid argument ID");
                return begin;
            }
            handler(idx);

            return begin;
        }

        template <typename CharT, typename IDHandler>
        constexpr const CharT* parse_arg_id(const CharT* begin,
                                            const CharT* end,
                                            IDHandler&& handler)
        {
            SCN_EXPECT(begin != end);
            if (*begin != '}' && *begin != ':') {
                return do_parse_arg_id(begin, end, SCN_FWD(handler));
            }

            handler();
            return begin;
        }

        template <typename CharT>
        constexpr presentation_type parse_presentation_type(CharT type)
        {
            switch (type) {
                case 'b':
                case 'B':
                    return presentation_type::int_binary;
                case 'd':
                    return presentation_type::int_decimal;
                case 'i':
                    return presentation_type::int_generic;
                case 'u':
                    return presentation_type::int_unsigned_decimal;
                case 'o':
                    return presentation_type::int_octal;
                case 'x':
                case 'X':
                    return presentation_type::int_hex;
                case 'r':
                case 'R':
                    return presentation_type::int_arbitrary_base;
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
                case '?':
                    return presentation_type::escaped_character;
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
            if (SCN_UNLIKELY(potential_fill_len == 0 ||
                             std::distance(begin, end) < potential_fill_len)) {
                SCN_UNLIKELY_ATTR
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
                if (SCN_UNLIKELY(potential_fill[0] == '{')) {
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

        template <typename CharT, typename Handler>
        constexpr const CharT* parse_width(const CharT* begin,
                                           const CharT* end,
                                           Handler&& handler)
        {
            SCN_EXPECT(begin != end);

            if (*begin >= CharT{'0'} && *begin <= CharT{'9'}) {
                int width = parse_simple_int(begin, end);
                if (SCN_LIKELY(width != -1)) {
                    handler.on_width(width);
                }
                else {
                    handler.on_error("Invalid field width");
                    return begin;
                }
            }
            return begin;
        }

        inline constexpr std::
            tuple<const char*, const wchar_t*, character_set_specifier>
                set_colon_specifiers[] = {
                    {"alnum:", L"alnum:", character_set_specifier::alnum},
                    {"alpha:", L"alpha:", character_set_specifier::alpha},
                    {"blank:", L"blank:", character_set_specifier::blank},
                    {"cntrl:", L"cntrl:", character_set_specifier::cntrl},
                    {"digit:", L"digit:", character_set_specifier::digit},
                    {"graph:", L"graph:", character_set_specifier::graph},
                    {"lower:", L"lower:", character_set_specifier::lower},
                    {"print:", L"print:", character_set_specifier::print},
                    {"punct:", L"punct:", character_set_specifier::punct},
                    {"space:", L"space:", character_set_specifier::space},
                    {"upper:", L"upper:", character_set_specifier::upper},
                    {"xdigit:", L"xdigit:", character_set_specifier::xdigit}};

        template <typename CharT, typename SpecHandler>
        constexpr void parse_presentation_set_colon_specifier(
            const CharT*& begin,
            const CharT* end,
            SpecHandler&& handler)
        {
            SCN_EXPECT(*begin == CharT{':'});
            SCN_EXPECT(begin != end);
            ++begin;

            const auto chars_left = static_cast<size_t>(end - begin);

            character_set_specifier found{};
            for (const auto& elem : set_colon_specifiers) {
                const auto [str_ch, str_wch, flag] = elem;

                const auto spec_len = std::char_traits<char>::length(str_ch);
                if (chars_left < spec_len) {
                    continue;
                }

                if constexpr (std::is_same_v<CharT, char>) {
                    if (std::string_view{&*begin, spec_len} !=
                        std::string_view{str_ch}) {
                        continue;
                    }
                }
                else {
                    if (std::wstring_view{&*begin, spec_len} !=
                        std::wstring_view{str_wch}) {
                        continue;
                    }
                }

                begin += spec_len;
                SCN_ENSURE(*(begin - 1) == CharT{':'});
                found = flag;
                break;
            }

            if (SCN_UNLIKELY(found == character_set_specifier{})) {
                handler.on_error(
                    "Invalid :colon-delimeted: specifier in a [character set] "
                    "format string argument");
            }
            else {
                handler.on_charset_specifier(found);
            }
        }

        inline constexpr std::pair<char, character_set_specifier>
            set_backslash_specifiers[] = {
                {'l', character_set_specifier::letters},
                {'L', character_set_specifier::inverted_letters},
                {'w', character_set_specifier::alnum_underscore},
                {'W', character_set_specifier::inverted_alnum_underscore},
                {'s', character_set_specifier::whitespace},
                {'S', character_set_specifier::inverted_whitespace},
                {'d', character_set_specifier::numbers},
                {'D', character_set_specifier::inverted_numbers},
        };

        template <typename CharT, typename SpecHandler>
        constexpr void parse_presentation_set_backslash_specifier(
            const CharT*& begin,
            const CharT* end,
            SpecHandler&& handler)
        {
            SCN_EXPECT(begin != end);
            SCN_EXPECT(*begin == CharT{'\\'});
            ++begin;

            for (const auto& elem : set_backslash_specifiers) {
                if (*begin == static_cast<CharT>(elem.first)) {
                    handler.on_charset_specifier(elem.second);
                    ++begin;
                    return;
                }
            }

            if (*begin == '\\') {
                handler.on_charset_single(code_point{'\\'});
                ++begin;
                return;
            }
            if (*begin == ':') {
                handler.on_charset_single(code_point{':'});
                ++begin;
                return;
            }

            SCN_UNLIKELY_ATTR
            handler.on_error(
                "Invalid \\backslash-prefixed specifier in a [character set] "
                "format string argument");
        }

        template <typename CharT, typename SpecHandler>
        constexpr code_point parse_presentation_set_code_point(
            const CharT*& begin,
            const CharT* end,
            SpecHandler&& handler)
        {
            SCN_EXPECT(begin != end);

            auto len = utf_code_point_length_by_starting_code_unit(*begin);
            if (SCN_UNLIKELY(len == 0 ||
                             static_cast<size_t>(end - begin) < len)) {
                handler.on_error(
                    "Invalid Unicode code point in format string argument");
                return invalid_code_point;
            }

            auto cp_begin = begin;
            begin += len;

            if constexpr (sizeof(CharT) == 1) {
                // UTF-8
                auto cp =
                    decode_utf8_code_point(std::string_view{&*cp_begin, len});
                if (SCN_UNLIKELY(cp == invalid_code_point)) {
                    handler.on_error(
                        "Invalid Unicode code point in format string argument");
                    return invalid_code_point;
                }
                return cp;
            }
            else if constexpr (sizeof(CharT) == 2) {
                // UTF-16
                auto cp =
                    decode_utf16_code_point(std::wstring_view{&*cp_begin, len});
                if (SCN_UNLIKELY(cp == invalid_code_point)) {
                    handler.on_error(
                        "Invalid Unicode code point in format string argument");
                    return invalid_code_point;
                }
                return cp;
            }
            else {
                SCN_EXPECT(len == 1);
                return static_cast<code_point>(*cp_begin);
            }
        }

        template <typename CharT, typename SpecHandler>
        constexpr void parse_presentation_set_literal(const CharT*& begin,
                                                      const CharT* end,
                                                      SpecHandler&& handler)
        {
            SCN_EXPECT(begin != end);
            SCN_EXPECT(*begin != CharT{':'} && *begin != CharT{'\\'});

            auto cp_first =
                parse_presentation_set_code_point(begin, end, handler);
            if (SCN_UNLIKELY(cp_first == invalid_code_point)) {
                return;
            }

            if (begin != end && *begin == CharT{'-'} && (begin + 1) != end &&
                *(begin + 1) != CharT{']'}) {
                ++begin;

                auto cp_second =
                    parse_presentation_set_code_point(begin, end, handler);
                if (SCN_UNLIKELY(cp_second == invalid_code_point)) {
                    return;
                }

                if (SCN_UNLIKELY(static_cast<unsigned>(cp_second) <
                                 static_cast<unsigned>(cp_first))) {
                    handler.on_error(
                        "Invalid range in [character set] format string "
                        "argument: Range end before the beginning");
                    return;
                }

                handler.on_charset_range(
                    cp_first, static_cast<code_point>(
                                  static_cast<uint32_t>(cp_second) + 1));
                return;
            }

            handler.on_charset_single(cp_first);
        }

        template <typename CharT, typename SpecHandler>
        constexpr std::basic_string_view<CharT> parse_presentation_set(
            const CharT*& begin,
            const CharT* end,
            SpecHandler&& handler)
        {
            SCN_EXPECT(begin != end);
            SCN_EXPECT(*begin == CharT{'['});

            auto start = begin;
            ++begin;

            if (SCN_UNLIKELY(begin == end)) {
                SCN_UNLIKELY_ATTR
                handler.on_error(
                    "Unexpected end of [character set] specifier in format "
                    "string");
                return {};
            }
            if (*begin == CharT{'^'}) {
                handler.on_charset_specifier(
                    character_set_specifier::has_inverted_flag);
                ++begin;
                if (*begin == CharT{']'}) {
                    handler.on_charset_single(code_point{']'});
                    ++begin;
                }
            }
            else if (*begin == CharT{']'}) {
                return {start,
                        static_cast<size_t>(std::distance(start, ++begin))};
            }

            while (begin != end) {
                if (SCN_UNLIKELY(!handler)) {
                    break;
                }

                if (*begin == CharT{']'}) {
                    return {start,
                            static_cast<size_t>(std::distance(start, ++begin))};
                }

                if (*begin == CharT{':'}) {
                    parse_presentation_set_colon_specifier(begin, end, handler);
                }
                else if (*begin == CharT{'\\'}) {
                    parse_presentation_set_backslash_specifier(begin, end,
                                                               handler);
                }
                else {
                    parse_presentation_set_literal(begin, end, handler);
                }
            }

            SCN_UNLIKELY_ATTR
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
                    if (SCN_UNLIKELY(set.size() <= 2)) {
                        SCN_UNLIKELY_ATTR
                        handler.on_error(
                            "Invalid (empty) [character set] specifier in "
                            "format string");
                        return begin;
                    }
                    handler.on_character_set_string(set);
                    return begin;
                }
                presentation_type type = parse_presentation_type(*begin++);
                if (SCN_UNLIKELY(type == presentation_type::none)) {
                    SCN_UNLIKELY_ATTR
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

            if (SCN_UNLIKELY(begin == end)) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            begin = parse_align(begin, end, handler);
            if (SCN_UNLIKELY(begin == end)) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            begin = parse_width(begin, end, handler);
            if (SCN_UNLIKELY(begin == end)) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            if (*begin == CharT{'L'}) {
                handler.on_localized();
                ++begin;
            }
            if (SCN_UNLIKELY(begin == end)) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            if (*begin == CharT{'\''}) {
                handler.on_thsep();
                ++begin;
            }
            if (SCN_UNLIKELY(begin == end)) {
                handler.on_error("Unexpected end of format string");
                return begin;
            }

            if (begin != end && *begin != CharT{'}'}) {
                do_presentation();
            }
            if (SCN_UNLIKELY(begin == end)) {
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
                    SCN_UNLIKELY_ATTR
                    handler.on_error(msg);
                }

                Handler& handler;
                std::size_t arg_id;
            };

            ++begin;
            if (SCN_UNLIKELY(begin == end)) {
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

                if (SCN_UNLIKELY(begin == end)) {
                    handler.on_error("Missing '}' in format string");
                    return begin;
                }

                if (*begin == CharT{'}'}) {
                    handler.on_replacement_field(adapter.arg_id, begin);
                }
                else if (*begin == CharT{':'}) {
                    begin =
                        handler.on_format_specs(adapter.arg_id, begin + 1, end);
                    if (SCN_UNLIKELY(begin == end || *begin != '}')) {
                        handler.on_error("Unknown format specifier");
                        return begin;
                    }
                }
                else {
                    SCN_UNLIKELY_ATTR
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
            if (SCN_LIKELY(format.size() < 32)) {
                // Small size -> use a simple loop instead of memchr
                auto begin = format.data();
                auto it = begin;
                const auto end = format.data() + format.size();

                while (it != end) {
                    const auto ch = *it++;
                    if (ch == CharT{'{'}) {
                        handler.on_literal_text(begin, it - 1);

                        begin = it =
                            parse_replacement_field(it - 1, end, handler);
                        if (!handler) {
                            return;
                        }
                    }
                    else if (ch == CharT{'}'}) {
                        if (SCN_UNLIKELY(it == end || *it != CharT{'}'})) {
                            handler.on_error("Unmatched '}' in format string");
                            return;
                        }

                        handler.on_literal_text(begin, it);
                        begin = ++it;
                    }
                }

                handler.on_literal_text(begin, end);
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
                    if (SCN_UNLIKELY(p == end || *p != CharT{'}'})) {
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

            auto begin = format.data();
            const auto end = format.data() + format.size();
            while (begin != end) {
                if (*begin != CharT{'{'}) {
                    reader(begin, end);
                    return;
                }

                auto p = find<IsConstexpr>(begin + 1, end, CharT{'{'});
                if (p == end) {
                    reader(begin, end);
                    return;
                }

                reader(begin, p);
                if (SCN_UNLIKELY(!handler)) {
                    return;
                }

                begin = parse_replacement_field(p, end, handler);
                if (SCN_UNLIKELY(!handler)) {
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
                    SCN_UNLIKELY_ATTR
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
                    SCN_UNLIKELY_ATTR
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
            if (SCN_UNLIKELY(specs.thsep)) {
                return handler.on_error(
                    "' specifier not allowed for this type");
            }
        }

        template <typename CharT, typename Handler>
        constexpr void check_int_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            if (SCN_UNLIKELY(specs.type > presentation_type::int_hex)) {
                return handler.on_error(
                    "Invalid type specifier for integer type");
            }
            if (specs.localized) {
                if (SCN_UNLIKELY(specs.type == presentation_type::int_binary)) {
                    return handler.on_error(
                        "'b'/'B' specifier not supported for localized "
                        "integers");
                }
                if (SCN_UNLIKELY(specs.type ==
                                 presentation_type::int_arbitrary_base)) {
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
            if (specs.type > presentation_type::int_hex ||
                specs.type == presentation_type::int_arbitrary_base) {
                SCN_UNLIKELY_ATTR
                return handler.on_error(
                    "Invalid type specifier for character type");
            }
        }

        template <typename CharT, typename Handler>
        constexpr void check_code_point_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            check_disallow_thsep(specs, handler);
            if (specs.type != presentation_type::none &&
                specs.type != presentation_type::character) {
                SCN_UNLIKELY_ATTR
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
                SCN_UNLIKELY_ATTR
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
            if (specs.type == presentation_type::character) {
                if (SCN_UNLIKELY(specs.width == 0)) {
                    return handler.on_error(
                        "'c' type specifier for strings requires the "
                        "field width to be specified");
                }
                return;
            }
            SCN_UNLIKELY_ATTR
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
                SCN_UNLIKELY_ATTR
                return handler.on_error("Invalid type specifier for pointer");
            }
        }

        template <typename CharT, typename Handler>
        constexpr void check_bool_type_specs(
            const basic_format_specs<CharT>& specs,
            Handler&& handler)
        {
            check_disallow_thsep(specs, handler);
            if (specs.type != presentation_type::none &&
                specs.type != presentation_type::string &&
                specs.type != presentation_type::int_generic &&
                specs.type != presentation_type::int_hex &&
                specs.type != presentation_type::int_binary &&
                specs.type != presentation_type::int_unsigned_decimal &&
                specs.type != presentation_type::int_octal &&
                specs.type != presentation_type::int_decimal) {
                SCN_UNLIKELY_ATTR
                return handler.on_error("Invalid type specifier for boolean");
            }
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
