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
enum class align_type : unsigned char {
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
    regex,                 // '/.../.'
    regex_escaped,         // '/..\/../.'
    character,             // 'c'
    escaped_character,     // '?'
    pointer,               // 'p'
};

enum class regex_flags {
    none = 0,
    multiline = 1,   // /m
    singleline = 2,  // /s
    nocase = 4,      // /i
    nocapture = 8,   // /n
    // TODO?
    // would probably need to go hand-in-hand with locale,
    // where it could even be the default/only option -> no flag?
    // why else would you even use locale with a regex?
    // collate = 16,
};

constexpr regex_flags operator&(regex_flags a, regex_flags b)
{
    return static_cast<regex_flags>(static_cast<unsigned>(a) &
                                    static_cast<unsigned>(b));
}
constexpr regex_flags operator|(regex_flags a, regex_flags b)
{
    return static_cast<regex_flags>(static_cast<unsigned>(a) |
                                    static_cast<unsigned>(b));
}
constexpr regex_flags operator^(regex_flags a, regex_flags b)
{
    return static_cast<regex_flags>(static_cast<unsigned>(a) ^
                                    static_cast<unsigned>(b));
}

constexpr regex_flags& operator&=(regex_flags& a, regex_flags b)
{
    return a = a & b;
}
constexpr regex_flags& operator|=(regex_flags& a, regex_flags b)
{
    return a = a | b;
}
constexpr regex_flags& operator^=(regex_flags& a, regex_flags b)
{
    return a = a ^ b;
}

class fill_type {
public:
    constexpr void operator=(char c)
    {
        m_data[0] = c;
        m_size = 1;
    }

    template <typename CharT>
    constexpr void operator=(std::basic_string_view<CharT> s)
    {
        SCN_EXPECT(!s.empty());
        SCN_EXPECT(s.size() * sizeof(CharT) <= max_size);
        if constexpr (sizeof(CharT) == 1) {
            for (size_t i = 0; i < s.size(); ++i) {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wstringop-overflow")  // false positive
                m_data[i] = s[i];
                SCN_GCC_POP
            }
            m_size = static_cast<unsigned char>(s.size());
        }
        else if constexpr (sizeof(CharT) == 2) {
            m_data[0] = static_cast<char>(static_cast<unsigned>(s.front()));
            m_data[1] =
                static_cast<char>(static_cast<unsigned>(s.front()) >> 8);
            if (s.size() == 1) {
                return;
            }
            m_data[2] = static_cast<char>(static_cast<unsigned>(s[1]));
            m_data[3] = static_cast<char>(static_cast<unsigned>(s[1]) >> 8);
        }
        else {
            const auto front = static_cast<unsigned>(s.front());
            m_data[0] = static_cast<char>(front);
            m_data[1] = static_cast<char>(front >> 8);
            m_data[2] = static_cast<char>(front >> 16);
            m_data[3] = static_cast<char>(front >> 24);
        }
    }

    constexpr size_t size() const
    {
        return m_size;
    }

    template <typename CharT>
    CharT get() const
    {
        SCN_EXPECT(m_size <= sizeof(CharT));
        CharT r{};
        std::memcpy(&r, m_data, m_size);
        return r;
    }

    template <typename CharT>
    constexpr const CharT* data() const
    {
        if constexpr (std::is_same_v<CharT, char>) {
            return m_data;
        }
        else {
            return nullptr;
        }
    }

    template <typename CharT>
    std::basic_string_view<CharT> view() const
    {
        return {reinterpret_cast<const CharT*>(m_data), m_size};
    }

private:
    static constexpr size_t max_size = 4;
    char m_data[max_size] = {' '};
    unsigned char m_size{1};
};

struct format_specs {
    int width{0};
    fill_type fill{};
    presentation_type type{presentation_type::none};
    std::array<uint8_t, 128 / 8> charset_literals{0};
    bool charset_has_nonascii{false}, charset_is_inverted{false};
    const void* charset_string_data{nullptr};
    size_t charset_string_size{0};
    regex_flags regexp_flags{regex_flags::none};
    unsigned char arbitrary_base{0};
    align_type align{align_type::none};
    bool localized{false};

    constexpr format_specs() = default;

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

    template <typename CharT>
    std::basic_string_view<CharT> charset_string() const
    {
        return {reinterpret_cast<const CharT*>(charset_string_data),
                charset_string_size};
    }
};

struct specs_setter {
public:
    explicit constexpr specs_setter(format_specs& specs) : m_specs(specs) {}

