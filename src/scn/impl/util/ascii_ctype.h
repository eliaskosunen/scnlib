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

#include <array>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        inline bool is_ascii_space(char ch) SCN_NOEXCEPT
        {
#if 0
            constexpr auto chars_per_lookup_cell = 256 / 8;
            static constexpr std::array<uint32_t, 8> lookup = {
                (1 << 0x09) | (1 << 0x0a) | (1 << 0x0b) | (1 << 0x0c) |
                    (1 << 0x0d),
                (1 << (0x20 - chars_per_lookup_cell)),
                0,
                0,
                0,
                0,
                0,
                0};
            const auto word =
                lookup[static_cast<unsigned char>(ch) / chars_per_lookup_cell];
            return (word >>
                    (static_cast<unsigned char>(ch) % chars_per_lookup_cell)) &
                   1;
#else
            static constexpr std::array<bool, 256> lookup = {
                {false, false, false, false, false, false, false, false, false,
                 true,  true,  true,  true,  true,  false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, true,  false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false}};

            return lookup[static_cast<size_t>(static_cast<unsigned char>(ch))];
#endif
        }

        constexpr bool is_ascii_space(wchar_t ch) SCN_NOEXCEPT
        {
            return ch == 0x20 || (ch >= 0x09 && ch <= 0x0d);
        }

        constexpr bool is_ascii_char(char ch) SCN_NOEXCEPT
        {
            return static_cast<unsigned char>(ch) <= 127;
        }

        constexpr bool is_ascii_char(wchar_t ch) SCN_NOEXCEPT
        {
#if WCHAR_MIN < 0
            return ch >= 0 && ch <= 127;
#else
            return ch <= 127;
#endif
        }

        constexpr bool is_ascii_char(char32_t cp) SCN_NOEXCEPT
        {
            return cp <= 127;
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
