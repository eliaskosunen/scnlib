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
struct priority_tag : priority_tag<I - 1> {
};
template <>
struct priority_tag<0> {
};

template <typename T>
using integer_type_for_char =
    std::conditional_t<std::is_signed_v<T>, int, unsigned>;

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T, typename Self>
inline constexpr bool is_not_self = !std::is_same_v<remove_cvref_t<T>, Self>;

template <typename T, template <typename...> class Templ>
struct is_specialization_of_impl : std::false_type {
};
template <typename... T, template <typename...> class Templ>
struct is_specialization_of_impl<Templ<T...>, Templ> : std::true_type {
};

template <typename T, template <typename...> class Templ>
using is_specialization_of =
    is_specialization_of_impl<remove_cvref_t<T>, Templ>;
template <typename T, template <typename...> class Templ>
inline constexpr bool is_specialization_of_v =
    is_specialization_of<T, Templ>::value;

// from mp11:

template <typename T>
struct mp_identity {
    using type = T;
};
template <typename T>
using mp_identity_t = typename mp_identity<T>::type;

template <bool B>
using mp_bool = std::integral_constant<bool, B>;
template <typename T>
using mp_to_bool = mp_bool<static_cast<bool>(T::value)>;
template <typename T>
using mp_not = mp_bool<!T::value>;

template <bool C, typename T, typename... E>
struct mp_if_c_impl;
template <typename T, typename... E>
struct mp_if_c_impl<true, T, E...> {
    using type = T;
};
template <typename T, typename E>
struct mp_if_c_impl<false, T, E> {
    using type = E;
};

template <bool C, typename T, typename... E>
using mp_if_c = typename mp_if_c_impl<C, T, E...>::type;
template <typename C, typename T, typename... E>
using mp_if = typename mp_if_c_impl<static_cast<bool>(C::value), T, E...>::type;

template <template <typename...> class F, typename... T>
struct mp_valid_impl {
    template <template <typename...> class G, typename = G<T...>>
    static std::true_type check(int);
    template <template <typename...> class>
    static std::false_type check(...);

    using type = decltype(check<F>(0));
};

template <template <typename...> class F, typename... T>
using mp_valid = typename mp_valid_impl<F, T...>::type;
template <template <typename...> class F, typename... T>
inline constexpr bool mp_valid_v = mp_valid<F, T...>::value;

struct mp_nonesuch {};
template <template <typename...> class F, typename... T>
struct mp_defer_impl {
    using type = F<T...>;
};

template <template <typename...> class F, typename... T>
using mp_defer = mp_if<mp_valid<F, T...>, mp_defer_impl<F, T...>, mp_nonesuch>;

template <bool C, class T, template <class...> class F, class... U>
struct mp_eval_if_c_impl;

template <class T, template <class...> class F, class... U>
struct mp_eval_if_c_impl<true, T, F, U...> {
    using type = T;
};

template <class T, template <class...> class F, class... U>
struct mp_eval_if_c_impl<false, T, F, U...> : mp_defer<F, U...> {
};

template <bool C, class T, template <class...> class F, class... U>
using mp_eval_if_c = typename mp_eval_if_c_impl<C, T, F, U...>::type;
template <class C, class T, template <class...> class F, class... U>
using mp_eval_if =
    typename mp_eval_if_c_impl<static_cast<bool>(C::value), T, F, U...>::type;
template <class C, class T, class Q, class... U>
using mp_eval_if_q = typename mp_eval_if_c_impl<static_cast<bool>(C::value),
                                                T,
                                                Q::template fn,
                                                U...>::type;

// mp_eval_if_not
template <class C, class T, template <class...> class F, class... U>
using mp_eval_if_not = mp_eval_if<mp_not<C>, T, F, U...>;
template <class C, class T, class Q, class... U>
using mp_eval_if_not_q = mp_eval_if<mp_not<C>, T, Q::template fn, U...>;

// mp_eval_or
template <class T, template <class...> class F, class... U>
using mp_eval_or = mp_eval_if_not<mp_valid<F, U...>, T, F, U...>;
template <class T, class Q, class... U>
using mp_eval_or_q = mp_eval_or<T, Q::template fn, U...>;

// mp_valid_and_true
template <template <class...> class F, class... T>
using mp_valid_and_true = mp_eval_or<std::false_type, F, T...>;
template <class Q, class... T>
using mp_valid_and_true_q = mp_valid_and_true<Q::template fn, T...>;

// extension
template <template <typename...> class F, typename... T>
using mp_valid_result =
    mp_if<mp_valid<F, T...>, mp_defer_impl<F, T...>, mp_identity<void>>;
template <template <typename...> class F, typename... T>
using mp_valid_result_t = typename mp_valid_result<F, T...>::type;

}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
