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

#include <scn/impl/locale.h>
#include <scn/impl/unicode/unicode.h>

#include <array>
#include <iterator>

#if SCN_POSIX
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <cwchar>
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
enum class text_width_algorithm {
    // Use POSIX wcswidth
    // Only on POSIX
    wcswidth,

    // 1 code unit = 1 width unit
    code_units,

    // 1 code point = 1 width unit
    code_points,

    // 1 (extended) grapheme cluster = 1 width unit
    // grapheme_clusters,  // TODO

    // 1 code point = 1 width unit, except some are 2
    // {fmt} uses this in v10.0.0
    fmt_v10,

    // Whatever {fmt} uses in its latest version
    fmt_latest = fmt_v10,

    // 1 (extended) grapheme cluster = 1 width unit, except some are 2
    // std::format uses this, in C++23
    // std_format_23,  // TODO

    // Whatever std::format uses in the latest C++ WD
    // std_format_latest = std_format_23,

    // Width according to UAX #11,
    // with the "ambiguous" category having a width of 1
    // uax11,  // TODO

    // Width according to UAX #11,
    // with the "ambiguous" category having a width of 2
    // uax11_cjk,  // TODO
};

inline constexpr auto default_text_width_algorithm =
    text_width_algorithm::fmt_latest;

constexpr std::size_t calculate_text_width_for_fmt_v10(char32_t cp)
{
    if (cp >= 0x1100 &&
        (cp <= 0x115f ||  // Hangul Jamo init. consonants
         cp == 0x2329 ||  // LEFT-POINTING ANGLE BRACKET
         cp == 0x232a ||  // RIGHT-POINTING ANGLE BRACKET
         // CJK ... Yi except IDEOGRAPHIC HALF FILL SPACE:
         (cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f) ||
         (cp >= 0xac00 && cp <= 0xd7a3) ||    // Hangul Syllables
         (cp >= 0xf900 && cp <= 0xfaff) ||    // CJK Compatibility Ideographs
         (cp >= 0xfe10 && cp <= 0xfe19) ||    // Vertical Forms
         (cp >= 0xfe30 && cp <= 0xfe6f) ||    // CJK Compatibility Forms
         (cp >= 0xff00 && cp <= 0xff60) ||    // Fullwidth Forms
         (cp >= 0xffe0 && cp <= 0xffe6) ||    // Fullwidth Forms
         (cp >= 0x20000 && cp <= 0x2fffd) ||  // CJK
         (cp >= 0x30000 && cp <= 0x3fffd) ||
         // Miscellaneous Symbols and Pictographs + Emoticons:
         (cp >= 0x1f300 && cp <= 0x1f64f) ||
         // Supplemental Symbols and Pictographs:
         (cp >= 0x1f900 && cp <= 0x1f9ff))) {
        return 2;
    }
    return 1;
}

template <typename Dependent = void>
std::size_t calculate_valid_text_width(
    char32_t cp,
    text_width_algorithm algo = default_text_width_algorithm)
{
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wswitch-enum")

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wcovered-switch-default")

    switch (algo) {
        case text_width_algorithm::wcswidth: {
#if SCN_POSIX
            set_clocale_classic_guard clocale_guard{LC_CTYPE};

            std::wstring winput;
            transcode_valid_to_string(std::u32string_view{&cp, 1}, winput);
            const auto n = ::wcswidth(winput.data(), winput.size());
            SCN_ENSURE(n != -1);
            return static_cast<size_t>(n);
#else
            SCN_ASSERT(false, "No wcswidth");
            SCN_UNREACHABLE;
#endif
        }

        case text_width_algorithm::code_units: {
            std::wstring winput;
            transcode_valid_to_string(std::u32string_view{&cp, 1}, winput);
            return winput.size();
        }

        case text_width_algorithm::code_points: {
            return 1;
        }

        case text_width_algorithm::fmt_v10: {
            return calculate_text_width_for_fmt_v10(cp);
        }

        default:
            SCN_ASSERT(false, "Not implemented");
            SCN_UNREACHABLE;
    }
    SCN_CLANG_POP    // -Wcovered-switch-default
        SCN_GCC_POP  // -Wswitch-enum
}