    constexpr void on_align(align_type align)
    {
        m_specs.align = align;
    }
    template <typename CharT>
    constexpr void on_fill(std::basic_string_view<CharT> fill)
    {
        m_specs.fill = fill;
    }
    constexpr void on_localized()
    {
        if constexpr (!SCN_DISABLE_LOCALE) {
            m_specs.localized = true;
        }
        else {
            on_error("'L' flag invalid when SCN_DISABLE_LOCALE is on");
        }
    }

    constexpr void on_width(int width)
    {
        m_specs.width = width;
    }

    constexpr void on_type(presentation_type type)
    {
        m_specs.type = type;
    }

    constexpr void on_charset_single(char32_t cp)
    {
        const auto cp_value = static_cast<unsigned>(cp);
        if (SCN_LIKELY(cp_value <= 127)) {
            m_specs.charset_literals[cp_value / 8] |=
                static_cast<unsigned char>(1ul << (cp_value % 8));
        }
        else {
            m_specs.charset_has_nonascii = true;
        }
    }

    constexpr void on_charset_range(char32_t begin, char32_t end)
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
            m_specs.charset_has_nonascii = true;
        }
    }

    constexpr void on_charset_inverted()
    {
        m_specs.charset_is_inverted = true;
    }

    template <typename CharT>
    constexpr void on_character_set_string(std::basic_string_view<CharT> fmt)
    {
        m_specs.charset_string_data = fmt.data();
        m_specs.charset_string_size = fmt.size();
        on_type(presentation_type::string_set);
    }

    template <typename CharT>
    constexpr void on_regex_pattern(std::basic_string_view<CharT> pattern)
    {
        m_specs.charset_string_data = pattern.data();
        m_specs.charset_string_size = pattern.size();
    }
    constexpr void on_regex_flags(regex_flags flags)
    {
        m_specs.regexp_flags = flags;
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
    format_specs& m_specs;
    scan_error m_error;
};

