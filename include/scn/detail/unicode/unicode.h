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

#ifndef SCN_DETAIL_UNICODE_UNICODE_H
#define SCN_DETAIL_UNICODE_UNICODE_H

#include "utf8.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename I, typename S>
        SCN_CONSTEXPR14 expected<I> parse_code_point(
            I begin,
            S end,
            code_point& cp,
            std::integral_constant<size_t, 1>)
        {
            return utf8::parse_code_point(begin, end, cp);
        }
    }  // namespace detail

    template <typename I, typename S>
    SCN_CONSTEXPR14 expected<I> parse_code_point(I begin, S end, code_point& cp)
    {
        return detail::parse_code_point(
            begin, end, cp,
            std::integral_constant<size_t, sizeof(typename std::iterator_traits<
                                                  I>::value_type)>{});
    }

    namespace detail {
        template <typename T>
        SCN_CONSTEXPR14 int get_sequence_length(
            T a,
            std::integral_constant<size_t, 1>)
        {
            return utf8::get_sequence_length(a);
        }
        template <typename T>
        SCN_CONSTEXPR14 int get_sequence_length(
            T,
            std::integral_constant<size_t, 4>)
        {
            return 1;
        }
    }  // namespace detail

    template <typename T>
    SCN_CONSTEXPR14 int get_sequence_length(T a)
    {
        return detail::get_sequence_length(
            a, std::integral_constant<size_t, sizeof(T)>{});
    }

    namespace detail {
        template <typename I, typename S>
        SCN_CONSTEXPR14 expected<std::ptrdiff_t>
        code_point_distance(I begin, S end, std::integral_constant<size_t, 1>)
        {
            return utf8::code_point_distance(begin, end);
        }
    }  // namespace detail

    template <typename I, typename S>
    SCN_CONSTEXPR14 expected<std::ptrdiff_t> code_point_distance(I begin, S end)
    {
        return detail::code_point_distance(
            begin, end,
            std::integral_constant<
                size_t, sizeof(std::iterator_traits<I>::value_type)>{});
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif
