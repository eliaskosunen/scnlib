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

#include <scn/impl/algorithms/unicode_algorithms.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {

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

constexpr std::size_t calculate_valid_text_width(char32_t cp)
{
    return calculate_text_width_for_fmt_v10(cp);
}

template <typename CharT>
std::size_t calculate_valid_text_width(std::basic_string_view<CharT> input)
{
    size_t count{0};
    for_each_code_point_valid(input, [&count](char32_t cp) {
        count += calculate_text_width_for_fmt_v10(cp);
    });
    return count;
}

constexpr std::size_t calculate_text_width(char32_t cp)
{
    return calculate_text_width_for_fmt_v10(cp);
}

template <typename CharT>
std::size_t calculate_text_width(std::basic_string_view<CharT> input)
{
    size_t count{0};
    for_each_code_point(input, [&count](char32_t cp) {
        count += calculate_text_width_for_fmt_v10(cp);
    });
    return count;
}

}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