template <typename CharT>
constexpr int parse_simple_int(const CharT*& begin, const CharT* end)
{
    SCN_EXPECT(begin != end);
    SCN_EXPECT(*begin >= '0' && *begin <= '9');

    unsigned long long value = 0;
    do {
        value *= 10;
        value += static_cast<unsigned long long>(*begin - '0');
        if (value >
            static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
            return -1;
        }
        ++begin;
    } while (begin != end && *begin >= '0' && *begin <= '9');
    return static_cast<int>(value);
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

    if (begin == end || (*begin != CharT{'}'} && *begin != CharT{':'})) {
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
        case '/':
            // Should be handled by parse_presentation_set and
            // parse_presentation_regex
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
        const int len = lengths[static_cast<unsigned char>(*begin) >> 3];
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
        handler.on_error("Invalid encoding in fill character");
        return begin;
    }

    auto potential_align_on_fill = check_align(static_cast<wchar_t>(*begin));

    auto potential_fill = std::basic_string_view<CharT>{
        begin, static_cast<size_t>(potential_fill_len)};
    const auto begin_before_fill = begin;
    begin += potential_fill_len;

    if (begin == end) {
        return begin_before_fill;
    }

    auto potential_align_after_fill = check_align(static_cast<wchar_t>(*begin));
    const auto begin_after_fill = begin;
    ++begin;

    if (potential_fill_len == 1) {
        if (SCN_UNLIKELY(potential_fill[0] == '{')) {
            handler.on_error("Invalid fill character '{' in format string");
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

template <typename CharT, typename SpecHandler>
constexpr char32_t parse_presentation_set_code_point(const CharT*& begin,
                                                     const CharT* end,
                                                     SpecHandler&& handler)
{
    SCN_EXPECT(begin != end);

    auto len = utf_code_point_length_by_starting_code_unit(*begin);
    if (SCN_UNLIKELY(len == 0 || static_cast<size_t>(end - begin) < len)) {
        handler.on_error("Invalid encoding in format string");
        return invalid_code_point;
    }

    const auto cp = decode_utf_code_point_exhaustive(
        std::basic_string_view<CharT>{begin, len});
    if (SCN_UNLIKELY(cp >= invalid_code_point)) {
        handler.on_error("Invalid encoding in format string");
        return invalid_code_point;
    }

    begin += len;
    return cp;
}

template <typename CharT, typename SpecHandler>
constexpr void parse_presentation_set_literal(const CharT*& begin,
                                              const CharT* end,
                                              SpecHandler&& handler)
{
    SCN_EXPECT(begin != end);

    auto cp_first = parse_presentation_set_code_point(begin, end, handler);
    if (SCN_UNLIKELY(cp_first >= invalid_code_point)) {
        return;
    }

    if (begin != end && *begin == CharT{'-'} && (begin + 1) != end &&
        *(begin + 1) != CharT{']'}) {
        ++begin;

        auto cp_second = parse_presentation_set_code_point(begin, end, handler);
        if (SCN_UNLIKELY(cp_second >= invalid_code_point)) {
            return;
        }

        if (SCN_UNLIKELY(cp_second < cp_first)) {
            // clang-format off
            handler.on_error("Invalid range in [character set] format string argument: Range end before the beginning");
            // clang-format on
            return;
        }

        handler.on_charset_range(cp_first, cp_second + 1);
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
        // clang-format off
        handler.on_error("Unexpected end of [character set] specifier in format string");
        // clang-format on
        return {};
    }
    if (*begin == CharT{'^'}) {
        handler.on_charset_inverted();
        ++begin;
        if (*begin == CharT{']'}) {
            handler.on_charset_single(char32_t{']'});
            ++begin;
        }
    }
    else if (*begin == CharT{']'}) {
        return {start, static_cast<size_t>(std::distance(start, ++begin))};
    }

    while (begin != end) {
        if (SCN_UNLIKELY(!handler)) {
            break;
        }

        if (*begin == CharT{']'}) {
            return {start, static_cast<size_t>(std::distance(start, ++begin))};
        }

        parse_presentation_set_literal(begin, end, handler);
    }

    SCN_UNLIKELY_ATTR
    handler.on_error("Invalid [character set] specifier in format string");
    return {};
}

template <typename CharT, typename SpecHandler>
constexpr const CharT* parse_presentation_regex(const CharT*& begin,
                                                const CharT* end,
                                                SpecHandler&& handler)
{
#if !SCN_DISABLE_REGEX
    SCN_EXPECT(begin != end);
    SCN_EXPECT(*begin == CharT{'/'});

    if constexpr (!SCN_REGEX_SUPPORTS_WIDE_STRINGS &&
                  std::is_same_v<CharT, wchar_t>) {
        handler.on_error("Regex backend doesn't support wide strings as input");
        return begin;
    }

    auto start = begin;
    ++begin;

    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    handler.on_type(presentation_type::regex);
    for (; begin != end; ++begin) {
        if (*begin == CharT{'/'}) {
            if (*(begin - 1) != CharT{'\\'}) {
                break;
            }
            else {
                handler.on_type(presentation_type::regex_escaped);
            }
        }
    }
    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    auto regex_end = begin;
    auto regex_pattern = make_string_view_from_pointers(start + 1, regex_end);
    if (SCN_UNLIKELY(regex_pattern.empty())) {
        handler.on_error("Invalid (empty) regex in format string");
        return begin;
    }
    handler.on_regex_pattern(regex_pattern);
    ++begin;

    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    regex_flags flags{regex_flags::none};
    constexpr std::array<std::pair<char, regex_flags>, 4> flag_map{
        {{'m', regex_flags::multiline},
         {'s', regex_flags::singleline},
         {'i', regex_flags::nocase},
         {'n', regex_flags::nocapture}}};
    for (; begin != end; ++begin) {
        if (*begin == CharT{'}'}) {
            break;
        }
        bool found_flag = false;
        for (auto flag : flag_map) {
            if (static_cast<CharT>(flag.first) != *begin) {
                continue;
            }
            if ((flags & flag.second) != regex_flags::none) {
                handler.on_error("Flag set multiple times in regex");
                return begin;
            }
#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
            if (*begin == CharT{'s'}) {
                // clang-format off
                handler.on_error("/s flag for regex isn't supported by regex backend");
                // clang-format on
            }
#if !SCN_HAS_STD_REGEX_MULTILINE
            if (*begin == CharT{'m'}) {
                // clang-format off
                handler.on_error("/m flag for regex isn't supported by regex backend");
                // clang-format on
            }
#endif
#endif
            flags |= flag.second;
            found_flag = true;
            break;
        }
        if (!found_flag) {
            handler.on_error("Invalid flag in regex");
            return begin;
        }
    }
    handler.on_regex_flags(flags);

    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    return begin;
#else
    handler.on_error("Regular expression support is disabled");
    return {};
#endif
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
                // clang-format off
                handler.on_error("Invalid (empty) [character set] specifier in format string");
                // clang-format on
                return begin;
            }
            handler.on_character_set_string(set);
            return begin;
        }
        if (*begin == CharT{'/'}) {
            return parse_presentation_regex(begin, end, handler);
        }
        presentation_type type = parse_presentation_type(*begin++);
        if (SCN_UNLIKELY(type == presentation_type::none)) {
            handler.on_error("Invalid type specifier in format string");
            return begin;
        }
        handler.on_type(type);
        return begin;
    };

    if (end - begin > 1 && *(begin + 1) == CharT{'}'} &&
        is_ascii_letter(*begin) && *begin != CharT{'L'}) {
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
            if (SCN_UNLIKELY(begin + 1 == end)) {
                handler.on_error("Unexpected end of replacement field");
                return begin;
            }
            begin = handler.on_format_specs(adapter.arg_id, begin + 1, end);
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
constexpr void parse_format_string_impl(std::basic_string_view<CharT> format,
                                        Handler&& handler)
{
    // TODO: memchr fast path with a larger (> 32) format string

    auto begin = format.data();
    auto it = begin;
    const auto end = format.data() + format.size();

    while (it != end) {
        const auto ch = *it++;
        if (ch == CharT{'{'}) {
            handler.on_literal_text(begin, it - 1);

            begin = it = parse_replacement_field(it - 1, end, handler);
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
}

template <bool IsConstexpr, typename CharT, typename Handler>
constexpr scan_error parse_format_string(std::basic_string_view<CharT> format,
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
            // clang-format off
            return this->on_error("'L' specifier can only be used with arguments of integer, floating-point, or boolean types");
            // clang-format on
        }

        Handler::on_localized();
    }

private:
    arg_type m_arg_type;
};

template <typename Handler>
constexpr void check_int_type_specs(const format_specs& specs,
                                    Handler&& handler)
{
    if (SCN_UNLIKELY(specs.type > presentation_type::int_hex)) {
        return handler.on_error("Invalid type specifier for integer type");
    }
    if (specs.localized) {
        if (SCN_UNLIKELY(specs.type == presentation_type::int_binary)) {
            // clang-format off
            handler.on_error("'b'/'B' specifier not supported for localized integers");
            // clang-format on
            return;
        }
        if (SCN_UNLIKELY(specs.type == presentation_type::int_arbitrary_base)) {
            // clang-format off
            return handler.on_error("Arbitrary bases not supported for localized integers");
            // clang-format on
        }
    }
}

template <typename Handler>
constexpr void check_char_type_specs(const format_specs& specs,
                                     Handler&& handler)
{
    if (specs.type > presentation_type::int_hex ||
        specs.type == presentation_type::int_arbitrary_base) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for character type");
    }
}

template <typename Handler>
constexpr void check_code_point_type_specs(const format_specs& specs,
                                           Handler&& handler)
{
    if (specs.type != presentation_type::none &&
        specs.type != presentation_type::character) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for character type");
    }
}

