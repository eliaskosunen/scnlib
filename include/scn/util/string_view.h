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
template <typename T>
struct is_string_view : std::false_type {};
template <typename CharT, typename Traits>
struct is_string_view<std::basic_string_view<CharT, Traits>> : std::true_type {
};

// workarounds for MSVC string_view debug iterators
template <typename CharT>
constexpr std::basic_string_view<CharT> make_string_view_from_iterators(
    typename std::basic_string_view<CharT>::iterator first,
    typename std::basic_string_view<CharT>::iterator last)
{
    if constexpr (std::is_constructible_v<std::basic_string_view<CharT>,
                                          decltype(first), decltype(last)> &&
                  !SCN_MSVC_DEBUG_ITERATORS) {
        return {first, last};
    }
    else {
        return {to_address(first), static_cast<size_t>(std::distance(
                                       to_address(first), to_address(last)))};
    }
}

template <typename CharT>
constexpr std::basic_string_view<CharT> make_string_view_from_pointers(
    const CharT* first,
    const CharT* last)
{
    if constexpr (std::is_constructible_v<std::basic_string_view<CharT>,
                                          const CharT*, const CharT*>) {
        return {first, last};
    }
    else {
        return {first, static_cast<size_t>(std::distance(first, last))};
    }
}

template <typename CharT>
constexpr auto make_string_view_iterator(
    std::basic_string_view<CharT> sv,
    typename std::basic_string_view<CharT>::iterator it) ->
    typename std::basic_string_view<CharT>::iterator
{
    if constexpr (std::is_constructible_v<
                      typename std::basic_string_view<CharT>::iterator,
                      decltype(it)> &&
                  !SCN_MSVC_DEBUG_ITERATORS) {
        SCN_UNUSED(sv);
        return it;
    }
    else {
        return sv.begin() + std::distance(sv.data(), detail::to_address(it));
    }
}

template <typename CharT>
constexpr auto make_string_view_iterator_from_pointer(
    std::basic_string_view<CharT> sv,
    const CharT* ptr) -> typename std::basic_string_view<CharT>::iterator
{
    if constexpr (std::is_constructible_v<
                      typename std::basic_string_view<CharT>::iterator,
                      const CharT*> &&
                  !SCN_MSVC_DEBUG_ITERATORS) {
        SCN_UNUSED(sv);
        return ptr;
    }
    else {
        return sv.begin() + std::distance(sv.data(), ptr);
    }
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
