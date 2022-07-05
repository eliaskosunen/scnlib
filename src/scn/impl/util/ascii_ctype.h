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
        }
        constexpr bool is_ascii_space(wchar_t ch) SCN_NOEXCEPT
        {
            return ch == 0x20 || (ch >= 0x09 && ch <= 0x0d);
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
