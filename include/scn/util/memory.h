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

#include <scn/detail/ranges.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {

template <typename Ptr, typename>
struct pointer_traits {
};

template <typename T>
struct pointer_traits<T*, void> {
    using pointer = T*;
    using element_type = T;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    using rebind = U*;

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    static constexpr pointer pointer_to(U& r) noexcept
    {
        return &r;
    }

    static constexpr pointer to_address(pointer p) noexcept
    {
        return p;
    }
};

template <typename Ptr>
using apply_member_difference_type = typename Ptr::difference_type;
template <typename Ptr>
using get_member_difference_type =
    mp_eval_or<std::ptrdiff_t, apply_member_difference_type, Ptr>;

template <typename Ptr, typename ElementType>
struct pointer_traits_generic_base {
    using pointer = Ptr;
    using element_type = ElementType;

    using difference_type = get_member_difference_type<Ptr>;
    static_assert(std::is_integral_v<difference_type>);

    // no rebind (TODO?)

    template <typename P = Ptr>
    static auto pointer_to(ElementType& r) -> decltype(P::pointer_to(r))
    {
        return Ptr::pointer_to(r);
    }
};

template <typename It, typename = void>
struct wrapped_pointer_iterator;

#ifdef _GLIBCXX_DEBUG
template <typename Elem, typename Container>
struct wrapped_pointer_iterator<__gnu_debug::_Safe_iterator<Elem*, Container>> {
    static constexpr auto to_address(
        const __gnu_debug::_Safe_iterator<Elem*, Container>& it) noexcept
    {
        return it.base();
    }
};
#endif
#if SCN_STDLIB_GLIBCXX
template <typename Elem, typename Container>
struct wrapped_pointer_iterator<
    __gnu_cxx::__normal_iterator<Elem*, Container>> {
    static constexpr auto to_address(
        const __gnu_cxx::__normal_iterator<Elem*, Container>& it) noexcept
    {
        return it.base();
    }
};
#endif
#if SCN_STDLIB_LIBCPP
template <typename Elem>
struct wrapped_pointer_iterator<std::__wrap_iter<Elem*>> {
    static constexpr auto to_address(const std::__wrap_iter<Elem*>& it) noexcept
    {
        return it.base();
    }
};
#endif

template <typename I>
using apply_member_unwrapped = decltype(SCN_DECLVAL(I&)._Unwrapped());
template <typename It>
struct wrapped_pointer_iterator<
    It,
    std::enable_if_t<ranges::random_access_iterator<It> &&
                     mp_valid_v<apply_member_unwrapped, It>>> {
    static constexpr auto to_address(const It& it) noexcept
    {
        return it._Unwrapped();
    }
};

template <typename I>
using apply_member_to_address =
    decltype(wrapped_pointer_iterator<I>::to_address(SCN_DECLVAL(const I&)));

template <typename Iterator>
struct pointer_traits<
    Iterator,
    std::enable_if_t<mp_valid_v<apply_member_to_address, Iterator>>>
    : pointer_traits_generic_base<
          Iterator,
          std::remove_reference_t<decltype(*SCN_DECLVAL(Iterator&))>> {
    static constexpr auto to_address(const Iterator& it) noexcept
    {
        return wrapped_pointer_iterator<Iterator>::to_address(it);
    }
};

template <typename It>
using apply_ptr_traits_to_address =
    decltype(pointer_traits<It>::to_address(SCN_DECLVAL(const It&)));
template <typename It>
inline constexpr bool can_make_address_from_iterator =
    std::is_pointer_v<mp_valid_result_t<apply_ptr_traits_to_address, It>>;

template <typename T>
constexpr T* to_address_impl(T* p, priority_tag<2>) noexcept
{
    return p;
}
template <typename Ptr>
constexpr auto to_address_impl(const Ptr& p, priority_tag<1>) noexcept
    -> decltype(::scn::detail::pointer_traits<Ptr>::to_address(p))
{
    return ::scn::detail::pointer_traits<Ptr>::to_address(p);
}
template <typename Ptr>
constexpr auto to_address_impl(const Ptr& p, priority_tag<0>) noexcept
    -> decltype(::scn::detail::to_address_impl(p.operator->(),
                                               priority_tag<2>{}))
{
    return ::scn::detail::to_address_impl(p.operator->(), priority_tag<2>{});
}

template <typename Ptr>
constexpr auto to_address(Ptr&& p) noexcept
    -> decltype(::scn::detail::to_address_impl(SCN_FWD(p), priority_tag<2>{}))
{
    return ::scn::detail::to_address_impl(SCN_FWD(p), priority_tag<2>{});
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