template <typename Handler>
constexpr void check_float_type_specs(const format_specs& specs,
                                      Handler&& handler)
{
    if (specs.type != presentation_type::none &&
        (specs.type < presentation_type::float_hex ||
         specs.type > presentation_type::float_general)) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for float type");
    }
}

template <typename Handler>
constexpr void check_string_type_specs(const format_specs& specs,
                                       Handler&& handler)
{
    if (specs.type == presentation_type::none ||
        specs.type == presentation_type::string ||
        specs.type == presentation_type::string_set ||
        specs.type == presentation_type::regex ||
        specs.type == presentation_type::regex_escaped) {
        return;
    }
    if (specs.type == presentation_type::character) {
        if (SCN_UNLIKELY(specs.width == 0)) {
            // clang-format off
            return handler.on_error("'c' type specifier for strings requires the field width to be specified");
            // clang-format on
        }
        return;
    }
    SCN_UNLIKELY_ATTR
    handler.on_error("Invalid type specifier for string");
}

template <typename Handler>
constexpr void check_pointer_type_specs(const format_specs& specs,
                                        Handler&& handler)
{
    if (specs.type != presentation_type::none &&
        specs.type != presentation_type::pointer) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for pointer");
    }
}

template <typename Handler>
constexpr void check_bool_type_specs(const format_specs& specs,
                                     Handler&& handler)
{
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

template <typename Handler>
constexpr void check_regex_type_specs(const format_specs& specs,
                                      Handler&& handler)
{
    if (SCN_UNLIKELY(specs.type == presentation_type::none ||
                     specs.charset_string_size == 0)) {
        // clang-format off
        return handler.on_error("Regular expression needs to specified when reading regex_matches");
        // clang-format on
    }
    if (specs.type == presentation_type::regex ||
        specs.type == presentation_type::regex_escaped) {
        return;
    }
    SCN_UNLIKELY_ATTR
    handler.on_error("Invalid type specifier for regex_matches");
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