template <typename CharT>
std::size_t calculate_valid_text_width(
    std::basic_string_view<CharT> input,
    text_width_algorithm algo = default_text_width_algorithm)
{
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wswitch-enum")

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wcovered-switch-default")

    switch (algo) {
        case text_width_algorithm::wcswidth: {
#if SCN_POSIX
            set_clocale_classic_guard clocale_guard{LC_CTYPE};

            std::wstring winput;
            transcode_valid_to_string(input, winput);
            const auto n = ::wcswidth(winput.data(), winput.size());
            SCN_ENSURE(n != -1);
            return static_cast<size_t>(n);
#else
            SCN_ASSERT(false, "No wcswidth");
            SCN_UNREACHABLE;
#endif
        }

        case text_width_algorithm::code_units: {
            return input.size();
        }

        case text_width_algorithm::code_points: {
            return count_valid_code_points(input);
        }

        case text_width_algorithm::fmt_v10: {
            size_t count{0};
            for_each_code_point_valid(input, [&count](char32_t cp) {
                count += calculate_text_width_for_fmt_v10(cp);
            });
            return count;
        }

        default:
            SCN_ASSERT(false, "Not implemented");
            SCN_UNREACHABLE;
    }
    SCN_CLANG_POP    // -Wcovered-switch-default
        SCN_GCC_POP  // -Wswitch-enum
}

template <typename Dependent = void>
std::size_t calculate_text_width(
    char32_t cp,
    text_width_algorithm algo = default_text_width_algorithm)
{
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wswitch-enum")

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wcovered-switch-default")

    switch (algo) {
        case text_width_algorithm::wcswidth: {
#if SCN_POSIX
            set_clocale_classic_guard clocale_guard{LC_CTYPE};

            std::wstring winput;
            transcode_to_string(std::u32string_view{&cp, 1}, winput);
            const auto n = ::wcswidth(winput.data(), winput.size());
            SCN_ENSURE(n != -1);
            return static_cast<size_t>(n);
#else
            SCN_ASSERT(false, "No wcswidth");
            SCN_UNREACHABLE;
#endif
        }

        case text_width_algorithm::code_units: {
            std::wstring winput;
            transcode_to_string(std::u32string_view{&cp, 1}, winput);
            return winput.size();
        }

        case text_width_algorithm::code_points: {
            return 1;
        }

        case text_width_algorithm::fmt_v10: {
            return calculate_text_width_for_fmt_v10(cp);
        }

        default:
            SCN_ASSERT(false, "Not implemented");
            SCN_UNREACHABLE;
    }
    SCN_CLANG_POP    // -Wcovered-switch-default
        SCN_GCC_POP  // -Wswitch-enum
}

template <typename CharT>
std::size_t calculate_text_width(
    std::basic_string_view<CharT> input,
    text_width_algorithm algo = default_text_width_algorithm)
{
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wswitch-enum")

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wcovered-switch-default")

    switch (algo) {
        case text_width_algorithm::wcswidth: {
#if SCN_POSIX
            set_clocale_classic_guard clocale_guard{LC_CTYPE};

            std::wstring winput;
            transcode_to_string(input, winput);
            const auto n = ::wcswidth(winput.data(), winput.size());
            SCN_ENSURE(n != -1);
            return static_cast<size_t>(n);
#else
            SCN_ASSERT(false, "No wcswidth");
            SCN_UNREACHABLE;
#endif
        }

        case text_width_algorithm::code_units: {
            return input.size();
        }

        case text_width_algorithm::code_points: {
            return count_valid_code_points(input);
        }

        case text_width_algorithm::fmt_v10: {
            size_t count{0};
            for_each_code_point(input, [&count](char32_t cp) {
                count += calculate_text_width_for_fmt_v10(cp);
            });
            return count;
        }

        default:
            SCN_ASSERT(false, "Not implemented");
            SCN_UNREACHABLE;
    }
    SCN_CLANG_POP    // -Wcovered-switch-default
        SCN_GCC_POP  // -Wswitch-enum
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
