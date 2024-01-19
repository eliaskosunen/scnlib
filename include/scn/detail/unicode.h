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

#include <scn/fwd.h>

#include <cstdint>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
constexpr inline bool is_ascii_code_point(char32_t cp)
{
    return cp <= 0x7f;
}

template <typename U8>
constexpr std::size_t utf8_code_point_length_by_starting_code_unit(U8 ch)
{
    static_assert(sizeof(U8) == 1);

    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wsign-conversion")
    constexpr char lengths[] =
        "\1\1\1\1\1\1\1\1"  // highest bit is 0 -> single-byte
        "\1\1\1\1\1\1\1\1"
        "\0\0\0\0\0\0\0\0"  // highest bits 10 -> error, non-initial
                            // byte
        "\2\2\2\2"          // highest bits 110 -> 2-byte cp
        "\3\3"              // highest bits 1110 -> 3-byte cp
        "\4";               // highest bits 11110 -> 4-byte cp
    return lengths[static_cast<unsigned char>(ch) >> 3];
    SCN_GCC_COMPAT_POP
}

template <typename U16>
constexpr std::size_t utf16_code_point_length_by_starting_code_unit(U16 ch)
{
    static_assert(sizeof(U16) == 2);

    const auto lead = static_cast<uint16_t>(0xffff & ch);
    if (lead >= 0xd800 && lead <= 0xdbff) {
        // high surrogate
        return 2;
    }
    if (lead >= 0xdc00 && lead <= 0xdfff) {
        // unpaired low surrogate
        return 0;
    }
    return 1;
}

template <typename U>
constexpr std::size_t utf_code_point_length_by_starting_code_unit(U ch)
{
    if constexpr (sizeof(U) == 1) {
        return utf8_code_point_length_by_starting_code_unit(ch);
    }
    else if constexpr (sizeof(U) == 2) {
        return utf16_code_point_length_by_starting_code_unit(ch);
    }
    else {
        // utf-32
        static_assert(sizeof(U) == 4);
        SCN_UNUSED(ch);
        return 1;
    }
}

inline constexpr char32_t invalid_code_point = 0x110000;

inline constexpr char32_t decode_utf8_code_point_exhaustive(
    std::string_view input)
{
    SCN_EXPECT(!input.empty() && input.size() <= 4);

    const auto is_trailing_code_unit = [](char ch) {
        return static_cast<unsigned char>(ch) >> 6 == 0x2;
    };

    if (input.size() == 1) {
        if (static_cast<unsigned char>(input[0]) >= 0x80) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        return static_cast<char32_t>(input[0]);
    }

    if (input.size() == 2) {
        if ((static_cast<unsigned char>(input[0]) & 0xe0) != 0xc0) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (!is_trailing_code_unit(input[1])) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x1f) << 6;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 3) {
        if ((static_cast<unsigned char>(input[0]) & 0xf0) != 0xe0) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (!is_trailing_code_unit(input[1]) ||
            !is_trailing_code_unit(input[2])) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x0f) << 12;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 4) {
        if ((static_cast<unsigned char>(input[0]) & 0xf8) != 0xf0) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (static_cast<unsigned char>(input[0]) > 0xf4) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (!is_trailing_code_unit(input[1]) ||
            !is_trailing_code_unit(input[2]) ||
            !is_trailing_code_unit(input[3])) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x07) << 18;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 12;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[3]) & 0x3f) << 0;
        return cp;
    }

    SCN_EXPECT(false);
    SCN_UNREACHABLE;
}

inline constexpr char32_t decode_utf8_code_point_exhaustive_valid(
    std::string_view input)
{
    SCN_EXPECT(!input.empty() && input.size() <= 4);

    const auto is_trailing_code_unit = [](char ch) {
        return static_cast<unsigned char>(ch) >> 6 == 0x2;
    };

    if (input.size() == 1) {
        SCN_EXPECT(static_cast<unsigned char>(input[0]) < 0x80);
        return static_cast<char32_t>(input[0]);
    }

    if (input.size() == 2) {
        SCN_EXPECT((static_cast<unsigned char>(input[0]) & 0xe0) == 0xc0);
        SCN_EXPECT(is_trailing_code_unit(input[1]));

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x1f) << 6;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 3) {
        SCN_EXPECT((static_cast<unsigned char>(input[0]) & 0xf0) == 0xe0);
        SCN_EXPECT(is_trailing_code_unit(input[1]));
        SCN_EXPECT(is_trailing_code_unit(input[2]));

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x0f) << 12;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 4) {
        SCN_EXPECT((static_cast<unsigned char>(input[0]) & 0xf8) == 0xf0);
        SCN_EXPECT(static_cast<unsigned char>(input[0]) <= 0xf4);
        SCN_EXPECT(is_trailing_code_unit(input[1]));
        SCN_EXPECT(is_trailing_code_unit(input[2]));
        SCN_EXPECT(is_trailing_code_unit(input[3]));

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x07) << 18;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 12;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[3]) & 0x3f) << 0;
        return cp;
    }

    SCN_EXPECT(false);
    SCN_UNREACHABLE;
}

template <typename CharT>
inline constexpr char32_t decode_utf16_code_point_exhaustive(
    std::basic_string_view<CharT> input)
{
    if constexpr (sizeof(CharT) == 2) {
        SCN_EXPECT(!input.empty() && input.size() <= 2);

        if (input.size() == 1) {
            return static_cast<char32_t>(input[0]);
        }

        const auto lead = static_cast<uint32_t>(input[0]) - 0xd800;
        const auto trail = static_cast<uint32_t>(input[1]) - 0xdc00;
        const auto cp = (lead << 10) | trail;
        return static_cast<char32_t>(cp + 0x10000);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

template <typename CharT>
inline constexpr char32_t decode_utf_code_point_exhaustive(
    std::basic_string_view<CharT> input)
{
    if constexpr (sizeof(CharT) == 1) {
        return decode_utf8_code_point_exhaustive(input);
    }
    else if constexpr (sizeof(CharT) == 2) {
        return decode_utf16_code_point_exhaustive(input);
    }
    else {
        SCN_EXPECT(input.size() == 1);
        return static_cast<char32_t>(input.front());
    }
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
