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

#ifndef SCN_DETAIL_UTIL_MATH_H
#define SCN_DETAIL_UTIL_MATH_H

#include "../fwd.h"

#include <cmath>
#include <limits>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Integral>
        SCN_CONSTEXPR14 int _max_digits(int base) noexcept
        {
            using lim = std::numeric_limits<Integral>;

            char base8_digits[8] = {3, 5, 0, 11, 0, 0, 0, 21};

            if (base == 10) {
                return lim::digits10;
            }
            if (base == 8) {
                return static_cast<int>(base8_digits[sizeof(Integral) - 1]);
            }
            if (base == lim::radix) {
                return lim::digits;
            }

            auto i = lim::max();

            Integral digits = 0;
            while (i) {
                i /= static_cast<Integral>(base);
                digits++;
            }
            return static_cast<int>(digits);
        }
        template <typename Integral>
        SCN_CONSTEXPR14 int max_digits(int base) noexcept
        {
            auto b = base == 0 ? 8 : base;
            auto d = _max_digits<Integral>(b) +
                     (std::is_signed<Integral>::value ? 1 : 0);
            if (base == 0) {
                return d + 2;  // accommondate for 0x/0o
            }
            return d;
        }

        template <typename T>
        constexpr std::pair<T, T> div(T l, T r) noexcept
        {
            return {l / r, l % r};
        }

        template <typename T>
        bool float_eq(T a, T b, T tolerance = std::numeric_limits<T>::epsilon())
        {
            T diff = std::abs(a - b);
            if (diff <= tolerance) {
                return true;
            }
            return diff < std::fmax(std::abs(a), std::abs(b)) * tolerance;
        }
        template <typename T>
        bool float_eq_zero(T a, T tolerance = std::numeric_limits<T>::epsilon())
        {
            return std::abs(a) < tolerance;
        }
        template <typename T>
        bool float_eq_within(T a, T b, std::size_t interval = 1)
        {
            T min_a =
                a - (a - std::nextafter(a, std::numeric_limits<T>::lowest())) *
                        interval;
            T max_a =
                a + (std::nextafter(a, std::numeric_limits<T>::max()) - a) *
                        interval;

            return min_a <= b && max_a >= b;
        }

        template <typename T>
        struct zero_value;
        template <>
        struct zero_value<float> {
            static constexpr float value = 0.0f;
        };
        template <>
        struct zero_value<double> {
            static constexpr double value = 0.0;
        };
        template <>
        struct zero_value<long double> {
            static constexpr long double value = 0.0l;
        };

        template <typename CharT>
        bool is_base_digit(CharT ch, int base)
        {
            if (base <= 10) {
                return ch >= static_cast<CharT>('0') &&
                       ch <= static_cast<CharT>('0') + base - 1;
            }
            return is_base_digit(ch, 10) ||
                   (ch >= static_cast<CharT>('a') &&
                    ch <= static_cast<CharT>('a') + base - 1) ||
                   (ch >= static_cast<CharT>('A') &&
                    ch <= static_cast<CharT>('A') + base - 1);
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#endif
