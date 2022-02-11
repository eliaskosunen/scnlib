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
        inline constexpr bool is_wide_multichar()
        {
            return sizeof(wchar_t) == 2;
        }

        inline constexpr bool is_multichar_type(char)
        {
            return true;
        }
        inline constexpr bool is_multichar_type(wchar_t)
        {
            return is_wide_multichar();
        }

        using utf8_tag = std::integral_constant<size_t, 1>;
        using utf16_tag = std::integral_constant<size_t, 2>;
        using utf32_tag = std::integral_constant<size_t, 4>;

#define SCN_MAKE_UTF_TAG(CharT) \
    std::integral_constant<size_t, sizeof(CharT)> {}

        template <typename I, typename S>
        SCN_CONSTEXPR14 expected<I> parse_code_point(I begin,
                                                     S end,
                                                     code_point& cp,
                                                     utf8_tag)
        {
            return utf8::parse_code_point(begin, end, cp);
        }
        template <typename I, typename S>
        SCN_CONSTEXPR14 expected<I> parse_code_point(I begin,
                                                     S end,
                                                     code_point& cp,
                                                     utf32_tag)
        {
            SCN_EXPECT(begin != end);
            cp = make_code_point(*begin);
            return {++begin};
        }
    }  // namespace detail

    template <typename I, typename S>
    SCN_CONSTEXPR14 expected<I> parse_code_point(I begin, S end, code_point& cp)
    {
        return detail::parse_code_point(
            begin, end, cp,
            SCN_MAKE_UTF_TAG(typename std::iterator_traits<I>::value_type));
    }

    namespace detail {
        template <typename T>
        SCN_CONSTEXPR14 int get_sequence_length(T a, utf8_tag)
        {
            return utf8::get_sequence_length(a);
        }
        template <typename T>
        SCN_CONSTEXPR14 int get_sequence_length(T, utf32_tag)
        {
            return 1;
        }
    }  // namespace detail

    template <typename T>
    SCN_CONSTEXPR14 int get_sequence_length(T a)
    {
        return detail::get_sequence_length(a, SCN_MAKE_UTF_TAG(T));
    }

    namespace detail {
        template <typename I, typename S>
        SCN_CONSTEXPR14 expected<std::ptrdiff_t> code_point_distance(I begin,
                                                                     S end,
                                                                     utf8_tag)
        {
            return utf8::code_point_distance(begin, end);
        }
        template <typename I, typename S>
        SCN_CONSTEXPR14 expected<std::ptrdiff_t> code_point_distance(I begin,
                                                                     S end,
                                                                     utf32_tag)
        {
            return {end - begin};
        }
    }  // namespace detail

    template <typename I, typename S>
    SCN_CONSTEXPR14 expected<std::ptrdiff_t> code_point_distance(I begin, S end)
    {
        return detail::code_point_distance(
            begin, end,
            SCN_MAKE_UTF_TAG(typename std::iterator_traits<I>::value_type));
    }

#undef SCN_MAKE_UTF_TAG

    SCN_END_NAMESPACE
}  // namespace scn

#endif
