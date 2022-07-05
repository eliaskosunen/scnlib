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

#if SCN_HAS_BITOPS
#include <bit>
#elif SCN_MSVC
#include <intrin.h>
#elif SCN_POSIX
#define _XOPEN_SOURCE 700
#include <strings.h>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        inline int count_trailing_zeroes(uint64_t val)
        {
            SCN_EXPECT(val != 0);
#if SCN_HAS_BITOPS
            return std::countr_zero(val);
#elif SCN_GCC_COMPAT
            return __builtin_ctzll(val);
#elif SCN_MSVC
            DWORD ret{};
            _BitScanForward(&ret, val);
            return ret;
#elif SCN_POSIX
            return ::ctzll(val);
#else
#error No CTZ implementation found
#endif
        }

        constexpr uint64_t has_zero_byte(uint64_t word)
        {
            return (word - 0x0101010101010101ull) & ~word &
                   0x8080808080808080ull;
        }

        constexpr uint64_t has_byte_between(uint64_t word, uint8_t a, uint8_t b)
        {
            const auto m = static_cast<uint64_t>(a) - 1,
                       n = static_cast<uint64_t>(b) + 1;
            return (((~0ul / 255 * (127 + (n)) - ((word) & ~0ul / 255 * 127)) &
                     ~(word) &
                     (((word) & ~0ul / 255 * 127) + ~0ul / 255 * (127 - (m)))) &
                    (~0ul / 255 * 128));
        }

        inline size_t get_index_of_first_nonmatching_byte(uint64_t word)
        {
            word ^= 0x8080808080808080ull;
            if (word == 0) {
                return 8;
            }
            return static_cast<size_t>(count_trailing_zeroes(word)) / 8;
        }

        inline size_t get_index_of_first_matching_byte(uint64_t word,
                                                       uint64_t pattern)
        {
            constexpr auto mask = 0x7f7f7f7f7f7f7f7full;
            auto input = word ^ pattern;
            auto tmp = (input & mask) + mask;
            tmp = ~(tmp | input | mask);
            return static_cast<size_t>(count_trailing_zeroes(tmp)) / 8;
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
