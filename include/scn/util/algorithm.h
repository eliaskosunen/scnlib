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

#include <cstring>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        /**
         * Implementation of `std::max` without including `<algorithm>`
         */
        template <typename T>
        constexpr T max(T a, T b) SCN_NOEXCEPT
        {
            return (a < b) ? b : a;
        }

        /**
         * Implementation of `std::min_element` without including `<algorithm>`
         */
        template <typename It>
        constexpr It min_element(It first, It last)
        {
            if (first == last) {
                return last;
            }

            It smallest = first;
            ++first;
            for (; first != last; ++first) {
                if (*first < *smallest) {
                    smallest = first;
                }
            }
            return smallest;
        }

        /**
         * Implementation of `std::min` without including `<algorithm>`
         */
        template <typename T>
        constexpr T min(T a, T b) SCN_NOEXCEPT
        {
            return (b < a) ? b : a;
        }

        template <bool IsConstexpr, typename T, typename Ptr = const T*>
        constexpr Ptr find(Ptr first, Ptr last, T value)
        {
            for (; first != last; ++first) {
                if (*first == value) {
                    return first;
                }
            }
            return last;
        }

        template <>
        inline const char* find<false, char>(const char* first,
                                             const char* last,
                                             char value)
        {
            auto ptr = static_cast<const char*>(
                std::memchr(first, value, static_cast<size_t>(last - first)));
            return ptr != nullptr ? ptr : last;
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
