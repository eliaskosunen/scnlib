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

    template <typename T>
    constexpr bool operator==(code_point a, T b)
    {
        return static_cast<uint32_t>(a) == static_cast<uint32_t>(b);
    }
    template <typename T>
    constexpr bool operator!=(code_point a, T b)
    {
        return static_cast<uint32_t>(a) != static_cast<uint32_t>(b);
    }
    template <typename T>
    constexpr bool operator<(code_point a, T b)
    {
        return static_cast<uint32_t>(a) < static_cast<uint32_t>(b);
    }
    template <typename T>
    constexpr bool operator>(code_point a, T b)
    {
        return static_cast<uint32_t>(a) > static_cast<uint32_t>(b);
    }
    template <typename T>
    constexpr bool operator<=(code_point a, T b)
    {
        return static_cast<uint32_t>(a) <= static_cast<uint32_t>(b);
    }
    template <typename T>
    constexpr bool operator>=(code_point a, T b)
    {
        return static_cast<uint32_t>(a) >= static_cast<uint32_t>(b);
    }

    template <typename T>
    constexpr code_point make_code_point(T ch)
    {
        return static_cast<code_point>(ch);
    }

    constexpr inline bool is_ascii_code_point(code_point cp)
    {
        return cp <= 0x7f;
    }

    SCN_END_NAMESPACE
}  // namespace scn
