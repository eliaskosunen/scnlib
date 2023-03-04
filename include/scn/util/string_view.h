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

#include <scn/util/algorithm.h>
#include <scn/util/span.h>

#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string_view>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        inline size_t strlen(const char* s) SCN_NOEXCEPT
        {
            return ::std::strlen(s);
        }
        inline size_t strlen(const wchar_t* s) SCN_NOEXCEPT
        {
            return ::std::wcslen(s);
        }
        inline size_t strlen(const char16_t* s) SCN_NOEXCEPT
        {
            SCN_EXPECT(s);
            auto end = s;
            for (; *end != u'\0'; ++end)
                ;
            return static_cast<size_t>(end - s);
        }
        inline size_t strlen(const char32_t* s) SCN_NOEXCEPT
        {
            SCN_EXPECT(s);
            auto end = s;
            for (; *end != U'\0'; ++end)
                ;
            return static_cast<size_t>(end - s);
        }
#if SCN_HAS_CHAR8
        inline size_t strlen(const char8_t* s) SCN_NOEXCEPT
        {
            return std::strlen(reinterpret_cast<const char*>(s));
        }
#endif
    }  // namespace detail

    namespace detail {
        template <typename CharT>
        constexpr std::basic_string_view<CharT> make_string_view_from_iterators(
            typename std::basic_string_view<CharT>::iterator first,
            typename std::basic_string_view<CharT>::iterator last)
        {
            return {to_address(first),
                    static_cast<size_t>(
                        std::distance(to_address(first), to_address(last)))};
        }

        template <typename CharT>
        constexpr auto make_string_view_iterator(
            std::basic_string_view<CharT> sv,
            const CharT* ptr) ->
            typename std::basic_string_view<CharT>::iterator
        {
            return sv.begin() + std::distance(sv.data(), ptr);
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
