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

#include <type_traits>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <std::size_t I>
struct priority_tag : priority_tag<I - 1> {};
template <>
struct priority_tag<0> {};

template <typename T>
using integer_type_for_char =
    typename std::conditional<std::is_signed<T>::value, int, unsigned>::type;

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T, typename Self>
inline constexpr bool is_not_self = !std::is_same_v<remove_cvref_t<T>, Self>;

template <typename T, template <typename...> class Templ>
struct is_specialization_of_impl : std::false_type {};
template <typename... T, template <typename...> class Templ>
struct is_specialization_of_impl<Templ<T...>, Templ> : std::true_type {};

template <typename T, template <typename...> class Templ>
using is_specialization_of =
    is_specialization_of_impl<remove_cvref_t<T>, Templ>;
template <typename T, template <typename...> class Templ>
inline constexpr bool is_specialization_of_v =
    is_specialization_of<T, Templ>::value;
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
