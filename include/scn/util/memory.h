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

#include <scn/util/meta.h>

#include <cstring>
#include <new>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
#include <iterator>
SCN_GCC_POP

#if SCN_STDLIB_MS_STL
#include <string>
#include <string_view>
#include <vector>
#if SCN_HAS_STD_SPAN
#include <span>
#endif
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <typename Ptr, typename>
struct pointer_traits {};

template <typename T>
struct pointer_traits<T*, void> {
    using pointer = T*;
    using element_type = T;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    using rebind = U*;

    template <typename U = T,
              typename std::enable_if<!std::is_void<U>::value>::type* = nullptr>
    static constexpr pointer pointer_to(U& r) SCN_NOEXCEPT
    {
        return &r;
    }

    static constexpr pointer to_address(pointer p) SCN_NOEXCEPT
    {
        return p;
    }
};

template <typename Ptr, typename = void>
struct difference_type_for_pointer_traits {
    using type = std::ptrdiff_t;
};
template <typename Ptr>
struct difference_type_for_pointer_traits<
    Ptr,
    std::void_t<typename Ptr::difference_type>> {
    using type = typename Ptr::difference_type;
};

template <typename Ptr, typename ElementType>
struct pointer_traits_generic_impl {
    using pointer = Ptr;
    using element_type = ElementType;
    using difference_type =
        typename difference_type_for_pointer_traits<Ptr>::type;

    // no rebind (TODO?)

    template <typename P = Ptr>
    static auto pointer_to(ElementType& r) -> decltype(P::pointer_to(r))
    {
        return Ptr::pointer_to(r);
    }
};

#if 0
        template <typename Ptr>
        struct pointer_traits<Ptr, std::void_t<typename Ptr::element_type>>
            : pointer_traits_generic_impl<Ptr, typename Ptr::element_type> {};

        template <template <class...> class Template,
                  typename T,
                  typename... Args>
        struct pointer_traits<Template<T, Args...>>
            : pointer_traits_generic_impl<Template<T, Args...>, T> {};
#endif

template <typename It, typename = void>
struct is_wrapped_pointer_iterator : std::false_type {};

// Workaround for MSVC _String_view_iterator
#if SCN_STDLIB_MS_STL
template <typename Traits>
struct is_wrapped_pointer_iterator<std::_String_view_iterator<Traits>>
    : std::true_type {};

template <typename Traits>
struct is_wrapped_pointer_iterator<std::_String_iterator<Traits>>
    : std::true_type {};
template <typename Traits>
struct is_wrapped_pointer_iterator<std::_String_const_iterator<Traits>>
    : std::true_type {};

template <typename Vec>
struct is_wrapped_pointer_iterator<std::_Vector_iterator<Vec>>
    : std::true_type {};
template <typename Vec>
struct is_wrapped_pointer_iterator<std::_Vector_const_iterator<Vec>>
    : std::true_type {};

#if SCN_HAS_STD_SPAN
template <typename T>
struct is_wrapped_pointer_iterator<std::_Span_iterator<T>> : std::true_type {};
#endif
#endif  // SCN_STDLIB_MS_STL

// Workaround for libstdc++ __normal_iterator
#if SCN_STDLIB_GLIBCXX
template <typename ElementType, typename Container>
struct is_wrapped_pointer_iterator<
    __gnu_cxx::__normal_iterator<ElementType*, Container>> : std::true_type {};
#endif  // SCN_STDLIB_GLIBCXX

// Workaround for libc++ __wrap_iter
#if SCN_STDLIB_LIBCPP
template <typename ElementType>
struct is_wrapped_pointer_iterator<std::__wrap_iter<ElementType*>>
    : std::true_type {};
#endif  // SCN_STDLIB_LIBCPP

template <typename Iterator>
struct pointer_traits<
    Iterator,
    std::enable_if_t<is_wrapped_pointer_iterator<Iterator>::value>>
    : pointer_traits_generic_impl<
          Iterator,
          std::remove_reference_t<decltype(*SCN_DECLVAL(Iterator&))>> {
    static constexpr auto to_address(const Iterator& it) SCN_NOEXCEPT
    {
#if SCN_STDLIB_MS_STL
        return it._Unwrapped();
#elif SCN_STDLIB_GLIBCXX || SCN_STDLIB_LIBCPP
        return it.base();
#else
        static_assert(dependent_false<Iterator>::value);
#endif
    }
};

template <typename It, typename = void>
struct can_make_address_from_iterator : std::false_type {};
template <typename It>
struct can_make_address_from_iterator<
    It,
    std::enable_if_t<std::is_pointer_v<decltype(pointer_traits<It>::to_address(
        SCN_DECLVAL(const It&)))>>> : std::true_type {};

template <typename T>
constexpr T* to_address_impl(T* p, priority_tag<2>) SCN_NOEXCEPT
{
    return p;
}
template <typename Ptr>
constexpr auto to_address_impl(const Ptr& p, priority_tag<1>)
    SCN_NOEXCEPT->decltype(::scn::detail::pointer_traits<Ptr>::to_address(p))
{
    return ::scn::detail::pointer_traits<Ptr>::to_address(p);
}
template <typename Ptr>
constexpr auto to_address_impl(const Ptr& p, priority_tag<0>)
    SCN_NOEXCEPT->decltype(::scn::detail::to_address_impl(p.operator->(),
                                                          priority_tag<2>{}))
{
    return ::scn::detail::to_address_impl(p.operator->(), priority_tag<2>{});
}

template <typename Ptr>
constexpr auto to_address(Ptr&& p) SCN_NOEXCEPT
    ->decltype(::scn::detail::to_address_impl(SCN_FWD(p), priority_tag<2>{}))
{
    return ::scn::detail::to_address_impl(SCN_FWD(p), priority_tag<2>{});
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
