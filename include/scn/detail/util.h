// Copyright 2017-2018 Elias Kosunen
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

#ifndef SCN_DETAIL_UTIL_H
#define SCN_DETAIL_UTIL_H

#include "config.h"

#include <limits>
#include <type_traits>

namespace scn {
    namespace detail {
        /**
         * Maximum digits potentially required to represent an integer of type
         * Integral. Includes possible sign.
         * \param base Base of the integer
         */
        template <typename Integral>
        int max_digits(int base) noexcept
        {
            auto i = std::numeric_limits<Integral>::max();

            int digits = 0;
            while (i) {
                i /= static_cast<Integral>(base);
                digits++;
            }

            return digits + (std::is_signed<Integral>::value ? 1 : 0);
        }
    }  // namespace detail
}  // namespace scn

#endif  // SCN_DETAIL_UTIL_H
