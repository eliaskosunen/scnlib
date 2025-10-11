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

// Includes <cassert>, <cstddef>, <cstdint>, and <type_traits>
#include <scn/fwd.h>

#include "scan.h"

#if defined(SCN_MODULE) && defined(SCN_IMPORT_STD)
import std;
#else
#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>

#if SCN_HAS_STD_F16 || SCN_HAS_STD_F32 || SCN_HAS_STD_F64 || \
    SCN_HAS_STD_F128 || SCN_HAS_STD_BF16
#include <stdfloat>
#endif
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////
// Metaprogramming facilities
/////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
using integer_type_for_char =
    std::conditional_t<std::is_signed_v<T>, int, unsigned>;

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
template <auto A>
using mp_value = std::integral_constant<decltype(A), A>;

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
struct mp_eval_if_c_impl<false, T, F, U...> : mp_defer<F, U...> {};

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

// mp_cond
template <class C, class T, class... E>
struct mp_cond_impl;

template <class C, class T, class... E>
using mp_cond = typename mp_cond_impl<C, T, E...>::type;

template <class C, class T, class... E>
using mp_cond_ = mp_eval_if<C, T, mp_cond, E...>;

template <class C, class T, class... E>
struct mp_cond_impl : mp_defer<mp_cond_, C, T, E...> {};

/////////////////////////////////////////////////////////////////
// pointer_traits and to_address
/////////////////////////////////////////////////////////////////

template <typename Ptr, typename>
struct pointer_traits {};

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

    SCN_FORCE_INLINE static constexpr pointer to_address(pointer p) noexcept
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
    SCN_FORCE_INLINE static constexpr auto to_address(
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
    SCN_FORCE_INLINE static constexpr auto to_address(
        const __gnu_cxx::__normal_iterator<Elem*, Container>& it) noexcept
    {
        return it.base();
    }
};
#endif
#if SCN_STDLIB_LIBCPP
template <typename Elem>
struct wrapped_pointer_iterator<std::__wrap_iter<Elem*>> {
    SCN_FORCE_INLINE static constexpr auto to_address(
        const std::__wrap_iter<Elem*>& it) noexcept
    {
        return it.base();
    }
};
#endif

template <typename I>
using apply_deref = decltype(*SCN_DECLVAL(I&));
template <typename I>
using apply_incr = decltype(++SCN_DECLVAL(I&));
template <typename I>
using apply_member_unwrapped = decltype(SCN_DECLVAL(I&)._Unwrapped());
template <typename It>
struct wrapped_pointer_iterator<
    It,
    std::enable_if_t<mp_valid_v<apply_deref, It> &&
                     mp_valid_v<apply_incr, It> &&
                     mp_valid_v<apply_member_unwrapped, It>>> {
    SCN_FORCE_INLINE static constexpr auto to_address(const It& it) noexcept
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
    SCN_FORCE_INLINE static constexpr auto to_address(
        const Iterator& it) noexcept
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
SCN_FORCE_INLINE constexpr T* to_address_impl(T* p, priority_tag<2>) noexcept
{
    return p;
}
template <typename Ptr>
SCN_FORCE_INLINE constexpr auto to_address_impl(const Ptr& p,
                                                priority_tag<1>) noexcept
    -> decltype(::scn::detail::pointer_traits<Ptr>::to_address(p))
{
    return ::scn::detail::pointer_traits<Ptr>::to_address(p);
}
template <typename Ptr>
SCN_FORCE_INLINE constexpr auto to_address_impl(const Ptr& p,
                                                priority_tag<0>) noexcept
    -> decltype(::scn::detail::to_address_impl(p.operator->(),
                                               priority_tag<2>{}))
{
    return ::scn::detail::to_address_impl(p.operator->(), priority_tag<2>{});
}

template <typename Ptr>
SCN_FORCE_INLINE constexpr auto to_address(Ptr&& p) noexcept
    -> decltype(::scn::detail::to_address_impl(SCN_FWD(p), priority_tag<2>{}))
{
    return ::scn::detail::to_address_impl(SCN_FWD(p), priority_tag<2>{});
}

}  // namespace detail

/////////////////////////////////////////////////////////////////
// <expected> implementation
/////////////////////////////////////////////////////////////////

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")

// The following implementation of expected is based on TartanLlama/expected,
// but is heavily modified.
//
// The original source is here:
//     https://github.com/TartanLlama/expected
// which is licensed under CC0 (Public Domain).

/**
 * \see `std::unexpected`
 */
template <typename E>
class SCN_TRIVIAL_ABI unexpected {
    static_assert(std::is_destructible_v<E>);

public:
    unexpected() = delete;

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wmaybe-uninitialized")

    template <
        typename Err = E,
        typename = std::enable_if_t<!std::is_same_v<Err, unexpected> &&
                                    !std::is_same_v<Err, std::in_place_t> &&
                                    std::is_constructible_v<E, Err>>>
    explicit constexpr unexpected(Err&& e) noexcept(
        std::is_nothrow_constructible_v<E, Err>)
        : m_unexpected(std::forward<Err>(e))
    {
        SCN_UNLIKELY_ATTR SCN_UNUSED(m_unexpected);
    }

    SCN_GCC_POP

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<E, Args...>>>
    explicit constexpr unexpected(std::in_place_t, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<E, Args...>)
        : m_unexpected(std::forward<Args>(args)...)
    {
        SCN_UNLIKELY_ATTR SCN_UNUSED(m_unexpected);
    }

    SCN_NODISCARD constexpr E& error() & noexcept
    {
        return m_unexpected;
    }
    SCN_NODISCARD constexpr const E& error() const& noexcept
    {
        return m_unexpected;
    }

    SCN_NODISCARD constexpr E&& error() && noexcept
    {
        return std::move(m_unexpected);
    }
    SCN_NODISCARD constexpr const E&& error() const&& noexcept
    {
        return std::move(m_unexpected);
    }

private:
    E m_unexpected;
};

template <typename E>
unexpected(E) -> unexpected<E>;

struct unexpect_t {};
inline constexpr unexpect_t unexpect{};

namespace detail {
template <typename T, typename... Args>
T* construct_at(T* p, Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args...>)
{
    return ::new (const_cast<void*>(static_cast<const volatile void*>(p)))
        T(std::forward<Args>(args)...);
}
template <typename T>
void destroy_at(T* p) noexcept
{
    if constexpr (std::is_array_v<T>) {
        for (auto& elem : *p) {
            scn::detail::destroy_at(&elem);
        }
    }
    else {
        p->~T();
    }
}

struct deferred_init_tag_t {};
inline constexpr deferred_init_tag_t deferred_init_tag{};

template <typename T,
          typename E,
          bool IsTriviallyDestructible =
              (std::is_void_v<T> || std::is_trivially_destructible_v<T>) &&
              std::is_trivially_destructible_v<E>>
struct expected_storage_base;

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_storage_base<T, E, true> {
    constexpr expected_storage_base() noexcept(
        std::is_nothrow_default_constructible_v<T>)
        : m_value(T{}), m_has_value(true)
    {
    }

    constexpr explicit expected_storage_base(deferred_init_tag_t) noexcept
        : m_deferred_init(), m_has_value(false)
    {
    }

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
    constexpr explicit expected_storage_base(
        std::in_place_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : m_value(std::forward<Args>(args)...), m_has_value(true)
    {
    }

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<E, Args...>>>
    constexpr explicit expected_storage_base(
        unexpect_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
        : m_unexpected(std::in_place, std::forward<Args>(args)...),
          m_has_value(false)
    {
    }

    SCN_NODISCARD constexpr T& get_value() & noexcept
    {
        return m_value;
    }
    SCN_NODISCARD constexpr const T& get_value() const& noexcept
    {
        return m_value;
    }
    SCN_NODISCARD constexpr T&& get_value() && noexcept
    {
        return std::move(m_value);
    }
    SCN_NODISCARD constexpr const T&& get_value() const&& noexcept
    {
        return std::move(m_value);
    }

    SCN_NODISCARD constexpr unexpected<E>& get_unexpected() & noexcept
    {
        return m_unexpected;
    }
    SCN_NODISCARD constexpr const unexpected<E>& get_unexpected()
        const& noexcept
    {
        return m_unexpected;
    }
    SCN_NODISCARD constexpr unexpected<E>&& get_unexpected() && noexcept
    {
        return std::move(m_unexpected);
    }
    SCN_NODISCARD constexpr const unexpected<E>&& get_unexpected()
        const&& noexcept
    {
        return std::move(m_unexpected);
    }

    SCN_NODISCARD constexpr bool has_value() const noexcept
    {
        return m_has_value;
    }

    template <typename... Args>
    void construct(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
    {
        scn::detail::construct_at(&get_value(), std::forward<Args>(args)...);
        m_has_value = true;
    }
    template <typename... Args>
    void construct_unexpected(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<E, Args...>)
    {
        scn::detail::construct_at(&get_unexpected(),
                                  std::forward<Args>(args)...);
        m_has_value = false;
    }

    // No-op, because T and E are trivially destructible
    static constexpr void destroy_value() noexcept {}
    static constexpr void destroy_unexpected() noexcept {}

private:
    union {
        T m_value;
        unexpected<E> m_unexpected;
        deferred_init_tag_t m_deferred_init;
    };
    bool m_has_value;
};

template <typename E>
struct SCN_TRIVIAL_ABI expected_storage_base<void, E, true> {
#if SCN_STD >= SCN_STD_20
    constexpr expected_storage_base() noexcept : m_has_value(true) {}
#else
    constexpr expected_storage_base() noexcept
        : m_deferred_init(), m_has_value(true)
    {
    }
#endif

    explicit constexpr expected_storage_base(deferred_init_tag_t) noexcept
        : m_deferred_init(), m_has_value(false)
    {
    }

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<E, Args...>>>
    explicit constexpr expected_storage_base(
        unexpect_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
        : m_unexpected(std::in_place, std::forward<Args>(args)...),
          m_has_value(false)
    {
    }

    SCN_NODISCARD constexpr unexpected<E>& get_unexpected() & noexcept
    {
        return m_unexpected;
    }
    SCN_NODISCARD constexpr const unexpected<E>& get_unexpected()
        const& noexcept
    {
        return m_unexpected;
    }
    SCN_NODISCARD constexpr unexpected<E>&& get_unexpected() && noexcept
    {
        return std::move(m_unexpected);
    }
    SCN_NODISCARD constexpr const unexpected<E>&& get_unexpected()
        const&& noexcept
    {
        return std::move(m_unexpected);
    }

    SCN_NODISCARD constexpr bool has_value() const noexcept
    {
        return m_has_value;
    }

    template <typename... Args>
    void construct(Args&&...) noexcept
    {
        m_has_value = true;
    }
    template <typename... Args>
    void construct_unexpected(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<E, Args...>)
    {
        scn::detail::construct_at(&get_unexpected(),
                                  std::forward<Args>(args)...);
        m_has_value = false;
    }

    static constexpr void destroy_value() noexcept {}
    static constexpr void destroy_unexpected() noexcept {}

private:
    union {
        unexpected<E> m_unexpected;
        deferred_init_tag_t m_deferred_init;
    };
    bool m_has_value;
};

/**
 * Implementation of `std::max` without including `<algorithm>`
 */
template <typename T>
constexpr T max(T a, T b) noexcept
{
    return (a < b) ? b : a;
}

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_storage_base<T, E, false> {
    constexpr expected_storage_base() noexcept : m_has_value(true)
    {
        construct();
    }

    explicit constexpr expected_storage_base(deferred_init_tag_t) noexcept
        : m_has_value(false)
    {
    }

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
    explicit constexpr expected_storage_base(
        std::in_place_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : m_has_value(true)
    {
        construct(std::forward<Args>(args)...);
    }

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<E, Args...>>>
    explicit constexpr expected_storage_base(
        unexpect_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
        : m_has_value(false)
    {
        construct_unexpected(std::in_place, std::forward<Args>(args)...);
    }

    ~expected_storage_base()
    {
        if (has_value()) {
            destroy_value();
        }
        else {
            destroy_unexpected();
        }
    }

    SCN_NODISCARD constexpr T& get_value() & noexcept
    {
        return *value_ptr();
    }
    SCN_NODISCARD constexpr const T& get_value() const& noexcept
    {
        return *value_ptr();
    }
    SCN_NODISCARD constexpr T&& get_value() && noexcept
    {
        return std::move(*value_ptr());
    }
    SCN_NODISCARD constexpr const T&& get_value() const&& noexcept
    {
        return std::move(*value_ptr());
    }

    SCN_NODISCARD constexpr unexpected<E>& get_unexpected() & noexcept
    {
        return *unexpected_ptr();
    }
    SCN_NODISCARD constexpr const unexpected<E>& get_unexpected()
        const& noexcept
    {
        return *unexpected_ptr();
    }
    SCN_NODISCARD constexpr unexpected<E>&& get_unexpected() && noexcept
    {
        return std::move(*unexpected_ptr());
    }
    SCN_NODISCARD constexpr const unexpected<E>&& get_unexpected()
        const&& noexcept
    {
        return std::move(*unexpected_ptr());
    }

    SCN_NODISCARD constexpr bool has_value() const noexcept
    {
        return m_has_value;
    }

    template <typename... Args>
    void construct(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
    {
        scn::detail::construct_at(value_ptr(), std::forward<Args>(args)...);
        m_has_value = true;
    }
    template <typename... Args>
    void construct_unexpected(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<E, Args...>)
    {
        scn::detail::construct_at(unexpected_ptr(),
                                  std::forward<Args>(args)...);
        m_has_value = false;
    }

    void destroy_value() noexcept
    {
        scn::detail::destroy_at(value_ptr());
    }
    void destroy_unexpected() noexcept
    {
        scn::detail::destroy_at(unexpected_ptr());
    }

private:
    SCN_NODISCARD T* value_ptr() noexcept
    {
        return reinterpret_cast<T*>(SCN_ASSUME_ALIGNED(m_memory, alignof(T)));
    }
    SCN_NODISCARD const T* value_ptr() const noexcept
    {
        return reinterpret_cast<const T*>(
            SCN_ASSUME_ALIGNED(m_memory, alignof(T)));
    }

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wcast-align")
    SCN_NODISCARD unexpected<E>* unexpected_ptr() noexcept
    {
        return reinterpret_cast<unexpected<E>*>(
            SCN_ASSUME_ALIGNED(m_memory, alignof(unexpected<E>)));
    }
    SCN_NODISCARD const unexpected<E>* unexpected_ptr() const noexcept
    {
        return reinterpret_cast<const unexpected<E>*>(
            SCN_ASSUME_ALIGNED(m_memory, alignof(unexpected<E>)));
    }
    SCN_GCC_POP

    static constexpr std::size_t required_size =
        detail::max(sizeof(T), sizeof(unexpected<E>));
    static constexpr std::size_t required_alignment =
        detail::max(alignof(T), alignof(unexpected<E>));

    alignas(required_alignment) unsigned char m_memory[required_size];
    bool m_has_value;
};

template <typename E>
struct SCN_TRIVIAL_ABI expected_storage_base<void, E, false> {
    constexpr expected_storage_base() noexcept : m_has_value(true) {}

    explicit constexpr expected_storage_base(deferred_init_tag_t) noexcept
        : m_has_value(false)
    {
    }

    explicit constexpr expected_storage_base(std::in_place_t) noexcept
        : m_has_value(true)
    {
    }

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<E, Args...>>>
    explicit constexpr expected_storage_base(
        unexpect_t,
        Args&&... args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
        : m_has_value(false)
    {
        construct_unexpected(std::in_place, std::forward<Args>(args)...);
    }

    ~expected_storage_base()
    {
        if (!has_value()) {
            destroy_unexpected();
        }
    }

    SCN_NODISCARD constexpr unexpected<E>& get_unexpected() & noexcept
    {
        return *unexpected_ptr();
    }
    SCN_NODISCARD constexpr const unexpected<E>& get_unexpected()
        const& noexcept
    {
        return *unexpected_ptr();
    }
    SCN_NODISCARD constexpr unexpected<E>&& get_unexpected() && noexcept
    {
        return std::move(*unexpected_ptr());
    }
    SCN_NODISCARD constexpr const unexpected<E>&& get_unexpected()
        const&& noexcept
    {
        return std::move(*unexpected_ptr());
    }

    SCN_NODISCARD constexpr bool has_value() const noexcept
    {
        return m_has_value;
    }

    template <typename... Args>
    constexpr void construct(Args&&...) noexcept
    {
        m_has_value = true;
    }
    template <typename... Args>
    void construct_unexpected(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<E, Args...>)
    {
        scn::detail::construct_at(unexpected_ptr(),
                                  std::forward<Args>(args)...);
        m_has_value = false;
    }

    static constexpr void destroy_value() noexcept {}
    void destroy_unexpected() noexcept
    {
        scn::detail::destroy_at(unexpected_ptr());
    }

private:
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wcast-align")
    SCN_NODISCARD unexpected<E>* unexpected_ptr() noexcept
    {
        return reinterpret_cast<unexpected<E>*>(
            SCN_ASSUME_ALIGNED(m_memory, alignof(unexpected<E>)));
    }
    SCN_NODISCARD const unexpected<E>* unexpected_ptr() const noexcept
    {
        return reinterpret_cast<const unexpected<E>*>(
            SCN_ASSUME_ALIGNED(m_memory, alignof(unexpected<E>)));
    }
    SCN_GCC_POP

    static constexpr std::size_t required_size = sizeof(unexpected<E>);
    static constexpr std::size_t required_alignment = alignof(unexpected<E>);

    alignas(required_alignment) unsigned char m_memory[required_size];
    bool m_has_value;
};

template <typename T, typename U>
using is_void_or = std::conditional_t<std::is_void_v<T>, std::true_type, U>;

template <typename T, typename E, typename Enable = void>
struct expected_operations_base;

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_operations_base<
    T,
    E,
    std::enable_if_t<(std::is_void_v<T> || std::is_trivially_copyable_v<T>) &&
                     std::is_trivially_copyable_v<E>>>
    : expected_storage_base<T, E> {
    using expected_storage_base<T, E>::expected_storage_base;
};

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_operations_base<
    T,
    E,
    std::enable_if_t<!std::is_void_v<T> && (!std::is_trivially_copyable_v<T> ||
                                            !std::is_trivially_copyable_v<E>)>>
    : expected_storage_base<T, E> {
    using expected_storage_base<T, E>::expected_storage_base;

    expected_operations_base(const expected_operations_base& other) noexcept(
        std::is_nothrow_copy_constructible_v<T> &&
        std::is_nothrow_copy_constructible_v<E>)
        : expected_storage_base<T, E>(deferred_init_tag)
    {
        construct_common(other);
    }
    expected_operations_base(expected_operations_base&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> &&
        std::is_nothrow_move_constructible_v<E>)
        : expected_storage_base<T, E>(deferred_init_tag)
    {
        construct_common(std::move(other));
    }

    expected_operations_base& operator=(const expected_operations_base& other)
    // gcc 11 and lower evaluate noexcept in a weird context
#if SCN_GCC && SCN_GCC < SCN_COMPILER(12, 0, 0)
        noexcept(noexcept(
            SCN_DECLVAL(expected_operations_base&).assign_common(other)))
#else
        noexcept(noexcept(assign_common(other)))
#endif
    {
        assign_common(other);
        return *this;
    }
    expected_operations_base& operator=(expected_operations_base&& other)
#if SCN_GCC && SCN_GCC < SCN_COMPILER(12, 0, 0)
        noexcept(noexcept(SCN_DECLVAL(expected_operations_base&)
                              .assign_common(std::move(other))))
#else
        noexcept(noexcept(assign_common(std::move(other))))
#endif
    {
        assign_common(std::move(other));
        return *this;
    }

    ~expected_operations_base() = default;

private:
    template <typename Other>
    void construct_common(Other&& other) noexcept(
        noexcept(SCN_DECLVAL(expected_storage_base<T, E>)
                     .construct(std::forward<Other>(other).get_value())) &&
        noexcept(SCN_DECLVAL(expected_storage_base<T, E>)
                     .construct_unexpected(
                         std::forward<Other>(other).get_unexpected())))
    {
        if (other.has_value()) {
            this->construct(std::forward<Other>(other).get_value());
        }
        else {
            this->construct_unexpected(
                std::forward<Other>(other).get_unexpected());
        }
    }

    template <typename Other>
    void assign_common(Other&& other)
#if SCN_GCC && SCN_GCC < SCN_COMPILER(12, 0, 0)
        noexcept(
            noexcept(SCN_DECLVAL(expected_operations_base&)
                         .reassign_value(std::forward<Other>(other))) &&
            noexcept(SCN_DECLVAL(expected_operations_base&)
                         .reassign_unexpected(std::forward<Other>(other))) &&
            noexcept(SCN_DECLVAL(expected_operations_base&)
                         .assign_value_over_unexpected(
                             std::forward<Other>(other))) &&
            noexcept(
                SCN_DECLVAL(expected_operations_base&)
                    .assign_unexpected_over_value(std::forward<Other>(other))))
#else
        noexcept(
            noexcept(reassign_value(std::forward<Other>(other))) &&
            noexcept(reassign_unexpected(std::forward<Other>(other))) &&
            noexcept(
                assign_value_over_unexpected(std::forward<Other>(other))) &&
            noexcept(assign_unexpected_over_value(std::forward<Other>(other))))
#endif
    {
        if (this->has_value()) {
            if (other.has_value()) {
                return reassign_value(std::forward<Other>(other));
            }
            return assign_unexpected_over_value(std::forward<Other>(other));
        }

        if (other.has_value()) {
            return assign_value_over_unexpected(std::forward<Other>(other));
        }
        return reassign_unexpected(std::forward<Other>(other));
    }

    template <typename Other>
    void reassign_value(Other&& other) noexcept(
        std::is_nothrow_assignable_v<
            T,
            decltype(std::forward<Other>(other).get_value())>)
    {
        this->get_value() = std::forward<Other>(other).get_value();
    }

    template <typename Other>
    void reassign_unexpected(Other&& other) noexcept(
        std::is_nothrow_assignable_v<
            E,
            decltype(std::forward<Other>(other).get_unexpected())>)
    {
        this->get_unexpected() = std::forward<Other>(other).get_unexpected();
    }

#if SCN_HAS_EXCEPTIONS
    void assign_value_over_unexpected(
        const expected_operations_base&
            other) noexcept(std::is_nothrow_copy_constructible_v<T> ||
                            std::is_nothrow_move_constructible_v<T>)
    {
        if constexpr (std::is_nothrow_copy_constructible_v<T>) {
            this->destroy_unexpected();
            this->construct(other.get_value());
        }
        else if constexpr (std::is_nothrow_move_constructible_v<T>) {
            T tmp = other.get_value();
            this->destroy_unexpected();
            this->construct(std::move(tmp));
        }
        else {
            auto tmp = std::move(this->get_unexpecetd());
            this->destroy_unexpected();

            try {
                this->construct(other.get());
            }
            catch (...) {
                this->construct_unexpected(std::move(tmp));
                throw;
            }
        }
    }

    void
    assign_value_over_unexpected(expected_operations_base&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>)
    {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            this->destroy_unexpected();
            this->construct(std::move(other).get_value());
        }
        else {
            auto tmp = std::move(this->get_unexpected());
            this->destroy_unexpected();

            try {
                this->construct(std::move(other).get_value());
            }
            catch (...) {
                this->construct_unexpected(std::move(tmp));
                throw;
            }
        }
    }
#else
    template <typename Other>
    void assing_value_over_unexpected(Other&& other) noexcept
    {
        this->destroy_unexpected();
        this->construct_value(std::forward<Other>(other).get_value());
    }
#endif

    template <typename Other>
    void assign_unexpected_over_value(Other&& other) noexcept(
        std::is_nothrow_constructible_v<
            E,
            decltype(std::forward<Other>(other).get_unexpected())>)
    {
        this->destroy_value();
        this->construct_unexpected(std::forward<Other>(other).get_unexpected());
    }
};

template <typename E>
struct SCN_TRIVIAL_ABI
    expected_operations_base<void,
                             E,
                             std::enable_if_t<!std::is_trivially_copyable_v<E>>>
    : expected_storage_base<void, E> {
    using expected_storage_base<void, E>::expected_storage_base;

    expected_operations_base(const expected_operations_base& other) noexcept(
        std::is_nothrow_copy_constructible_v<E>)
        : expected_storage_base<void, E>(deferred_init_tag)
    {
        construct_common(other);
    }
    expected_operations_base(expected_operations_base&& other) noexcept(
        std::is_nothrow_move_constructible_v<E>)
        : expected_storage_base<void, E>(deferred_init_tag)
    {
        construct_common(std::move(other));
    }

    expected_operations_base& operator=(const expected_operations_base& other)
    // gcc 11 and lower evaluate noexcept in a weird context
#if SCN_GCC && SCN_GCC < SCN_COMPILER(12, 0, 0)
        noexcept(noexcept(
            SCN_DECLVAL(expected_operations_base&).assign_common(other)))
#else
        noexcept(noexcept(assign_common(other)))
#endif
    {
        assign_common(other);
        return *this;
    }
    expected_operations_base& operator=(expected_operations_base&& other)
#if SCN_GCC && SCN_GCC < SCN_COMPILER(12, 0, 0)
        noexcept(noexcept(SCN_DECLVAL(expected_operations_base&)
                              .assign_common(std::move(other))))
#else
        noexcept(noexcept(assign_common(std::move(other))))
#endif
    {
        assign_common(std::move(other));
        return *this;
    }

    ~expected_operations_base() = default;

private:
    template <typename Other>
    void construct_common(Other&& other) noexcept(
        noexcept(expected_storage_base<void, E>::construct_unexpected(
            std::forward<Other>(other).get_unexpected())))
    {
        if (other.has_value()) {
            this->construct();
        }
        else {
            this->construct_unexpected(
                std::forward<Other>(other).get_unexpected());
        }
    }

    template <typename Other>
    void assign_common(Other&& other)
#if SCN_GCC && SCN_GCC < SCN_COMPILER(12, 0, 0)
        noexcept(
            noexcept(SCN_DECLVAL(expected_operations_base&)
                         .reassign_unexpected(std::forward<Other>(other))) &&
            noexcept(
                SCN_DECLVAL(expected_operations_base&)
                    .assign_unexpected_over_value(std::forward<Other>(other))))
#else
        noexcept(
            noexcept(reassign_unexpected(std::forward<Other>(other))) &&
            noexcept(assign_unexpected_over_value(std::forward<Other>(other))))
#endif
    {
        if (this->has_value()) {
            if (other.has_value()) {
                return reassign_value();
            }
            return assign_unexpected_over_value(std::forward<Other>(other));
        }

        if (other.has_value()) {
            return assign_value_over_unexpected();
        }
        return reassign_unexpected(std::forward<Other>(other));
    }

    void reassign_value() noexcept {}

    template <typename Other>
    void reassign_unexpected(Other&& other) noexcept(
        std::is_nothrow_assignable_v<
            E,
            decltype(std::forward<Other>(other).get_unexpected())>)
    {
        this->get_unexpected() = std::forward<Other>(other).get_unexpected();
    }

    void assign_value_over_unexpected() noexcept
    {
        this->destroy_unexpected();
        this->construct();
    }

    template <typename Other>
    void assign_unexpected_over_value(Other&& other) noexcept(
        std::is_nothrow_constructible_v<
            E,
            decltype(std::forward<Other>(other).get_unexpected())>)
    {
        this->destroy_value();
        this->construct_unexpected(std::forward<Other>(other).get_unexpected());
    }
};

/*
 * Base class trickery to conditionally mark copy and move
 * constructors of an expected as =deleted.
 *
 * We need to do this, because otherwise utilities like
 * std::is_copy_constructible wouldn't work for expected: the
 * constructors need to be explicitly =deleted, not just cause a
 * compiler error when trying to copy a value of a non-copyable
 * type.
 *
 * Rationale for doing this with base classes is above.
 */
template <
    typename T,
    typename E,
    bool EnableCopy = ((std::is_copy_constructible_v<T> || std::is_void_v<T>) &&
                       std::is_copy_constructible_v<E>),
    bool EnableMove = ((std::is_move_constructible_v<T> || std::is_void_v<T>) &&
                       std::is_move_constructible_v<E>)>
struct expected_delete_ctor_base;

// Implementation for types that are both copy and move
// constructible: Copy and move constructors are =defaulted
template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, true, true> {
    expected_delete_ctor_base() = default;
    expected_delete_ctor_base& operator=(const expected_delete_ctor_base&) =
        default;
    expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) = default;
    ~expected_delete_ctor_base() = default;

    expected_delete_ctor_base(const expected_delete_ctor_base&) = default;
    expected_delete_ctor_base(expected_delete_ctor_base&&) = default;
};

// Implementation for types that are neither copy nor move
// constructible: Copy and move constructors are =deleted
template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, false, false> {
    expected_delete_ctor_base() = default;
    expected_delete_ctor_base& operator=(const expected_delete_ctor_base&) =
        default;
    expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) = default;
    ~expected_delete_ctor_base() = default;

    expected_delete_ctor_base(const expected_delete_ctor_base&) = delete;
    expected_delete_ctor_base(expected_delete_ctor_base&&) = delete;
};

// Implementation for types that are move constructible, but not
// copy constructible Copy constructor is =deleted, but move
// constructor is =defaulted
template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, false, true> {
    expected_delete_ctor_base() = default;
    expected_delete_ctor_base& operator=(const expected_delete_ctor_base&) =
        default;
    expected_delete_ctor_base& operator=(expected_delete_ctor_base&&) = default;
    ~expected_delete_ctor_base() = default;

    expected_delete_ctor_base(const expected_delete_ctor_base&) = delete;
    expected_delete_ctor_base(expected_delete_ctor_base&&) = default;
};

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_ctor_base<T, E, true, false> {
    static_assert(dependent_false<T>::value,
                  "Nonsensical type: copy constructible, but not move "
                  "constructible");
};

// Same as above, but for assignment
template <
    typename T,
    typename E,
    bool EnableCopy = ((std::is_copy_constructible_v<T> || std::is_void_v<T>) &&
                       std::is_copy_constructible_v<E> &&
                       (std::is_copy_assignable_v<T> || std::is_void_v<T>) &&
                       std::is_copy_assignable_v<E>),
    bool EnableMove = ((std::is_move_constructible_v<T> || std::is_void_v<T>) &&
                       std::is_move_constructible_v<E> &&
                       (std::is_move_assignable_v<T> || std::is_void_v<T>) &&
                       std::is_move_assignable_v<E>)>
struct expected_delete_assign_base;

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, true, true> {
    expected_delete_assign_base() = default;
    expected_delete_assign_base(const expected_delete_assign_base&) = default;
    expected_delete_assign_base(expected_delete_assign_base&&) = default;
    ~expected_delete_assign_base() = default;

    expected_delete_assign_base& operator=(const expected_delete_assign_base&) =
        default;
    expected_delete_assign_base& operator=(expected_delete_assign_base&&) =
        default;
};

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, false, false> {
    expected_delete_assign_base() = default;
    expected_delete_assign_base(const expected_delete_assign_base&) = default;
    expected_delete_assign_base(expected_delete_assign_base&&) = default;
    ~expected_delete_assign_base() = default;

    expected_delete_assign_base& operator=(const expected_delete_assign_base&) =
        delete;
    expected_delete_assign_base& operator=(expected_delete_assign_base&&) =
        delete;
};

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, false, true> {
    expected_delete_assign_base() = default;
    expected_delete_assign_base(const expected_delete_assign_base&) = default;
    expected_delete_assign_base(expected_delete_assign_base&&) = default;
    ~expected_delete_assign_base() = default;

    expected_delete_assign_base& operator=(const expected_delete_assign_base&) =
        delete;
    expected_delete_assign_base& operator=(expected_delete_assign_base&&) =
        default;
};

template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_delete_assign_base<T, E, true, false> {
    static_assert(dependent_false<T>::value,
                  "Nonsensical type: copy assignable, but not move "
                  "assignable");
};

struct non_default_ctor_tag_t {};

/*
 * Same as above, but for the default constructor
 *
 * The constructor taking a non_default_ctor_tag_t is needed, to
 * signal that we're not default constructing.
 */
template <typename T,
          typename E,
          bool = std::is_default_constructible_v<T> || std::is_void_v<T>>
struct SCN_TRIVIAL_ABI expected_default_ctor_base {
    constexpr expected_default_ctor_base() = default;
    constexpr explicit expected_default_ctor_base(
        non_default_ctor_tag_t) noexcept
    {
    }
};
template <typename T, typename E>
struct SCN_TRIVIAL_ABI expected_default_ctor_base<T, E, false> {
    constexpr expected_default_ctor_base() = delete;
    constexpr explicit expected_default_ctor_base(
        non_default_ctor_tag_t) noexcept
    {
    }
};

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
struct is_expected_impl : std::false_type {};
template <typename T, typename E>
struct is_expected_impl<expected<T, E>> : std::true_type {};
template <typename T>
using is_expected = is_expected_impl<remove_cvref_t<T>>;

template <typename Exp>
using is_exp_void = std::is_void<typename remove_cvref_t<Exp>::value_type>;

template <typename Exp>
using expected_value_type = typename remove_cvref_t<Exp>::value_type;

template <
    typename F,
    typename... Args,
    typename = std::void_t<decltype(SCN_DECLVAL(F&&)(SCN_DECLVAL(Args&&)...))>>
constexpr decltype(auto) trivial_invoke(F&& f, Args&&... args) noexcept(
    noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
{
    return std::forward<F>(f)(std::forward<Args>(args)...);
}

// and_then

template <typename Exp,
          typename F,
          std::enable_if_t<!is_exp_void<Exp>::value>* = nullptr,
          typename Ret = decltype(trivial_invoke(SCN_DECLVAL(F),
                                                 *SCN_DECLVAL(Exp)))>
constexpr auto and_then_impl(Exp&& exp, F&& f) noexcept(
    noexcept(trivial_invoke(std::forward<F>(f), *std::forward<Exp>(exp))) &&
    std::is_nothrow_constructible_v<Ret,
                                    unexpect_t,
                                    decltype(std::forward<Exp>(exp).error())>)
{
    static_assert(is_expected<Ret>::value, "F must return an expected");

    return SCN_LIKELY(exp.has_value())
               ? trivial_invoke(std::forward<F>(f), *std::forward<Exp>(exp))
               : Ret(unexpect, std::forward<Exp>(exp).error());
}
template <typename Exp,
          typename F,
          std::enable_if_t<is_exp_void<Exp>::value>* = nullptr,
          typename Ret = decltype(trivial_invoke(SCN_DECLVAL(F)))>
constexpr auto and_then_impl(Exp&& exp, F&& f) noexcept(
    noexcept(trivial_invoke(std::forward<F>(f))) &&
    std::is_nothrow_constructible_v<Ret,
                                    unexpect_t,
                                    decltype(std::forward<Exp>(exp).error())>)
{
    static_assert(is_expected<Ret>::value, "F must return an expected");

    return SCN_LIKELY(exp.has_value())
               ? trivial_invoke(std::forward<F>(f))
               : Ret(unexpect, std::forward<Exp>(exp).error());
}

// or_else

template <typename Exp,
          typename F,
          typename Ret = decltype(trivial_invoke(SCN_DECLVAL(F),
                                                 SCN_DECLVAL(Exp).error()))>
constexpr auto or_else_impl(Exp&& exp, F&& f) noexcept(
    noexcept(trivial_invoke(std::forward<F>(f),
                            std::forward<Exp>(exp).error())) &&
    std::is_nothrow_constructible_v<Ret, decltype(std::forward<Exp>(exp))>)
{
    static_assert(is_expected<Ret>::value, "F must return an expected");

    return SCN_LIKELY(exp.has_value())
               ? Ret(std::forward<Exp>(exp))
               : trivial_invoke(std::forward<F>(f),
                                std::forward<Exp>(exp).error());
}

// transform

template <typename Exp,
          typename F,
          std::enable_if_t<!is_exp_void<Exp>::value>* = nullptr,
          typename Ret = decltype(trivial_invoke(SCN_DECLVAL(F),
                                                 *SCN_DECLVAL(Exp)))>
constexpr auto transform_impl(Exp&& exp, F&& f)
{
    using result = typename remove_cvref_t<Exp>::template rebind<Ret>;
    if constexpr (std::is_void_v<Ret>) {
        if (SCN_LIKELY(exp.has_value())) {
            trivial_invoke(std::forward<F>(f), *std::forward<Exp>(exp));
            return result();
        }
        return result(unexpect, std::forward<Exp>(exp).error());
    }
    else {
        return SCN_LIKELY(exp.has_value())
                   ? result(trivial_invoke(std::forward<F>(f),
                                           *std::forward<Exp>(exp)))
                   : result(unexpect, std::forward<Exp>(exp).error());
    }
}
template <typename Exp,
          typename F,
          std::enable_if_t<is_exp_void<Exp>::value>* = nullptr,
          typename Ret = decltype(trivial_invoke(SCN_DECLVAL(F)))>
constexpr auto transform_impl(Exp&& exp, F&& f)
{
    using result = typename remove_cvref_t<Exp>::template rebind<Ret>;
    if constexpr (std::is_void_v<Ret>) {
        if (SCN_LIKELY(exp.has_value())) {
            trivial_invoke(std::forward<F>(f));
            return result();
        }
        return result(unexpect, std::forward<Exp>(exp).error());
    }
    else {
        return SCN_LIKELY(exp.has_value())
                   ? result(trivial_invoke(std::forward<F>(f)))
                   : result(unexpect, std::forward<Exp>(exp).error());
    }
}

// transform_error

template <typename Exp,
          typename F,
          std::enable_if_t<!is_exp_void<Exp>::value>* = nullptr,
          typename Ret = decltype(trivial_invoke(SCN_DECLVAL(F),
                                                 SCN_DECLVAL(Exp).error()))>
constexpr auto transform_error_impl(Exp&& exp, F&& f)
{
    if constexpr (std::is_void_v<Ret>) {
        using result = expected<expected_value_type<Exp>, monostate>;
        if (SCN_LIKELY(exp.has_value())) {
            return result(*std::forward<Exp>(exp));
        }

        trivial_invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
        return result(unexpect, monostate{});
    }
    else {
        using result = expected<expected_value_type<Exp>, remove_cvref_t<Ret>>;
        return SCN_LIKELY(exp.has_value())
                   ? result(*std::forward<Exp>(exp))
                   : result(unexpect,
                            trivial_invoke(std::forward<F>(f),
                                           std::forward<Exp>(exp).error()));
    }
}
template <typename Exp,
          typename F,
          std::enable_if_t<is_exp_void<Exp>::value>* = nullptr,
          typename Ret = decltype(trivial_invoke(SCN_DECLVAL(F),
                                                 SCN_DECLVAL(Exp).error()))>
constexpr auto transform_error_impl(Exp&& exp, F&& f)
{
    if constexpr (std::is_void_v<Ret>) {
        using result = expected<expected_value_type<Exp>, monostate>;
        if (SCN_LIKELY(exp.has_value())) {
            return result();
        }

        trivial_invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
        return result(unexpect, monostate{});
    }
    else {
        using result = expected<expected_value_type<Exp>, remove_cvref_t<Ret>>;
        return SCN_LIKELY(exp.has_value())
                   ? result()
                   : result(unexpect,
                            trivial_invoke(std::forward<F>(f),
                                           std::forward<Exp>(exp).error()));
    }
}

template <class T, class E, class U, class G, class UR, class GR>
using enable_from_other =
    std::enable_if_t<std::is_constructible_v<T, UR> &&
                     std::is_constructible_v<E, GR> &&
                     !std::is_constructible_v<T, expected<U, G>&> &&
                     !std::is_constructible_v<T, expected<U, G>&&> &&
                     !std::is_constructible_v<T, const expected<U, G>&> &&
                     !std::is_constructible_v<T, const expected<U, G>&&> &&
                     !std::is_convertible_v<expected<U, G>&, T> &&
                     !std::is_convertible_v<expected<U, G>&&, T> &&
                     !std::is_convertible_v<const expected<U, G>&, T> &&
                     !std::is_convertible_v<const expected<U, G>&&, T>>;
}  // namespace detail

/**
 * \see `std::expected`
 */
template <typename T, typename E>
class SCN_TRIVIAL_ABI expected
    : private detail::expected_operations_base<T, E>,
      private detail::expected_delete_ctor_base<T, E>,
      private detail::expected_delete_assign_base<T, E>,
      private detail::expected_default_ctor_base<T, E> {
    using base = detail::expected_operations_base<T, E>;
    using ctor_base = detail::expected_default_ctor_base<T, E>;

    static_assert(std::is_void_v<T> || std::is_destructible_v<T>,
                  "T must be void or Destructible");
    static_assert(std::is_destructible_v<E>, "E must be Destructible");

    static_assert(!std::is_same_v<std::remove_cv_t<T>, std::in_place_t>);
    static_assert(!std::is_same_v<std::remove_cv_t<T>, unexpect_t>);
    static_assert(!std::is_same_v<std::remove_cv_t<T>, unexpected<E>>);

public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = unexpected<E>;

    template <typename U>
    using rebind = expected<U, error_type>;

    // Special member functions are defaulted, implementations provided
    // by base classes

    constexpr expected() = default;

    constexpr expected(const expected&) = default;
    constexpr expected(expected&&) = default;
    constexpr expected& operator=(const expected&) = default;
    constexpr expected& operator=(expected&&) = default;

    ~expected() = default;

    /**
     * Construct an expected value.
     * Intentionally non-explicit, to make constructing an expected
     * value as transparent as possible.
     */
    template <typename U = value_type,
              typename = std::enable_if_t<std::is_convertible_v<U, value_type>>>
    SCN_IMPLICIT constexpr expected(U&& val) noexcept(
        std::is_nothrow_constructible_v<base, std::in_place_t, U&&>)
        : base(std::in_place, std::forward<U>(val)),
          ctor_base(detail::non_default_ctor_tag_t{})
    {
    }

    /// Construct an expected value directly in-place
    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
    explicit constexpr expected(std::in_place_t, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<base, std::in_place_t, Args&&...>)
        : base(std::in_place, std::forward<Args>(args)...),
          ctor_base(detail::non_default_ctor_tag_t{})
    {
    }

    template <typename G = E,
              std::enable_if_t<std::is_constructible_v<E, const G&>>* = nullptr,
              std::enable_if_t<!std::is_convertible_v<const G&, E>>* = nullptr>
    explicit constexpr expected(const unexpected<G>& e) noexcept(
        std::is_nothrow_constructible_v<base, unexpect_t, const G&>)
        : base(unexpect, e.error()), ctor_base(detail::non_default_ctor_tag_t{})
    {
    }
    template <typename G = E,
              std::enable_if_t<std::is_constructible_v<E, const G&>>* = nullptr,
              std::enable_if_t<std::is_convertible_v<const G&, E>>* = nullptr>
    SCN_IMPLICIT constexpr expected(const unexpected<G>& e) noexcept(
        std::is_nothrow_constructible_v<base, unexpect_t, const G&>)
        : base(unexpect, e.error()), ctor_base(detail::non_default_ctor_tag_t{})
    {
    }

    template <typename G = E,
              std::enable_if_t<std::is_constructible_v<E, G&&>>* = nullptr,
              std::enable_if_t<!std::is_convertible_v<G&&, E>>* = nullptr>
    explicit constexpr expected(unexpected<G>&& e) noexcept(
        std::is_nothrow_constructible_v<base, unexpect_t, G&&>)
        : base(unexpect, std::move(e.error())),
          ctor_base(detail::non_default_ctor_tag_t{})
    {
    }
    template <typename G = E,
              std::enable_if_t<std::is_constructible_v<E, G&&>>* = nullptr,
              std::enable_if_t<std::is_convertible_v<G&&, E>>* = nullptr>
    SCN_IMPLICIT constexpr expected(unexpected<G>&& e) noexcept(
        std::is_nothrow_constructible_v<base, unexpect_t, G&&>)
        : base(unexpect, std::move(e.error())),
          ctor_base(detail::non_default_ctor_tag_t{})
    {
    }

    /// Construct an unexpected value directly in-place
    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<E, Args...>>>
    explicit constexpr expected(unexpect_t, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<base, unexpect_t, Args&&...>)
        : base(unexpect, std::forward<Args>(args)...),
          ctor_base(detail::non_default_ctor_tag_t{})
    {
    }

    template <
        typename U,
        typename G,
        std::enable_if_t<!(std::is_convertible_v<const U&, T> &&
                           std::is_convertible_v<const G&, E>)>* = nullptr,
        detail::enable_from_other<T, E, U, G, const U&, const G&>* = nullptr>
    explicit constexpr expected(const expected<U, G>& other) noexcept(
        std::is_nothrow_constructible_v<T, const U&> &&
        std::is_nothrow_constructible_v<E, const G&>)
        : base(), ctor_base(detail::non_default_ctor_tag_t{})
    {
        if (other.has_value()) {
            this->construct(*other);
        }
        else {
            this->construct_unexpected(other.error());
        }
    }
    template <
        typename U,
        typename G,
        std::enable_if_t<(std::is_convertible_v<const U&, T> &&
                          std::is_convertible_v<const G&, E>)>* = nullptr,
        detail::enable_from_other<T, E, U, G, const U&, const G&>* = nullptr>
    constexpr expected(const expected<U, G>& other) noexcept(
        std::is_nothrow_constructible_v<T, const U&> &&
        std::is_nothrow_constructible_v<E, const G&>)
        : base(), ctor_base(detail::non_default_ctor_tag_t{})
    {
        if (other.has_value()) {
            this->construct(*other);
        }
        else {
            this->construct_unexpected(other.error());
        }
    }

    template <typename U,
              typename G,
              std::enable_if_t<!(std::is_convertible_v<U&&, T> &&
                                 std::is_convertible_v<G&&, E>)>* = nullptr,
              detail::enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
    explicit constexpr expected(expected<U, G>&& other) noexcept(
        std::is_nothrow_constructible_v<T, U&&> &&
        std::is_nothrow_constructible_v<E, G&&>)
        : base(), ctor_base(detail::non_default_ctor_tag_t{})
    {
        if (other.has_value()) {
            this->construct(std::move(*other));
        }
        else {
            this->construct_unexpected(std::move(other.error()));
        }
    }
    template <typename U,
              typename G,
              std::enable_if_t<(std::is_convertible_v<U&&, T> &&
                                std::is_convertible_v<G&&, E>)>* = nullptr,
              detail::enable_from_other<T, E, U, G, U&&, G&&>* = nullptr>
    constexpr expected(expected<U, G>&& other) noexcept(
        std::is_nothrow_constructible_v<T, U&&> &&
        std::is_nothrow_constructible_v<E, G&&>)
        : base(), ctor_base(detail::non_default_ctor_tag_t{})
    {
        if (other.has_value()) {
            this->construct(std::move(*other));
        }
        else {
            this->construct_unexpected(std::move(other.error()));
        }
    }

    template <typename U = value_type,
              typename = std::enable_if_t<std::is_convertible_v<U, value_type>>>
    expected& operator=(U&& val) noexcept(
        noexcept(assign_value(std::forward<U>(val))))
    {
        assign_value(std::forward<U>(val));
        return *this;
    }

    expected& operator=(const unexpected_type& unex) noexcept(
        noexcept(assign_unexpected(unex)))
    {
        assign_unexpected(unex);
        return *this;
    }
    expected& operator=(unexpected_type&& unex) noexcept(
        noexcept(assign_unexpected(std::move(unex))))
    {
        assign_unexpected(std::move(unex));
        return *this;
    }

    /// Destroys the contained value, and then initializes the expected
    /// value directly in-place.
    template <typename... Args,
              std::enable_if_t<std::is_constructible_v<T, Args...>>* = nullptr>
    decltype(auto) emplace(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
    {
        emplace_impl(std::forward<Args>(args)...);
        if constexpr (!std::is_void_v<T>) {
            return this->get_value();
        }
    }

    using base::has_value;
    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    /// Get the unexpected value, if one is contained in *this
    SCN_NODISCARD constexpr error_type& error() & noexcept
    {
        SCN_EXPECT(!has_value());
        return this->get_unexpected().error();
    }
    SCN_NODISCARD constexpr const error_type& error() const& noexcept
    {
        SCN_EXPECT(!has_value());
        return this->get_unexpected().error();
    }
    SCN_NODISCARD constexpr error_type&& error() && noexcept
    {
        SCN_EXPECT(!has_value());
        return std::move(this->get_unexpected().error());
    }
    SCN_NODISCARD constexpr const error_type&& error() const&& noexcept
    {
        SCN_EXPECT(!has_value());
        return std::move(this->get_unexpected().error());
    }

    /// Get the expected value, if one is contained in *this
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr U& value() & noexcept
    {
        SCN_EXPECT(has_value());
        return this->get_value();
    }
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr const U& value() const& noexcept
    {
        SCN_EXPECT(has_value());
        return this->get_value();
    }
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr U&& value() && noexcept
    {
        SCN_EXPECT(has_value());
        return std::move(this->get_value());
    }
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr const U&& value() const&& noexcept
    {
        SCN_EXPECT(has_value());
        return std::move(this->get_value());
    }

    /// Get the expected value, if one is contained in *this
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr U& operator*() & noexcept
    {
        return value();
    }
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr const U& operator*() const& noexcept
    {
        return value();
    }
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr U&& operator*() && noexcept
    {
        return std::move(value());
    }
    template <typename U = T, std::enable_if_t<!std::is_void_v<U>>* = nullptr>
    SCN_NODISCARD constexpr const U&& operator*() const&& noexcept
    {
        return std::move(value());
    }

    SCN_NODISCARD constexpr value_type* operator->() noexcept
    {
        return &value();
    }
    SCN_NODISCARD constexpr const value_type* operator->() const noexcept
    {
        return &value();
    }

    /// Returns the expected value if *this contains one, otherwise
    /// returns default_value
    template <typename U,
              typename = std::enable_if_t<std::is_copy_constructible_v<T> &&
                                          std::is_convertible_v<U, T>>>
    SCN_NODISCARD constexpr T value_or(U&& default_value) const& noexcept(
        std::is_nothrow_copy_constructible_v<T> &&
        std::is_nothrow_constructible_v<T, U&&>)
    {
        if (has_value()) {
            return value();
        }
        return std::forward<U>(default_value);
    }
    template <typename U,
              typename = std::enable_if_t<std::is_move_constructible_v<T> &&
                                          std::is_convertible_v<U, T>>>
    SCN_NODISCARD constexpr T value_or(U&& default_value) && noexcept(
        std::is_nothrow_move_constructible_v<T> &&
        std::is_nothrow_constructible_v<T, U&&>)
    {
        if (has_value()) {
            return std::move(value());
        }
        return std::forward<U>(default_value);
    }

    template <typename G,
              typename = std::enable_if_t<std::is_copy_constructible_v<E> &&
                                          std::is_convertible_v<G, E>>>
    SCN_NODISCARD constexpr E error_or(G&& default_error) const& noexcept(
        std::is_nothrow_copy_constructible_v<E> &&
        std::is_nothrow_constructible_v<E, G&&>)
    {
        if (!has_value()) {
            return error();
        }
        return std::forward<G>(default_error);
    }
    template <typename G,
              typename = std::enable_if_t<std::is_move_constructible_v<E> &&
                                          std::is_convertible_v<G, E>>>
    SCN_NODISCARD constexpr E error_or(G&& default_error) && noexcept(
        std::is_nothrow_move_constructible_v<E> &&
        std::is_nothrow_constructible_v<E, G&&>)
    {
        if (!has_value()) {
            return std::move(error());
        }
        return std::forward<G>(default_error);
    }

    template <typename F>
    constexpr auto and_then(F&& f) & noexcept(noexcept(
        detail::and_then_impl(SCN_DECLVAL(expected&), std::forward<F>(f))))
        -> decltype(detail::and_then_impl(*this, std::forward<F>(f)))
    {
        return detail::and_then_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto and_then(F&& f) const& noexcept(
        noexcept(detail::and_then_impl(SCN_DECLVAL(const expected&),
                                       std::forward<F>(f))))
        -> decltype(detail::and_then_impl(*this, std::forward<F>(f)))
    {
        return detail::and_then_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto and_then(F&& f) && noexcept(noexcept(
        detail::and_then_impl(SCN_DECLVAL(expected&&), std::forward<F>(f))))
        -> decltype(detail::and_then_impl(*this, std::forward<F>(f)))
    {
        return detail::and_then_impl(std::move(*this), std::forward<F>(f));
    }
    template <typename F>
    constexpr auto and_then(F&& f) const&& noexcept(
        noexcept(detail::and_then_impl(SCN_DECLVAL(const expected&&),
                                       std::forward<F>(f))))
        -> decltype(detail::and_then_impl(*this, std::forward<F>(f)))
    {
        return detail::and_then_impl(std::move(*this), std::forward<F>(f));
    }

    template <typename F>
    constexpr auto or_else(F&& f) & noexcept(noexcept(
        detail::or_else_impl(SCN_DECLVAL(expected&), std::forward<F>(f))))
        -> decltype(detail::or_else_impl(*this, std::forward<F>(f)))
    {
        return detail::or_else_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto or_else(F&& f) const& noexcept(noexcept(
        detail::or_else_impl(SCN_DECLVAL(const expected&), std::forward<F>(f))))
        -> decltype(detail::or_else_impl(*this, std::forward<F>(f)))
    {
        return detail::or_else_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto or_else(F&& f) && noexcept(noexcept(
        detail::or_else_impl(SCN_DECLVAL(expected&&), std::forward<F>(f))))
        -> decltype(detail::or_else_impl(*this, std::forward<F>(f)))
    {
        return detail::or_else_impl(std::move(*this), std::forward<F>(f));
    }
    template <typename F>
    constexpr auto or_else(F&& f) const&& noexcept(
        noexcept(detail::or_else_impl(SCN_DECLVAL(const expected&&),
                                      std::forward<F>(f))))
        -> decltype(detail::or_else_impl(*this, std::forward<F>(f)))
    {
        return detail::or_else_impl(std::move(*this), std::forward<F>(f));
    }

    template <typename F>
    constexpr auto transform(
        F&& f) & -> decltype(detail::transform_impl(*this, std::forward<F>(f)))
    {
        return detail::transform_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto transform(F&& f)
        const& -> decltype(detail::transform_impl(*this, std::forward<F>(f)))
    {
        return detail::transform_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto transform(
        F&& f) && -> decltype(detail::transform_impl(*this, std::forward<F>(f)))
    {
        return detail::transform_impl(std::move(*this), std::forward<F>(f));
    }
    template <typename F>
    constexpr auto transform(F&& f)
        const&& -> decltype(detail::transform_impl(*this, std::forward<F>(f)))
    {
        return detail::transform_impl(std::move(*this), std::forward<F>(f));
    }

    template <typename F>
    constexpr auto transform_error(
        F&& f) & -> decltype(detail::transform_error_impl(*this,
                                                          std::forward<F>(f)))
    {
        return detail::transform_error_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto transform_error(F&& f)
        const& -> decltype(detail::transform_error_impl(*this,
                                                        std::forward<F>(f)))
    {
        return detail::transform_error_impl(*this, std::forward<F>(f));
    }
    template <typename F>
    constexpr auto transform_error(
        F&& f) && -> decltype(detail::transform_error_impl(*this,
                                                           std::forward<F>(f)))
    {
        return detail::transform_error_impl(std::move(*this),
                                            std::forward<F>(f));
    }
    template <typename F>
    constexpr auto transform_error(F&& f)
        const&& -> decltype(detail::transform_error_impl(*this,
                                                         std::forward<F>(f)))
    {
        return detail::transform_error_impl(std::move(*this),
                                            std::forward<F>(f));
    }

private:
    template <typename... Args>
    void emplace_impl(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args&&...>)
    {
        if (this->has_value()) {
            this->destroy_value();
            this->construct(std::forward<Args>(args)...);
        }
        else {
#if SCN_HAS_EXCEPTIONS
            if constexpr (std::is_nothrow_constructible_v<T, Args&&...>) {
                auto tmp = std::move(error());
                this->destroy_unexpected();

                try {
                    this->construct(std::forward<Args>(args)...);
                }
                catch (...) {
                    this->construct_unexpected(std::move(tmp));
                    throw;
                }
            }
            else {
                this->construct(std::forward<Args>(args)...);
            }
#else
            this->destroy_unexpected();
            this->construct(std::forward<Args>(args)...);
#endif
        }
    }

    template <typename Value>
    void assign_value(Value&& val) noexcept(
        std::is_nothrow_constructible_v<detail::remove_cvref_t<Value>,
                                        Value&&> &&
        std::is_nothrow_assignable_v<T, Value&&>)
    {
        if (has_value()) {
            this->get_value() = std::forward<Value>(val);
            return;
        }

#if SCN_HAS_EXCEPTIONS
        if constexpr (std::is_nothrow_constructible_v<T, Value&&>) {
            this->destroy_unexpected();
            this->construct(std::forward<Value>(val));
        }
        else {
            auto tmp = std::move(this->get_unexpected());
            this->destroy_unexpected();

            try {
                this->construct(std::forward<Value>(val));
            }
            catch (...) {
                this->construct_unexpected(std::move(tmp));
            }
        }
#else
        this->destroy_unexpected();
        this->construct(std::forward<Value>(val));
#endif
    }

    template <typename Unexpected>
    void assign_unexpected(Unexpected&& unex) noexcept(
        std::is_nothrow_constructible_v<E, Unexpected&&> &&
        std::is_nothrow_assignable_v<E, Unexpected&&>)
    {
        if (!has_value()) {
            this->get_unexpected() = std::forward<Unexpected>(unex);
            return;
        }

        this->destroy_value();
        this->construct_unexpected(std::forward<Unexpected>(unex));
    }
};

/////////////////////////////////////////////////////////////////
// <ranges> implementation
/////////////////////////////////////////////////////////////////

/**
 * Contains a very minimal `<ranges>` implementation.
 *
 * This is a heavily stripped-down and adapted version of NanoRange:
 * https://github.com/tcbrindle/NanoRange.
 *
 * NanoRange is provided under the Boost license.
 * Copyright (c) 2018 Tristan Brindle (tcbrindle at gmail dot com)
 */
namespace ranges {
namespace detail {
using namespace scn::detail;

template <typename T>
constexpr auto decay_copy(T&& t) noexcept(
    noexcept(static_cast<std::decay_t<T>>(SCN_FWD(t)))) -> std::decay_t<T>
{
    return SCN_FWD(t);
}

template <bool>
struct conditional {
    template <typename T, typename>
    using type = T;
};

template <>
struct conditional<false> {
    template <typename, typename U>
    using type = U;
};

template <bool B, typename T, typename U>
using conditional_t = typename conditional<B>::template type<T, U>;

template <template <class...> class AliasT, typename... Args>
auto exists_helper(long) -> std::false_type;

template <template <class...> class AliasT,
          typename... Args,
          typename = AliasT<Args...>>
auto exists_helper(int) -> std::true_type;

template <template <class...> class AliasT, typename... Args>
inline constexpr bool exists_v =
    decltype(exists_helper<AliasT, Args...>(0))::value;

template <typename, typename...>
auto test_requires_fn(long) -> std::false_type;

template <typename R,
          typename... Args,
          typename = decltype(&R::template requires_<Args...>)>
auto test_requires_fn(int) -> std::true_type;

template <typename R, typename... Args>
inline constexpr bool requires_ =
    decltype(test_requires_fn<R, Args...>(0))::value;

template <bool Expr>
using requires_expr = std::enable_if_t<Expr, int>;
}  // namespace detail

template <typename>
inline constexpr bool enable_borrowed_range = false;

namespace detail {
template <typename T>
inline constexpr bool boolean_testable_impl = std::is_convertible_v<T, bool>;

struct boolean_testable_concept {
    template <typename T>
    auto requires_(T&& t)
        -> requires_expr<boolean_testable_impl<decltype(!std::forward<T>(t))>>;
};

template <typename T>
inline constexpr bool boolean_testable =
    boolean_testable_impl<T> && detail::requires_<boolean_testable_concept, T>;
}  // namespace detail

namespace detail {
struct weakly_equality_comparable_with_concept {
    template <typename T, typename U>
    auto requires_(const std::remove_reference_t<T>& t,
                   const std::remove_reference_t<U>& u)
        -> decltype(requires_expr<boolean_testable<decltype(t == u)>>{},
                    requires_expr<boolean_testable<decltype(t != u)>>{},
                    requires_expr<boolean_testable<decltype(u == t)>>{},
                    requires_expr<boolean_testable<decltype(u != t)>>{});
};

template <typename T, typename U>
inline constexpr bool weakly_equality_comparable_with =
    requires_<weakly_equality_comparable_with_concept, T, U>;
}  // namespace detail

template <typename T>
inline constexpr bool equality_comparable =
    detail::weakly_equality_comparable_with<T, T>;

namespace detail {
struct equality_comparable_with_concept {
    template <typename, typename>
    static auto test(long) -> std::false_type;

    template <typename T, typename U>
    static auto test(int)
        -> std::enable_if_t<equality_comparable<T> && equality_comparable<U> &&
#if 0
            common_reference_with<const std::remove_reference_t<T>&,
                                  const std::remove_reference_t<U>&> &&
            equality_comparable<
                common_reference_t<const std::remove_reference_t<T>&,
                                   const std::remove_reference_t<U>&>> &&
#endif
                                weakly_equality_comparable_with<T, U>,
                            std::true_type>;
};
}  // namespace detail

template <typename T, typename U>
inline constexpr bool equality_comparable_with =
    decltype(detail::equality_comparable_with_concept::test<T, U>(0))::value;

namespace detail {
struct partially_ordered_with_concept {
    template <typename T, typename U>
    auto requires_(const std::remove_reference_t<T>& t,
                   const std::remove_reference_t<U>& u)
        -> decltype(requires_expr<boolean_testable<decltype(t < u)>>{},
                    requires_expr<boolean_testable<decltype(t > u)>>{},
                    requires_expr<boolean_testable<decltype(t <= u)>>{},
                    requires_expr<boolean_testable<decltype(t >= u)>>{},
                    requires_expr<boolean_testable<decltype(u < t)>>{},
                    requires_expr<boolean_testable<decltype(u > t)>>{},
                    requires_expr<boolean_testable<decltype(u <= t)>>{},
                    requires_expr<boolean_testable<decltype(u >= t)>>{});
};

template <typename T, typename U>
inline constexpr bool partially_ordered_with =
    detail::requires_<detail::partially_ordered_with_concept, T, U>;
}  // namespace detail

template <typename T>
inline constexpr bool totally_ordered =
    equality_comparable<T> && detail::partially_ordered_with<T, T>;

namespace detail {
struct totally_ordered_with_concept {
    template <typename, typename>
    static auto test(long) -> std::false_type;

    template <typename T, typename U>
    static auto test(int)
        -> std::enable_if_t<totally_ordered<T> && totally_ordered<U> &&
                                equality_comparable_with<T, U> &&
#if 0
                                totally_ordered<common_reference_t<
                                    const std::remove_reference_t<T>&,
                                    const std::remove_reference_t<U>&>> &&
#endif
                                partially_ordered_with<T, U>,
                            std::true_type>;
};
}  // namespace detail

template <typename T, typename U>
inline constexpr bool totally_ordered_with =
    decltype(detail::totally_ordered_with_concept::test<T, U>(0))::value;

namespace detail {
struct movable_concept {
    template <typename T>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int) -> std::enable_if_t<
        std::is_object_v<T> && std::is_move_constructible_v<T> &&
            std::is_move_assignable_v<T> && std::is_swappable_v<T>,
        std::true_type>;
};
}  // namespace detail

template <typename T>
inline constexpr bool movable =
    decltype(detail::movable_concept::test<T>(0))::value;

namespace detail {
struct copyable_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int)
        -> std::enable_if_t<std::is_copy_constructible_v<T> && movable<T> &&
                                std::is_copy_assignable_v<T>,
                            std::true_type>;
};
}  // namespace detail

template <typename T>
inline constexpr bool copyable =
    decltype(detail::copyable_concept::test<T>(0))::value;

template <typename T>
inline constexpr bool semiregular =
    copyable<T> && std::is_default_constructible_v<T>;

template <typename T>
inline constexpr bool regular = semiregular<T> && equality_comparable<T>;

template <typename>
struct incrementable_traits;

namespace detail {
struct empty {};

template <typename T>
struct with_difference_type {
    using difference_type = T;
};

template <typename, typename = void>
struct incrementable_traits_helper {};

// Workaround for GCC silliness: void* has no difference_type
// FIXME: This is required to stop WeaklyIncrementable<void*> being a hard
// error Can we formulate the concept differently to avoid the need for this
// hack?
template <>
struct incrementable_traits_helper<void*> {};

template <typename T>
struct incrementable_traits_helper<T*>
    : detail::conditional_t<std::is_object_v<T>,
                            with_difference_type<std::ptrdiff_t>,
                            empty> {};

template <class I>
struct incrementable_traits_helper<const I>
    : incrementable_traits<std::decay_t<I>> {};

template <typename, typename = void>
struct has_member_difference_type : std::false_type {};

template <typename T>
struct has_member_difference_type<T, std::void_t<typename T::difference_type>>
    : std::true_type {};

template <typename T>
constexpr bool has_member_difference_type_v =
    has_member_difference_type<T>::value;

template <typename T>
struct incrementable_traits_helper<
    T,
    std::enable_if_t<has_member_difference_type_v<T>>> {
    using difference_type = typename T::difference_type;
};

template <typename T, typename = void>
struct subtraction_result_type {
    using type = void;
};
template <typename T>
struct subtraction_result_type<
    T,
    std::void_t<decltype(std::declval<const T&>() -
                         std::declval<const T&>())>> {
    using type = decltype(std::declval<const T&>() - std::declval<const T&>());
};

template <typename T>
inline constexpr bool enable_incrtraits_subtractable =
    !std::is_pointer_v<T> && !has_member_difference_type_v<T> &&
    std::is_integral_v<typename subtraction_result_type<T>::type>;

template <typename T>
struct incrementable_traits_helper<
    T,
    std::enable_if_t<enable_incrtraits_subtractable<T>>>
    : with_difference_type<
          std::make_signed_t<typename subtraction_result_type<T>::type>> {};
}  // namespace detail

template <typename T>
struct incrementable_traits : detail::incrementable_traits_helper<T> {};

template <typename T>
using iter_difference_t = typename incrementable_traits<T>::difference_type;

template <typename>
struct readable_traits;

namespace detail {
template <typename T>
struct with_value_type {
    using value_type = T;
};

template <typename, typename = void>
struct readable_traits_helper {};

template <typename T>
struct readable_traits_helper<T*>
    : detail::conditional_t<std::is_object_v<T>,
                            with_value_type<std::remove_cv_t<T>>,
                            empty> {};

template <typename I>
struct readable_traits_helper<I, std::enable_if_t<std::is_array_v<I>>>
    : readable_traits<std::decay_t<I>> {};

template <typename I>
struct readable_traits_helper<const I, std::enable_if_t<!std::is_array_v<I>>>
    : readable_traits<std::decay_t<I>> {};

template <typename T, typename V = typename T::value_type>
struct member_value_type
    : detail::conditional_t<std::is_object_v<V>, with_value_type<V>, empty> {};

template <typename T, typename E = typename T::element_type>
struct member_element_type
    : detail::conditional_t<std::is_object_v<E>,
                            with_value_type<std::remove_cv_t<E>>,
                            empty> {};

template <typename T>
using member_value_type_t = typename T::value_type;

template <typename T>
constexpr bool has_member_value_type_v = exists_v<member_value_type_t, T>;

template <typename T>
using member_element_type_t = typename T::element_type;

template <typename T>
constexpr bool has_member_element_type_v = exists_v<member_element_type_t, T>;

template <typename T>
struct readable_traits_helper<T,
                              std::enable_if_t<has_member_value_type_v<T> &&
                                               !has_member_element_type_v<T>>>
    : member_value_type<T> {};

template <typename T>
struct readable_traits_helper<T,
                              std::enable_if_t<has_member_element_type_v<T> &&
                                               !has_member_value_type_v<T>>>
    : member_element_type<T> {};

// A type which has both value_type and element_type members must specialise
// readable_traits to tell us which one to prefer -- see
// https://github.com/ericniebler/stl2/issues/562
template <typename T>
struct readable_traits_helper<T,
                              std::enable_if_t<has_member_element_type_v<T> &&
                                               has_member_value_type_v<T>>> {};
}  // namespace detail

template <typename T>
struct readable_traits : detail::readable_traits_helper<T> {};

template <typename T>
using iter_value_t = typename readable_traits<T>::value_type;

namespace detail {
template <typename T>
using with_reference = T&;

struct can_reference_concept {
    template <typename T>
    auto requires_() -> with_reference<T>;
};

template <typename T>
inline constexpr bool can_reference =
    detail::requires_<can_reference_concept, T>;

struct dereferencable_concept {
    template <typename T>
    auto requires_(T& t)
        -> decltype(requires_expr<can_reference<decltype(*t)>>{});
};

template <typename T>
inline constexpr bool dereferenceable = requires_<dereferencable_concept, T>;

// GCC and Clang allow dereferencing void* as an extension.
// Let's kill that off now.

template <>
inline constexpr bool dereferenceable<void*> = false;
}  // namespace detail

using std::bidirectional_iterator_tag;
using std::forward_iterator_tag;
using std::input_iterator_tag;
using std::output_iterator_tag;
using std::random_access_iterator_tag;

#if SCN_STD >= SCN_STD_20 && defined(__cpp_lib_ranges) && \
    __cpp_lib_ranges >= 201911L
using std::contiguous_iterator_tag;
#else
struct contiguous_iterator_tag : random_access_iterator_tag {};
#endif

template <typename T>
struct iterator_category;

namespace detail {
template <typename T, typename = void>
struct iterator_category_ {};
template <typename T>
struct iterator_category_<T*>
    : std::enable_if<std::is_object_v<T>, contiguous_iterator_tag> {};
template <typename T>
struct iterator_category_<const T> : iterator_category<T> {};
template <typename T>
struct iterator_category_<T, std::void_t<typename T::iterator_category>> {
    using type = typename T::iterator_category;
};
}  // namespace detail

template <typename T>
struct iterator_category : detail::iterator_category_<T> {};
template <typename T>
using iterator_category_t = typename iterator_category<T>::type;

namespace detail {

template <typename T, typename = void>
struct legacy_iterator_category : iterator_category<T> {};

template <typename T>
struct legacy_iterator_category<
    T,
    std::enable_if_t<
        std::is_same_v<iterator_category_t<T>, contiguous_iterator_tag>>> {
    using type = random_access_iterator_tag;
};

template <typename T>
using legacy_iterator_category_t = typename legacy_iterator_category<T>::type;

}  // namespace detail

template <typename T>
using iter_reference_t =
    std::enable_if_t<detail::dereferenceable<T>, decltype(*std::declval<T&>())>;

namespace detail {

struct readable_concept {
    template <typename In>
    auto requires_() -> decltype(std::declval<iter_value_t<In>>(),
                                 std::declval<iter_reference_t<In>>());
    // std::declval<iter_rvalue_reference_t<In>>());

    template <typename>
    static auto test(long) -> std::false_type;

    template <typename In>
    static auto test(int)
        -> std::enable_if_t<detail::requires_<readable_concept, In>,
#if 0
            common_reference_with<iter_reference_t<In>&&, iter_value_t<In>&> &&
            common_reference_with<iter_reference_t<In>&&,
                                  iter_rvalue_reference_t<In>&&> &&
            common_reference_with<iter_rvalue_reference_t<In>&&,
                                  const iter_value_t<In>&>,
#endif
                            std::true_type>;
};

}  // namespace detail

template <typename In>
inline constexpr bool readable =
    decltype(detail::readable_concept::test<In>(0))::value;

namespace detail {

struct writable_concept {
    template <typename Out, typename T>
    auto requires_(Out&& o, T&& t)
        -> decltype(*o = std::forward<T>(t),
                    *std::forward<Out>(o) = std::forward<T>(t),
                    const_cast<const iter_reference_t<Out>&&>(*o) =
                        std::forward<T>(t),
                    const_cast<const iter_reference_t<Out>&&>(
                        *std::forward<Out>(o)) = std::forward<T>(t));
};

}  // namespace detail

template <typename Out, typename T>
inline constexpr bool writable =
    detail::requires_<detail::writable_concept, Out, T>;

namespace detail {

template <typename T>
inline constexpr bool is_integer_like = std::is_integral_v<T>;

template <typename T>
inline constexpr bool is_signed_integer_like =
    std::is_integral_v<T> && std::is_signed_v<T>;

struct weakly_incrementable_concept {
    template <typename I>
    auto requires_(I i)
        -> decltype(std::declval<iter_difference_t<I>>(),
                    requires_expr<
                        is_signed_integer_like<iter_difference_t<I>>>{},
                    requires_expr<std::is_same_v<decltype(++i), I&>>{},
                    i++);
};

}  // namespace detail

template <typename I>
inline constexpr bool weakly_incrementable =
    /* std::is_default_constructible_v<I> && */
    movable<I> && detail::requires_<detail::weakly_incrementable_concept, I>;

namespace detail {

struct incrementable_concept {
    template <typename I>
    auto requires_(I i)
        -> decltype(requires_expr<std::is_same_v<decltype(i++), I>>{});
};

}  // namespace detail

template <typename I>
inline constexpr bool incrementable =
    regular<I> && weakly_incrementable<I> &&
    detail::requires_<detail::incrementable_concept, I>;

namespace detail {

struct input_or_output_iterator_concept {
    template <typename I>
    auto requires_(I i)
        -> decltype(requires_expr<can_reference<decltype(*i)>>{});
};

}  // namespace detail

template <typename I>
inline constexpr bool input_or_output_iterator =
    detail::requires_<detail::input_or_output_iterator_concept, I> &&
    weakly_incrementable<I>;

template <typename S, typename I>
inline constexpr bool sentinel_for =
    semiregular<S> && input_or_output_iterator<I> &&
    detail::weakly_equality_comparable_with<S, I>;

template <typename S, typename I>
inline constexpr bool disable_sized_sentinel = false;

namespace detail {

struct sized_sentinel_for_concept {
    template <typename S, typename I>
    auto requires_(const S& s, const I& i)
        -> decltype(requires_expr<std::is_same_v<decltype(s - i),
                                                 iter_difference_t<I>>>{},
                    requires_expr<std::is_same_v<decltype(i - s),
                                                 iter_difference_t<I>>>{});
};

}  // namespace detail

template <typename S, typename I>
inline constexpr bool sized_sentinel_for =
    sentinel_for<S, I> &&
    !disable_sized_sentinel<std::remove_cv_t<S>, std::remove_cv_t<I>> &&
    detail::requires_<detail::sized_sentinel_for_concept, S, I>;

// This is a hack, but I'm fed up with my tests breaking because GCC
// has a silly extension
template <typename S>
inline constexpr bool sized_sentinel_for<S, void*> = false;

template <typename I>
inline constexpr bool sized_sentinel_for<void*, I> = false;

template <>
inline constexpr bool sized_sentinel_for<void*, void*> = false;

namespace detail {

// FIXME: Use ITER_CONCEPT, not iterator_category_t
struct input_iterator_concept {
    template <typename I>
    auto requires_() -> iterator_category_t<I>;

    template <typename>
    static auto test(long) -> std::false_type;

    template <typename I>
    static auto test(int) -> std::enable_if_t<
        input_or_output_iterator<I> && readable<I> &&
            detail::requires_<input_iterator_concept, I> &&
            std::is_base_of_v<input_iterator_tag, iterator_category_t<I>>,
        std::true_type>;
};

}  // namespace detail

template <typename I>
inline constexpr bool input_iterator =
    decltype(detail::input_iterator_concept::test<I>(0))::value;

namespace detail {

struct output_iterator_concept {
    template <typename I, typename T>
    auto requires_(I i, T&& t) -> decltype(*i++ = std::forward<T>(t));
};

}  // namespace detail

template <typename I, typename T>
inline constexpr bool output_iterator =
    input_or_output_iterator<I> && writable<I, T> &&
    detail::requires_<detail::output_iterator_concept, I, T>;

namespace detail {

struct forward_iterator_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename I>
    static auto test(int) -> std::enable_if_t<
        input_iterator<I> &&
            std::is_base_of_v<forward_iterator_tag, iterator_category_t<I>> &&
            incrementable<I> && sentinel_for<I, I>,
        std::true_type>;
};

}  // namespace detail

template <typename I>
inline constexpr bool forward_iterator =
    decltype(detail::forward_iterator_concept::test<I>(0))::value;

namespace detail {

struct bidirectional_iterator_concept {
    template <typename I>
    auto requires_(I i)
        -> decltype(requires_expr<std::is_same_v<decltype(--i), I&>>{},
                    requires_expr<std::is_same_v<decltype(i--), I>>{});

    template <typename>
    static auto test(long) -> std::false_type;

    template <typename I>
    static auto test(int) -> std::enable_if_t<
        forward_iterator<I> &&
            std::is_base_of_v<bidirectional_iterator_tag,
                              iterator_category_t<I>> &&
            detail::requires_<bidirectional_iterator_concept, I>,
        std::true_type>;
};

}  // namespace detail

template <typename I>
inline constexpr bool bidirectional_iterator =
    decltype(detail::bidirectional_iterator_concept::test<I>(0))::value;

namespace detail {

struct random_access_iterator_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename I>
    static auto test(int) -> std::enable_if_t<
        bidirectional_iterator<I> &&
            std::is_base_of_v<random_access_iterator_tag,
                              iterator_category_t<I>> &&
            totally_ordered<I> && sized_sentinel_for<I, I> &&
            detail::requires_<random_access_iterator_concept, I>,
        std::true_type>;

    template <typename I>
    auto requires_(I i, const I j, const iter_difference_t<I> n)
        -> decltype(requires_expr<std::is_same_v<decltype(i += n), I&>>{},
                    requires_expr<std::is_same_v<decltype(j + n), I>>{},
#ifndef _MSC_VER
                    requires_expr<std::is_same_v<decltype(n + j),
                                                 I>>{},  // FIXME: MSVC doesn't
                                                         // like this when I =
                                                         // int*
#endif
                    requires_expr<std::is_same_v<decltype(i -= n), I&>>{},
                    requires_expr<std::is_same_v<decltype(j - n), I>>{},
                    requires_expr<
                        std::is_same_v<decltype(j[n]), iter_reference_t<I>>>{});
};

}  // namespace detail

template <typename I>
inline constexpr bool random_access_iterator =
    decltype(detail::random_access_iterator_concept::test<I>(0))::value;

namespace detail {

struct contiguous_iterator_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename I>
    static auto test(int) -> std::enable_if_t<
        random_access_iterator<I> &&
            /*std::is_base_of_v<contiguous_iterator_tag,
                              iterator_category_t<I>> &&*/
            detail::can_make_address_from_iterator<I> &&
            std::is_lvalue_reference_v<iter_reference_t<I>> &&
            std::is_same_v<iter_value_t<I>,
                           remove_cvref_t<iter_reference_t<I>>>,
        std::true_type>;
};

}  // namespace detail

template <typename I>
inline constexpr bool contiguous_iterator =
    decltype(detail::contiguous_iterator_concept::test<I>(0))::value;

namespace detail::begin_ {

template <typename T>
void begin(T&) = delete;
template <typename T>
void begin(const T&) = delete;

struct fn {
private:
    template <typename T,
              std::enable_if_t<!std::is_lvalue_reference_v<T> &&
                               !enable_borrowed_range<std::remove_cv_t<T>>>* =
                  nullptr>
    static constexpr void impl(T&&, priority_tag<3>) = delete;

    template <typename T,
              std::enable_if_t<std::is_array_v<remove_cvref_t<T>>>* = nullptr>
    static constexpr auto impl(T&& t, priority_tag<2>) noexcept
        -> decltype(t + 0)
    {
        static_assert(ranges::input_or_output_iterator<decltype(t + 0)>);
        return t + 0;
    }

    template <typename T>
    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
        noexcept(detail::decay_copy(SCN_FWD(t).begin())))
        -> std::enable_if_t<
            input_or_output_iterator<
                decltype(detail::decay_copy(SCN_FWD(t).begin()))>,
            decltype(detail::decay_copy(SCN_FWD(t).begin()))>
    {
        return detail::decay_copy(t.begin());
    }

    template <typename T>
    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
        noexcept(detail::decay_copy(begin(SCN_FWD(t)))))
        -> std::enable_if_t<
            input_or_output_iterator<
                decltype(detail::decay_copy(begin(SCN_FWD(t))))>,
            decltype(detail::decay_copy(begin(SCN_FWD(t))))>
    {
        return detail::decay_copy(begin(SCN_FWD(t)));
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t), priority_tag<3>{})))
            -> decltype(fn::impl(SCN_FWD(t), priority_tag<3>{}))
    {
        return fn::impl(SCN_FWD(t), priority_tag<3>{});
    }
};

}  // namespace detail::begin_

inline constexpr auto begin = detail::begin_::fn{};

namespace detail::end_ {

template <typename T>
void end(T&) = delete;
template <typename T>
void end(const T&) = delete;

struct fn {
private:
    template <typename T,
              std::enable_if_t<!std::is_lvalue_reference_v<T> &&
                               !enable_borrowed_range<std::remove_cv_t<T>>>* =
                  nullptr>
    static constexpr void impl(T&&, priority_tag<3>) = delete;

    template <typename T,
              std::enable_if_t<std::is_array_v<remove_cvref_t<T>>>* = nullptr>
    static constexpr auto impl(T&& t, priority_tag<2>) noexcept
        -> decltype(t + std::extent_v<remove_cvref_t<T>>)
    {
        return t + std::extent_v<remove_cvref_t<T>>;
    }

    template <typename T,
              typename S = decltype(detail::decay_copy(SCN_DECLVAL(T).end())),
              typename I = decltype(::scn::ranges::begin(SCN_DECLVAL(T)))>
    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
        noexcept(detail::decay_copy(SCN_FWD(t).end())))
        -> std::enable_if_t<sentinel_for<S, I>,
                            decltype(detail::decay_copy(SCN_FWD(t).end()))>
    {
        return detail::decay_copy(SCN_FWD(t).end());
    }

    template <typename T,
              typename S = decltype(detail::decay_copy(end(SCN_DECLVAL(T)))),
              typename I = decltype(::scn::ranges::begin(SCN_DECLVAL(T)))>
    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
        noexcept(detail::decay_copy(end(SCN_FWD(t)))))
        -> std::enable_if_t<sentinel_for<S, I>, S>
    {
        return detail::decay_copy(end(SCN_FWD(t)));
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t), priority_tag<3>{})))
            -> decltype(fn::impl(SCN_FWD(t), priority_tag<3>{}))
    {
        return fn::impl(SCN_FWD(t), priority_tag<3>{});
    }
};

}  // namespace detail::end_

inline constexpr auto end = detail::end_::fn{};

namespace detail {
struct range_concept {
    template <typename T>
    auto requires_(T& t) -> decltype(ranges::begin(t), ranges::end(t));
};
}  // namespace detail

template <typename T>
inline constexpr bool range = detail::requires_<detail::range_concept, T>;

template <typename T>
using iterator_t = decltype(ranges::begin(std::declval<T&>()));

template <typename R>
using sentinel_t =
    std::enable_if_t<range<R>, decltype(ranges::end(std::declval<R&>()))>;

template <typename R>
using range_difference_t =
    std::enable_if_t<range<R>, iter_difference_t<iterator_t<R>>>;

template <typename R>
using range_value_t = std::enable_if_t<range<R>, iter_value_t<iterator_t<R>>>;

template <typename R>
using range_reference_t =
    std::enable_if_t<range<R>, iter_reference_t<iterator_t<R>>>;

namespace detail {
template <typename, typename = void>
inline constexpr bool is_object_pointer_v = false;
template <typename P>
inline constexpr bool
    is_object_pointer_v<P,
                        std::enable_if_t<std::is_pointer_v<P> &&
                                         std::is_object_v<iter_value_t<P>>>> =
        true;

namespace data_ {

struct fn {
private:
    template <typename T,
              typename D = decltype(detail::decay_copy(SCN_DECLVAL(T&).data()))>
    static constexpr auto impl(T& t, priority_tag<1>) noexcept(
        noexcept(detail::decay_copy(t.data())))
        -> std::enable_if_t<is_object_pointer_v<D>, D>
    {
        return detail::decay_copy(t.data());
    }

    template <typename T>
    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
        noexcept(scn::ranges::begin(SCN_FWD(t))))
        -> std::enable_if_t<
            is_object_pointer_v<decltype(scn::ranges::begin(SCN_FWD(t)))>,
            decltype(scn::ranges::begin(SCN_FWD(t)))>
    {
        return scn::ranges::begin(SCN_FWD(t));
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t), priority_tag<1>{})))
            -> decltype(fn::impl(SCN_FWD(t), priority_tag<1>{}))
    {
        return fn::impl(SCN_FWD(t), priority_tag<1>{});
    }
};

}  // namespace data_
}  // namespace detail

inline constexpr auto data = detail::data_::fn{};

template <typename T>
inline constexpr bool disable_sized_range = false;

namespace detail::size_ {

template <typename T>
void size(T&&) = delete;
template <typename T>
void size(T&) = delete;

struct fn {
private:
    template <typename T, std::size_t N>
    static constexpr std::size_t impl(const T (&&)[N], priority_tag<3>) noexcept
    {
        return N;
    }

    template <typename T, std::size_t N>
    static constexpr std::size_t impl(const T (&)[N], priority_tag<3>) noexcept
    {
        return N;
    }

    template <
        typename T,
        typename I = decltype(detail::decay_copy(std::declval<T>().size()))>
    static constexpr auto impl(T&& t, priority_tag<2>) noexcept(
        noexcept(detail::decay_copy(SCN_FWD(t).size())))
        -> std::enable_if_t<std::is_integral_v<I> &&
                                !disable_sized_range<remove_cvref_t<T>>,
                            I>
    {
        return detail::decay_copy(SCN_FWD(t).size());
    }

    template <
        typename T,
        typename I = decltype(detail::decay_copy(size(std::declval<T>())))>
    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
        noexcept(detail::decay_copy(size(SCN_FWD(t)))))
        -> std::enable_if_t<std::is_integral_v<I> &&
                                !disable_sized_range<remove_cvref_t<T>>,
                            I>
    {
        return detail::decay_copy(size(SCN_FWD(t)));
    }

    template <typename T,
              typename I = decltype(scn::ranges::begin(SCN_DECLVAL(T))),
              typename S = decltype(scn::ranges::end(SCN_DECLVAL(T))),
              typename D = decltype(static_cast<std::size_t>(SCN_DECLVAL(S) -
                                                             SCN_DECLVAL(I)))>
    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(noexcept(
        static_cast<size_t>(scn::ranges::end(t) - scn::ranges::begin(t))))
        -> std::enable_if_t<!std::is_array_v<remove_cvref_t<T>> &&
                                sized_sentinel_for<S, I> && forward_iterator<I>,
                            D>
    {
        return static_cast<size_t>(scn::ranges::end(t) - scn::ranges::begin(t));
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(std::forward<T>(t), priority_tag<3>{})))
            -> decltype(fn::impl(std::forward<T>(t), priority_tag<3>{}))
    {
        return fn::impl(std::forward<T>(t), priority_tag<3>{});
    }
};

}  // namespace detail::size_

inline constexpr auto size = detail::size_::fn{};

namespace detail {
namespace ssize_ {
struct fn {
private:
    template <typename T>
    using ssize_return_t = std::conditional_t<sizeof(range_difference_t<T>) <
                                                  sizeof(std::ptrdiff_t),
                                              std::ptrdiff_t,
                                              range_difference_t<T>>;

    template <typename T>
    static constexpr auto impl(T&& t) noexcept(
        noexcept(ranges::size(std::forward<T>(t))))
        -> decltype(ranges::size(std::forward<T>(t)), ssize_return_t<T>())
    {
        return static_cast<ssize_return_t<T>>(ranges::size(std::forward<T>(t)));
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(std::forward<T>(t))))
            -> decltype(fn::impl(std::forward<T>(t)))
    {
        return fn::impl(std::forward<T>(t));
    }
};
}  // namespace ssize_
}  // namespace detail

inline constexpr auto ssize = detail::ssize_::fn{};

namespace detail::empty_ {
struct fn {
private:
    template <typename T>
    static constexpr auto impl(T&& t, priority_tag<2>) noexcept(
        noexcept(static_cast<bool>(SCN_FWD(t).empty())))
        -> decltype(static_cast<bool>(SCN_FWD(t).empty()))
    {
        return static_cast<bool>(SCN_FWD(t).empty());
    }

    template <typename T>
    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
        noexcept(ranges::size(SCN_FWD(t)) == 0))
        -> decltype(ranges::size(SCN_FWD(t)) == 0)
    {
        return ranges::size(SCN_FWD(t)) == 0;
    }

    template <typename T,
              typename I = decltype(ranges::begin(std::declval<T>()))>
    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
        noexcept(ranges::begin(t) == ranges::end(t)))
        -> std::enable_if_t<forward_iterator<I>,
                            decltype(ranges::begin(t) == ranges::end(t))>
    {
        return ranges::begin(t) == ranges::end(t);
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t), priority_tag<2>{})))
            -> decltype(fn::impl(SCN_FWD(t), priority_tag<2>{}))
    {
        return fn::impl(SCN_FWD(t), priority_tag<2>{});
    }
};
}  // namespace detail::empty_

inline constexpr auto empty = detail::empty_::fn{};

/**
 * \see `std::ranges::borrowed_range`
 */
template <typename T>
inline constexpr bool borrowed_range =
    range<T> && (std::is_lvalue_reference_v<T> ||
                 enable_borrowed_range<detail::remove_cvref_t<T>>);

namespace detail {
struct sized_range_concept {
    template <typename T>
    auto requires_(T& t) -> decltype(ranges::size(t));
};
}  // namespace detail

template <typename T>
inline constexpr bool sized_range =
    range<T> && !disable_sized_range<detail::remove_cvref_t<T>> &&
    detail::requires_<detail::sized_range_concept, T>;

namespace detail {
struct output_range_concept {
    template <typename, typename>
    static auto test(long) -> std::false_type;

    template <typename R, typename T>
    static auto test(int)
        -> std::enable_if_t<range<R> && output_iterator<iterator_t<R>, T>,
                            std::true_type>;
};
}  // namespace detail

template <typename R, typename T>
inline constexpr bool output_range =
    decltype(detail::output_range_concept::test<R, T>(0))::value;

namespace detail {
struct input_range_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int)
        -> std::enable_if_t<range<T> && input_iterator<iterator_t<T>>,
                            std::true_type>;
};
}  // namespace detail

template <typename T>
inline constexpr bool input_range =
    decltype(detail::input_range_concept::test<T>(0))::value;

namespace detail {
struct forward_range_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int)
        -> std::enable_if_t<input_range<T> && forward_iterator<iterator_t<T>>,
                            std::true_type>;
};
}  // namespace detail

template <typename T>
inline constexpr bool forward_range =
    decltype(detail::forward_range_concept::test<T>(0))::value;

namespace detail {
struct bidirectional_range_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int)
        -> std::enable_if_t<forward_range<T> &&
                                bidirectional_iterator<iterator_t<T>>,
                            std::true_type>;
};
}  // namespace detail

template <typename T>
inline constexpr bool bidirectional_range =
    decltype(detail::bidirectional_range_concept::test<T>(0))::value;

namespace detail {
struct random_access_range_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int)
        -> std::enable_if_t<bidirectional_range<T> &&
                                random_access_iterator<iterator_t<T>>,
                            std::true_type>;
};
}  // namespace detail

template <typename T>
inline constexpr bool random_access_range =
    decltype(detail::random_access_range_concept::test<T>(0))::value;

namespace detail {
// FIXME: Not to spec
// We only require random_access_iterator, not contiguous_iterator
// This is so that vector::iterator, string::iterator etc can model
// contiguous_range.
// If we do range-v3-style deep integration with iterator_traits then
// this could be fixed
struct contiguous_range_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int) -> std::enable_if_t<
        random_access_range<T> && /* contiguous_iterator<iterator_t<T>>
                                   * &&
                                   */
            detail::requires_<contiguous_range_concept, T>,
        std::true_type>;

    template <typename T>
    auto requires_(T& t)
        -> decltype(requires_expr<std::is_same_v<
                        decltype(ranges::data(t)),
                        std::add_pointer_t<range_reference_t<T>>>>{});
};
}  // namespace detail

template <typename R>
inline constexpr bool contiguous_range =
    decltype(detail::contiguous_range_concept::test<R>(0))::value;

namespace detail {
struct common_range_concept {
    template <typename>
    static auto test(long) -> std::false_type;

    template <typename T>
    static auto test(int)
        -> std::enable_if_t<range<T> &&
                                std::is_same_v<iterator_t<T>, sentinel_t<T>>,
                            std::true_type>;
};
}  // namespace detail

template <typename T>
inline constexpr bool common_range =
    decltype(detail::common_range_concept::test<T>(0))::value;

/**
 * \see `std::ranges::dangling`
 */
struct dangling {
    constexpr dangling() noexcept = default;

    template <typename... Args>
    constexpr dangling(Args&&...) noexcept
    {
    }
};

/**
 * \see `std::ranges::borrowed_iterator_t`
 */
template <typename R>
using borrowed_iterator_t =
    std::conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;

struct view_base {};

template <typename D>
class view_interface : public view_base {
    static_assert(std::is_class<D>::value, "");
    static_assert(std::is_same_v<D, std::remove_cv_t<D>>, "");

private:
    constexpr D& derived() noexcept
    {
        return static_cast<D&>(*this);
    }

    constexpr const D& derived() const noexcept
    {
        return static_cast<const D&>(*this);
    }

public:
    template <typename R = D>
    SCN_NODISCARD constexpr auto empty()
        -> std::enable_if_t<forward_range<R>, bool>
    {
        return ranges::begin(derived()) == ranges::end(derived());
    }

    template <typename R = D>
    SCN_NODISCARD constexpr auto empty() const
        -> std::enable_if_t<forward_range<const R>, bool>
    {
        return ranges::begin(derived()) == ranges::end(derived());
    }

    template <typename R = D,
              typename = decltype(ranges::empty(std::declval<R&>()))>
    constexpr explicit operator bool()
    {
        return !ranges::empty(derived());
    }

    template <typename R = D,
              typename = decltype(ranges::empty(std::declval<const R&>()))>
    constexpr explicit operator bool() const
    {
        return !ranges::empty(derived());
    }

    template <typename R = D,
              typename = std::enable_if_t<contiguous_iterator<iterator_t<R>>>>
    constexpr auto data()
    {
        return detail::to_address(ranges::begin(derived()));
    }

    template <typename R = D,
              typename = std::enable_if_t<
                  range<const R> && contiguous_iterator<iterator_t<const R>>>>
    constexpr auto data() const
    {
        return detail::to_address(ranges::begin(derived()));
    }

    template <typename R = D,
              typename = std::enable_if_t<
                  forward_range<R> &&
                  sized_sentinel_for<sentinel_t<R>, iterator_t<R>>>>
    constexpr auto size()
    {
        return ranges::end(derived()) - ranges::begin(derived());
    }

    template <typename R = D,
              typename = std::enable_if_t<
                  forward_range<const R> &&
                  sized_sentinel_for<sentinel_t<const R>, iterator_t<const R>>>>
    constexpr auto size() const
    {
        return ranges::end(derived()) - ranges::begin(derived());
    }

#if 0
    template <typename R = D, typename = std::enable_if_t<forward_range<R>>>
    constexpr decltype(auto) front()
    {
        return *ranges::begin(derived());
    }

    template <typename R = D,
              typename = std::enable_if_t<forward_range<const R>>>
    constexpr decltype(auto) front() const
    {
        return *ranges::begin(derived());
    }

    template <
        typename R = D,
        typename = std::enable_if_t<bidirectional_range<R> && common_range<R>>>
    constexpr decltype(auto) back()
    {
        return *ranges::prev(ranges::end(derived()));
    }

    template <typename R = D,
              typename = std::enable_if_t<bidirectional_range<const R> &&
                                          common_range<const R>>>
    constexpr decltype(auto) back() const
    {
        return *ranges::prev(ranges::end(derived()));
    }

    template <typename R = D,
              typename = std::enable_if_t<random_access_range<R>>>
    constexpr decltype(auto) operator[](iter_difference_t<iterator_t<R>> n)
    {
        return ranges::begin(derived())[n];
    }
#endif

    template <typename R = D,
              typename = std::enable_if_t<random_access_range<const R>>>
    constexpr decltype(auto) operator[](
        iter_difference_t<iterator_t<const R>> n) const
    {
        return ranges::begin(derived())[n];
    }
};

namespace detail::subrange_ {

template <typename I,
          typename S,
          bool DefaultConstruct = std::is_default_constructible_v<I>>
class subrange_base;

template <typename I, typename S>
class subrange_base<I, S, true> {
public:
    constexpr subrange_base() = default;

    constexpr subrange_base(I i, S s)
        : m_iterator(SCN_MOVE(i)), m_sentinel(SCN_MOVE(s))
    {
    }

protected:
    SCN_NO_UNIQUE_ADDRESS I m_iterator{};
    SCN_NO_UNIQUE_ADDRESS S m_sentinel{};
};

template <typename I, typename S>
class subrange_base<I, S, false> {
public:
    constexpr subrange_base() = delete;

    constexpr subrange_base(I i, S s)
        : m_iterator(SCN_MOVE(i)), m_sentinel(SCN_MOVE(s))
    {
    }

protected:
    SCN_NO_UNIQUE_ADDRESS I m_iterator{};
    SCN_NO_UNIQUE_ADDRESS S m_sentinel{};
};

template <typename I, typename S = I>
class subrange : public view_interface<subrange<I, S>>,
                 private subrange_base<I, S> {
    static_assert(input_or_output_iterator<I>);
    static_assert(sentinel_for<S, I>);

public:
    constexpr subrange() = default;

    template <typename I_,
              std::enable_if_t<std::is_convertible_v<I_, I>>* = nullptr>
    constexpr subrange(I_ i, S s)
        : subrange_base<I, S>(SCN_MOVE(i), SCN_MOVE(s))
    {
    }

    template <
        typename R,
        std::enable_if_t<is_not_self<R, subrange> && borrowed_range<R> &&
                         std::is_convertible_v<iterator_t<R>, I> &&
                         std::is_convertible_v<sentinel_t<R>, S>>* = nullptr>
    constexpr subrange(R&& r)
        : subrange_base<I, S>(scn::ranges::begin(r), scn::ranges::end(r))
    {
    }

    template <typename II = I, std::enable_if_t<copyable<II>>* = nullptr>
    SCN_NODISCARD constexpr I begin() const
        noexcept(std::is_nothrow_copy_constructible_v<I>)
    {
        return this->m_iterator;
    }
    template <typename II = I, std::enable_if_t<!copyable<II>>* = nullptr>
    SCN_NODISCARD constexpr I begin() noexcept
    {
        return SCN_MOVE(this->m_iterator);
    }

    SCN_NODISCARD constexpr S end() const
        noexcept(std::is_nothrow_copy_constructible_v<S>)
    {
        return this->m_sentinel;
    }

    SCN_NODISCARD constexpr bool empty() const noexcept
    {
        return this->m_iterator == this->m_sentinel;
    }

    template <typename I_ = I,
              std::enable_if_t<sized_sentinel_for<S, I_>>* = nullptr>
    SCN_NODISCARD constexpr std::size_t size() const noexcept
    {
        return static_cast<size_t>(this->m_sentinel - this->m_iterator);
    }
};

template <typename I,
          typename S,
          std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>>* =
              nullptr>
subrange(I, S) -> subrange<I, S>;

template <typename R, std::enable_if_t<borrowed_range<R>>* = nullptr>
subrange(R&&) -> subrange<iterator_t<R>, sentinel_t<R>>;

}  // namespace detail::subrange_

/**
 * \see `std::ranges::subrange`
 */
using detail::subrange_::subrange;

template <typename I, typename S>
inline constexpr bool enable_borrowed_range<subrange<I, S>> = true;

struct default_sentinel_t {};
inline constexpr default_sentinel_t default_sentinel{};

// viewable_range (C++20)

namespace detail {

template <typename T>
inline constexpr bool is_basic_view =
    range<T> && std::is_base_of_v<view_base, T>;
template <typename CharT, typename Traits>
inline constexpr bool is_basic_view<std::basic_string_view<CharT, Traits>> =
    true;
template <typename I, typename S>
inline constexpr bool is_basic_view<subrange<I, S>> = true;

template <typename R>
inline constexpr bool is_initializer_list_impl = false;
template <typename T>
inline constexpr bool is_initializer_list_impl<std::initializer_list<T>> = true;
template <typename R>
inline constexpr bool is_initializer_list =
    is_initializer_list_impl<remove_cvref_t<R>>;

}  // namespace detail

template <typename T>
inline constexpr bool viewable_range =
    range<T> &&
    ((detail::is_basic_view<detail::remove_cvref_t<T>> &&
      std::is_constructible_v<detail::remove_cvref_t<T>, T>) ||
     (!detail::is_basic_view<detail::remove_cvref_t<T>> &&
      (std::is_lvalue_reference_v<T> || (movable<std::remove_reference_t<T>> &&
                                         !detail::is_initializer_list<T>))));

// ref_view (C++20)

namespace detail {

template <typename T, typename U>
inline constexpr bool different_from =
    !std::is_same_v<remove_cvref_t<T>, remove_cvref_t<U>>;

}

template <typename R>
class ref_view : view_interface<ref_view<R>> {
    static_assert(std::is_object_v<R>);
    static_assert(range<R>);

    struct constructor_req {
        static void fun(R&);
        static void fun(R&&) = delete;

        template <typename T>
        auto requires_() -> decltype(fun(SCN_DECLVAL(T)));
    };

public:
    template <typename T,
              std::enable_if_t<detail::different_from<T, ref_view> &&
                               detail::requires_<constructor_req, T> &&
                               std::is_convertible_v<T, R&>>* = nullptr>
    constexpr ref_view(T&& t) : m_range(&static_cast<R&>(SCN_FWD(t)))
    {
    }

    constexpr R& base() const
    {
        return *m_range;
    }

    constexpr iterator_t<R> begin() const
    {
        return ranges::begin(*m_range);
    }
    constexpr sentinel_t<R> end() const
    {
        return ranges::end(*m_range);
    }

    template <typename T = R,
              typename = decltype(ranges::empty(SCN_DECLVAL(const T&)))>
    constexpr bool empty() const
    {
        return ranges::empty(*m_range);
    }

    template <typename T = R, std::enable_if_t<sized_range<T>>* = nullptr>
    constexpr auto size() const
    {
        return ranges::size(*m_range);
    }

    template <typename T = R, std::enable_if_t<contiguous_range<T>>* = nullptr>
    constexpr auto data() const
    {
        return ranges::data(*m_range);
    }

private:
    R* m_range{};
};

template <typename R>
ref_view(R&) -> ref_view<R>;

template <typename T>
inline constexpr bool enable_borrowed_range<ref_view<T>> = true;

// owning_view (C++20)

namespace detail {

template <typename R,
          bool DefaultConstructible = std::is_default_constructible_v<R>>
struct owning_view_base;

template <typename R>
struct owning_view_base<R, true> {
    owning_view_base() = default;
    owning_view_base(R&& r) : m_range(SCN_MOVE(r)) {}

    R m_range{};
};

template <typename R>
struct owning_view_base<R, false> {
    owning_view_base(R&& r) : m_range(SCN_MOVE(r)) {}

    R m_range;
};

}  // namespace detail

template <typename R>
class owning_view : private detail::owning_view_base<R>,
                    public view_interface<owning_view<R>> {
    static_assert(std::is_object_v<R>);
    static_assert(range<R>);
    static_assert(movable<R>);
    static_assert(!detail::is_initializer_list<R>);

    using base_type = detail::owning_view_base<R>;

public:
    owning_view() = default;

    constexpr owning_view(R&& r) : base_type(SCN_MOVE(r)) {}

    constexpr R& base() & noexcept
    {
        return base_type::m_range;
    }
    constexpr const R& base() const& noexcept
    {
        return base_type::m_range;
    }
    constexpr R&& base() && noexcept
    {
        return SCN_MOVE(base_type::m_range);
    }
    constexpr const R&& base() const&& noexcept
    {
        return SCN_MOVE(base_type::m_range);
    }

    template <typename T = R>
    constexpr iterator_t<T> begin()
    {
        return ranges::begin(base_type::m_range);
    }
    template <typename T = R, std::enable_if_t<range<const T>>* = nullptr>
    constexpr iterator_t<const T> begin() const
    {
        return ranges::begin(base_type::m_range);
    }

    template <typename T = R>
    constexpr sentinel_t<T> end()
    {
        return ranges::end(base_type::m_range);
    }
    template <typename T = R, std::enable_if_t<range<const T>>* = nullptr>
    constexpr sentinel_t<const T> end() const
    {
        return ranges::end(base_type::m_range);
    }

    template <typename T = R,
              typename = decltype(ranges::empty(SCN_DECLVAL(T&)))>
    constexpr bool empty()
    {
        return ranges::empty(base_type::m_range);
    }
    template <typename T = R,
              typename = decltype(ranges::empty(SCN_DECLVAL(const T&)))>
    constexpr bool empty() const
    {
        return ranges::empty(base_type::m_range);
    }

    template <typename T = R, std::enable_if_t<sized_range<T>>* = nullptr>
    constexpr auto size()
    {
        return ranges::size(base_type::m_range);
    }
    template <typename T = R, std::enable_if_t<sized_range<const T>>* = nullptr>
    constexpr auto size() const
    {
        return ranges::size(base_type::m_range);
    }

    template <typename T = R, std::enable_if_t<contiguous_range<T>>* = nullptr>
    constexpr auto data()
    {
        return ranges::data(base_type::m_range);
    }
    template <typename T = R,
              std::enable_if_t<contiguous_range<const T>>* = nullptr>
    constexpr auto data() const
    {
        return ranges::data(base_type::m_range);
    }
};

// No-op deduction guide,
// silences a clang warning about possibly unintentional CTAD
template <typename R, std::enable_if_t<!std::is_reference_v<R>>* = nullptr>
owning_view(R&&) -> owning_view<detail::remove_cvref_t<R>>;

template <typename T>
inline constexpr bool enable_borrowed_range<owning_view<T>> =
    enable_borrowed_range<T>;

// all (C++20)

namespace views {

namespace all_ {

struct fn {
private:
    template <typename T>
    static constexpr auto impl(T&& t, detail::priority_tag<2>) noexcept(
        noexcept(detail::decay_copy(SCN_FWD(t))))
        -> std::enable_if_t<movable<std::decay_t<T>> &&
                                detail::is_basic_view<std::decay_t<T>>,
                            decltype(detail::decay_copy(SCN_FWD(t)))>
    {
        return detail::decay_copy(SCN_FWD(t));
    }

    template <typename T>
    static constexpr auto impl(T&& t, detail::priority_tag<1>) noexcept
        -> decltype(scn::ranges::ref_view(SCN_FWD(t)))
    {
        return scn::ranges::ref_view(SCN_FWD(t));
    }

    template <typename T>
    static constexpr auto impl(T&& t, detail::priority_tag<0>) noexcept(
        noexcept(scn::ranges::owning_view(SCN_FWD(t))))
        -> decltype(scn::ranges::owning_view(SCN_FWD(t)))
    {
        return scn::ranges::owning_view(SCN_FWD(t));
    }

public:
    template <typename T, std::enable_if_t<viewable_range<T>>* = nullptr>
    auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t), detail::priority_tag<2>{})))
            -> decltype(fn::impl(SCN_FWD(t), detail::priority_tag<2>{}))
    {
        return fn::impl(SCN_FWD(t), detail::priority_tag<2>{});
    }
};

}  // namespace all_

inline constexpr auto all = all_::fn{};

template <typename R, std::enable_if_t<viewable_range<R>>* = nullptr>
using all_t = decltype(all(SCN_DECLVAL(R)));

}  // namespace views

// common_iterator (C++20)

namespace detail {
template <typename I,
          typename S,
          bool Trivial = std::is_trivially_copyable_v<I> &&
                         std::is_trivially_copyable_v<S>>
class common_iterator_storage;

template <typename I, typename S>
class common_iterator_storage<I, S, true> {
public:
    template <typename II = I,
              std::enable_if_t<std::is_default_constructible_v<II>>* = nullptr>
    constexpr common_iterator_storage() : m_iterator{}, m_is_iterator{true}
    {
    }

    constexpr common_iterator_storage(I iter)
        : m_iterator{SCN_MOVE(iter)}, m_is_iterator{true}
    {
    }
    constexpr common_iterator_storage(S sent)
        : m_sentinel{SCN_MOVE(sent)}, m_is_iterator{false}
    {
    }

    // TODO? converting constructors

protected:
    bool _is_iterator() const
    {
        return m_is_iterator;
    }

    template <typename T>
    T& _get_as()
    {
        if constexpr (std::is_same_v<T, I>) {
            return m_iterator;
        }
        else if constexpr (std::is_same_v<T, S>) {
            return m_sentinel;
        }
    }

    template <typename T>
    const T& _get_as() const
    {
        if constexpr (std::is_same_v<T, I>) {
            return m_iterator;
        }
        else if constexpr (std::is_same_v<T, S>) {
            return m_sentinel;
        }
    }

    void _destroy() {}

    template <typename... Args>
    void _make_iterator(Args&&... args)
    {
        ::new (&m_iterator) I(SCN_FWD(args)...);
        m_is_iterator = true;
    }
    template <typename... Args>
    void _make_sentinel(Args&&... args)
    {
        ::new (&m_sentinel) S(SCN_FWD(args)...);
        m_is_iterator = false;
    }

private:
    union {
        I m_iterator;
        S m_sentinel;
    };
    bool m_is_iterator{true};
};

template <typename I, typename S>
class common_iterator_storage<I, S, false> {
public:
    template <typename II = I,
              std::enable_if_t<std::is_default_constructible_v<II>>* = nullptr>
    common_iterator_storage() : m_is_iterator{true}
    {
        _make_iterator();
    }

    common_iterator_storage(I iter) : m_is_iterator{true}
    {
        _make_iterator(SCN_MOVE(iter));
    }
    common_iterator_storage(S sent) : m_is_iterator{false}
    {
        _make_sentinel(SCN_MOVE(sent));
    }

    // Not exception safe,
    // but let's just assume these never throw and leave it at that.

    template <typename II = I,
              typename SS = S,
              std::enable_if_t<std::is_copy_constructible_v<II> &&
                               std::is_copy_constructible_v<SS>>* = nullptr>
    common_iterator_storage(const common_iterator_storage& other)
    {
        if (other.m_is_iterator) {
            _make_iterator(other._get_as<I>());
        }
        else {
            _make_sentinel(other._get_as<S>());
        }
    }

    template <typename II = I,
              typename SS = S,
              std::enable_if_t<std::is_copy_constructible_v<II> &&
                               std::is_copy_constructible_v<SS>>* = nullptr>
    common_iterator_storage& operator=(const common_iterator_storage& other)
    {
        _destroy();
        if (other.m_is_iterator) {
            _make_iterator(other._get_as<I>());
        }
        else {
            _make_sentinel(other._get_as<S>());
        }
        return *this;
    }

    template <typename II = I,
              typename SS = S,
              std::enable_if_t<std::is_move_constructible_v<II> &&
                               std::is_move_constructible_v<SS>>* = nullptr>
    common_iterator_storage(common_iterator_storage&& other)
    {
        if (other.m_is_iterator) {
            _make_iterator(SCN_MOVE(other._get_as<I>()));
        }
        else {
            _make_sentinel(SCN_MOVE(other._get_as<S>()));
        }
    }

    template <typename II = I,
              typename SS = S,
              std::enable_if_t<std::is_move_constructible_v<II> &&
                               std::is_move_constructible_v<SS>>* = nullptr>
    common_iterator_storage& operator=(common_iterator_storage&& other)
    {
        _destroy();
        if (other.m_is_iterator) {
            _make_iterator(SCN_MOVE(other._get_as<I>()));
        }
        else {
            _make_sentinel(SCN_MOVE(other._get_as<S>()));
        }
        return *this;
    }

    ~common_iterator_storage()
    {
        _destroy();
    }

protected:
    bool _is_iterator() const
    {
        return m_is_iterator;
    }

    template <typename T>
    T& _get_as()
    {
        static_assert(std::is_object_v<T>);
        return *reinterpret_cast<T*>(m_storage);
    }
    template <typename T>
    const T& _get_as() const
    {
        static_assert(std::is_object_v<T>);
        return *reinterpret_cast<const T*>(m_storage);
    }

    void _destroy()
    {
        if (m_is_iterator) {
            _get_as<I>().~I();
        }
        else {
            _get_as<S>().~S();
        }
    }

    template <typename... Args>
    void _make_iterator(Args&&... args)
    {
        ::new (m_storage) I(SCN_FWD(args)...);
        m_is_iterator = true;
    }
    template <typename... Args>
    void _make_sentinel(Args&&... args)
    {
        ::new (m_storage) S(SCN_FWD(args)...);
        m_is_iterator = false;
    }

private:
    static constexpr auto storage_size = detail::max(sizeof(I), sizeof(S));
    static constexpr auto storage_alignment =
        detail::max(alignof(I), alignof(S));

    alignas(storage_alignment) unsigned char m_storage[storage_size];
    bool m_is_iterator{true};
};

}  // namespace detail

template <typename I, typename S>
class common_iterator : private detail::common_iterator_storage<I, S> {
    static_assert(input_or_output_iterator<I>);
    static_assert(sentinel_for<S, I>);
    static_assert(!std::is_same_v<I, S>);
    static_assert(copyable<I>);

    using base_type = detail::common_iterator_storage<I, S>;

public:
    constexpr common_iterator() = default;

    constexpr common_iterator(I i) : base_type(SCN_MOVE(i)) {}
    constexpr common_iterator(S s) : base_type(SCN_MOVE(s)) {}

    // base() is an extension
    constexpr I base() const&
    {
        SCN_EXPECT(base_type::_is_iterator());
        return base_type::template _get_as<I>();
    }
    constexpr I&& base() &&
    {
        SCN_EXPECT(base_type::_is_iterator());
        return SCN_MOVE(base_type::template _get_as<I>());
    }

    template <typename = I>
    constexpr decltype(auto) operator*()
    {
        SCN_EXPECT(base_type::_is_iterator());
        return *base_type::template _get_as<I>();
    }
    template <typename II = I,
              std::enable_if_t<detail::dereferenceable<const II>>* = nullptr>
    constexpr decltype(auto) operator*() const
    {
        SCN_EXPECT(base_type::_is_iterator());
        return *base_type::template _get_as<I>();
    }

    // TODO: operator->

    constexpr common_iterator& operator++()
    {
        SCN_EXPECT(base_type::_is_iterator());
        ++base_type::template _get_as<I>();
        return *this;
    }
    template <typename II = I,
              typename = std::void_t<decltype(SCN_DECLVAL(I&)++)>>
    constexpr decltype(auto) operator++(int)
    {
        // This is a non-conforming implementation
        SCN_EXPECT(base_type::_is_iterator());
        if constexpr (forward_iterator<I>) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }
        else {
            return base_type::template _get_as<I>()++;
        }
    }

    // TODO: compare with convertible common_iterators

    friend constexpr bool operator==(const common_iterator& x,
                                     const common_iterator& y)
    {
        if (!x._is_iterator() && !y._is_iterator()) {
            return true;
        }
        if (x._is_iterator() && y._is_iterator()) {
            return x.template _get_as<I>() == y.template _get_as<I>();
        }
        if (x._is_iterator() && !y._is_iterator()) {
            return x.template _get_as<I>() == y.template _get_as<S>();
        }
        if (!x._is_iterator() && y._is_iterator()) {
            return x.template _get_as<S>() == y.template _get_as<I>();
        }
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    friend constexpr bool operator!=(const common_iterator& x,
                                     const common_iterator& y)
    {
        return !(x == y);
    }

    template <typename II = I,
              typename SS = S,
              std::enable_if_t<sized_sentinel_for<SS, II>>* = nullptr>
    friend constexpr iter_difference_t<II> operator-(const common_iterator& x,
                                                     const common_iterator& y)
    {
        if (!x._is_iterator() && !y._is_iterator()) {
            return 0;
        }
        if (x._is_iterator() && y._is_iterator()) {
            return x.template _get_as<I>() - y.template _get_as<I>();
        }
        if (x._is_iterator() && !y._is_iterator()) {
            return x.template _get_as<I>() - y.template _get_as<S>();
        }
        if (!x._is_iterator() && y._is_iterator()) {
            return x.template _get_as<S>() - y.template _get_as<I>();
        }
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    // TODO: iter_move, iter_swap
};

template <typename I, typename S>
struct incrementable_traits<common_iterator<I, S>> {
    using difference_type = iter_difference_t<I>;
};

}  // namespace ranges
SCN_END_NAMESPACE
}  // namespace scn

namespace std {

template <typename I, typename S>
struct iterator_traits<scn::ranges::common_iterator<I, S>> {
    using difference_type =
        scn::ranges::iter_difference_t<scn::ranges::common_iterator<I, S>>;
    using value_type =
        scn::ranges::iter_value_t<scn::ranges::common_iterator<I, S>>;
    using pointer = std::add_pointer_t<
        scn::ranges::iter_reference_t<scn::ranges::common_iterator<I, S>>>;
    using reference =
        scn::ranges::iter_reference_t<scn::ranges::common_iterator<I, S>>;
    using iterator_category =
        std::conditional_t<scn::ranges::forward_iterator<I>,
                           std::forward_iterator_tag,
                           std::input_iterator_tag>;
    using iterator_concept = iterator_category;
};

}  // namespace std

namespace scn {
SCN_BEGIN_NAMESPACE

namespace ranges {

// common_view (C++20)

namespace detail {

template <typename R, typename = void>
inline constexpr bool simple_view = false;
template <typename R>
inline constexpr bool simple_view<R, std::enable_if_t<range<const R>>> =
    std::is_same_v<iterator_t<R>, iterator_t<const R>> &&
    std::is_same_v<sentinel_t<R>, sentinel_t<const R>>;

}  // namespace detail

template <typename V,
          std::enable_if_t<detail::is_basic_view<V> && !common_range<V>>* =
              nullptr>
class common_view : public view_interface<common_view<V>> {
    static_assert(copyable<iterator_t<V>>);

public:
    template <typename T = V,
              std::enable_if_t<std::is_default_constructible_v<T>>* = nullptr>
    common_view() : m_base()
    {
    }

    constexpr explicit common_view(V r) : m_base(SCN_MOVE(r)) {}

    template <typename T = V,
              std::enable_if_t<std::is_copy_constructible_v<T>>* = nullptr>
    constexpr V base() const&
    {
        return m_base;
    }
    template <typename T = V>
    constexpr V base() &&
    {
        return SCN_MOVE(m_base);
    }

    template <typename T = V,
              std::enable_if_t<!detail::simple_view<T>>* = nullptr>
    constexpr auto begin()
    {
        if constexpr (random_access_range<V> && sized_range<V>) {
            return ranges::begin(m_base);
        }
        else {
            return common_iterator<iterator_t<V>, sentinel_t<V>>(
                ranges::begin(m_base));
        }
    }

    template <typename T = V, std::enable_if_t<range<const T>>* = nullptr>
    constexpr auto begin() const
    {
        if constexpr (random_access_range<const V> && sized_range<const V>) {
            return ranges::begin(m_base);
        }
        else {
            return common_iterator<iterator_t<const V>, sentinel_t<const V>>(
                ranges::begin(m_base));
        }
    }

    template <typename T = V,
              std::enable_if_t<!detail::simple_view<T>>* = nullptr>
    constexpr auto end()
    {
        if constexpr (random_access_range<V> && sized_range<V>) {
            return ranges::begin(m_base) +
                   static_cast<range_difference_t<V>>(ranges::size(m_base));
        }
        else {
            return common_iterator<iterator_t<V>, sentinel_t<V>>(
                ranges::end(m_base));
        }
    }

    template <typename T = V, std::enable_if_t<range<const T>>* = nullptr>
    constexpr auto end() const
    {
        if constexpr (random_access_range<const V> && sized_range<const V>) {
            return ranges::begin(m_base) +
                   static_cast<range_difference_t<V>>(ranges::size(m_base));
        }
        else {
            return common_iterator<iterator_t<const V>, sentinel_t<const V>>(
                ranges::end(m_base));
        }
    }

    template <typename T = V, std::enable_if_t<sized_range<T>>* = nullptr>
    constexpr auto size()
    {
        return ranges::size(m_base);
    }
    template <typename T = V, std::enable_if_t<sized_range<const T>>* = nullptr>
    constexpr auto size() const
    {
        return ranges::size(m_base);
    }

private:
    V m_base;
};

template <typename R>
common_view(R&&) -> common_view<views::all_t<R>>;

template <typename R>
inline constexpr bool enable_borrowed_range<common_view<R>> =
    enable_borrowed_range<R>;

namespace views {

namespace common_ {

struct fn {
private:
    template <typename T, std::enable_if_t<common_range<T>>* = nullptr>
    static constexpr auto impl(T&& t, detail::priority_tag<1>) noexcept(
        noexcept(views::all(SCN_FWD(t)))) -> decltype(views::all(SCN_FWD(t)))
    {
        return views::all(SCN_FWD(t));
    }
    template <typename T>
    static constexpr auto impl(T&& t, detail::priority_tag<0>) noexcept(
        noexcept(common_view(SCN_FWD(t)))) -> decltype(common_view(SCN_FWD(t)))
    {
        return common_view(SCN_FWD(t));
    }

public:
    template <typename T, std::enable_if_t<viewable_range<T>>* = nullptr>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t), detail::priority_tag<1>{})))
            -> decltype(fn::impl(SCN_FWD(t), detail::priority_tag<1>{}))
    {
        return fn::impl(SCN_FWD(t), detail::priority_tag<1>{});
    }
};

}  // namespace common_

inline constexpr auto common = common_::fn{};

}  // namespace views

// to_input_view (C++26)

namespace detail {

template <bool Const, typename T>
using maybe_const = conditional_t<Const, const T, T>;

template <typename V,
          bool DefaultConstructible = std::is_default_constructible_v<V>>
struct to_input_view_base;

template <typename V>
struct to_input_view_base<V, true> {
    to_input_view_base() = default;
    explicit constexpr to_input_view_base(V b) : m_base(SCN_MOVE(b)) {}

    V m_base{};
};

template <typename V>
struct to_input_view_base<V, false> {
    explicit constexpr to_input_view_base(V b) : m_base(SCN_MOVE(b)) {}

    V m_base;
};

template <typename BaseIterator,
          bool DefaultConstructible =
              std::is_default_constructible_v<BaseIterator>>
struct to_input_view_iterator_base;

template <typename BaseIterator>
struct to_input_view_iterator_base<BaseIterator, true> {
    to_input_view_iterator_base() = default;
    explicit constexpr to_input_view_iterator_base(BaseIterator current)
        : m_current(SCN_MOVE(current))
    {
    }

    BaseIterator m_current{};
};

template <typename BaseIterator>
struct to_input_view_iterator_base<BaseIterator, false> {
    explicit constexpr to_input_view_iterator_base(BaseIterator current)
        : m_current(SCN_MOVE(current))
    {
    }

    BaseIterator m_current;
};

}  // namespace detail

template <typename V>
class to_input_view : private detail::to_input_view_base<V>,
                      public view_interface<to_input_view<V>> {
    static_assert(input_range<V>);
    static_assert(detail::is_basic_view<V>);

    using base_type = detail::to_input_view_base<V>;

    template <bool Const>
    class iterator : private detail::to_input_view_iterator_base<
                         iterator_t<detail::maybe_const<Const, V>>> {
        friend class iterator<!Const>;
        friend class to_input_view;

        using parent_type = detail::maybe_const<Const, V>;
        using base_type =
            detail::to_input_view_iterator_base<iterator_t<parent_type>>;

        constexpr explicit iterator(iterator_t<parent_type> current)
            : base_type(SCN_MOVE(current))
        {
        }

    public:
        using difference_type = range_difference_t<parent_type>;
        using value_type = range_value_t<parent_type>;
        using iterator_concept = input_iterator_tag;

        // non-conforming
        using iterator_category = input_iterator_tag;

        iterator() = default;

        iterator(iterator&&) = default;
        iterator& operator=(iterator&&) = default;

        template <
            bool C = Const,
            std::enable_if_t<
                C && std::is_convertible_v<iterator_t<V>,
                                           iterator_t<parent_type>>>* = nullptr>
        constexpr iterator(iterator<!C> other)
            : base_type(SCN_MOVE(other.m_current))
        {
        }

        constexpr iterator_t<parent_type> base() &&
        {
            return SCN_MOVE(base_type::m_current);
        }
        constexpr const iterator_t<parent_type>& base() const& noexcept
        {
            return base_type::m_current;
        }

        constexpr decltype(auto) operator*() const
        {
            return *base_type::m_current;
        }

        constexpr iterator& operator++()
        {
            ++base_type::m_current;
            return *this;
        }
        constexpr void operator++(int)
        {
            ++*this;
        }

        friend constexpr bool operator==(const iterator& x,
                                         const sentinel_t<parent_type>& y)
        {
            return x.m_current == y;
        }
        friend constexpr bool operator!=(const iterator& x,
                                         const sentinel_t<parent_type>& y)
        {
            return !(x == y);
        }

        template <bool = Const,
                  std::enable_if_t<
                      sized_sentinel_for<sentinel_t<parent_type>,
                                         iterator_t<parent_type>>>* = nullptr>
        friend constexpr difference_type operator-(
            const sentinel_t<parent_type>& y,
            const iterator& x)
        {
            return y - x.m_current;
        }
        template <bool = Const,
                  std::enable_if_t<
                      sized_sentinel_for<sentinel_t<parent_type>,
                                         iterator_t<parent_type>>>* = nullptr>
        friend constexpr difference_type operator-(
            const iterator& x,
            const sentinel_t<parent_type>& y)
        {
            return x.m_current - y;
        }

        // TODO: iter_move and iter_swap, with range_rvalue_reference_t
    };

public:
    to_input_view() = default;
    explicit constexpr to_input_view(V b) : base_type(SCN_MOVE(b)) {}

    template <typename U = V,
              std::enable_if_t<std::is_copy_constructible_v<U>>* = nullptr>
    constexpr V base() const&
    {
        return base_type::m_base;
    }
    template <typename U = V>
    constexpr U base() &&
    {
        return SCN_MOVE(base_type::m_base);
    }

    template <typename U = V,
              std::enable_if_t<!detail::simple_view<U>>* = nullptr>
    constexpr auto begin()
    {
        return iterator<false>{ranges::begin(base_type::m_base)};
    }
    template <typename U = V, std::enable_if_t<range<const U>>* = nullptr>
    constexpr auto begin() const
    {
        return iterator<true>{ranges::begin(base_type::m_base)};
    }

    template <typename U = V,
              std::enable_if_t<!detail::simple_view<U>>* = nullptr>
    constexpr auto end()
    {
        return ranges::end(base_type::m_base);
    }
    template <typename U = V, std::enable_if_t<range<const U>>* = nullptr>
    constexpr auto end() const
    {
        return ranges::end(base_type::m_base);
    }

    template <typename U = V, std::enable_if_t<sized_range<U>>* = nullptr>
    constexpr auto size()
    {
        return ranges::size(base_type::m_base);
    }
    template <typename U = V, std::enable_if_t<sized_range<const U>>* = nullptr>
    constexpr auto size() const
    {
        return ranges::size(base_type::m_base);
    }
};

template <typename V>
inline constexpr bool enable_borrowed_range<to_input_view<V>> =
    enable_borrowed_range<V>;

template <typename R>
to_input_view(R&&) -> to_input_view<views::all_t<R>>;

static_assert(input_range<to_input_view<std::string_view>>);
static_assert(!forward_range<to_input_view<std::string_view>>);

namespace views {

namespace to_input_ {

struct fn {
private:
    template <typename T>
    static auto impl(T&& t, detail::priority_tag<1>) noexcept(
        noexcept(views::all(SCN_FWD(t))))
        -> std::enable_if_t<input_range<T> && !forward_range<T> &&
                                !common_range<T>,
                            decltype(views::all(SCN_FWD(t)))>
    {
        return views::all(SCN_FWD(t));
    }

    template <typename T>
    static auto impl(T&& t, detail::priority_tag<0>) noexcept(
        noexcept(ranges::to_input_view(SCN_FWD(t))))
        -> decltype(ranges::to_input_view(SCN_FWD(t)))
    {
        return ranges::to_input_view(SCN_FWD(t));
    }

public:
    template <typename T, std::enable_if_t<viewable_range<T>>* = nullptr>
    auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t), detail::priority_tag<1>{})))
            -> decltype(fn::impl(SCN_FWD(t), detail::priority_tag<1>{}))
    {
        return fn::impl(SCN_FWD(t), detail::priority_tag<1>{});
    }
};

}  // namespace to_input_

inline constexpr auto to_input = to_input_::fn{};

}  // namespace views

// scnlib extension:
// pair_concat_view
// based on concat_view from C++26, but heavily stripped down
// (always two ranges, must share a value type, only ever an input_range)

namespace detail {
struct pair_concat_first_tag {};
struct pair_concat_second_tag {};

template <typename FirstIt, typename SecondIt>
struct pair_concat_iterator_storage {
protected:
    using first_iterator = FirstIt;
    using second_iterator = SecondIt;

    pair_concat_iterator_storage() = default;

    pair_concat_iterator_storage(pair_concat_first_tag, FirstIt f)
        : m_is_first(true)
    {
        _make_first(SCN_MOVE(f));
    }
    pair_concat_iterator_storage(pair_concat_second_tag, SecondIt s)
        : m_is_first(false)
    {
        _make_second(SCN_MOVE(s));
    }

    // Not exception safe, but I don't care enough (yet)

    template <typename F = FirstIt,
              typename S = SecondIt,
              std::enable_if_t<std::is_copy_constructible_v<F> &&
                               std::is_copy_constructible_v<S>>* = nullptr>
    pair_concat_iterator_storage(const pair_concat_iterator_storage& other)
    {
        if (other.m_is_first) {
            _make_first(other._get_first());
        }
        else {
            _make_second(other._get_second());
        }
    }

    template <typename F = FirstIt,
              typename S = SecondIt,
              std::enable_if_t<std::is_copy_constructible_v<F> &&
                               std::is_copy_constructible_v<S>>* = nullptr>
    pair_concat_iterator_storage& operator=(
        const pair_concat_iterator_storage& other)
    {
        _destroy();
        if (other.m_is_first) {
            _make_first(other._get_first());
        }
        else {
            _make_second(other._get_second());
        }
        return *this;
    }

    template <typename F = FirstIt,
              typename S = SecondIt,
              std::enable_if_t<std::is_move_constructible_v<F> &&
                               std::is_move_constructible_v<S>>* = nullptr>
    pair_concat_iterator_storage(pair_concat_iterator_storage&& other)
    {
        if (other.m_is_first) {
            _make_first(SCN_MOVE(other._get_first()));
        }
        else {
            _make_second(SCN_MOVE(other._get_second()));
        }
    }

    template <typename F = FirstIt,
              typename S = SecondIt,
              std::enable_if_t<std::is_move_constructible_v<F> &&
                               std::is_move_constructible_v<S>>* = nullptr>
    pair_concat_iterator_storage& operator=(
        pair_concat_iterator_storage&& other)
    {
        _destroy();
        if (other.m_is_first) {
            _make_first(SCN_MOVE(other._get_first()));
        }
        else {
            _make_second(SCN_MOVE(other._get_second()));
        }
        return *this;
    }

    ~pair_concat_iterator_storage()
    {
        _destroy();
    }

    bool _is_first() const
    {
        return m_is_first;
    }

    FirstIt& _get_first()
    {
        SCN_EXPECT(m_is_first);
        return *reinterpret_cast<FirstIt*>(
            SCN_ASSUME_ALIGNED(m_storage, alignof(FirstIt)));
    }
    const FirstIt& _get_first() const
    {
        SCN_EXPECT(m_is_first);
        return *reinterpret_cast<const FirstIt*>(
            SCN_ASSUME_ALIGNED(m_storage, alignof(FirstIt)));
    }

    SecondIt& _get_second()
    {
        SCN_EXPECT(!m_is_first);
        return *reinterpret_cast<SecondIt*>(
            SCN_ASSUME_ALIGNED(m_storage, alignof(SecondIt)));
    }
    const SecondIt& _get_second() const
    {
        SCN_EXPECT(!m_is_first);
        return *reinterpret_cast<const SecondIt*>(
            SCN_ASSUME_ALIGNED(m_storage, alignof(SecondIt)));
    }

    void _destroy()
    {
        if (m_is_first) {
            _get_first().~FirstIt();
        }
        else {
            _get_second().~SecondIt();
        }
    }

    template <typename... Args>
    void _make_first(Args&&... args)
    {
        ::new (m_storage) FirstIt(SCN_FWD(args)...);
        m_is_first = true;
    }
    template <typename... Args>
    void _make_second(Args&&... args)
    {
        ::new (m_storage) SecondIt(SCN_FWD(args)...);
        m_is_first = false;
    }

private:
    static constexpr auto storage_size =
        detail::max(sizeof(FirstIt), sizeof(SecondIt));
    static constexpr auto storage_alignment =
        detail::max(alignof(FirstIt), alignof(SecondIt));

    alignas(storage_alignment) unsigned char m_storage[storage_size];
    bool m_is_first{true};
};

struct pair_concat_access;

}  // namespace detail

template <typename First, typename Second>
class pair_concat_view : view_interface<pair_concat_view<First, Second>> {
    static_assert(input_range<First>);
    static_assert(input_range<Second>);
    static_assert(detail::is_basic_view<First>);
    static_assert(detail::is_basic_view<Second>);
    static_assert(std::is_same_v<range_value_t<First>, range_value_t<Second>>);
    static_assert(
        std::is_same_v<range_difference_t<First>, range_difference_t<Second>>);

    friend struct pair_concat_access;

    template <typename R>
    using const_iterator_t =
        typename detail::mp_if_c<range<const R>,
                                 detail::mp_defer<iterator_t, const R>,
                                 detail::mp_identity<void>>::type;

    template <bool Const>
    class iterator : private detail::pair_concat_iterator_storage<
                         iterator_t<detail::maybe_const<Const, First>>,
                         iterator_t<detail::maybe_const<Const, Second>>> {
        using base = detail::pair_concat_iterator_storage<
            iterator_t<detail::maybe_const<Const, First>>,
            iterator_t<detail::maybe_const<Const, Second>>>;

        using first_iterator = typename base::first_iterator;
        using second_iterator = typename base::second_iterator;

        static_assert(std::is_same_v<iter_value_t<first_iterator>,
                                     iter_value_t<second_iterator>>);
        static_assert(std::is_same_v<iter_difference_t<first_iterator>,
                                     iter_difference_t<second_iterator>>);

        friend struct pair_concat_access;

        template <std::size_t N>
        void _satisfy()
        {
            if constexpr (N == 0) {
                if (base::_get_first() == ranges::end(m_parent->m_first)) {
                    base::_destroy();
                    base::_make_second(ranges::begin(m_parent->m_second));
                    _satisfy<N + 1>();
                }
            }
        }

    public:
        using iterator_category = std::input_iterator_tag;
        using iterator_concept = std::input_iterator_tag;
        using value_type = iter_value_t<first_iterator>;
        using difference_type = iter_difference_t<first_iterator>;

        template <
            bool Other,
            std::enable_if_t<Const && !Other &&
                             std::is_convertible_v<iterator_t<First>,
                                                   const_iterator_t<First>> &&
                             std::is_convertible_v<iterator_t<Second>,
                                                   const_iterator_t<Second>>>* =
                nullptr>
        iterator(iterator<Other> other) : base(), m_parent(other.m_parent)
        {
            if (other._is_first()) {
                base::_make_first(SCN_MOVE(other._get_first()));
            }
            else {
                base::_make_second(SCN_MOVE(other._get_second()));
            }
        }

        using base::_get_first;
        using base::_get_second;
        using base::_is_first;

        iterator& _until_second()
        {
            while (_is_first()) {
                operator++();
            }
            return *this;
        }

        value_type operator*() const
        {
            if (_is_first()) {
                return *_get_first();
            }
            else {
                return *_get_second();
            }
        }

        iterator& operator++()
        {
            if (_is_first()) {
                ++_get_first();
                _satisfy<0>();
            }
            else {
                ++_get_second();
                _satisfy<1>();
            }
            return *this;
        }

        void operator++(int)
        {
            ++*this;
        }

        template <
            typename F = First,
            typename S = Second,
            std::enable_if_t<equality_comparable<
                                 iterator_t<detail::maybe_const<Const, F>>> &&
                             equality_comparable<iterator_t<
                                 detail::maybe_const<Const, S>>>>* = nullptr>
        friend bool operator==(const iterator& x, const iterator& y)
        {
            if (x._is_first() != y._is_first()) {
                return false;
            }
            if (x._is_first()) {
                return x._get_first() == y._get_first();
            }
            return x._get_second() == y._get_second();
        }
        template <
            typename F = First,
            typename S = Second,
            std::enable_if_t<equality_comparable<
                                 iterator_t<detail::maybe_const<Const, F>>> &&
                             equality_comparable<iterator_t<
                                 detail::maybe_const<Const, S>>>>* = nullptr>
        friend bool operator!=(const iterator& x, const iterator& y)
        {
            return !(x == y);
        }

        friend bool operator==(const iterator& x, default_sentinel_t)
        {
            return !x._is_first() &&
                   x._get_second() == ranges::end(x.m_parent->m_second);
        }
        friend bool operator!=(const iterator& x, default_sentinel_t)
        {
            return !(x == default_sentinel);
        }

        // TODO: iter_move, iter_swap

    private:
        friend class pair_concat_view;
        friend class iterator<!Const>;

        using parent_type = detail::maybe_const<Const, pair_concat_view>;

        explicit iterator(detail::pair_concat_first_tag,
                          parent_type* p,
                          first_iterator f)
            : base(detail::pair_concat_first_tag{}, SCN_MOVE(f)), m_parent(p)
        {
        }
        explicit iterator(detail::pair_concat_second_tag,
                          parent_type* p,
                          second_iterator s)
            : base(detail::pair_concat_second_tag{}, SCN_MOVE(s)), m_parent(p)
        {
        }

        parent_type* m_parent{};
    };

    template <bool Const>
    friend class iterator;

public:
    using first_range_type = First;
    using second_range_type = Second;

    constexpr pair_concat_view() = default;

    constexpr explicit pair_concat_view(First first, Second second)
        : m_first(SCN_MOVE(first)), m_second(SCN_MOVE(second))
    {
    }

    template <typename F = First,
              typename S = Second,
              std::enable_if_t<!detail::simple_view<F> &&
                               !detail::simple_view<S>>* = nullptr>
    iterator<false> begin()
    {
        auto it = iterator<false>{detail::pair_concat_first_tag{}, this,
                                  ranges::begin(m_first)};
        it.template _satisfy<0>();
        return it;
    }

    template <
        typename F = First,
        typename S = Second,
        std::enable_if_t<range<const F> && range<const S> &&
                         std::is_same_v<range_value_t<const F>,
                                        range_value_t<const S>>>* = nullptr>
    iterator<true> begin() const
    {
        auto it = iterator<true>{detail::pair_concat_first_tag{}, this,
                                 ranges::begin(m_first)};
        it.template _satisfy<0>();
        return it;
    }

    template <typename F = First,
              typename S = Second,
              std::enable_if_t<!detail::simple_view<F> &&
                               !detail::simple_view<S>>* = nullptr>
    auto end()
    {
        if constexpr (common_range<Second>) {
            return iterator<false>{detail::pair_concat_second_tag{}, this,
                                   ranges::end(m_second)};
        }
        else {
            return default_sentinel;
        }
    }

    template <
        typename F = First,
        typename S = Second,
        std::enable_if_t<range<const F> && range<const S> &&
                         std::is_same_v<range_value_t<const F>,
                                        range_value_t<const S>>>* = nullptr>
    auto end() const
    {
        if constexpr (common_range<Second>) {
            return iterator<true>{detail::pair_concat_second_tag{}, this,
                                  ranges::end(m_second)};
        }
        else {
            return default_sentinel;
        }
    }

    template <typename F = First,
              typename S = Second,
              std::enable_if_t<sized_range<F> && sized_range<S>>* = nullptr>
    auto size()
    {
        return ranges::size(m_first) + ranges::size(m_second);
    }

    template <typename F = First,
              typename S = Second,
              std::enable_if_t<sized_range<const F> && sized_range<const S>>* =
                  nullptr>
    auto size() const
    {
        return ranges::size(m_first) + ranges::size(m_second);
    }

    First& _get_first()
    {
        return m_first;
    }

    Second& _get_second()
    {
        return m_second;
    }

private:
    First m_first;
    Second m_second;
};

template <typename First, typename Second>
pair_concat_view(First&&, Second&&)
    -> pair_concat_view<views::all_t<First>, views::all_t<Second>>;

namespace detail {
struct pair_concat_access {
    template <typename First, typename Second>
    static First& get_first(pair_concat_view<First, Second>& v)
    {
        return v.m_first;
    }

    template <typename Iterator>
    static bool is_iterator_first(const Iterator& it)
    {
        return it._is_first();
    }

    template <typename Iterator>
    static decltype(auto) get_iterator_first(Iterator& it)
    {
        return it._get_first();
    }
};
}  // namespace detail

namespace views {

namespace pair_concat_ {
struct fn {
    template <typename T,
              std::enable_if_t<input_range<T> && viewable_range<T>>* = nullptr>
    auto operator()(T&& t) const noexcept(noexcept(views::all(SCN_FWD(t))))
        -> decltype(views::all(SCN_FWD(t)))
    {
        return views::all(SCN_FWD(t));
    }

    template <
        typename F,
        typename S,
        std::enable_if_t<viewable_range<F> && viewable_range<S>>* = nullptr>
    auto operator()(F&& f, S&& s) const
        noexcept(noexcept(pair_concat_view(SCN_FWD(f), SCN_FWD(s))))
            -> decltype(pair_concat_view(SCN_FWD(f), SCN_FWD(s)))
    {
        return pair_concat_view(SCN_FWD(f), SCN_FWD(s));
    }
};
}  // namespace pair_concat_

inline constexpr auto pair_concat = pair_concat_::fn{};

}  // namespace views

}  // namespace ranges

namespace detail {

namespace char_t_fn {
template <typename T, typename = typename T::value_type>
constexpr auto impl_nonarray(priority_tag<2>)
{
    return tag_type<typename T::value_type>{};
}
template <typename T, typename = decltype(SCN_DECLVAL(T&).begin())>
constexpr auto impl_nonarray(priority_tag<1>)
{
    return tag_type<remove_cvref_t<decltype(*(SCN_DECLVAL(T&).begin()))>>{};
}
template <typename T>
constexpr auto impl_nonarray(priority_tag<0>)
{
    return tag_type<remove_cvref_t<decltype(*begin(SCN_DECLVAL(T&)))>>{};
}

template <typename T,
          typename = std::enable_if_t<ranges::range<remove_cvref_t<T>>>>
constexpr auto impl()
{
    using T_nocvref = remove_cvref_t<T>;
    if constexpr (std::is_array_v<T_nocvref>) {
        return tag_type<std::remove_all_extents_t<T_nocvref>>{};
    }
    else {
        return impl_nonarray<T_nocvref>(priority_tag<2>{});
    }
}

template <typename Range>
using result = decltype(impl<Range>());
}  // namespace char_t_fn

template <typename Range>
using char_t = typename char_t_fn::result<Range>::type;

template <typename R, bool Borrowed = ranges::borrowed_range<R>>
struct borrowed_tail_subrange {
    using type = ranges::subrange<ranges::iterator_t<R>, ranges::sentinel_t<R>>;
};
template <typename R>
struct borrowed_tail_subrange<R, false> {
    using type = ranges::dangling;
};

/// Equivalent to
/// `ranges::subrange<ranges::iterator_t<R>,
/// ranges::sentinel_t<R>>` if `R` is a `borrowed_range`, and
/// `ranges::dangling` otherwise.
///
/// Similar to `ranges::borrowed_subrange_t<R>`, expect this preserves
/// the range sentinel.
template <typename R>
using borrowed_tail_subrange_t = typename borrowed_tail_subrange<R>::type;

}  // namespace detail

namespace ranges {
template <typename CharT, typename Traits>
inline constexpr bool
    enable_borrowed_range<std::basic_string_view<CharT, Traits>> = true;
}

SCN_GCC_POP  // -Wnoexcept (for the entirety of the expected and ranges impls)

    // reset formatting
    namespace detail
{
}

/////////////////////////////////////////////////////////////////
// Small generic algorithms
/////////////////////////////////////////////////////////////////

namespace detail {

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
constexpr T min(T a, T b) noexcept
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

/////////////////////////////////////////////////////////////////
// Errors
/////////////////////////////////////////////////////////////////

/**
 * Error class.
 * Used as a return value for functions without a success value.
 * Doesn't have a success state, and isn't default constructible:
 * use `expected<void, scan_error>` to achieve that.
 *
 * \ingroup result
 */
class SCN_TRIVIAL_ABI scan_error {
public:
    /// Error code
    enum code {
        /// Input ended unexpectedly.
        end_of_input,

        /// Format string was invalid.
        /// Often a compile-time error, if supported and/or enabled.
        invalid_format_string,

        /// Scanned value was invalid for given type,
        /// or a value of the given couldn't be scanned.
        invalid_scanned_value,

        /// Literal character specified in format string not found in source.
        invalid_literal,

        /// Too many fill characters scanned,
        /// field precision (max width) exceeded.
        invalid_fill,

        /// Scanned field width was shorter than
        /// what was specified as the minimum field width.
        length_too_short,

        /// Source range is in an invalid state,
        /// failed to continue reading.
        invalid_source_state,

        /// Value out of range, too large (higher than the maximum value)
        /// i.e. >2^32 for int32
        value_positive_overflow,

        /// Value out of range, too small (lower than the minimum value)
        /// i.e. <2^32 for int32
        value_negative_overflow,

        /// Value out of range, magnitude too small, sign +
        /// (between 0 and the smallest subnormal float)
        value_positive_underflow,

        /// Value out of range, magnitude too small, sign -
        /// (between 0 and the smallest subnormal float)
        value_negative_underflow,

        /// Value of this type can't be parsed,
        /// either from this source or not at all.
        type_not_supported,

        max_error
    };

private:
    using code_t = code;

public:
    /// Constructs an error with `c` and `m`
    constexpr scan_error(code_t c, const char* m) noexcept : m_msg(m), m_code(c)
    {
        SCN_UNLIKELY_ATTR SCN_UNUSED(m_code);
    }

    constexpr explicit operator code_t() const noexcept
    {
        return m_code;
    }

    /// Get error code
    SCN_NODISCARD constexpr code_t code() const noexcept
    {
        return m_code;
    }
    /// Get error message
    SCN_NODISCARD constexpr auto msg() const noexcept -> const char*
    {
        return m_msg;
    }

    /// Convert to a `std::errc`.
    SCN_NODISCARD constexpr std::errc to_errc() const noexcept
    {
        switch (m_code) {
            case end_of_input:
            case invalid_format_string:
            case invalid_scanned_value:
            case invalid_literal:
            case invalid_fill:
            case length_too_short:
            case type_not_supported:
                return std::errc::invalid_argument;
            case invalid_source_state:
                return std::errc::io_error;
            case value_positive_overflow:
            case value_negative_overflow:
            case value_positive_underflow:
            case value_negative_underflow:
                return std::errc::result_out_of_range;
            case max_error:
                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")
            default:
#if !SCN_GCC || SCN_GCC >= SCN_COMPILER(9, 0, 0)
                // gcc 7 thinks we'll get here, even when we won't.
                // gcc 8 has a bug in debug mode:
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86678
                SCN_EXPECT(false);
#endif
                SCN_UNREACHABLE;
                SCN_CLANG_POP
        }
    }

private:
    const char* m_msg;
    code_t m_code;
};

constexpr bool operator==(scan_error a, scan_error b) noexcept
{
    return a.code() == b.code();
}
constexpr bool operator!=(scan_error a, scan_error b) noexcept
{
    return !(a == b);
}

constexpr bool operator==(scan_error a, enum scan_error::code b) noexcept
{
    return a.code() == b;
}
constexpr bool operator!=(scan_error a, enum scan_error::code b) noexcept
{
    return !(a == b);
}

constexpr bool operator==(enum scan_error::code a, scan_error b) noexcept
{
    return a == b.code();
}
constexpr bool operator!=(enum scan_error::code a, scan_error b) noexcept
{
    return !(a == b);
}

namespace detail {
// Intentionally not constexpr, to give out a compile-time error
SCN_PUBLIC SCN_COLD scan_error handle_error(scan_error e);
}  // namespace detail

#if SCN_HAS_EXCEPTIONS

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wweak-vtables")

namespace detail {

class scan_format_string_error_base : public std::runtime_error {
public:
    explicit scan_format_string_error_base(const std::string& what_arg)
        : runtime_error(what_arg)
    {
    }

    explicit scan_format_string_error_base(std::false_type,
                                           const char* what_arg)
        : runtime_error(what_arg)
    {
    }

    explicit scan_format_string_error_base(std::true_type, const char* what_arg)
        : runtime_error(what_arg), m_internal_literal_msg(what_arg)
    {
    }

    // Doing everything in a `detail::` base class
    // to make this a `friend` inside that namespace
    // (essentially private)
    friend const char* get_internal_literal_msg(
        const scan_format_string_error_base& m)
    {
        return m.m_internal_literal_msg;
    }

private:
    const char* m_internal_literal_msg{nullptr};
};

}  // namespace detail

/**
 * An exception type used to report format string parsing errors.
 */
class scan_format_string_error : public detail::scan_format_string_error_base {
public:
    // Not `using` constructors to document them explicitly.

    /**
     * Construct from a `std::string`.
     */
    explicit scan_format_string_error(const std::string& what_arg)
        : scan_format_string_error_base(what_arg)
    {
    }

    /**
     * Construct from a `const char*`.
     */
    explicit scan_format_string_error(const char* what_arg)
        : scan_format_string_error_base(std::false_type{}, what_arg)
    {
    }

    /**
     * Construct from a string literal.
     */
    template <std::size_t N>
    explicit scan_format_string_error(const char (&what_arg)[N])
        : scan_format_string_error_base(std::true_type{}, what_arg)
    {
    }
};

SCN_CLANG_POP

#endif

namespace detail {
}

/**
 * An `expected<T, scan_error>`.
 *
 * Not a type alias to shorten template names.
 *
 * \ingroup result
 */
template <typename T>
struct scan_expected : public expected<T, scan_error> {
    using expected<T, scan_error>::expected;

    scan_expected(const expected<T, scan_error>& other)
        : expected<T, scan_error>(other)
    {
    }
    scan_expected(expected<T, scan_error>&& other)
        : expected<T, scan_error>(SCN_MOVE(other))
    {
    }
};

namespace detail {
constexpr auto unexpected_scan_error(enum scan_error::code c, const char* m)
{
    return unexpected(scan_error{c, m});
}

template <typename T>
struct is_expected_impl<scan_expected<T>> : std::true_type {};
}  // namespace detail

#define SCN_TRY_IMPL_CONCAT(a, b)  a##b
#define SCN_TRY_IMPL_CONCAT2(a, b) SCN_TRY_IMPL_CONCAT(a, b)
#define SCN_TRY_TMP                SCN_TRY_IMPL_CONCAT2(_scn_try_tmp_, __LINE__)

#define SCN_TRY_DISCARD(x)                                          \
    do {                                                            \
        if (auto&& SCN_TRY_TMP = (x); SCN_UNLIKELY(!SCN_TRY_TMP)) { \
            return ::scn::unexpected(SCN_TRY_TMP.error());          \
        }                                                           \
    } while (false)

#define SCN_TRY_ASSIGN(init, x)                        \
    auto&& SCN_TRY_TMP = (x);                          \
    if (SCN_UNLIKELY(!SCN_TRY_TMP)) {                  \
        return ::scn::unexpected(SCN_TRY_TMP.error()); \
    }                                                  \
    init = *SCN_FWD(SCN_TRY_TMP)
#define SCN_TRY(name, x) SCN_TRY_ASSIGN(auto name, x)

/////////////////////////////////////////////////////////////////
// string_view utilities
/////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////
// Lightweight Unicode facilities
/////////////////////////////////////////////////////////////////

constexpr inline bool is_ascii_code_point(char32_t cp)
{
    return cp <= 0x7f;
}

template <typename U8>
constexpr std::size_t utf8_code_point_length_by_starting_code_unit(U8 ch)
{
    static_assert(sizeof(U8) == 1);

    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wsign-conversion")
    constexpr char lengths[] =
        "\1\1\1\1\1\1\1\1"  // highest bit is 0 -> single-byte
        "\1\1\1\1\1\1\1\1"
        "\0\0\0\0\0\0\0\0"  // highest bits 10 -> error, non-initial
                            // byte
        "\2\2\2\2"          // highest bits 110 -> 2-byte cp
        "\3\3"              // highest bits 1110 -> 3-byte cp
        "\4";               // highest bits 11110 -> 4-byte cp
    return lengths[static_cast<unsigned char>(ch) >> 3];
    SCN_GCC_COMPAT_POP
}

template <typename U16>
constexpr std::size_t utf16_code_point_length_by_starting_code_unit(U16 ch)
{
    static_assert(sizeof(U16) == 2);

    const auto lead = static_cast<uint16_t>(0xffff & ch);
    if (lead >= 0xd800 && lead <= 0xdbff) {
        // high surrogate
        return 2;
    }
    if (lead >= 0xdc00 && lead <= 0xdfff) {
        // unpaired low surrogate
        return 0;
    }
    return 1;
}

template <typename U>
constexpr std::size_t code_point_length_by_starting_code_unit(U ch)
{
    if constexpr (sizeof(U) == 1) {
        return utf8_code_point_length_by_starting_code_unit(ch);
    }
    else if constexpr (sizeof(U) == 2) {
        return utf16_code_point_length_by_starting_code_unit(ch);
    }
    else {
        // utf-32
        static_assert(sizeof(U) == 4);
        SCN_UNUSED(ch);
        return 1;
    }
}

inline constexpr char32_t invalid_code_point = 0x110000;

inline constexpr char32_t decode_utf8_code_point_exhaustive(
    std::string_view input)
{
    SCN_EXPECT(!input.empty() && input.size() <= 4);

    const auto is_trailing_code_unit = [](char ch) {
        return static_cast<unsigned char>(ch) >> 6 == 0x2;
    };

    if (input.size() == 1) {
        if (static_cast<unsigned char>(input[0]) >= 0x80) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        return static_cast<char32_t>(input[0]);
    }

    if (input.size() == 2) {
        if ((static_cast<unsigned char>(input[0]) & 0xe0) != 0xc0) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (!is_trailing_code_unit(input[1])) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x1f) << 6;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 3) {
        if ((static_cast<unsigned char>(input[0]) & 0xf0) != 0xe0) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (!is_trailing_code_unit(input[1]) ||
            !is_trailing_code_unit(input[2])) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x0f) << 12;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 4) {
        if ((static_cast<unsigned char>(input[0]) & 0xf8) != 0xf0) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (static_cast<unsigned char>(input[0]) > 0xf4) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }
        if (!is_trailing_code_unit(input[1]) ||
            !is_trailing_code_unit(input[2]) ||
            !is_trailing_code_unit(input[3])) {
            SCN_UNLIKELY_ATTR
            return invalid_code_point;
        }

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x07) << 18;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 12;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[3]) & 0x3f) << 0;
        return cp;
    }

#if !SCN_GCC || SCN_GCC >= SCN_COMPILER(9, 0, 0)
    // gcc 7 thinks we'll get here, even when we won't
    // gcc 8 has a bug in debug mode:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86678
    SCN_EXPECT(false);
#endif
    SCN_UNREACHABLE;
}

inline constexpr char32_t decode_utf8_code_point_exhaustive_valid(
    std::string_view input)
{
    SCN_EXPECT(!input.empty() && input.size() <= 4);

    const auto is_trailing_code_unit = [](char ch) {
        return static_cast<unsigned char>(ch) >> 6 == 0x2;
    };

    if (input.size() == 1) {
        SCN_EXPECT(static_cast<unsigned char>(input[0]) < 0x80);
        return static_cast<char32_t>(input[0]);
    }

    if (input.size() == 2) {
        SCN_EXPECT((static_cast<unsigned char>(input[0]) & 0xe0) == 0xc0);
        SCN_EXPECT(is_trailing_code_unit(input[1]));

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x1f) << 6;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 3) {
        SCN_EXPECT((static_cast<unsigned char>(input[0]) & 0xf0) == 0xe0);
        SCN_EXPECT(is_trailing_code_unit(input[1]));
        SCN_EXPECT(is_trailing_code_unit(input[2]));

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x0f) << 12;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 0;
        return cp;
    }

    if (input.size() == 4) {
        SCN_EXPECT((static_cast<unsigned char>(input[0]) & 0xf8) == 0xf0);
        SCN_EXPECT(static_cast<unsigned char>(input[0]) <= 0xf4);
        SCN_EXPECT(is_trailing_code_unit(input[1]));
        SCN_EXPECT(is_trailing_code_unit(input[2]));
        SCN_EXPECT(is_trailing_code_unit(input[3]));

        char32_t cp{};
        cp |= (static_cast<char32_t>(input[0]) & 0x07) << 18;
        cp |= (static_cast<char32_t>(input[1]) & 0x3f) << 12;
        cp |= (static_cast<char32_t>(input[2]) & 0x3f) << 6;
        cp |= (static_cast<char32_t>(input[3]) & 0x3f) << 0;
        return cp;
    }

#if !SCN_GCC || SCN_GCC >= SCN_COMPILER(9, 0, 0)
    // gcc 7 thinks we'll get here, even when we won't
    // gcc 8 has a bug in debug mode:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86678
    SCN_EXPECT(false);
#endif
    SCN_UNREACHABLE;
}

template <typename CharT>
inline constexpr char32_t decode_utf16_code_point_exhaustive(
    std::basic_string_view<CharT> input)
{
    static_assert(sizeof(CharT) == 2);

    SCN_EXPECT(!input.empty() && input.size() <= 2);

    if (input.size() == 1) {
        return static_cast<char32_t>(input[0]);
    }

    const auto lead = static_cast<uint32_t>(input[0]);
    const auto trail = static_cast<uint32_t>(input[1]);
    if (lead < 0xd800 || lead > 0xdbff || trail < 0xdc00 || trail > 0xdfff) {
        SCN_UNLIKELY_ATTR
        return invalid_code_point;
    }

    const auto cp = ((lead - 0xd800) << 10) | (trail - 0xdc00);
    return static_cast<char32_t>(cp + 0x10000);
}

template <typename CharT>
inline constexpr char32_t decode_utf16_code_point_exhaustive_valid(
    std::basic_string_view<CharT> input)
{
    static_assert(sizeof(CharT) == 2);

    SCN_EXPECT(!input.empty() && input.size() <= 2);

    if (input.size() == 1) {
        return static_cast<char32_t>(input[0]);
    }

    const auto lead = static_cast<uint32_t>(input[0]);
    const auto trail = static_cast<uint32_t>(input[1]);
    SCN_EXPECT(lead >= 0xd800);
    SCN_EXPECT(lead <= 0xdbff);
    SCN_EXPECT(trail >= 0xdc00);
    SCN_EXPECT(trail <= 0xdfff);

    const auto cp = ((lead - 0xd800) << 10) | (trail - 0xdc00);
    return static_cast<char32_t>(cp + 0x10000);
}

template <typename CharT>
inline constexpr char32_t decode_code_point_exhaustive(
    std::basic_string_view<CharT> input)
{
    if constexpr (sizeof(CharT) == 1) {
        return decode_utf8_code_point_exhaustive(input);
    }
    else if constexpr (sizeof(CharT) == 2) {
        return decode_utf16_code_point_exhaustive(input);
    }
    else {
        SCN_EXPECT(input.size() == 1);
        auto cp = static_cast<char32_t>(input.front());
        if (SCN_UNLIKELY(cp >= invalid_code_point)) {
            return invalid_code_point;
        }
        return cp;
    }
}

template <typename CharT>
inline constexpr char32_t decode_code_point_exhaustive_valid(
    std::basic_string_view<CharT> input)
{
    if constexpr (sizeof(CharT) == 1) {
        return decode_utf8_code_point_exhaustive_valid(input);
    }
    else if constexpr (sizeof(CharT) == 2) {
        return decode_utf16_code_point_exhaustive_valid(input);
    }
    else {
        SCN_EXPECT(input.size() == 1);
        return static_cast<char32_t>(input.front());
    }
}

inline constexpr bool is_cp_space(char32_t cp) noexcept
{
    // Pattern_White_Space property
    return (cp >= 0x09 && cp <= 0x0d) ||
           cp == 0x20 ||    // ASCII space characters
           cp == 0x85 ||    // NEXT LINE (NEL)
           cp == 0x200e ||  // LEFT-TO-RIGHT MARK
           cp == 0x200f ||  // RIGHT-TO-LEFT MARK
           cp == 0x2028 ||  // LINE SEPARATOR
           cp == 0x2029;    // PARAGRAPH SEPARATOR
}

}  // namespace detail

/////////////////////////////////////////////////////////////////
// scan_buffer
/////////////////////////////////////////////////////////////////

namespace detail {
struct scan_file_access;
}

class scan_file {
    friend struct detail::scan_file_access;

public:
    explicit scan_file(std::FILE* file) noexcept : m_file(file) {}

    ~scan_file() = default;

    scan_file(const scan_file&) = delete;
    scan_file& operator=(const scan_file&) = delete;

    scan_file(scan_file&& other) noexcept
        : m_prelude(std::move(other.m_prelude)), m_file(other.m_file)
    {
        other.m_prelude = {};
        other.m_file = nullptr;
    }
    scan_file& operator=(scan_file&& other) noexcept
    {
        m_prelude = std::move(other.m_prelude);
        m_file = other.m_file;
        other.m_prelude = {};
        other.m_file = nullptr;
        return *this;
    }

    SCN_NODISCARD std::optional<std::FILE*> handle() const
    {
        if (m_prelude.empty()) {
            SCN_EXPECT(m_file);
            return m_file;
        }
        return std::nullopt;
    }

    SCN_NODISCARD std::string_view prelude() const
    {
        return m_prelude;
    }

    SCN_NODISCARD std::pair<std::string_view, std::FILE*> contents() const
    {
        return {m_prelude, m_file};
    }

private:
    std::string m_prelude{};
    std::FILE* m_file{nullptr};
};

namespace detail {

struct scan_file_access {
    static std::FILE* get_handle(scan_file& f)
    {
        SCN_EXPECT(f.m_file != nullptr);
        return f.m_file;
    }

    static std::string& get_prelude(scan_file& f)
    {
        return f.m_prelude;
    }
};

template <typename CharT>
class basic_scan_buffer {
public:
    class forward_iterator;

    using char_type = CharT;
    using range_type =
        ranges::subrange<forward_iterator, ranges::default_sentinel_t>;

    basic_scan_buffer(const basic_scan_buffer&) = delete;
    basic_scan_buffer& operator=(const basic_scan_buffer&) = delete;
    basic_scan_buffer(basic_scan_buffer&&) = delete;
    basic_scan_buffer& operator=(basic_scan_buffer&&) = delete;

    virtual ~basic_scan_buffer() = default;

    virtual bool fill() = 0;

    virtual bool sync(std::ptrdiff_t position)
    {
        SCN_UNUSED(position);
        return true;
    }

    SCN_NODISCARD std::ptrdiff_t chars_available() const
    {
        return static_cast<std::ptrdiff_t>(m_putback_buffer.size() +
                                           m_current_view.size());
    }

    SCN_NODISCARD std::basic_string_view<CharT> current_view() const
    {
        return m_current_view;
    }

    SCN_NODISCARD std::basic_string<CharT>& putback_buffer()
    {
        return m_putback_buffer;
    }
    SCN_NODISCARD std::basic_string_view<CharT> putback_buffer() const
    {
        return m_putback_buffer;
    }

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Warray-bounds")

    SCN_NODISCARD std::basic_string_view<CharT> get_segment_starting_at(
        std::ptrdiff_t pos) const
    {
        SCN_EXPECT(pos >= 0);
        const auto upos = static_cast<std::size_t>(pos);
        if (SCN_UNLIKELY(upos < m_putback_buffer.size())) {
            return std::basic_string_view<CharT>(m_putback_buffer).substr(upos);
        }
        const auto start = upos - m_putback_buffer.size();
        SCN_EXPECT(start <= m_current_view.size());
        return m_current_view.substr(start);
    }

    SCN_NODISCARD CharT get_character_at(std::ptrdiff_t pos) const
    {
        SCN_EXPECT(pos >= 0);
        const auto upos = static_cast<std::size_t>(pos);
        if (SCN_UNLIKELY(upos < m_putback_buffer.size())) {
            return m_putback_buffer[upos];
        }
        const auto start = upos - m_putback_buffer.size();
        SCN_EXPECT(start < m_current_view.size());
        return m_current_view[start];
    }

    SCN_GCC_POP

    SCN_NODISCARD bool is_contiguous() const
    {
        return m_is_contiguous;
    }

    SCN_NODISCARD auto get_contiguous() const
    {
        SCN_EXPECT(is_contiguous());
        return ranges::subrange<const CharT*>{
            current_view().data(),
            current_view().data() + current_view().size()};
    }

    SCN_NODISCARD range_type get();

    SCN_NODISCARD scan_expected<void> get_source_error() const
    {
        return m_source_error;
    }

    void set_skip_whitespace(bool skip)
    {
        m_skip_whitespace = skip;
    }
    SCN_NODISCARD bool get_skip_whitespace() const
    {
        return m_skip_whitespace;
    }

protected:
    friend class forward_iterator;

    struct contiguous_tag {};
    struct non_contiguous_tag {};

    basic_scan_buffer(contiguous_tag, std::basic_string_view<char_type> sv)
        : m_current_view(sv), m_is_contiguous(true)
    {
    }

    basic_scan_buffer(non_contiguous_tag,
                      std::basic_string_view<char_type> sv = {})
        : m_current_view(sv), m_is_contiguous(false)
    {
    }

    basic_scan_buffer(bool is_contiguous, std::basic_string_view<char_type> sv)
        : m_current_view(sv), m_is_contiguous(is_contiguous)
    {
    }

    std::basic_string_view<char_type> m_current_view{};
    std::basic_string<char_type> m_putback_buffer{};
    scan_expected<void> m_source_error{};
    bool m_is_contiguous{false};
    bool m_skip_whitespace{false};
};

template <typename CharT>
class basic_scan_buffer<CharT>::forward_iterator {
public:
    using value_type = CharT;
    using reference = CharT;
    using pointer = CharT*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    forward_iterator() = default;

    forward_iterator(basic_scan_buffer<CharT>* parent, std::ptrdiff_t pos)
        : m_parent(parent), m_end(nullptr), m_position(pos)
    {
        SCN_EXPECT(parent);
        SCN_EXPECT(!parent->is_contiguous());
    }

    forward_iterator(std::basic_string_view<CharT> view, std::ptrdiff_t pos)
        : m_begin(const_cast<CharT*>(view.data())),
          m_end(const_cast<CharT*>(view.data() + view.size())),
          m_position(pos)
    {
    }

    std::ptrdiff_t position() const
    {
        return m_position;
    }

    bool stores_parent() const
    {
        return m_end == nullptr;
    }

    basic_scan_buffer<CharT>* parent()
    {
        SCN_EXPECT(stores_parent());
        SCN_EXPECT(m_parent);
        return m_parent;
    }
    const basic_scan_buffer<CharT>* parent() const
    {
        SCN_EXPECT(stores_parent());
        SCN_EXPECT(m_parent);
        return m_parent;
    }

    std::basic_string_view<CharT> contiguous_segment() const
    {
        if (!stores_parent()) {
            return make_string_view_from_pointers(m_begin + position(), m_end);
        }
        return parent()->get_segment_starting_at(position());
    }
    auto to_contiguous_segment_iterator() const
    {
        return contiguous_segment().data();
    }

    forward_iterator& operator++()
    {
        ++m_position;
        return *this;
    }

    forward_iterator operator++(int)
    {
        auto copy = *this;
        operator++();
        return copy;
    }

    CharT operator*() const
    {
        if (!stores_parent()) {
            SCN_EXPECT(m_begin);
            auto ptr = m_begin + position();
            SCN_EXPECT(ptr != m_end);
            return *ptr;
        }

        SCN_EXPECT(m_parent);
        auto res = read_at_position();
        SCN_EXPECT(res);
        return parent()->get_character_at(m_position);
    }

    forward_iterator& batch_advance(std::ptrdiff_t n)
    {
        SCN_EXPECT(n >= 0);
        m_position += n;
        return *this;
    }

    forward_iterator& batch_advance_to(std::ptrdiff_t i)
    {
        SCN_EXPECT(i >= m_position);
        m_position = i;
        return *this;
    }

    friend bool operator==(const forward_iterator& lhs,
                           const forward_iterator& rhs)
    {
        (void)lhs.read_at_position();
        (void)rhs.read_at_position();
        return lhs.erased_data_ptr() == rhs.erased_data_ptr() &&
               lhs.m_position == rhs.m_position;
    }
    friend bool operator!=(const forward_iterator& lhs,
                           const forward_iterator& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator==(const forward_iterator& x,
                           ranges::default_sentinel_t)
    {
        return x.is_at_end();
    }
    friend bool operator==(ranges::default_sentinel_t,
                           const forward_iterator& x)
    {
        return x.is_at_end();
    }

    friend bool operator!=(const forward_iterator& x,
                           ranges::default_sentinel_t)
    {
        return !x.is_at_end();
    }
    friend bool operator!=(ranges::default_sentinel_t,
                           const forward_iterator& x)
    {
        return !x.is_at_end();
    }

private:
    friend class basic_scan_buffer<CharT>;

    SCN_NODISCARD bool read_at_position() const
    {
        if (!stores_parent()) {
            SCN_EXPECT(m_begin);
            return true;
        }

        SCN_EXPECT(m_parent);
        if (SCN_LIKELY(m_position < parent()->chars_available())) {
            return true;
        }

        while (m_position >= parent()->chars_available()) {
            if (!const_cast<basic_scan_buffer<CharT>*>(parent())->fill()) {
                return false;
            }
        }
        return true;
    }

    SCN_NODISCARD bool is_at_end() const
    {
        if (m_end) {
            SCN_EXPECT(m_begin);
            return (m_begin + position()) == m_end;
        }
        if (!m_parent) {
            return true;
        }
        return !read_at_position();
    }

    const void* erased_data_ptr() const
    {
        if (m_end) {
            return m_begin;
        }
        return m_parent;
    }

    // If m_end is null, m_parent points to the parent scan_buffer
    // Otherwise, [m_begin, m_end) is the range of this iterator (and of
    // the entire range)
    union {
        mutable CharT* m_begin;
        mutable basic_scan_buffer<CharT>* m_parent{nullptr};
    };
    mutable CharT* m_end{nullptr};
    std::ptrdiff_t m_position{0};
};

template <typename CharT>
SCN_NODISCARD auto basic_scan_buffer<CharT>::get() -> range_type
{
    if (is_contiguous()) {
        return ranges::subrange{forward_iterator{m_current_view, 0},
                                ranges::default_sentinel};
    }
    return ranges::subrange{forward_iterator{this, 0},
                            ranges::default_sentinel};
}

static_assert(ranges::forward_range<scan_buffer::range_type>);

template <typename CharT>
class basic_scan_string_buffer : public basic_scan_buffer<CharT> {
    using base = basic_scan_buffer<CharT>;

public:
    basic_scan_string_buffer(std::basic_string_view<CharT> sv)
        : base(typename base::contiguous_tag{}, sv)
    {
    }

    bool fill() override
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
};

template <typename CharT>
basic_scan_string_buffer(std::basic_string_view<CharT>)
    -> basic_scan_string_buffer<CharT>;

template <typename I, typename S>
using less_than_compare =
    decltype(SCN_DECLVAL(const I&) < SCN_DECLVAL(const S&));

template <typename Range>
class basic_scan_forward_range_buffer
    : public basic_scan_buffer<detail::char_t<Range>> {
    static_assert(ranges::forward_range<const Range> &&
                  std::is_object_v<Range>);

    using _char_type = detail::char_t<Range>;
    using base = basic_scan_buffer<_char_type>;

public:
    using char_type = _char_type;
    using range_type = Range;
    using iterator = ranges::iterator_t<const Range>;
    using sentinel = ranges::sentinel_t<const Range>;

    basic_scan_forward_range_buffer(const Range& r)
        : base(typename base::non_contiguous_tag{}),
          m_range(&r),
          m_cursor(ranges::begin(*m_range))
    {
    }

    bool fill() override
    {
        if (m_cursor == ranges::end(*m_range)) {
            return false;
        }
        if constexpr (mp_valid_v<less_than_compare, iterator, sentinel>) {
            SCN_EXPECT(m_cursor < ranges::end(*m_range));
        }
        if (!this->m_current_view.empty()) {
            this->m_putback_buffer.insert(this->m_putback_buffer.end(),
                                          this->m_current_view.begin(),
                                          this->m_current_view.end());
        }
        m_latest = *m_cursor;
        ++m_cursor;
        this->m_current_view = std::basic_string_view<char_type>{&m_latest, 1};
        if constexpr (mp_valid_v<less_than_compare, iterator, sentinel>) {
            SCN_EXPECT(m_cursor <= ranges::end(*m_range));
        }
        return true;
    }

private:
    const Range* m_range;
    iterator m_cursor;
    char_type m_latest{};
};

template <typename R>
basic_scan_forward_range_buffer(const R&) -> basic_scan_forward_range_buffer<R>;

template <typename T>
struct move_only_wrapper {
    explicit move_only_wrapper(T&& val) : value(SCN_MOVE(val)) {}

    move_only_wrapper(const move_only_wrapper&) = delete;
    move_only_wrapper& operator=(const move_only_wrapper&) = delete;

    move_only_wrapper(move_only_wrapper&&) = default;
    move_only_wrapper& operator=(move_only_wrapper&&) = default;

    ~move_only_wrapper() = default;

    T value;
};

template <typename CharT>
inline void set_prelude_after_sync(std::basic_string<CharT>& prelude,
                                   std::ptrdiff_t expected_position,
                                   std::ptrdiff_t synced_position,
                                   std::basic_string_view<CharT>& current_view,
                                   std::basic_string<CharT>& putback_buffer)
{
    SCN_EXPECT(synced_position > expected_position);
    const auto n_needed = synced_position - expected_position;

    prelude.clear();
    prelude.reserve(static_cast<std::size_t>(n_needed));

    const auto n_from_current_view =
        std::min(n_needed, static_cast<std::ptrdiff_t>(current_view.size()));
    const auto n_from_putback_buffer =
        std::min(n_needed - n_from_current_view,
                 static_cast<std::ptrdiff_t>(putback_buffer.size()));
    SCN_EXPECT(n_from_putback_buffer + n_from_current_view == n_needed);

    prelude.append(putback_buffer.end() - n_from_putback_buffer,
                   putback_buffer.end());
    prelude.append(current_view.end() - n_from_current_view,
                   current_view.end());
}

template <typename Range>
class basic_scan_input_range_buffer
    : public basic_scan_buffer<detail::char_t<Range>> {
    static_assert(ranges::input_range<Range> && !ranges::forward_range<Range>);

    using _char_type = detail::char_t<Range>;
    using base = basic_scan_buffer<_char_type>;

public:
    using char_type = _char_type;
    using range_type = Range;
    using iterator = ranges::iterator_t<Range>;
    using sentinel = ranges::sentinel_t<Range>;

    template <typename R,
              std::enable_if_t<is_not_self<R, basic_scan_input_range_buffer>>* =
                  nullptr>
    basic_scan_input_range_buffer(R&& r)
        : base(typename base::non_contiguous_tag{}),
          m_range(SCN_FWD(r)),
          m_cursor(ranges::begin(m_range))
    {
    }

    template <typename R,
              std::enable_if_t<is_not_self<R, basic_scan_input_range_buffer>>* =
                  nullptr>
    basic_scan_input_range_buffer(R&& r,
                                  std::basic_string_view<char_type> prelude)
        : base(typename base::non_contiguous_tag{}, prelude),
          m_range(SCN_FWD(r)),
          m_cursor(ranges::begin(m_range))
    {
    }

    iterator& get_iterator()
    {
        return m_cursor.value;
    }

    std::basic_string<char_type>& get_prelude()
    {
        return m_prelude;
    }

    bool fill() override
    {
        if (m_cursor.value == ranges::end(m_range)) {
            return false;
        }
        if constexpr (mp_valid_v<less_than_compare, iterator, sentinel>) {
            SCN_EXPECT(m_cursor.value < ranges::end(m_range));
        }
        if (!this->m_current_view.empty()) {
            this->m_putback_buffer.insert(this->m_putback_buffer.end(),
                                          this->m_current_view.begin(),
                                          this->m_current_view.end());
        }
        m_latest = *m_cursor.value;
        ++m_cursor.value;
        this->m_current_view = std::basic_string_view<char_type>{&m_latest, 1};
        if constexpr (mp_valid_v<less_than_compare, iterator, sentinel>) {
            SCN_EXPECT(m_cursor.value <= ranges::end(m_range));
        }
        return true;
    }

    bool sync(std::ptrdiff_t position) override
    {
        if (position != this->chars_available()) {
            detail::set_prelude_after_sync(
                m_prelude, position, this->chars_available(),
                this->m_current_view, this->m_putback_buffer);
        }
        return true;
    }

private:
    std::basic_string<char_type> m_prelude{};
    Range m_range;
    move_only_wrapper<iterator> m_cursor;
    char_type m_latest{};
};

template <typename R>
basic_scan_input_range_buffer(R&) -> basic_scan_input_range_buffer<R&>;
template <typename R, std::enable_if_t<!std::is_reference_v<R>>* = nullptr>
basic_scan_input_range_buffer(R&&) -> basic_scan_input_range_buffer<R>;

class scan_cfile_buffer : public basic_scan_buffer<char> {
    using base = basic_scan_buffer<char>;

public:
    SCN_PUBLIC explicit scan_cfile_buffer(std::FILE* file);
    SCN_PUBLIC ~scan_cfile_buffer() override;

    SCN_PUBLIC bool fill() override;

    SCN_PUBLIC bool sync(std::ptrdiff_t position) override;

protected:
    std::FILE* m_file;
    std::optional<char> m_latest{std::nullopt};
};

class scan_file_buffer : public scan_cfile_buffer {
    using base = scan_cfile_buffer;

public:
    SCN_PUBLIC explicit scan_file_buffer(scan_file& file);
    SCN_PUBLIC ~scan_file_buffer() override;

    SCN_PUBLIC bool fill() override;

    SCN_PUBLIC bool sync(std::ptrdiff_t position) override;

private:
    std::string& m_prelude;
};

template <typename Range>
auto make_string_scan_buffer(const Range& range)
{
    return basic_scan_string_buffer(std::basic_string_view<char_t<Range>>{
        ranges::data(range), ranges::size(range)});
}

template <typename Range>
auto make_range_scan_buffer(Range&& range)
{
    if constexpr (ranges::forward_range<Range>) {
        return basic_scan_forward_range_buffer(range);
    }
    else {
        return basic_scan_input_range_buffer(SCN_FWD(range));
    }
}

}  // namespace detail

/////////////////////////////////////////////////////////////////
// make_scan_buffer
/////////////////////////////////////////////////////////////////

/**
 * \defgroup scannable Scannable sources
 *
 * \brief Description of the `scannable_range` and `scannable_source`
 * concepts.
 *
 * A range is considered scannable, if it models at least `forward_range`,
 * and its character type is correct (its value type is the same as the one
 * of the format string).
 * If the range additionally models `contiguous_range` and `sized_range`,
 * additional optimizations are enabled.
 *
 * \code{.cpp}
 * // Exposition only
 * template <typename Range, typename CharT>
 * concept scannable_range =
 *     ranges::forward_range<Range> &&
 *     std::same_as<ranges::range_value_t<Range>, CharT>;
 * \endcode
 *
 * Additionally, files (`std::FILE*`) can be scanned from.
 * Files are always considered to be narrow (`char`-oriented).
 * Thus, the entire concept is:
 *
 * \code{.cpp}
 * // Exposition only
 * template <typename Source, typename CharT>
 * concept scannable_source =
 *   (std::same_as<std::remove_cvref_t<Source>, std::FILE*> &&
 *    std::same_as<CharT, char>) ||
 *   scannable_range<Source, CharT>;
 * \endcode
 */

/**
 * Tag type to indicate an invalid range given to `scn::scan`
 *
 * \ingroup scannable
 */
struct invalid_input_range {};

struct invalid_char_type : invalid_input_range {};
struct custom_char_traits : invalid_input_range {};
struct insufficient_range : invalid_input_range {};

namespace detail {
template <typename CharT>
inline constexpr bool is_valid_char_type =
    std::is_same_v<std::remove_const_t<CharT>, char> ||
    std::is_same_v<std::remove_const_t<CharT>, wchar_t>;

struct make_scan_buffer_tag {};

namespace _make_scan_buffer {

// buffer range -> self
inline decltype(auto) impl(scan_buffer::range_type& r, priority_tag<5>) noexcept
{
    return r;
}
inline decltype(auto) impl(wscan_buffer::range_type& r,
                           priority_tag<5>) noexcept
{
    return r;
}

// customization point
template <typename Source>
auto impl(Source&& s, priority_tag<4>) noexcept(
    noexcept(make_scan_buffer(SCN_FWD(s), make_scan_buffer_tag{})))
    -> decltype(make_scan_buffer(SCN_FWD(s), make_scan_buffer_tag{}))
{
    return make_scan_buffer(SCN_FWD(s), make_scan_buffer_tag{});
}

// string_view -> string_buffer
template <typename CharT>
auto impl(std::basic_string_view<CharT> r, priority_tag<3>) noexcept
{
    if constexpr (is_valid_char_type<CharT>) {
        return r;
        // return make_string_scan_buffer(r);
    }
    else {
        return invalid_char_type{};
    }
}

// string& -> string_buffer
template <typename CharT, typename Traits, typename Allocator>
auto impl(const std::basic_string<CharT, Traits, Allocator>& r,
          priority_tag<3>) noexcept
{
    if constexpr (!is_valid_char_type<CharT>) {
        return invalid_char_type{};
    }
    else if constexpr (!std::is_same_v<Traits, std::char_traits<CharT>>) {
        return custom_char_traits{};
    }
    else {
        return std::basic_string_view<CharT>{r.data(), r.size()};
    }
}

// String literals:
// CharT(&)[] -> string_buffer
template <typename CharT,
          std::size_t N,
          std::enable_if_t<is_valid_char_type<CharT>>* = nullptr>
auto impl(const CharT (&r)[N], priority_tag<3>) noexcept
{
    return std::basic_string_view<CharT>{r, N - 1};
}

// FILE* -> file_buffer
[[deprecated(
    "Prefer using scn::scan_file over a C FILE*, "
    "as using the latter will lead to problems, "
    "if the putback buffer in the FILE gets full.")]]
inline auto impl(std::FILE* file, priority_tag<3>)
{
    return scan_cfile_buffer{file};
}

inline auto impl(scan_file& file, priority_tag<3>)
{
    SCN_EXPECT(scan_file_access::get_handle(file) != nullptr);
    return scan_file_buffer{file};
}
auto impl(scan_file&&, priority_tag<3>) = delete;

// (at least) forward_range -> appropriate range buffer
template <typename Range,
          std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
auto impl(const Range& r, priority_tag<2>)
{
    if constexpr (!is_valid_char_type<char_t<Range>>) {
        return invalid_char_type{};
    }
    else if constexpr (ranges::contiguous_range<Range> &&
                       ranges::sized_range<Range>) {
        // contiguous + sized -> treat as string
        return std::basic_string_view<detail::char_t<Range>>{ranges::data(r),
                                                             ranges::size(r)};
    }
    else if constexpr (ranges::random_access_range<Range> &&
                       can_make_address_from_iterator<
                           ranges::iterator_t<Range>>) {
        // random-access + iterator can be converted to a pointer
        // looks like a debug iterator for a contiguous source
        //  -> treat as string
        return detail::make_string_view_from_pointers(
            to_address(ranges::begin(r)), to_address(ranges::end(r)));
    }
    else {
        // regular forward_range -> forward_buffer
        return detail::make_range_scan_buffer(r);
    }
}

// input_range -> input_range_buffer
template <typename Range,
          std::enable_if_t<ranges::input_range<Range> &&
                           !ranges::forward_range<Range>>* = nullptr>
auto impl(Range&& r, priority_tag<1>)
{
    if constexpr (!is_valid_char_type<char_t<Range>>) {
        return invalid_char_type{};
    }
    else {
        return detail::make_range_scan_buffer(SCN_FWD(r));
    }
}

template <typename Source,
          std::enable_if_t<!ranges::input_range<Source>>* = nullptr>
auto impl(Source&&, priority_tag<0>)
{
    if constexpr (ranges::range<Source>) {
        return insufficient_range{};
    }
    else {
        return invalid_input_range{};
    }
}
}  // namespace _make_scan_buffer

template <typename Source>
inline constexpr bool is_scannable_source =
    !std::is_base_of_v<invalid_input_range,
                       decltype(_make_scan_buffer::impl(SCN_DECLVAL(Source&)),
                                priority_tag<4>{})>;

template <typename Source>
decltype(auto) make_scan_buffer(Source&& source)
{
    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wdeprecated-declarations")
    // Get -Wdeprecated-declarations warnings when impl() is called below,
    // not here

    using T =
        decltype(_make_scan_buffer::impl(SCN_FWD(source), priority_tag<5>{}));

    SCN_GCC_COMPAT_POP

    static_assert(!std::is_same_v<T, invalid_char_type>,
                  "\n"
                  "Unsupported range type given as input to a scanning "
                  "function.\n"
                  "A range needs to have a character type (value type) "
                  "of either `char` or `wchar_t` to be scannable.\n"
                  "For proper `wchar_t` support, <scn/xchar.h> needs "
                  "to be included.\n"
                  "See the scnlib documentation for more details.");
    static_assert(
        !std::is_same_v<T, custom_char_traits>,
        "\n"
        "Unsupported range type given as input to a scanning "
        "function.\n"
        "String types (std::basic_string, and std::basic_string_view) "
        "need to use std::char_traits. Strings with custom Traits are "
        "not supported.");
    static_assert(!std::is_same_v<T, insufficient_range>,
                  "\n"
                  "Unsupported range type given as input to a scanning "
                  "function.\n"
                  "In order to be scannable, a range needs to satisfy "
                  "`forward_range`. `input_range` is not sufficient.");
    static_assert(!std::is_same_v<T, invalid_input_range>,
                  "\n"
                  "Unsupported range type given as input to a scanning "
                  "function.\n"
                  "A range needs to model forward_range and have a valid "
                  "character type (char or wchar_t) to be scannable.\n"
                  "If the input is not a range, it must be a scn::scan_file "
                  "(or a C FILE*, but that's deprecated).\n"
                  "In addition, to scan from a std::istream, "
                  "the header <scn/istream.h> needs to be included.\n"
                  "Examples of scannable ranges are std::string, "
                  "std::string_view, std::list<char>, "
                  "or a large set of C++20 ranges, "
                  "as long as their value type is char or wchar_t.\n"
                  "See the scnlib documentation for more details.");

    return _make_scan_buffer::impl(SCN_FWD(source), priority_tag<5>{});
}

template <typename Source>
using dt_make_scan_buffer =
    decltype(_make_scan_buffer::impl(SCN_DECLVAL(Source), priority_tag<5>{}));
template <typename Source>
using scan_buffer_char_t =
    typename remove_cvref_t<dt_make_scan_buffer<Source>>::char_type;

template <typename CharT, typename Source, typename = void>
inline constexpr bool is_source_impl = false;
template <typename CharT, typename Source>
inline constexpr bool
    is_source_impl<CharT, Source, std::enable_if_t<ranges::range<Source>>> =
        std::is_same_v<char_t<Source>, CharT>;
template <typename CharT, typename Source>
inline constexpr bool is_source_impl<
    CharT,
    Source,
    std::enable_if_t<!ranges::range<Source> &&
                     mp_valid<scan_buffer_char_t, Source>::value>> =
    std::is_same_v<scan_buffer_char_t<Source>, CharT>;

template <typename Source>
inline constexpr bool is_narrow_source = is_source_impl<char, Source>;
template <typename Source>
inline constexpr bool is_wide_source = is_source_impl<wchar_t, Source>;

}  // namespace detail

/////////////////////////////////////////////////////////////////
// Argument type erasure
/////////////////////////////////////////////////////////////////

namespace detail {
enum class arg_type : unsigned char {
    none_type,
    schar_type,
    short_type,
    int_type,
    long_type,
    llong_type,
    int128_type,
    uchar_type,
    ushort_type,
    uint_type,
    ulong_type,
    ullong_type,
    uint128_type,
    bool_type,
    narrow_character_type,
    wide_character_type,
    code_point_type,
    pointer_type,
    float_type,
    double_type,
    ldouble_type,
    float16_type,
    float32_type,
    float64_type,
    float128_type,
    bfloat16_type,
    // Only a single string_view_type,
    // no separate narrow/wide versions,
    // because only one of them is valid for each CharT
    string_view_type,
    narrow_string_type,
    wide_string_type,
    custom_type,
    last_type = custom_type
};

template <typename>
inline constexpr bool is_type_disabled = SCN_DISABLE_TYPE_CUSTOM;

template <typename CharT>
inline constexpr bool is_type_disabled<basic_regex_matches<CharT>> =
    SCN_DISABLE_REGEX;

template <typename T, typename CharT>
struct arg_type_constant
    : std::integral_constant<arg_type, arg_type::custom_type> {
    using type = T;
};

#define SCN_TYPE_CONSTANT(Type, C, Disabled)              \
    template <typename CharT>                             \
    struct arg_type_constant<Type, CharT>                 \
        : std::integral_constant<arg_type, arg_type::C> { \
        using type = Type;                                \
    };                                                    \
    template <>                                           \
    inline constexpr bool is_type_disabled<Type> = Disabled

SCN_TYPE_CONSTANT(signed char, schar_type, SCN_DISABLE_TYPE_SCHAR);
SCN_TYPE_CONSTANT(short, short_type, SCN_DISABLE_TYPE_SHORT);
SCN_TYPE_CONSTANT(int, int_type, SCN_DISABLE_TYPE_INT);
SCN_TYPE_CONSTANT(long, long_type, SCN_DISABLE_TYPE_LONG);
SCN_TYPE_CONSTANT(long long, llong_type, SCN_DISABLE_TYPE_LONG_LONG);
SCN_TYPE_CONSTANT(unsigned char, uchar_type, SCN_DISABLE_TYPE_UCHAR);
SCN_TYPE_CONSTANT(unsigned short, ushort_type, SCN_DISABLE_TYPE_USHORT);
SCN_TYPE_CONSTANT(unsigned int, uint_type, SCN_DISABLE_TYPE_UINT);
SCN_TYPE_CONSTANT(unsigned long, ulong_type, SCN_DISABLE_TYPE_ULONG);
SCN_TYPE_CONSTANT(unsigned long long, ullong_type, SCN_DISABLE_TYPE_ULONG_LONG);
SCN_TYPE_CONSTANT(bool, bool_type, SCN_DISABLE_TYPE_BOOL);
SCN_TYPE_CONSTANT(char, narrow_character_type, SCN_DISABLE_TYPE_CHAR);
SCN_TYPE_CONSTANT(wchar_t, wide_character_type, SCN_DISABLE_TYPE_CHAR);
SCN_TYPE_CONSTANT(char32_t, code_point_type, SCN_DISABLE_TYPE_CHAR32);
SCN_TYPE_CONSTANT(void*, pointer_type, SCN_DISABLE_TYPE_POINTER);
SCN_TYPE_CONSTANT(const void*, pointer_type, SCN_DISABLE_TYPE_POINTER);
SCN_TYPE_CONSTANT(float, float_type, SCN_DISABLE_TYPE_FLOAT);
SCN_TYPE_CONSTANT(double, double_type, SCN_DISABLE_TYPE_DOUBLE);
SCN_TYPE_CONSTANT(long double, ldouble_type, SCN_DISABLE_TYPE_LONG_DOUBLE);
SCN_TYPE_CONSTANT(std::string_view,
                  string_view_type,
                  SCN_DISABLE_TYPE_STRING_VIEW);
SCN_TYPE_CONSTANT(std::wstring_view,
                  string_view_type,
                  SCN_DISABLE_TYPE_STRING_VIEW);
SCN_TYPE_CONSTANT(std::string, narrow_string_type, SCN_DISABLE_TYPE_STRING);
SCN_TYPE_CONSTANT(std::wstring, wide_string_type, SCN_DISABLE_TYPE_STRING);

#if SCN_HAS_INT128
SCN_TYPE_CONSTANT(int128, int128_type, SCN_DISABLE_TYPE_INT128);
SCN_TYPE_CONSTANT(uint128, uint128_type, SCN_DISABLE_TYPE_UINT128);
#endif

#if SCN_HAS_STD_F16
SCN_TYPE_CONSTANT(std::float16_t, float16_type, SCN_DISABLE_TYPE_FLOAT16);
#endif
#if SCN_HAS_STD_F32
SCN_TYPE_CONSTANT(std::float32_t, float32_type, SCN_DISABLE_TYPE_FLOAT32);
#endif
#if SCN_HAS_STD_F64
SCN_TYPE_CONSTANT(std::float64_t, float64_type, SCN_DISABLE_TYPE_FLOAT64);
#endif
#if SCN_HAS_STD_F128
SCN_TYPE_CONSTANT(std::float128_t, float128_type, SCN_DISABLE_TYPE_FLOAT128);
#endif
#if SCN_HAS_STD_BF16
SCN_TYPE_CONSTANT(std::bfloat16_t, bfloat16_type, SCN_DISABLE_TYPE_BFLOAT16);
#endif

#undef SCN_TYPE_CONSTANT

struct custom_value_type {
    void* value;
    auto (*scan)(void* arg, void* pctx, void* ctx) -> scan_expected<void>;
};

struct unscannable {};
struct unscannable_char : unscannable {};
struct unscannable_const : unscannable {};
struct unscannable_disabled : unscannable {
    unscannable_disabled() = default;

    template <typename T>
    constexpr unscannable_disabled(T&&)
    {
    }
};

struct needs_context_tag {};

template <typename Context>
struct context_tag {
    using type = Context;
};

template <typename T, typename Context>
struct custom_wrapper {
    using context_type = Context;
    T& val;
};

template <typename T, typename Scanner, typename ParseCtx>
scan_expected<void> parse_custom_arg(T&, Scanner& s, ParseCtx& pctx)
{
#if SCN_HAS_EXCEPTIONS
    auto fmt_it = pctx.begin();
    try {
        fmt_it = s.parse(pctx);
    }
    catch (const detail::scan_format_string_error_base& ex) {
        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wexit-time-destructors")
        // scan_error takes a const char*.
        // scan_format_string_error (or, actually, std::runtime_error)
        // stores a reference-counted string,
        // that will go out of scope here.
        // We need to provide a const char* that will stay in scope.
        // If scan_format_string_error was thrown with a string literal,
        // use that, otherwise refer to a thread_local std::string
        if (const char* m = get_internal_literal_msg(ex)) {
            return unexpected_scan_error(scan_error::invalid_format_string, m);
        }
        thread_local std::string err_msg{};
        err_msg = ex.what();
        return unexpected_scan_error(scan_error::invalid_format_string,
                                     err_msg.c_str());
        SCN_CLANG_POP
    }
#else
    auto fmt_it = s.parse(pctx_ref);
#endif
    if (auto e = pctx.get_error(); SCN_UNLIKELY(!e)) {
        return e;
    }
    pctx.advance_to(fmt_it);
    return {};
}

class arg_value {
public:
    // trivial default initialization in constexpr
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201907L && \
    SCN_STD > SCN_STD_20
    constexpr arg_value() = default;
#else
    arg_value() = default;
#endif

    template <typename T>
    explicit constexpr arg_value(T& val) : ref_value{std::addressof(val)}
    {
    }

    template <typename T, typename Context>
    explicit constexpr arg_value(custom_wrapper<T, Context> val)
        : custom_value{static_cast<void*>(&val.val),
                       scan_custom_arg<T, Context>}
    {
    }

    arg_value(unscannable);
    arg_value(unscannable_char);
    arg_value(unscannable_const);
    arg_value(unscannable_disabled);

    union {
        void* ref_value{nullptr};
        custom_value_type custom_value;
    };

private:
    template <typename T, typename Context>
    static scan_expected<void> scan_custom_arg(void* arg, void* pctx, void* ctx)
    {
        static_assert(!is_type_disabled<T>,
                      "Scanning of custom types is disabled by "
                      "SCN_DISABLE_TYPE_CUSTOM");
        SCN_EXPECT(arg && pctx && ctx);

        using context_type = Context;
        using parse_context_type = typename context_type::parse_context_type;
        using scanner_type = typename context_type::template scanner_type<T>;

        auto s = scanner_type{};

        auto& arg_ref = *static_cast<T*>(arg);
        auto& pctx_ref = *static_cast<parse_context_type*>(pctx);
        auto& ctx_ref = *static_cast<context_type*>(ctx);

        SCN_TRY_DISCARD(parse_custom_arg(arg_ref, s, pctx_ref));
        SCN_TRY(it, s.scan(arg_ref, ctx_ref));
        ctx_ref.advance_to(SCN_MOVE(it));

        return {};
    }
};

template <typename CharT>
struct arg_mapper {
    using char_type = CharT;
    using other_char_type =
        std::conditional_t<std::is_same_v<char_type, char>, wchar_t, char>;

#define SCN_ARG_MAPPER(T)                                                    \
    static auto map(T& val)                                                  \
        -> std::conditional_t<is_type_disabled<T>, unscannable_disabled, T&> \
    {                                                                        \
        return val;                                                          \
    }

    SCN_ARG_MAPPER(signed char)
    SCN_ARG_MAPPER(short)
    SCN_ARG_MAPPER(int)
    SCN_ARG_MAPPER(long)
    SCN_ARG_MAPPER(long long)
    SCN_ARG_MAPPER(unsigned char)
    SCN_ARG_MAPPER(unsigned short)
    SCN_ARG_MAPPER(unsigned)
    SCN_ARG_MAPPER(unsigned long)
    SCN_ARG_MAPPER(unsigned long long)
    SCN_ARG_MAPPER(wchar_t)
    SCN_ARG_MAPPER(char32_t)
    SCN_ARG_MAPPER(bool)
    SCN_ARG_MAPPER(void*)
    SCN_ARG_MAPPER(const void*)
    SCN_ARG_MAPPER(float)
    SCN_ARG_MAPPER(double)
    SCN_ARG_MAPPER(long double)

    SCN_ARG_MAPPER(std::basic_string_view<char_type>)
    SCN_ARG_MAPPER(std::string)
    SCN_ARG_MAPPER(std::wstring)

#if SCN_HAS_INT128
    SCN_ARG_MAPPER(int128)
    SCN_ARG_MAPPER(uint128)
#endif

#if SCN_HAS_STD_F16
    SCN_ARG_MAPPER(std::float16_t)
#endif
#if SCN_HAS_STD_F32
    SCN_ARG_MAPPER(std::float32_t)
#endif
#if SCN_HAS_STD_F64
    SCN_ARG_MAPPER(std::float64_t)
#endif
#if SCN_HAS_STD_F128
    SCN_ARG_MAPPER(std::float128_t)
#endif
#if SCN_HAS_STD_BF16
    SCN_ARG_MAPPER(std::bfloat16_t)
#endif

#undef SCN_ARG_MAPPER

    static decltype(auto) map(char& val)
    {
        if constexpr (std::is_same_v<char_type, char> &&
                      !is_type_disabled<char_type>) {
            return val;
        }
        else if constexpr (is_type_disabled<char_type>) {
            return unscannable_disabled{val};
        }
        else {
            SCN_UNUSED(val);
            return unscannable_char{};
        }
    }

#if !SCN_DISABLE_REGEX
    // regex_matches treated as a custom type, not packed,
    // to save bits in the packed value,
    // and since regex reading isn't fast anyway
    template <typename T, typename Context>
    static auto map(basic_regex_matches<char_type>& val)
    {
        if constexpr (is_type_disabled<char_type>) {
            return unscannable_disabled{val};
        }
        else {
            return custom_wrapper<T, Context>{val};
        }
    }
    static unscannable_char map(basic_regex_matches<other_char_type>&)
    {
        return {};
    }
#endif

    static unscannable_char map(std::basic_string_view<other_char_type>&)
    {
        return {};
    }

    template <typename T,
              std::enable_if_t<std::is_default_constructible_v<
                  scanner<T, char_type>>>* = nullptr>
    static needs_context_tag map(T&)
    {
        return {};
    }

    template <typename T,
              typename Context,
              std::enable_if_t<std::is_default_constructible_v<
                  scanner<T, char_type>>>* = nullptr>
    static custom_wrapper<T, Context> map(T& val, context_tag<Context>)
    {
        return {val};
    }

    static unscannable map(...)
    {
        return {};
    }
};

template <typename T, typename CharT>
using mapped_type_constant = arg_type_constant<
    std::remove_reference_t<decltype(arg_mapper<CharT>().map(SCN_DECLVAL(T&)))>,
    CharT>;

template <typename T, typename CharT>
using is_scannable = std::integral_constant<
    bool,
    !std::is_base_of_v<
        unscannable,
        remove_cvref_t<decltype(arg_mapper<CharT>().map(SCN_DECLVAL(T&)))>>>;

inline constexpr std::size_t packed_arg_bits = 5;
static_assert((1 << packed_arg_bits) > static_cast<int>(arg_type::last_type),
              "If this fails, there are more `arg_type` values than values "
              "that can fit in `packed_arg_bits`. Either something needs to be "
              "removed from `arg_type` (spilling them to the stack), or "
              "`packed_arg_bits` must be increased (causing the number of "
              "arguments that can be packed to decrease)");
inline constexpr std::size_t bits_in_sz = sizeof(std::size_t) * 8;
inline constexpr std::size_t max_packed_args =
    (bits_in_sz - 2) / packed_arg_bits - 1;
inline constexpr std::size_t is_unpacked_bit = std::size_t{1}
                                               << (bits_in_sz - 1);
inline constexpr std::size_t has_custom_types_bit = std::size_t{1}
                                                    << (bits_in_sz - 2);

template <typename>
constexpr size_t encode_types_impl()
{
    return 0;
}
template <typename CharT, typename T, typename... Others>
constexpr size_t encode_types_impl()
{
    return static_cast<unsigned>(mapped_type_constant<T, CharT>::value) |
           (encode_types_impl<CharT, Others...>() << packed_arg_bits);
}

template <typename CharT, typename... Ts>
constexpr size_t encode_types()
{
    if constexpr (sizeof...(Ts) < (1 << packed_arg_bits)) {
        return sizeof...(Ts) |
               (encode_types_impl<CharT, Ts...>() << packed_arg_bits);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

template <typename T, typename Arg>
constexpr auto make_value_impl(Arg&& arg)
{
    using arg_nocvref_t = remove_cvref_t<Arg>;
    static_assert(!std::is_same_v<arg_nocvref_t, needs_context_tag>);

    constexpr bool scannable_char =
        !std::is_same_v<arg_nocvref_t, unscannable_char>;
    static_assert(scannable_char,
                  "Cannot scan an argument of an unsupported character "
                  "type (i.e. char from a wchar_t source)");

    constexpr bool scannable_const =
        !std::is_same_v<arg_nocvref_t, unscannable_const>;
    static_assert(scannable_const, "Cannot scan a const argument");

    constexpr bool scannable_disabled =
        !std::is_same_v<arg_nocvref_t, unscannable_disabled>;
    static_assert(scannable_disabled,
                  "Cannot scan an argument that has been disabled by "
                  "flag (SCN_DISABLE_TYPE_*)");

    constexpr bool scannable = !std::is_same_v<arg_nocvref_t, unscannable>;
    static_assert(
        scannable,
        "Cannot scan an argument. To make a type T scannable, provide "
        "a scn::scanner<T, CharT> specialization.");

    return arg_value{arg};
}

template <typename Context, typename T>
constexpr auto make_value(T& value)
{
    auto&& arg = arg_mapper<typename Context::char_type>().map(value);

    if constexpr (!std::is_same_v<remove_cvref_t<decltype(arg)>,
                                  needs_context_tag>) {
        return make_value_impl<T>(SCN_FWD(arg));
    }
    else {
        return make_value_impl<T>(arg_mapper<typename Context::char_type>().map(
            value, context_tag<Context>{}));
    }
}

template <typename... Args>
constexpr bool check_scan_arg_types()
{
    constexpr bool default_constructible_constraint =
        std::conjunction_v<std::is_default_constructible<Args>...>;
    static_assert(default_constructible_constraint,
                  "Scan argument types must be default constructible");

    constexpr bool destructible_constraint =
        std::conjunction_v<std::is_destructible<Args>...>;
    static_assert(destructible_constraint,
                  "Scan argument types must be Destructible");

    constexpr bool non_reference_constraint =
        !std::conjunction_v<std::false_type, std::is_reference<Args>...>;
    static_assert(non_reference_constraint,
                  "Scan argument types must not be references");

    constexpr bool movable_constraint =
        std::conjunction_v<std::is_move_constructible<Args>...>;
    static_assert(movable_constraint,
                  "Scan argument types must be move constructible");

    return default_constructible_constraint && destructible_constraint &&
           non_reference_constraint && movable_constraint;
}

enum class scan_arg_store_kind {
    // only built-in types (no custom types), packed
    builtin,

    packed,
    unpacked
};

template <typename Context, typename T>
constexpr basic_scan_arg<Context> make_arg(T& value) noexcept
{
    check_scan_arg_types<T>();

    basic_scan_arg<Context> arg;
    arg.m_type = mapped_type_constant<T, typename Context::char_type>::value;
    arg.m_value = make_value<Context>(value);
    return arg;
}

template <scan_arg_store_kind Kind,
          typename Context,
          arg_type,
          typename T,
          typename = std::enable_if_t<Kind == scan_arg_store_kind::builtin>>
constexpr void* make_arg(T& value) noexcept
{
    return make_value<Context>(value).ref_value;
}
template <scan_arg_store_kind Kind,
          typename Context,
          arg_type,
          typename T,
          typename = std::enable_if_t<Kind == scan_arg_store_kind::packed>>
constexpr arg_value make_arg(T& value) noexcept
{
    return make_value<Context>(value);
}
template <scan_arg_store_kind Kind,
          typename Context,
          arg_type,
          typename T,
          typename = std::enable_if_t<Kind == scan_arg_store_kind::unpacked>>
constexpr basic_scan_arg<Context> make_arg(T&& value) noexcept
{
    return make_arg<Context>(SCN_FWD(value));
}

template <typename Context>
constexpr arg_value& get_arg_value(basic_scan_arg<Context>& arg) noexcept;
template <typename Context>
constexpr arg_value get_arg_value(const basic_scan_arg<Context>& arg) noexcept;

template <typename Context>
constexpr arg_type& get_arg_type(basic_scan_arg<Context>& arg) noexcept;
template <typename Context>
constexpr arg_type get_arg_type(const basic_scan_arg<Context>& arg) noexcept;

template <typename Visitor, typename Context>
constexpr decltype(auto) visit_impl(Visitor&& vis,
                                    basic_scan_arg<Context>& arg);
}  // namespace detail

/**
 * Type-erased scanning argument.
 *
 * Contains a pointer to the value contained in a `scan_arg_store`.
 */
template <typename Context>
class basic_scan_arg {
public:
    /**
     * Enables scanning of a user-defined type.
     *
     * Contains a pointer to the value contained in a `scan_arg_store`, and
     * a callback for parsing the format string, and scanning the value.
     *
     * \see scn::visit_scan_arg
     */
    class handle {
    public:
        /**
         * Parse the format string in `parse_ctx`, and scan the value from
         * `ctx`.
         *
         * \return Any error returned by the scanner
         */
        scan_expected<void> scan(
            typename Context::parse_context_type& parse_ctx,
            Context& ctx) const
        {
            return m_custom.scan(m_custom.value, &parse_ctx, &ctx);
        }

    private:
        explicit handle(detail::custom_value_type custom) noexcept
            : m_custom(custom)
        {
        }

        template <typename Visitor, typename C>
        friend constexpr decltype(auto) detail::visit_impl(
            Visitor&& vis,
            basic_scan_arg<C>& arg);

        detail::custom_value_type m_custom;
    };

    /// Construct a `basic_scan_arg` which doesn't contain an argument.
    constexpr basic_scan_arg() = default;

    /**
     * @return `true` if `*this` contains an argument
     */
    constexpr explicit operator bool() const noexcept
    {
        return m_type != detail::arg_type::none_type;
    }

    /**
     * Visit a `basic_scan_arg` with `Visitor`.
     * Calls `vis` with the value stored in `*this`.
     * If no value is contained in `*this`, calls `vis` with a `monostate`.
     *
     * \return `vis(x)`, where `x` is either a reference to the value contained
     * in `*this`, or a `basic_scan_arg::handle`.
     */
    template <typename Visitor>
    constexpr decltype(auto) visit(Visitor&& vis);

    template <typename R, typename Visitor>
    constexpr R visit(Visitor&& vis);

private:
    template <typename ContextType, typename T>
    friend constexpr basic_scan_arg<ContextType> detail::make_arg(
        T& value) noexcept;

    template <typename C>
    friend constexpr detail::arg_type& detail::get_arg_type(
        basic_scan_arg<C>& arg) noexcept;
    template <typename C>
    friend constexpr detail::arg_type detail::get_arg_type(
        const basic_scan_arg<C>& arg) noexcept;

    template <typename C>
    friend constexpr detail::arg_value& detail::get_arg_value(
        basic_scan_arg<C>& arg) noexcept;
    template <typename C>
    friend constexpr detail::arg_value detail::get_arg_value(
        const basic_scan_arg<C>& arg) noexcept;

    friend class basic_scan_args<Context>;

    detail::arg_value m_value{};
    detail::arg_type m_type{detail::arg_type::none_type};
};

namespace detail {
template <typename Context>
constexpr arg_type& get_arg_type(basic_scan_arg<Context>& arg) noexcept
{
    return arg.m_type;
}

template <typename Context>
constexpr arg_type get_arg_type(const basic_scan_arg<Context>& arg) noexcept
{
    return arg.m_type;
}

template <typename Context>
constexpr arg_value& get_arg_value(basic_scan_arg<Context>& arg) noexcept
{
    return arg.m_value;
}

template <typename Context>
constexpr arg_value get_arg_value(const basic_scan_arg<Context>& arg) noexcept
{
    return arg.m_value;
}

template <typename CharT>
constexpr bool all_types_builtin() noexcept
{
    return true;
}
template <typename CharT, typename T, typename... Args>
constexpr bool all_types_builtin() noexcept
{
    return mapped_type_constant<T, CharT>::value != arg_type::custom_type &&
           all_types_builtin<CharT, Args...>();
}

template <typename CharT, typename... Args>
constexpr scan_arg_store_kind determine_arg_store_kind() noexcept
{
    if constexpr (sizeof...(Args) > max_packed_args) {
        return scan_arg_store_kind::unpacked;
    }
#if !(SCN_CLANG && SCN_APPLE)
    // This doesn't work on Apple Clang. I don't know why
    if constexpr (all_types_builtin<CharT, Args...>()) {
        return scan_arg_store_kind::builtin;
    }
#endif
    return scan_arg_store_kind::packed;
}

template <scan_arg_store_kind Kind, typename CharT, typename... Args>
constexpr size_t compute_arg_store_desc() noexcept
{
    if constexpr (Kind == scan_arg_store_kind::builtin) {
        return encode_types<CharT, Args...>();
    }
    else if constexpr (Kind == scan_arg_store_kind::packed) {
        return encode_types<CharT, Args...>() | has_custom_types_bit;
    }
    else {
        return sizeof...(Args) | is_unpacked_bit;
    }
}

template <typename Context, typename... Args>
class scan_arg_store {
public:
    static constexpr scan_arg_store_kind kind =
        determine_arg_store_kind<typename Context::char_type, Args...>();
    static constexpr size_t desc =
        compute_arg_store_desc<kind, typename Context::char_type, Args...>();

    using argptr_type = std::conditional_t<
        kind == scan_arg_store_kind::builtin,
        void*,
        std::conditional_t<kind == scan_arg_store_kind::packed,
                           arg_value,
                           basic_scan_arg<Context>>>;
    using argptrs_type = std::array<argptr_type, sizeof...(Args)>;

    constexpr explicit scan_arg_store(std::tuple<Args...>& a) noexcept
        : args(std::apply(make_argptrs<Args...>, a))
    {
    }

    argptrs_type args;

private:
    template <typename... A>
    static constexpr argptrs_type make_argptrs(A&... args) noexcept
    {
        return {detail::make_arg<
            kind, Context,
            mapped_type_constant<remove_cvref_t<A>,
                                 typename Context::char_type>::value>(args)...};
    }
};

}  // namespace detail

/**
 * Creates a type-erased argument store over the arguments in `values`.
 */
template <typename Context = scan_context, typename... Args>
constexpr auto make_scan_args(std::tuple<Args...>& values)
{
    detail::check_scan_arg_types<Args...>();
    return detail::scan_arg_store<Context, Args...>(values);
}

template <typename... Args>
constexpr auto make_wscan_args(std::tuple<Args...>& values)
{
    detail::check_scan_arg_types<Args...>();
    return detail::scan_arg_store<wscan_context, Args...>(values);
}

/**
 * A view over a collection of scanning arguments (`scan_arg_store`).
 *
 * Passed to `scn::vscan`, where it's automatically constructed from a
 * `scan_arg_store`.
 */
template <typename Context>
class basic_scan_args {
public:
    constexpr basic_scan_args() = default;

    template <typename... Args>
    SCN_IMPLICIT constexpr basic_scan_args(
        const detail::scan_arg_store<Context, Args...>& store) noexcept
        : basic_scan_args(store.desc, store.args.data())
    {
    }

    /**
     * \return `basic_scan_arg` at index `id`. Empty `basic_scan_arg` if
     * there's no argument at index `id`.
     */
    SCN_NODISCARD constexpr basic_scan_arg<Context> get(std::size_t id) const
    {
        if (SCN_UNLIKELY(!is_packed())) {
            if (SCN_LIKELY(id < max_size())) {
                return m_args[id];
            }
            return {};
        }

        if (SCN_UNLIKELY(id >= detail::max_packed_args)) {
            return {};
        }

        const auto t = type(id);
        if (SCN_UNLIKELY(t == detail::arg_type::none_type)) {
            return {};
        }

        basic_scan_arg<Context> arg;
        arg.m_type = t;
        if (is_only_builtin()) {
            arg.m_value.ref_value = m_builtin_values[id];
        }
        else {
            arg.m_value = m_values[id];
        }
        return arg;
    }

    /**
     * \return Number of arguments in `*this`.
     */
    SCN_NODISCARD constexpr std::size_t size() const
    {
        if (SCN_UNLIKELY(!is_packed())) {
            return max_size();
        }

        return m_desc & ((1ull << detail::packed_arg_bits) - 1ull);
    }

private:
    constexpr explicit basic_scan_args(size_t desc, void* const* data)
        : m_desc(desc), m_builtin_values(data)
    {
    }
    constexpr explicit basic_scan_args(size_t desc,
                                       const detail::arg_value* data)
        : m_desc(desc), m_values(data)
    {
    }
    constexpr explicit basic_scan_args(size_t desc,
                                       const basic_scan_arg<Context>* data)
        : m_desc(desc), m_args(data)
    {
    }

    SCN_NODISCARD constexpr bool is_packed() const
    {
        return (m_desc & detail::is_unpacked_bit) == 0;
    }
    SCN_NODISCARD constexpr bool is_only_builtin() const
    {
        return (m_desc & detail::has_custom_types_bit) == 0;
    }

    SCN_NODISCARD constexpr detail::arg_type type(std::size_t index) const
    {
        // First (0th) index is size, types start after that
        const auto shift = (index + 1) * detail::packed_arg_bits;
        const std::size_t mask = (1 << detail::packed_arg_bits) - 1;
        return static_cast<detail::arg_type>((m_desc >> shift) & mask);
    }

    SCN_NODISCARD constexpr std::size_t max_size() const
    {
        return SCN_LIKELY(is_packed()) ? detail::max_packed_args
                                       : (m_desc & ~detail::is_unpacked_bit &
                                          ~detail::has_custom_types_bit);
    }

    size_t m_desc{0};
    union {
        void* const* m_builtin_values;
        const detail::arg_value* m_values;
        const basic_scan_arg<Context>* m_args{nullptr};
    };
};

template <typename Context, typename... Args>
basic_scan_args(const detail::scan_arg_store<Context, Args...>&)
    -> basic_scan_args<Context>;

/////////////////////////////////////////////////////////////////
// scan_parse_context
/////////////////////////////////////////////////////////////////

template <typename T>
struct source_tag_type {
    using type = T;
};
template <typename T>
inline constexpr auto source_tag = source_tag_type<T>{};

/**
 * Format string parsing context, wrapping the format string being parsed,
 * and a counter for argument indexing.
 *
 * \ingroup ctx
 */
template <typename CharT>
class basic_scan_parse_context {
public:
    using char_type = CharT;
    using iterator = typename std::basic_string_view<CharT>::const_pointer;
    using const_iterator = iterator;

    /**
     * Construct a `basic_scan_parse_context` over a format string `format`.
     */
    [[deprecated(
        "Use the source_tag constructor instead,"
        "to get more compile-time checking")]]
    explicit constexpr basic_scan_parse_context(
        std::basic_string_view<CharT> format,
        int next_arg_id = 0)
        : m_format{format}, m_next_arg_id{next_arg_id}
    {
    }

    template <typename Source>
    explicit constexpr basic_scan_parse_context(
        source_tag_type<Source>,
        std::basic_string_view<CharT> format,
        int next_arg_id = 0)
        : m_format{format},
          m_next_arg_id{next_arg_id},
          m_is_contiguous(ranges::range<Source> &&
                          ranges::contiguous_range<Source>),
          m_is_borrowed(ranges::range<Source> && ranges::borrowed_range<Source>)
    {
    }

    basic_scan_parse_context(const basic_scan_parse_context&) = delete;
    basic_scan_parse_context& operator=(const basic_scan_parse_context&) =
        delete;
    basic_scan_parse_context(basic_scan_parse_context&&) = delete;
    basic_scan_parse_context& operator=(basic_scan_parse_context&&) = delete;
    ~basic_scan_parse_context() = default;

    /// Returns an iterator pointing to the beginning of the format string
    constexpr auto begin() const noexcept
    {
        return m_format.data();
    }
    /// Returns an iterator pointing to the end of the format string
    constexpr auto end() const noexcept
    {
        return m_format.data() + m_format.size();
    }

    /// Advance the beginning of the format string to `it`
    constexpr void advance_to(iterator it)
    {
        m_format.remove_prefix(static_cast<std::size_t>(it - begin()));
    }

    constexpr size_t next_arg_id()
    {
        if (SCN_UNLIKELY(m_next_arg_id < 0)) {
            on_error(
                "Cannot switch from manual to automatic argument indexing");
            return 0;
        }

        auto id = static_cast<size_t>(m_next_arg_id++);
        do_check_arg_id(id);
        return id;
    }

    constexpr void check_arg_id(std::size_t id)
    {
        if (SCN_UNLIKELY(m_next_arg_id > 0)) {
            on_error(
                "Cannot switch from manual to automatic argument indexing");
            return;
        }
        m_next_arg_id = -1;
        do_check_arg_id(id);
    }

    /**
     * Fail format string parsing with the message `msg`.
     * Calling this member function is not a constant expression,
     * causing a compile-time error, if compile-time format string checking is
     * enabled.
     */
    scan_error on_error(const char* msg)
    {
        m_error = unexpected(detail::handle_error(
            scan_error{scan_error::invalid_format_string, msg}));
        return m_error.error();
    }

    scan_expected<void> get_error()
    {
        return m_error;
    }

    SCN_NODISCARD constexpr bool is_source_contiguous() const
    {
        return m_is_contiguous;
    }

    SCN_NODISCARD constexpr bool is_source_borrowed() const
    {
        return m_is_borrowed;
    }

protected:
    constexpr void do_check_arg_id(size_t id);

    std::basic_string_view<CharT> m_format;
    scan_expected<void> m_error{};
    int m_next_arg_id{0};
    bool m_is_contiguous{false}, m_is_borrowed{false};
};

/////////////////////////////////////////////////////////////////
// Result types
/////////////////////////////////////////////////////////////////

namespace detail {

template <typename... Args>
struct scan_result_value_storage {
public:
    using tuple_type = std::tuple<Args...>;

    constexpr scan_result_value_storage() = default;

    constexpr scan_result_value_storage(tuple_type&& values)
        : m_values(SCN_MOVE(values))
    {
    }

    /// Access the scanned values
    tuple_type& values() &
    {
        return m_values;
    }
    /// Access the scanned values
    const tuple_type& values() const&
    {
        return m_values;
    }
    /// Access the scanned values
    tuple_type&& values() &&
    {
        return SCN_MOVE(m_values);
    }
    /// Access the scanned values
    const tuple_type&& values() const&&
    {
        return SCN_MOVE(m_values);
    }

    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() &
    {
        return std::get<0>(m_values);
    }
    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() const&
    {
        return std::get<0>(m_values);
    }
    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() &&
    {
        return SCN_MOVE(std::get<0>(m_values));
    }
    /// Access the single scanned value
    template <size_t N = sizeof...(Args), std::enable_if_t<N == 1>* = nullptr>
    decltype(auto) value() const&&
    {
        return SCN_MOVE(std::get<0>(m_values));
    }

private:
    SCN_NO_UNIQUE_ADDRESS tuple_type m_values{};
};

template <class T, class U>
constexpr auto&& forward_like(U&& x) noexcept
{
    constexpr bool is_adding_const =
        std::is_const_v<std::remove_reference_t<T>>;
    if constexpr (std::is_lvalue_reference_v<T&&>) {
        if constexpr (is_adding_const) {
            return static_cast<std::add_const_t<U>&>(x);
        }
        else {
            return static_cast<U&>(x);
        }
    }
    else {
        if constexpr (is_adding_const) {
            return std::move(static_cast<std::add_const_t<U>&>(x));
        }
        else {
            return std::move(x);
        }
    }
}

struct scan_result_source_access;

template <typename Range>
class scan_result_subrange_storage {
    static_assert(
        is_specialization_of_v<Range, ranges::subrange>,
        "scan_result_subrange_storage accepts only subranges as Ranges");
    static_assert(ranges::forward_range<Range>);
    static_assert(ranges::copyable<Range>);

    friend struct scan_result_source_access;

public:
    using source_type = Range;
    using range_type = Range;
    using iterator = ranges::iterator_t<Range>;
    using sentinel = ranges::sentinel_t<Range>;

    constexpr scan_result_subrange_storage() = default;

    explicit constexpr scan_result_subrange_storage(range_type&& r)
        : m_range(SCN_MOVE(r))
    {
    }

    template <
        typename R,
        std::enable_if_t<is_not_self<R, scan_result_subrange_storage> &&
                         std::is_constructible_v<range_type, R&&>>* = nullptr>
    explicit constexpr scan_result_subrange_storage(R&& r) : m_range(SCN_FWD(r))
    {
    }

    /// Access the ununsed source range
    SCN_NODISCARD range_type range() const
    {
        return m_range;
    }

    /// The beginning of the unused source range
    SCN_NODISCARD iterator begin() const
    {
        return ranges::begin(m_range);
    }

    /// The end of the unused source range
    SCN_NODISCARD sentinel end() const
    {
        return ranges::end(m_range);
    }

private:
    template <
        typename Other,
        std::enable_if_t<std::is_assignable_v<range_type&, Other&&>>* = nullptr>
    void set(Other&& r)
    {
        m_range = SCN_FWD(r);
    }

    SCN_NO_UNIQUE_ADDRESS range_type m_range{};
};

template <typename Range>
class scan_result_pair_concat_storage {
    static_assert(is_specialization_of_v<Range, ranges::pair_concat_view>,
                  "scan_result_subrange_storage accepts only pair_concat_views "
                  "as Ranges");
    static_assert(ranges::input_range<Range> && !ranges::forward_range<Range>);

    using char_type = detail::char_t<Range>;

    friend struct scan_result_source_access;

public:
    using source_type = Range;
    using range_type = source_type;
    using iterator = ranges::iterator_t<range_type>;
    using sentinel = ranges::sentinel_t<range_type>;

    scan_result_pair_concat_storage() = default;

    explicit scan_result_pair_concat_storage(range_type&& r)
        : m_range(SCN_MOVE(r))
    {
    }

    SCN_NODISCARD range_type& range() &
    {
        SCN_EXPECT(m_range.has_value());
        return *m_range;
    }
    SCN_NODISCARD range_type range() &&
    {
        SCN_EXPECT(m_range.has_value());
        return SCN_MOVE(*m_range);
    }

    SCN_NODISCARD iterator& begin() &
    {
        SCN_EXPECT(m_range.has_value());
        return ranges::begin(*m_range);
    }
    SCN_NODISCARD iterator begin() &&
    {
        SCN_EXPECT(m_range.has_value());
        return SCN_MOVE(ranges::begin(*m_range));
    }

    SCN_NODISCARD sentinel end()
    {
        SCN_EXPECT(m_range.has_value());
        return ranges::end(*m_range);
    }

private:
    template <typename Other,
              std::enable_if_t<std::is_constructible_v<range_type, Other&&>>* =
                  nullptr>
    void set(Other&& r)
    {
        m_range.emplace(SCN_FWD(r));
    }

    template <typename>
    struct debug;

    template <typename Other,
              std::enable_if_t<!std::is_constructible_v<range_type, Other&&>>* =
                  nullptr>
    void set(Other&&)
    {
        debug<Other&&>{};
    }

    std::optional<range_type> m_range{};
};

class scan_result_cfile_storage {
    friend struct scan_result_source_access;

public:
    using source_type = std::FILE*;

    scan_result_cfile_storage() = default;

    explicit scan_result_cfile_storage(std::FILE* f) : m_file(f) {}

    /// File used for scanning
    SCN_NODISCARD std::FILE* file() const
    {
        return m_file;
    }

private:
    void set(std::FILE* f)
    {
        m_file = f;
    }

    std::FILE* m_file{nullptr};
};

class scan_result_file_storage {
    friend struct scan_result_source_access;

public:
    using source_type = scan_file;

    scan_result_file_storage() = default;

    explicit scan_result_file_storage(scan_file& f) : m_file(&f) {}
    explicit scan_result_file_storage(scan_file* f) : m_file(f)
    {
        SCN_EXPECT(f);
    }

    /// File used for scanning
    SCN_NODISCARD scan_file& file()
    {
        SCN_EXPECT(m_file);
        return *m_file;
    }

private:
    void set(scan_file& f)
    {
        m_file = &f;
    }
    void set(scan_file* f)
    {
        SCN_EXPECT(f);
        m_file = f;
    }

    scan_file* m_file{nullptr};
};

struct scan_result_dangling {
    friend struct scan_result_source_access;

    using source_type = ranges::dangling;
    using range_type = ranges::dangling;
    using iterator = ranges::dangling;
    using sentinel = ranges::dangling;

    constexpr scan_result_dangling() = default;

    template <typename... Args>
    explicit constexpr scan_result_dangling(Args&&...)
    {
    }

    SCN_NODISCARD range_type range() const
    {
        return {};
    }

    SCN_NODISCARD ranges::dangling begin() const
    {
        return {};
    }
    SCN_NODISCARD ranges::dangling end() const
    {
        return {};
    }

private:
    template <typename... Args>
    void set(Args&&...)
    {
    }
};

template <typename Source>
struct invalid_scan_result_source {
    static_assert(detail::dependent_false<Source>::value,
                  "No scan_result implementation found for this Source type");
};

template <typename Source, typename = void>
struct custom_scan_result_source_storage;

template <typename Source>
using custom_scan_result_storage_t =
    typename custom_scan_result_source_storage<Source>::type;

template <typename Source>
using scan_result_source_storage = typename mp_cond<
    std::is_same<remove_cvref_t<Source>, ranges::dangling>,
    mp_identity<scan_result_dangling>,
    std::is_same<remove_cvref_t<Source>, std::FILE*>,
    mp_identity<scan_result_cfile_storage>,
    std::is_same<std::remove_pointer_t<remove_cvref_t<Source>>, scan_file>,
    mp_identity<scan_result_file_storage>,
    mp_valid<custom_scan_result_storage_t, Source>,
    mp_defer<custom_scan_result_storage_t, Source>,
    mp_bool<ranges::forward_range<Source>>,
    mp_defer<scan_result_subrange_storage, Source>,
    mp_bool<ranges::input_range<Source>>,
    mp_defer<scan_result_pair_concat_storage, Source>,
    std::true_type,
    mp_defer<invalid_scan_result_source, Source>>::type;

struct scan_result_source_access {
    template <typename R, typename... Args>
    static auto set(R& result, Args&&... args)
        -> decltype(result.set(SCN_FWD(args)...))
    {
        return result.set(SCN_FWD(args)...);
    }

    template <typename R>
    static auto get(R& result) -> decltype(result.range())
    {
        return result.range();
    }
    template <typename R>
    static auto get(R& result) -> decltype(result.file())
    {
        return result.file();
    }
    template <typename R>
    static auto get(R& result) -> decltype(result.stream())
    {
        return result.stream();
    }
};

}  // namespace detail

/**
 * \defgroup result Result types
 *
 * \brief Result and error types
 *
 * Instead of using exceptions, `scn::scan` and others return an object of
 * type `scn::scan_result`, wrapped inside a `scn::scan_expected`.
 */

/**
 * Type returned by `scan`, contains the unused input as a subrange, and the
 * scanned values in a tuple.
 */
template <typename Source, typename... Args>
class scan_result : public detail::scan_result_source_storage<Source>,
                    public detail::scan_result_value_storage<Args...> {
    using source_base = detail::scan_result_source_storage<Source>;
    using value_base = detail::scan_result_value_storage<Args...>;

public:
    using source_type = typename source_base::source_type;
    using tuple_type = typename value_base::tuple_type;

    constexpr scan_result() = default;

    constexpr scan_result(const scan_result&) = default;
    constexpr scan_result(scan_result&&) = default;
    constexpr scan_result& operator=(const scan_result&) = default;
    constexpr scan_result& operator=(scan_result&&) = default;

    ~scan_result() = default;

    template <typename Src,
              std::enable_if_t<std::is_constructible_v<source_base, Src&&>>* =
                  nullptr>
    scan_result(Src&& src, tuple_type&& values)
        : source_base(SCN_FWD(src)), value_base(SCN_MOVE(values))
    {
    }

    template <typename OtherSrc,
              typename ThisSrc = Source,
              std::enable_if_t<std::is_constructible_v<source_type, OtherSrc> &&
                               std::is_convertible_v<const OtherSrc&,
                                                     source_type>>* = nullptr>
    SCN_IMPLICIT scan_result(const scan_result<OtherSrc, Args...>& o)
        : source_base(detail::scan_result_source_access::get(o)),
          value_base(o.values())
    {
    }
    template <typename OtherSrc,
              typename ThisSrc = Source,
              std::enable_if_t<std::is_constructible_v<source_type, OtherSrc> &&
                               !std::is_convertible_v<const OtherSrc&,
                                                      source_type>>* = nullptr>
    explicit scan_result(const scan_result<OtherSrc, Args...>& o)
        : source_base(detail::scan_result_source_access::get(o)),
          value_base(o.values())
    {
    }

    template <typename OtherSrc,
              typename ThisSrc = Source,
              std::enable_if_t<
                  std::is_constructible_v<source_type, OtherSrc> &&
                  std::is_convertible_v<OtherSrc&&, source_type>>* = nullptr>
    SCN_IMPLICIT scan_result(scan_result<OtherSrc, Args...>&& o)
        : source_base(SCN_MOVE(detail::scan_result_source_access::get(o))),
          value_base(SCN_MOVE(o.values()))
    {
    }
    template <typename OtherSrc,
              typename ThisSrc = Source,
              std::enable_if_t<
                  std::is_constructible_v<source_type, OtherSrc> &&
                  !std::is_convertible_v<OtherSrc&&, source_type>>* = nullptr>
    explicit scan_result(scan_result<OtherSrc, Args...>&& o)
        : source_base(SCN_MOVE(detail::scan_result_source_access::get(o))),
          value_base(SCN_MOVE(o.values()))
    {
    }

    template <typename OtherSrc,
              typename = std::enable_if_t<
                  std::is_assignable_v<source_type&, const OtherSrc&>>>
    scan_result& operator=(const scan_result<OtherSrc, Args...>& o)
    {
        detail::scan_result_source_access::set(
            *this, detail::scan_result_source_access::get(o));
        this->values() = o.values();
        return *this;
    }

    template <typename OtherSrc,
              typename = std::enable_if_t<
                  std::is_constructible_v<source_type, OtherSrc>>>
    scan_result& operator=(scan_result<OtherSrc, Args...>&& o)
    {
        detail::scan_result_source_access::set(
            *this, SCN_MOVE(detail::scan_result_source_access::get(o)));
        this->values() = SCN_MOVE(o.values());
        return *this;
    }
};

template <typename R, typename... Args>
scan_result(R, std::tuple<Args...>) -> scan_result<R, Args...>;
template <typename R, typename Ctx, typename... Args>
scan_result(R, detail::scan_arg_store<Ctx, Args...>&)
    -> scan_result<R, Args...>;

namespace detail {
template <typename Range>
using borrowed_input_range_tail_subrange_t = std::conditional_t<
    ranges::borrowed_range<Range>,
    ranges::pair_concat_view<
        ranges::owning_view<std::basic_string<detail::char_t<Range>>>,
        ranges::subrange<ranges::iterator_t<Range>, ranges::sentinel_t<Range>>>,
    ranges::dangling>;

template <typename Range>
using borrowed_flattened_concat_tail_subrange_t = std::conditional_t<
    ranges::borrowed_range<Range>,
    ranges::pair_concat_view<
        ranges::owning_view<
            std::basic_string<detail::char_t<remove_cvref_t<Range>>>>,
        ranges::subrange<ranges::iterator_t<
                             typename remove_cvref_t<Range>::second_range_type>,
                         ranges::sentinel_t<typename remove_cvref_t<
                             Range>::second_range_type>>>,
    ranges::dangling>;

template <typename SourceRange>
auto make_vscan_result_end(SourceRange& source)
{
    return ranges::end(source);
}
template <typename CharT, size_t N>
auto make_vscan_result_end(CharT (&source)[N])
    -> ranges::sentinel_t<CharT (&)[N]>
{
    return source + N - 1;
}

template <typename SourceRange,
          typename Buffer,
          std::enable_if_t<ranges::forward_range<SourceRange>>* = nullptr>
auto make_vscan_result(SourceRange&& source, Buffer&, std::ptrdiff_t n)
    -> borrowed_tail_subrange_t<SourceRange>
{
    if constexpr (ranges::random_access_iterator<
                      ranges::iterator_t<SourceRange>>) {
        return {ranges::begin(source) + n, make_vscan_result_end(source)};
    }
    else {
        auto it = ranges::begin(source);
        auto end = ranges::end(source);
        while (n > 0 && it != end) {
            --n;
            ++it;
        }
        return {SCN_MOVE(it), SCN_MOVE(end)};
    }
}
template <typename SourceRange,
          typename Buffer,
          std::enable_if_t<ranges::input_range<SourceRange> &&
                           !ranges::forward_range<SourceRange> &&
                           !is_specialization_of_v<remove_cvref_t<SourceRange>,
                                                   ranges::pair_concat_view>>* =
              nullptr>
auto make_vscan_result(SourceRange&& source, Buffer& buffer, std::ptrdiff_t)
    -> borrowed_input_range_tail_subrange_t<SourceRange>
{
    return ranges::pair_concat_view{
        ranges::owning_view{std::basic_string<detail::char_t<SourceRange>>(
            SCN_MOVE(buffer.get_prelude()))},
        ranges::subrange{SCN_MOVE(buffer.get_iterator()), ranges::end(source)}};
}
template <typename SourceRange,
          typename Buffer,
          std::enable_if_t<ranges::input_range<SourceRange> &&
                           !ranges::forward_range<SourceRange> &&
                           is_specialization_of_v<remove_cvref_t<SourceRange>,
                                                  ranges::pair_concat_view>>* =
              nullptr>
auto make_vscan_result(SourceRange&& source, Buffer& buffer, std::ptrdiff_t)
    -> borrowed_flattened_concat_tail_subrange_t<SourceRange>
{
    using R = remove_cvref_t<SourceRange>;
    if (buffer.get_iterator()._is_first()) {
        auto it = SCN_MOVE(buffer.get_iterator()._get_first());
        auto& s = source._get_first();
        s.base().erase(s.begin(), it);
        return ranges::pair_concat_view{SCN_MOVE(s),
                                        SCN_MOVE(source._get_second())};
    }

    return ranges::pair_concat_view{
        ranges::owning_view{std::basic_string<detail::char_t<R>>{}},
        ranges::subrange{SCN_MOVE(buffer.get_iterator()._get_second()),
                         ranges::end(source._get_second())}};
}
inline auto make_vscan_result(std::FILE* source,
                              const scan_cfile_buffer&,
                              std::ptrdiff_t)
{
    return source;
}
inline auto make_vscan_result(scan_file& source,
                              const scan_file_buffer&,
                              std::ptrdiff_t)
{
    return &source;
}
inline auto make_vscan_result(scan_file&& source,
                              const scan_file_buffer&,
                              std::ptrdiff_t) = delete;

}  // namespace detail

/////////////////////////////////////////////////////////////////
// Format string parsing
/////////////////////////////////////////////////////////////////

namespace detail {
/// Parse context with extra data used only for compile-time checks
template <typename CharT>
class compile_parse_context : public basic_scan_parse_context<CharT> {
    using base = basic_scan_parse_context<CharT>;

public:
    template <typename Source>
    explicit constexpr compile_parse_context(
        source_tag_type<Source>,
        std::basic_string_view<CharT> format_str,
        int num_args,
        const arg_type* types,
        int next_arg_id = 0)
        : base(source_tag<Source>, format_str, next_arg_id),
          m_num_args(num_args),
          m_types(types)
    {
    }

    SCN_NODISCARD constexpr int get_num_args() const
    {
        return m_num_args;
    }
    SCN_NODISCARD constexpr arg_type get_arg_type(std::size_t id) const
    {
        return m_types[id];
    }

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wsign-conversion")

    constexpr std::size_t next_arg_id()
    {
        auto id = base::next_arg_id();
        if (SCN_UNLIKELY(id >= static_cast<size_t>(m_num_args))) {
            this->on_error("Argument not found");
        }
        return id;
    }

    constexpr void check_arg_id(std::size_t id)
    {
        base::check_arg_id(id);
        if (SCN_UNLIKELY(id >= static_cast<size_t>(m_num_args))) {
            this->on_error("Argument not found");
        }
    }
    using base::check_arg_id;

private:
    int m_num_args;
    const arg_type* m_types;

    SCN_GCC_POP  // -Wsign-conversion
};

constexpr inline bool is_constant_evaluated(bool default_value = false) noexcept
{
#ifdef __cpp_lib_is_constant_evaluated
    SCN_UNUSED(default_value);
    return std::is_constant_evaluated();
#else
    return default_value;
#endif
}
}  // namespace detail

template <typename CharT>
constexpr void basic_scan_parse_context<CharT>::do_check_arg_id(size_t id)
{
    if (detail::is_constant_evaluated() &&
        (!SCN_GCC || SCN_GCC >= SCN_COMPILER(12, 0, 0))) {
        // The cast below will cause an error on gcc pre-12
        using parse_context_type = detail::compile_parse_context<CharT>;
        if (static_cast<int>(id) >=
            static_cast<parse_context_type*>(this)->get_num_args()) {
            SCN_UNLIKELY_ATTR
            on_error("Argument not found");
        }
    }
}

namespace detail {
enum class align_type : unsigned char {
    none = 0,
    left = 1,   // '<'
    right = 2,  // '>'
    center = 3  // '^'
};

enum class presentation_type {
    none,
    int_binary,            // 'b', 'B'
    int_decimal,           // 'd'
    int_generic,           // 'i'
    int_unsigned_decimal,  // 'u'
    int_octal,             // 'o'
    int_hex,               // 'x', 'X'
    int_arbitrary_base,    // 'rnn', 'Rnn' (R for radix)
    float_hex,             // 'a', 'A'
    float_scientific,      // 'e', 'E'
    float_fixed,           // 'f', 'F'
    float_general,         // 'g', 'G'
    string,                // 's'
    string_set,            // '[...]'
#if !SCN_DISABLE_REGEX
    regex,          // '/.../.'
    regex_escaped,  // '/..\/../.'
#endif
    character,          // 'c'
    escaped_character,  // '?'
    pointer,            // 'p'
};

#if !SCN_DISABLE_REGEX
enum class regex_flags {
    none = 0,
    multiline = 1,   // /m
    singleline = 2,  // /s
    nocase = 4,      // /i
    nocapture = 8,   // /n
    // TODO?
    // would probably need to go hand-in-hand with locale,
    // where it could even be the default/only option -> no flag?
    // why else would you even use locale with a regex?
    // collate = 16,
};

constexpr regex_flags operator&(regex_flags a, regex_flags b)
{
    return static_cast<regex_flags>(static_cast<unsigned>(a) &
                                    static_cast<unsigned>(b));
}
constexpr regex_flags operator|(regex_flags a, regex_flags b)
{
    return static_cast<regex_flags>(static_cast<unsigned>(a) |
                                    static_cast<unsigned>(b));
}
constexpr regex_flags operator^(regex_flags a, regex_flags b)
{
    return static_cast<regex_flags>(static_cast<unsigned>(a) ^
                                    static_cast<unsigned>(b));
}

constexpr regex_flags& operator&=(regex_flags& a, regex_flags b)
{
    return a = a & b;
}
constexpr regex_flags& operator|=(regex_flags& a, regex_flags b)
{
    return a = a | b;
}
constexpr regex_flags& operator^=(regex_flags& a, regex_flags b)
{
    return a = a ^ b;
}
#endif

class fill_type {
public:
    constexpr void operator=(char c)
    {
        m_data[0] = c;
        m_size = 1;
    }

    template <typename CharT>
    constexpr void operator=(std::basic_string_view<CharT> s)
    {
        SCN_EXPECT(!s.empty());
        SCN_EXPECT(s.size() * sizeof(CharT) <= max_size);
        if constexpr (sizeof(CharT) == 1) {
            for (size_t i = 0; i < s.size(); ++i) {
                SCN_GCC_PUSH
                SCN_GCC_IGNORE("-Wstringop-overflow")  // false positive
                m_data[i] = s[i];
                SCN_GCC_POP
            }
            m_size = static_cast<unsigned char>(s.size());
        }
        else if constexpr (sizeof(CharT) == 2) {
            m_data[0] = static_cast<char>(static_cast<unsigned>(s.front()));
            m_data[1] =
                static_cast<char>(static_cast<unsigned>(s.front()) >> 8);
            if (s.size() == 1) {
                return;
            }
            m_data[2] = static_cast<char>(static_cast<unsigned>(s[1]));
            m_data[3] = static_cast<char>(static_cast<unsigned>(s[1]) >> 8);
        }
        else {
            const auto front = static_cast<unsigned>(s.front());
            m_data[0] = static_cast<char>(front);
            m_data[1] = static_cast<char>(front >> 8);
            m_data[2] = static_cast<char>(front >> 16);
            m_data[3] = static_cast<char>(front >> 24);
        }
    }

    constexpr size_t size() const
    {
        return m_size;
    }

    template <typename CharT>
    CharT get_code_unit() const
    {
        SCN_EXPECT(m_size <= sizeof(CharT));
        CharT r{};
        std::memcpy(&r, m_data, m_size);
        return r;
    }

    template <typename CharT>
    std::basic_string_view<CharT> get_code_units() const
    {
        return {reinterpret_cast<const CharT*>(
                    SCN_ASSUME_ALIGNED(m_data, alignof(CharT))),
                m_size};
    }

private:
    static constexpr size_t max_size = 4;
    alignas(char32_t) char m_data[max_size] = {' '};
    unsigned char m_size{1};
};

struct format_specs {
    int width{0}, precision{0};
    fill_type fill{};
    presentation_type type{presentation_type::none};
    std::array<uint8_t, 128 / 8> charset_literals{0};
    bool charset_has_nonascii{false}, charset_is_inverted{false};
    const void* charset_string_data{nullptr};
    size_t charset_string_size{0};
#if !SCN_DISABLE_REGEX
    regex_flags regexp_flags{regex_flags::none};
#endif
    unsigned char arbitrary_base{0};
    align_type align{align_type::none};
    bool localized{false};

    constexpr format_specs() = default;

    SCN_NODISCARD constexpr int get_base() const
    {
        SCN_GCC_COMPAT_PUSH
        SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")
        switch (type) {
            case presentation_type::none:
                return 10;
            case presentation_type::int_generic:
                return 0;
            case presentation_type::int_arbitrary_base:
                return arbitrary_base;

            case presentation_type::int_binary:
                return 2;
            case presentation_type::int_octal:
                return 8;
            case presentation_type::int_decimal:
            case presentation_type::int_unsigned_decimal:
                return 10;
            case presentation_type::int_hex:
                return 16;

            default:
#if !SCN_GCC || SCN_GCC >= SCN_COMPILER(9, 0, 0)
                // gcc 7 thinks we'll get here, even when we won't
                // gcc 8 has a bug in debug mode:
                // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86678
                SCN_EXPECT(false);
#endif
                SCN_UNREACHABLE;
        }
        SCN_GCC_COMPAT_POP
    }

    template <typename CharT>
    std::basic_string_view<CharT> charset_string() const
    {
        return {reinterpret_cast<const CharT*>(charset_string_data),
                charset_string_size};
    }
};

struct specs_setter {
public:
    explicit constexpr specs_setter(format_specs& specs) : m_specs(specs) {}

    constexpr void on_align(align_type align)
    {
        m_specs.align = align;
    }
    template <typename CharT>
    constexpr void on_fill(std::basic_string_view<CharT> fill)
    {
        m_specs.fill = fill;
    }
    template <bool Dependent = true>
    constexpr void on_localized()
    {
        if constexpr (!SCN_DISABLE_LOCALE) {
            m_specs.localized = true;
        }
        else {
            on_error("'L' flag invalid when SCN_DISABLE_LOCALE is on");
        }
    }

    constexpr void on_width(int width)
    {
        if (m_specs.precision != 0 && width > m_specs.precision) {
            // clang-format off
            return this->on_error("Width (i.e., minimum field length) cannot be larger than precision (i.e., maximum field length)");
            // clang-format on
        }

        m_specs.width = width;
    }
    constexpr void on_precision(int prec)
    {
        if (m_specs.width > prec) {
            // clang-format off
            return this->on_error("Width (i.e., minimum field length) cannot be larger than precision (i.e., maximum field length)");
            // clang-format on
        }

        m_specs.precision = prec;
    }

    constexpr void on_type(presentation_type type)
    {
        m_specs.type = type;
    }

    constexpr void on_charset_single(char32_t cp)
    {
        const auto cp_value = static_cast<unsigned>(cp);
        if (SCN_LIKELY(cp_value <= 127)) {
            m_specs.charset_literals[cp_value / 8] |=
                static_cast<unsigned char>(1ul << (cp_value % 8));
        }
        else {
            m_specs.charset_has_nonascii = true;
        }
    }

    constexpr void on_charset_range(char32_t begin, char32_t end)
    {
        const auto begin_value = static_cast<unsigned>(begin);
        const auto end_value = static_cast<unsigned>(end);
        SCN_EXPECT(begin_value < end_value);

        if (SCN_LIKELY(end_value <= 127)) {
            // No need to bit-twiddle with a mask, because with the
            // SCN_ASSUME, -O3 will optimize this to a single operation
            SCN_ASSUME(begin_value < end_value);
            for (auto v = begin_value; v != end_value; ++v) {
                m_specs.charset_literals[v / 8] |=
                    static_cast<unsigned char>(1ul << (v % 8));
            }
        }
        else {
            m_specs.charset_has_nonascii = true;
        }
    }

    constexpr void on_charset_inverted()
    {
        m_specs.charset_is_inverted = true;
    }

    template <typename CharT>
    constexpr void on_character_set_string(std::basic_string_view<CharT> fmt)
    {
        m_specs.charset_string_data = fmt.data();
        m_specs.charset_string_size = fmt.size();
        on_type(presentation_type::string_set);
    }

#if !SCN_DISABLE_REGEX
    template <typename CharT>
    constexpr void on_regex_pattern(std::basic_string_view<CharT> pattern)
    {
        m_specs.charset_string_data = pattern.data();
        m_specs.charset_string_size = pattern.size();
    }
    constexpr void on_regex_flags(regex_flags flags)
    {
        m_specs.regexp_flags = flags;
    }
#endif

    // Intentionally not constexpr to get a compiler-time error when called
    /*not-constexpr*/ void on_error(const char* msg)
    {
        SCN_UNLIKELY_ATTR
        m_error = unexpected_scan_error(scan_error::invalid_format_string, msg);
    }
    /*not-constexpr*/ void on_error(scan_error err)
    {
        SCN_UNLIKELY_ATTR
        m_error = unexpected(err);
    }

    constexpr scan_expected<void> get_error() const
    {
        return m_error;
    }

protected:
    format_specs& m_specs;
    scan_expected<void> m_error;
};

template <typename CharT>
constexpr int parse_simple_int(const CharT*& begin, const CharT* end)
{
    SCN_EXPECT(begin != end);
    SCN_EXPECT(*begin >= '0' && *begin <= '9');

    unsigned long long value = 0;
    do {
        value *= 10;
        value += static_cast<unsigned long long>(*begin - '0');
        if (value >
            static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
            return -1;
        }
        ++begin;
    } while (begin != end && *begin >= '0' && *begin <= '9');
    return static_cast<int>(value);
}

template <typename CharT, typename IDHandler>
constexpr const CharT* do_parse_arg_id(const CharT* begin,
                                       const CharT* end,
                                       IDHandler&& handler)
{
    SCN_EXPECT(begin != end);

    CharT c = *begin;
    if (c < CharT{'0'} || c > CharT{'9'}) {
        handler.on_error("Invalid argument ID");
        return begin;
    }

    int idx = 0;
    if (c != CharT{'0'}) {
        idx = parse_simple_int(begin, end);
    }
    else {
        ++begin;
    }

    if (begin == end || (*begin != CharT{'}'} && *begin != CharT{':'})) {
        handler.on_error("Invalid argument ID");
        return begin;
    }
    handler(static_cast<std::size_t>(idx));

    return begin;
}

template <typename CharT, typename IDHandler>
constexpr const CharT* parse_arg_id(const CharT* begin,
                                    const CharT* end,
                                    IDHandler&& handler)
{
    SCN_EXPECT(begin != end);
    if (*begin != '}' && *begin != ':') {
        return do_parse_arg_id(begin, end, SCN_FWD(handler));
    }

    handler();
    return begin;
}

template <typename CharT>
constexpr presentation_type parse_presentation_type(CharT type)
{
    switch (type) {
        case 'b':
        case 'B':
            return presentation_type::int_binary;
        case 'd':
            return presentation_type::int_decimal;
        case 'i':
            return presentation_type::int_generic;
        case 'u':
            return presentation_type::int_unsigned_decimal;
        case 'o':
            return presentation_type::int_octal;
        case 'x':
        case 'X':
            return presentation_type::int_hex;
        case 'r':
        case 'R':
            return presentation_type::int_arbitrary_base;
        case 'a':
        case 'A':
            return presentation_type::float_hex;
        case 'e':
        case 'E':
            return presentation_type::float_scientific;
        case 'f':
        case 'F':
            return presentation_type::float_fixed;
        case 'g':
        case 'G':
            return presentation_type::float_general;
        case 's':
            return presentation_type::string;
        case 'c':
            return presentation_type::character;
        case '?':
            return presentation_type::escaped_character;
        case 'p':
            return presentation_type::pointer;
        case '[':
        case '/':
            // Should be handled by parse_presentation_set and
            // parse_presentation_regex
#if !SCN_GCC || SCN_GCC >= SCN_COMPILER(9, 0, 0)
            // gcc 7 thinks we'll get here, even when we won't
            // gcc 8 has a bug in debug mode:
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86678
            SCN_EXPECT(false);
#endif
            SCN_UNREACHABLE;
        default:
            return presentation_type::none;
    }
}

template <typename CharT>
constexpr bool is_ascii_letter(CharT ch)
{
    return (ch >= CharT{'a'} && ch <= CharT{'z'}) ||
           (ch >= CharT{'A'} && ch <= CharT{'Z'});
}

template <typename CharT>
constexpr int code_point_length(const CharT* begin, const CharT* end)
{
    SCN_EXPECT(begin != end);
    if constexpr (sizeof(CharT) != 1) {
        return 1;
    }
    else {
        const auto lengths =
            "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\2\2\2\2\3"
            "\3\4";
        const int len = lengths[static_cast<unsigned char>(*begin) >> 3];
        return len;
    }
}

template <typename CharT, typename Handler>
constexpr const CharT* parse_align(const CharT* begin,
                                   const CharT* end,
                                   Handler&& handler)
{
    SCN_EXPECT(begin != end);

    auto check_align = [](wchar_t ch) {
        switch (ch) {
            case L'<':
                return align_type::left;
            case L'>':
                return align_type::right;
            case L'^':
                return align_type::center;
            default:
                return align_type::none;
        }
    };

    auto potential_fill_len = code_point_length(begin, end);
    if (SCN_UNLIKELY(potential_fill_len == 0 ||
                     std::distance(begin, end) < potential_fill_len)) {
        handler.on_error("Invalid encoding in fill character");
        return begin;
    }

    auto potential_align_on_fill = check_align(static_cast<wchar_t>(*begin));

    auto potential_fill = std::basic_string_view<CharT>{
        begin, static_cast<size_t>(potential_fill_len)};
    const auto begin_before_fill = begin;
    begin += potential_fill_len;

    if (begin == end) {
        return begin_before_fill;
    }

    auto potential_align_after_fill = check_align(static_cast<wchar_t>(*begin));
    const auto begin_after_fill = begin;
    ++begin;

    if (potential_fill_len == 1) {
        if (SCN_UNLIKELY(potential_fill[0] == '{')) {
            handler.on_error("Invalid fill character '{' in format string");
            return begin;
        }
        if (potential_fill[0] == '[') {
            return begin_before_fill;
        }
    }

    if (potential_align_after_fill == align_type::none) {
        if (potential_align_on_fill != align_type::none) {
            handler.on_align(potential_align_on_fill);
            return begin_after_fill;
        }
        return begin_before_fill;
    }

    handler.on_fill(potential_fill);
    handler.on_align(potential_align_after_fill);
    return begin;
}

template <typename CharT, typename Handler>
constexpr const CharT* parse_width(const CharT* begin,
                                   const CharT* end,
                                   Handler&& handler)
{
    SCN_EXPECT(begin != end);

    if (*begin >= CharT{'0'} && *begin <= CharT{'9'}) {
        int width = parse_simple_int(begin, end);
        if (SCN_LIKELY(width != -1)) {
            handler.on_width(width);
        }
        else {
            handler.on_error("Invalid field width");
            return begin;
        }
    }
    return begin;
}

template <typename CharT, typename Handler>
constexpr const CharT* parse_precision(const CharT* begin,
                                       const CharT* end,
                                       Handler&& handler)
{
    SCN_EXPECT(begin != end);

    if (*begin >= CharT{'0'} && *begin <= CharT{'9'}) {
        int prec = parse_simple_int(begin, end);
        if (SCN_LIKELY(prec != -1)) {
            handler.on_precision(prec);
        }
        else {
            handler.on_error("Invalid field precision");
            return begin;
        }
    }
    return begin;
}

template <typename CharT, typename SpecHandler>
constexpr char32_t parse_presentation_set_code_point(const CharT*& begin,
                                                     const CharT* end,
                                                     SpecHandler&& handler)
{
    SCN_EXPECT(begin != end);

    auto len = code_point_length_by_starting_code_unit(*begin);
    if (SCN_UNLIKELY(len == 0 || static_cast<size_t>(end - begin) < len)) {
        handler.on_error("Invalid encoding in format string");
        return invalid_code_point;
    }

    const auto cp =
        decode_code_point_exhaustive(std::basic_string_view<CharT>{begin, len});
    if (SCN_UNLIKELY(cp >= invalid_code_point)) {
        handler.on_error("Invalid encoding in format string");
        return invalid_code_point;
    }

    begin += len;
    return cp;
}

template <typename CharT, typename SpecHandler>
constexpr void parse_presentation_set_literal(const CharT*& begin,
                                              const CharT* end,
                                              SpecHandler&& handler)
{
    SCN_EXPECT(begin != end);

    auto cp_first = parse_presentation_set_code_point(begin, end, handler);
    if (SCN_UNLIKELY(cp_first >= invalid_code_point)) {
        return;
    }

    if (begin != end && *begin == CharT{'-'} && (begin + 1) != end &&
        *(begin + 1) != CharT{']'}) {
        ++begin;

        auto cp_second = parse_presentation_set_code_point(begin, end, handler);
        if (SCN_UNLIKELY(cp_second >= invalid_code_point)) {
            return;
        }

        if (SCN_UNLIKELY(cp_second < cp_first)) {
            // clang-format off
            handler.on_error("Invalid range in [character set] format string argument: Range end before the beginning");
            // clang-format on
            return;
        }

        handler.on_charset_range(cp_first, cp_second + 1);
        return;
    }

    handler.on_charset_single(cp_first);
}

template <typename CharT, typename SpecHandler>
constexpr std::basic_string_view<CharT> parse_presentation_set(
    const CharT*& begin,
    const CharT* end,
    SpecHandler&& handler)
{
    SCN_EXPECT(begin != end);
    SCN_EXPECT(*begin == CharT{'['});

    auto start = begin;
    ++begin;

    if (SCN_UNLIKELY(begin == end)) {
        // clang-format off
        handler.on_error("Unexpected end of [character set] specifier in format string");
        // clang-format on
        return {};
    }
    if (*begin == CharT{'^'}) {
        handler.on_charset_inverted();
        ++begin;
        if (SCN_UNLIKELY(begin == end)) {
            // clang-format off
            handler.on_error("Unexpected end of [character set] specifier in format string");
            // clang-format on
            return {};
        }
        if (*begin == CharT{']'}) {
            handler.on_charset_single(char32_t{']'});
            ++begin;
        }
    }
    else if (*begin == CharT{']'}) {
        return {start, static_cast<size_t>(std::distance(start, ++begin))};
    }

    while (begin != end) {
        if (SCN_UNLIKELY(!handler.get_error())) {
            break;
        }

        if (*begin == CharT{']'}) {
            return {start, static_cast<size_t>(std::distance(start, ++begin))};
        }

        parse_presentation_set_literal(begin, end, handler);
    }

    SCN_UNLIKELY_ATTR
    handler.on_error("Invalid [character set] specifier in format string");
    return {};
}

template <typename CharT, typename SpecHandler>
constexpr const CharT* parse_presentation_regex(const CharT*& begin,
                                                const CharT* end,
                                                SpecHandler&& handler)
{
#if !SCN_DISABLE_REGEX
    SCN_EXPECT(begin != end);
    SCN_EXPECT(*begin == CharT{'/'});

    if constexpr (!SCN_REGEX_SUPPORTS_WIDE_STRINGS &&
                  std::is_same_v<CharT, wchar_t>) {
        handler.on_error("Regex backend doesn't support wide strings as input");
        return begin;
    }

    auto start = begin;
    ++begin;

    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    handler.on_type(presentation_type::regex);
    for (; begin != end; ++begin) {
        if (*begin == CharT{'/'}) {
            if (*(begin - 1) != CharT{'\\'}) {
                break;
            }
            else {
                handler.on_type(presentation_type::regex_escaped);
            }
        }
    }
    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    auto regex_end = begin;
    auto regex_pattern = make_string_view_from_pointers(start + 1, regex_end);
    if (SCN_UNLIKELY(regex_pattern.empty())) {
        handler.on_error("Invalid (empty) regex in format string");
        return begin;
    }
    handler.on_regex_pattern(regex_pattern);
    ++begin;

    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    regex_flags flags{regex_flags::none};
    constexpr std::array<std::pair<char, regex_flags>, 4> flag_map{
        {{'m', regex_flags::multiline},
         {'s', regex_flags::singleline},
         {'i', regex_flags::nocase},
         {'n', regex_flags::nocapture}}};
    for (; begin != end; ++begin) {
        if (*begin == CharT{'}'}) {
            break;
        }
        bool found_flag = false;
        for (auto flag : flag_map) {
            if (static_cast<CharT>(flag.first) != *begin) {
                continue;
            }
            if ((flags & flag.second) != regex_flags::none) {
                handler.on_error("Flag set multiple times in regex");
                return begin;
            }
#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
            if (*begin == CharT{'s'}) {
                // clang-format off
                handler.on_error("/s flag for regex isn't supported by regex backend");
                // clang-format on
            }
#if !SCN_HAS_STD_REGEX_MULTILINE
            if (*begin == CharT{'m'}) {
                // clang-format off
                handler.on_error("/m flag for regex isn't supported by regex backend");
                // clang-format on
            }
#endif
#endif
            flags |= flag.second;
            found_flag = true;
            break;
        }
        if (!found_flag) {
            handler.on_error("Invalid flag in regex");
            return begin;
        }
    }
    handler.on_regex_flags(flags);

    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of regex in format string");
        return begin;
    }

    return begin;
#else
    SCN_UNUSED(begin);
    SCN_UNUSED(end);
    handler.on_error("Regular expression support is disabled");
    return {};
#endif
}

template <typename CharT, typename SpecHandler>
constexpr const CharT* parse_format_specs(const CharT* begin,
                                          const CharT* end,
                                          SpecHandler&& handler)
{
    auto do_presentation = [&]() -> const CharT* {
        if (*begin == CharT{'['}) {
            auto set = parse_presentation_set(begin, end, handler);
            if (SCN_UNLIKELY(set.size() <= 2)) {
                // clang-format off
                handler.on_error("Invalid (empty) [character set] specifier in format string");
                // clang-format on
                return begin;
            }
            handler.on_character_set_string(set);
            return begin;
        }
        if (*begin == CharT{'/'}) {
            return parse_presentation_regex(begin, end, handler);
        }
        presentation_type type = parse_presentation_type(*begin++);
        if (SCN_UNLIKELY(type == presentation_type::none)) {
            handler.on_error("Invalid type specifier in format string");
            return begin;
        }
        handler.on_type(type);
        return begin;
    };

    if (end - begin > 1 && *(begin + 1) == CharT{'}'} &&
        is_ascii_letter(*begin) && *begin != CharT{'L'}) {
        return do_presentation();
    }

    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of format string");
        return begin;
    }

    begin = parse_align(begin, end, handler);
    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of format string");
        return begin;
    }

    begin = parse_width(begin, end, handler);
    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of format string");
        return begin;
    }

    if (*begin == CharT{'.'}) {
        ++begin;
        if (SCN_UNLIKELY(begin == end)) {
            handler.on_error("Unexpected end of format string");
            return begin;
        }
        begin = parse_precision(begin, end, handler);
        if (SCN_UNLIKELY(begin == end)) {
            handler.on_error("Unexpected end of format string");
            return begin;
        }
    }

    if (*begin == CharT{'L'}) {
        handler.on_localized();
        ++begin;
    }
    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of format string");
        return begin;
    }

    if (begin != end && *begin != CharT{'}'}) {
        do_presentation();
    }
    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of format string");
        return begin;
    }

    return begin;
}

template <typename CharT, typename Handler>
constexpr const CharT* parse_replacement_field(const CharT* begin,
                                               const CharT* end,
                                               Handler& handler)
{
    struct id_adapter {
        constexpr void operator()()
        {
            arg_id = handler.on_arg_id();
        }
        constexpr void operator()(std::size_t id)
        {
            arg_id = handler.on_arg_id(id);
        }

        constexpr void on_error(const char* msg)
        {
            SCN_UNLIKELY_ATTR
            handler.on_error(msg);
        }

        Handler& handler;
        std::size_t arg_id;
    };

    ++begin;
    if (SCN_UNLIKELY(begin == end)) {
        handler.on_error("Unexpected end of replacement field");
        return begin;
    }

    if (*begin == CharT{'}'}) {
        handler.on_replacement_field(handler.on_arg_id(), begin);
    }
    else if (*begin == CharT{'{'}) {
        handler.on_literal_text(begin, begin + 1);
    }
    else {
        auto adapter = id_adapter{handler, 0};
        begin = parse_arg_id(begin, end, adapter);

        if (SCN_UNLIKELY(begin == end)) {
            handler.on_error("Missing '}' in format string");
            return begin;
        }

        if (*begin == CharT{'}'}) {
            handler.on_replacement_field(adapter.arg_id, begin);
        }
        else if (*begin == CharT{':'}) {
            if (SCN_UNLIKELY(begin + 1 == end)) {
                handler.on_error("Unexpected end of replacement field");
                return begin;
            }
            begin = handler.on_format_specs(adapter.arg_id, begin + 1, end);
            if (SCN_UNLIKELY(begin == end || *begin != '}')) {
                handler.on_error("Unknown format specifier");
                return begin;
            }
        }
        else {
            SCN_UNLIKELY_ATTR
            handler.on_error("Missing '}' in format string");
            return begin;
        }
    }
    return begin + 1;
}

template <bool IsConstexpr, typename CharT, typename Handler>
constexpr void parse_format_string_impl(std::basic_string_view<CharT> format,
                                        Handler&& handler)
{
    // TODO: memchr fast path with a larger (> 32) format string

    auto begin = format.data();
    auto it = begin;
    const auto end = format.data() + format.size();

    while (it != end) {
        const auto ch = *it++;
        if (ch == CharT{'{'}) {
            handler.on_literal_text(begin, it - 1);

            begin = it = parse_replacement_field(it - 1, end, handler);
            if (!handler.get_error()) {
                return;
            }
        }
        else if (ch == CharT{'}'}) {
            if (SCN_UNLIKELY(it == end || *it != CharT{'}'})) {
                handler.on_error("Unmatched '}' in format string");
                return;
            }

            handler.on_literal_text(begin, it);
            begin = ++it;
        }
    }

    handler.on_literal_text(begin, end);
}

template <bool IsConstexpr, typename CharT, typename Handler>
constexpr scan_expected<void> parse_format_string(
    std::basic_string_view<CharT> format,
    Handler&& handler)
{
    parse_format_string_impl<IsConstexpr>(format, handler);
    handler.check_args_exhausted();
    return handler.get_error();
}

enum class arg_type_category {
    none,
    integer,
    unsigned_integer,
    floating,
    string,
    pointer,
    boolean,
    character,
    custom
};

constexpr arg_type_category get_category_for_arg_type(arg_type type)
{
    switch (type) {
        case arg_type::none_type:
            return arg_type_category::none;

        case arg_type::schar_type:
        case arg_type::short_type:
        case arg_type::int_type:
        case arg_type::long_type:
        case arg_type::llong_type:
        case arg_type::int128_type:
            return arg_type_category::integer;

        case arg_type::uchar_type:
        case arg_type::ushort_type:
        case arg_type::uint_type:
        case arg_type::ulong_type:
        case arg_type::ullong_type:
        case arg_type::uint128_type:
            return arg_type_category::unsigned_integer;

        case arg_type::pointer_type:
            return arg_type_category::pointer;
        case arg_type::bool_type:
            return arg_type_category::boolean;
        case arg_type::narrow_character_type:
        case arg_type::wide_character_type:
        case arg_type::code_point_type:
            return arg_type_category::character;

        case arg_type::float_type:
        case arg_type::double_type:
        case arg_type::ldouble_type:
        case arg_type::float16_type:
        case arg_type::float32_type:
        case arg_type::float64_type:
        case arg_type::float128_type:
        case arg_type::bfloat16_type:
            return arg_type_category::floating;

        case arg_type::narrow_string_type:
        case arg_type::wide_string_type:
        case arg_type::string_view_type:
            return arg_type_category::string;

        case arg_type::custom_type:
            return arg_type_category::custom;

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wcovered-switch-default")
        default:
            SCN_ENSURE(false);
            SCN_UNREACHABLE;
            SCN_CLANG_POP
    }

    SCN_UNREACHABLE;
}

template <typename Handler>
class specs_checker : public Handler {
public:
    template <typename H>
    constexpr specs_checker(H&& handler, arg_type type)
        : Handler(SCN_FWD(handler)), m_arg_type(type)
    {
        SCN_EXPECT(m_arg_type != arg_type::custom_type);
    }

    constexpr void on_localized()
    {
        const auto cat = get_category_for_arg_type(m_arg_type);
        if (cat != arg_type_category::integer &&
            cat != arg_type_category::unsigned_integer &&
            cat != arg_type_category::floating &&
            cat != arg_type_category::boolean) {
            SCN_UNLIKELY_ATTR
            // clang-format off
            return this->on_error("'L' specifier can only be used with arguments of integer, floating-point, or boolean types");
            // clang-format on
        }

        Handler::on_localized();
    }

private:
    arg_type m_arg_type;
};

template <typename Handler>
constexpr void check_int_type_specs(const format_specs& specs,
                                    Handler&& handler)
{
    if (SCN_UNLIKELY(specs.type > presentation_type::int_hex)) {
        return handler.on_error("Invalid type specifier for integer type");
    }
    if (specs.localized) {
        if (SCN_UNLIKELY(specs.type == presentation_type::int_binary)) {
            // clang-format off
            handler.on_error("'b'/'B' specifier not supported for localized integers");
            // clang-format on
            return;
        }
        if (SCN_UNLIKELY(specs.type == presentation_type::int_arbitrary_base)) {
            // clang-format off
            return handler.on_error("Arbitrary bases not supported for localized integers");
            // clang-format on
        }
    }
}

template <typename Handler>
constexpr void check_char_type_specs(const format_specs& specs,
                                     Handler&& handler)
{
    if (specs.type > presentation_type::int_hex ||
        specs.type == presentation_type::int_arbitrary_base) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for character type");
    }
}

template <typename Handler>
constexpr void check_code_point_type_specs(const format_specs& specs,
                                           Handler&& handler)
{
    if (specs.type != presentation_type::none &&
        specs.type != presentation_type::character) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for code point type");
    }
}

template <typename Handler>
constexpr void check_float_type_specs(const format_specs& specs,
                                      Handler&& handler)
{
    if (specs.type != presentation_type::none &&
        (specs.type < presentation_type::float_hex ||
         specs.type > presentation_type::float_general)) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for float type");
    }
}

template <typename Handler>
constexpr void check_string_type_specs(const format_specs& specs,
                                       Handler&& handler)
{
    if (specs.type == presentation_type::none ||
        specs.type == presentation_type::string ||
        specs.type == presentation_type::string_set
#if !SCN_DISABLE_REGEX
        || specs.type == presentation_type::regex ||
        specs.type == presentation_type::regex_escaped
#endif
    ) {
        return;
    }
    if (specs.type == presentation_type::character) {
        if (SCN_UNLIKELY(specs.precision == 0)) {
            // clang-format off
            return handler.on_error("'c' type specifier for strings requires the field precision to be specified");
            // clang-format on
        }
        return;
    }
    SCN_UNLIKELY_ATTR
    handler.on_error("Invalid type specifier for string");
}

template <typename Handler>
constexpr void check_pointer_type_specs(const format_specs& specs,
                                        Handler&& handler)
{
    if (specs.type != presentation_type::none &&
        specs.type != presentation_type::pointer) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for pointer");
    }
}

template <typename Handler>
constexpr void check_bool_type_specs(const format_specs& specs,
                                     Handler&& handler)
{
    if (specs.type != presentation_type::none &&
        specs.type != presentation_type::string &&
        specs.type != presentation_type::int_generic &&
        specs.type != presentation_type::int_hex &&
        specs.type != presentation_type::int_binary &&
        specs.type != presentation_type::int_unsigned_decimal &&
        specs.type != presentation_type::int_octal &&
        specs.type != presentation_type::int_decimal) {
        SCN_UNLIKELY_ATTR
        return handler.on_error("Invalid type specifier for boolean");
    }
}

#if !SCN_DISABLE_REGEX
template <typename Handler>
constexpr void check_regex_type_specs(const format_specs& specs,
                                      Handler&& handler)
{
    if (SCN_UNLIKELY(specs.type == presentation_type::none ||
                     specs.charset_string_size == 0)) {
        // clang-format off
        return handler.on_error("Regular expression needs to be specified when reading regex_matches");
        // clang-format on
    }
    if (specs.type == presentation_type::regex ||
        specs.type == presentation_type::regex_escaped) {
        return;
    }
    SCN_UNLIKELY_ATTR
    handler.on_error("Invalid type specifier for regex_matches");
}
#endif

}  // namespace detail

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wdocumentation-unknown-command")

/**
 * \defgroup format-string Format strings
 *
 * \brief Format string description
 *
 * The format string syntax is heavily influenced by {fmt} and
 * `std::format`, and is largely compatible with it. Scanning functions,
 * such as `scn::scan` and
 * `scn::input`, use the format string syntax described in this section.
 *
 * Format strings consist of:
 *
 *  * Replacement fields, which are surrounded by curly braces `{}`.
 *
 *  * Non-whitespace characters (except `{}`; for literal braces, use
 *    `{{` and `}}`), which consume exactly one identical character from the
 *    input
 *
 *  * Whitespace characters, which consume any and all available consecutive
 *    whitespace from the input.
 *
 * Literal characters are matched by code point one-to-one, with no
 * normalization being done.
 * `Ä` (U+00C4, UTF-8 0xc3 0x84) only matches another U+00C4, and not, for
 * example, U+00A8 (DIAERESIS) and U+0041 (LATIN CAPITAL LETTER A).
 *
 * Characters (code points) are considered to be whitespace characters by
 * the Unicode Pattern_White_Space property, as defined by UAX31-R3a.
 * These code points are:
 *
 *  * ASCII whitespace characters ("\t\n\v\f\r ")
 *  * U+0085 (next line)
 *  * U+200E and U+200F (LEFT-TO-RIGHT MARK and RIGHT-TO-LEFT MARK)
 *  * U+2028 and U+2029 (LINE SEPARATOR and PARAGRAPH SEPARATOR)
 *
 * The grammar for a replacement field is as follows:
 *
 * \code
 * replacement-field   ::= '{' [arg-id] [':' format-spec] '}'
 * arg-id              ::= positive-integer
 *
 * format-spec         ::= [fill-and-align]
 *                         [width] [precision]
 *                         ['L'] [type]
 * fill-and-align      ::= [fill] align
 * fill                ::= any character other than
 *                         '{' or '}'
 * align               ::= one of '<' '>' '^'
 * width               ::= positive-integer
 * precision           ::= '.' nonnegative-integer
 * type                ::= 'a' | 'A' | 'b' | 'B' | 'c' | 'd' |
 *                         'e' | 'E' | 'f' | 'F' | 'g' | 'G' |
 *                         'o' | 'p' | 's' | 'x' | 'X' | 'i' | 'u'
 * \endcode
 *
 * \section arg-ids Argument IDs
 *
 * The `arg-id` specifier can be used to index arguments manually.
 * If manual indexing is used, all of the indices in a format string must be
 * stated explicitly. The same `arg-id` can appear in the format string
 * only once, and must refer to a valid argument.
 *
 * \code{.cpp}
 * // Format string equivalent to "{0} to {1}"
 * auto a = scn::scan<int, int>("2 to 300", "{} to {}");
 * // a->values() == (2, 300)
 *
 * // Manual indexing
 * auto b = scn::scan<int, int>("2 to 300", "{1} to {0}");
 * // b->values() == (300, 2)
 *
 * // INVALID:
 * // Automatic and manual indexing is mixed
 * auto c = scn::scan<int, int>("2 to 300", "{} to {0}");
 *
 * // INVALID:
 * // Same argument is referred to multiple times
 * auto d = scn::scan<int, int>("2 to 300", "{0} to {0}");
 *
 * // INVALID:
 * // {2} does not refer to an argument
 * auto e = scn::scan<int, int>("2 to 300", "{0} to {2}");
 * \endcode
 *
 * \section fill-and-align Fill and align
 *
 * Alignment allows for skipping character before and/or after a value.
 * There are three possible values for alignment:
 *
 * <table>
 * <caption id="align-table">
 * Alignment options
 * </caption>
 *
 * <tr><th>Option</th> <th>Meaning</th></tr>
 *
 * <tr>
 * <td>`<`</td>
 * <td>
 * Align the value to the left (skips fill characters after the value)
 * </td>
 * </tr>
 *
 * <tr>
 * <td>`>`</td>
 * <td>
 * Align the value to the right (skips fill characters before the value)
 * </td>
 * </tr>
 *
 * <tr>
 * <td>`^`</td>
 * <td>
 * Align the value to the center
 * (skips fill characters both before and after the value)
 * </td>
 * </tr>
 * </table>
 *
 * The fill character can be any Unicode code point, except for `{` and `}`.
 * The default fill is the space character `' '`.
 *
 * For format type specifiers other than `c` (default for `char` and `wchar_t`,
 * available for `string` and `string_view`), `[...]`, and the regex `/.../`,
 * the default alignment is `>`.
 * Otherwise, the default alignment is `<`.
 *
 * In addition to the skipping of fill characters,
 * for format type specifiers with the `>` default alignment,
 * preceding whitespace is automatically skipped.
 * This preceding whitespace isn't counted as part of the field width,
 * as described below.
 *
 * The number of fill characters consumed can be controlled with the width and
 * precision specifiers.
 *
 * \section width Width
 *
 * Width specifies the minimum number of characters that will be read from
 * the source range. It can be any unsigned integer. Any fill characters skipped
 * are included in the width
 *
 * For the purposes of width calculation, the same algorithm is used that in
 * {fmt}. Every code point has a width of one, except the following ones
 * have a width of 2:
 *
 * * any code point with the East_Asian_Width="W" or East_Asian_Width="F"
 *   Derived Extracted Property as described by UAX#44
 * * U+4DC0 – U+4DFF (Yijing Hexagram Symbols)
 * * U+1F300 – U+1F5FF (Miscellaneous Symbols and Pictographs)
 * * U+1F900 – U+1F9FF (Supplemental Symbols and Pictographs)
 *
 * \section precision Precision
 *
 * Precision specifies the maximum number of characters that will be read from
 * the source range. The method for counting characters is the same as above,
 * with the width field.
 *
 * \section localized Localized
 *
 * The `L` flag enables localized scanning.
 * Its effects are different for each type it is used with:
 *
 *  * For integers, it enables locale-specific thousands separators
 *  * For floating-point numbers, it enables locale-specific thousands and
 *    radix (decimal) separators
 *  * For booleans, it enables locale-specific textual representations (for
 *    `true` and `false`)
 *  * For other types, it has no effect
 *
 * \section type Type specifier
 *
 * The type specifier determines how the data is to be scanned.
 * The type of the argument to be scanned determines what flags are valid.
 *
 * \subsection type-string Type specifier: strings
 *
 * <table>
 * <caption id="type-string-table">
 * String types (`std::basic_string` and `std::basic_string_view`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>none, `s`</td>
 * <td>
 * Copies from the input until a whitespace character is encountered, or,
 * if using the `<` (left) or `^` (center) alignment,
 * a fill character is encountered.
 * </td>
 * </tr>
 * <tr>
 * <td>`c`</td>
 * <td>
 * Copies from the input until the field width is exhausted.
 * Doesn't skip preceding whitespace.
 * Errors if no field precision is provided.
 * </td>
 * </tr>
 * <tr>
 * <td>`[...]`</td>
 * <td>
 * Character set matching: copies from the input until a character not specified
 * in the set is encountered. Character ranges can be specified with `-`, and
 * the entire selection can be inverted with a prefix `^`. Matches and supports
 * arbitrary Unicode code points.
 * Doesn't skip preceding whitespace.
 * </td>
 * </tr>
 * <tr>
 * <td>`/<regex>/<flags>`</td>
 * <td>
 * Regular expression matching: copies from the input until the input does not
 * match the regex.
 * Doesn't skip preceding whitespace.
 * \see regex
 * </td>
 * </tr>
 * </table>
 *
 * \note `std::basic_string_view` can only be scanned if the source is
 * contiguous.
 *
 * \subsection type-int Type specifier: integers
 *
 * Integer values are scanned as if by using `std::from_chars`,
 * except a positive `+` sign and a base prefix (like `0x`) are always
 * allowed to be present.
 *
 * <table>
 * <caption id="type-int-table">
 * Integer types (`signed` and `unsigned` variants of `char`, `short`,
 * `int`, `long`, and `long long`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>`b`, `B`</td>
 * <td>
 * `std::from_chars` with base `2`. The base prefix is `0b` or `0B`.
 * </td>
 * </tr>
 * <tr>
 * <td>`o`, `O`</td>
 * <td>
 * `std::from_chars` with base `8`. The base prefix is `0o` or `0O`, or just
 * `0`.
 * </td>
 * </tr>
 * <tr>
 * <td>`x`, `X`</td>
 * <td>
 * `std::from_chars` with base `16`. The base prefix is `0x` or `0X`.
 * </td>
 * </tr>
 * <tr>
 * <td>`d`</td>
 * <td>
 * `std::from_chars` with base `10`. No base prefix allowed.
 * </td>
 * </tr>
 * <tr>
 * <td>`u`</td>
 * <td>
 * `std::from_chars` with base `10`. No base prefix or `-` sign allowed.
 * </td>
 * </tr>
 * <tr>
 * <td>`i`</td>
 * <td>
 * Detect the base from a possible prefix, defaulting to decimal (base-10).
 * </td>
 * </tr>
 * <tr>
 * <td>`rXX` (where XX = [2, 36])</td>
 * <td>
 * Custom base, without a base prefix (r stands for radix).
 * </td>
 * </tr>
 * <tr>
 * <td>`c`</td>
 * <td>
 * Copies a character (code unit) from the input.
 * </td>
 * </tr>
 * <tr>
 * <td>none</td>
 * <td>
 * Same as `d`.
 * </td>
 * </tr>
 * </table>
 *
 * \subsection type-char Type specifier: characters
 *
 * <table>
 * <caption id="type-char-table">
 * Character types (`char` and `wchar_t`), and code points (`char32_t`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>none, `c`</td>
 * <td>
 * Copies a character (code point for `char32_t`, code unit otherwise) from the
 * input.
 * </td>
 * </tr>
 * <tr>
 * <td>`b`, `B`, `d`, `i`, `o`, `O`, `u`, `x`, `X`</td>
 * <td>
 * Same as for integers, see above \ref type-int. Not allowed for `char32_t`.
 * </td>
 * </tr>
 * </table>
 *
 * \note When scanning characters (`char` and `wchar_t`), the source range is
 * read a single code unit at a time, and encoding is not respected.
 *
 * \subsection type-float Type specifier: floating-point values
 *
 * Floating-point values are scanned as if by using `std::from_chars`,
 * except a positive `+` sign and a base prefix (like `0x`) are always
 * allowed to be present.
 *
 * <table>
 * <caption id="type-float-table">
 * Floating-point types (`float`, `double`, and `long double`)
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>`a`, `A`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::hex`.
 * Prefix `0x`/`0X` is allowed.
 * </td>
 * </tr>
 * <tr>
 * <td>`e`, `E`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::scientific`.
 * </td>
 * </tr>
 * <tr>
 * <td>`f`, `F`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::fixed`.
 * </td>
 * </tr>
 * <tr>
 * <td>`g`, `G`</td>
 * <td>
 * `std::from_chars` with `std::chars_format::general`.
 * </td>
 * </tr>
 * <tr>
 * <td>none</td>
 * <td>
 * `std::from_chars` with `std::chars_format::general | std::chars_format::hex`.
 * Prefix `0x`/`0X` is allowed.
 * </td>
 * </tr>
 * </table>
 *
 * \subsection type-bool Type specifier: booleans
 *
 * <table>
 * <caption id="type-bool-table">
 * `bool`
 * </caption>
 * <tr><th>Type</th> <th>Meaning</th></tr>
 * <tr>
 * <td>`s`</td>
 * <td>
 * Allows for the textual representation (`true` or `false`).
 * </td>
 * </tr>
 * <tr>
 * <td>`b`, `B`, `d`, `i`, `o`, `O`, `u`, `x`, `X`</td>
 * <td>
 * Allows for the integral/numeric representation (`0` or `1`).
 * </td>
 * </tr>
 * <tr>
 * <td>none</td>
 * <td>
 * Allows for both the textual and the integral/numeric representation.
 * </td>
 * </tr>
 * </table>
 */

SCN_CLANG_POP  // -Wdocumentation-unknown-command

    namespace detail
{
}

namespace detail {
/**
 * A runtime format string
 *
 * \ingroup format-string
 */
template <typename CharT>
struct basic_runtime_format_string {
    basic_runtime_format_string(std::basic_string_view<CharT> s) : str(s) {}

    basic_runtime_format_string(const basic_runtime_format_string&) = delete;
    basic_runtime_format_string(basic_runtime_format_string&&) = delete;
    basic_runtime_format_string& operator=(const basic_runtime_format_string&) =
        delete;
    basic_runtime_format_string& operator=(basic_runtime_format_string&&) =
        delete;
    ~basic_runtime_format_string() = default;

    std::basic_string_view<CharT> str;
};
}  // namespace detail

/**
 * Create a runtime format string
 *
 * Can be used to avoid compile-time format string checking
 *
 * \ingroup format-string
 */
inline detail::basic_runtime_format_string<char> runtime_format(
    std::string_view s)
{
    return s;
}
inline detail::basic_runtime_format_string<wchar_t> runtime_format(
    std::wstring_view s)
{
    return s;
}

namespace detail {
struct compile_string {};

template <typename Str>
inline constexpr bool is_compile_string_v =
    std::is_base_of_v<compile_string, Str>;

template <typename Scanner, typename ParseCtx>
using dt_scanner_parse =
    decltype(SCN_DECLVAL(Scanner&).parse(SCN_DECLVAL(ParseCtx&)));
template <typename Scanner, typename T, typename Ctx>
using dt_scanner_scan = decltype(SCN_DECLVAL(const Scanner&)
                                     .scan(SCN_DECLVAL(T&), SCN_DECLVAL(Ctx&)));

template <typename Scanner, typename T, typename Ctx, typename ParseCtx>
constexpr typename ParseCtx::iterator parse_format_specs_impl(
    ParseCtx& parse_ctx)
{
    static_assert(
        std::is_default_constructible_v<Scanner>,
        "Specializations of scn::scanner must be default constructible");
    static_assert(mp_valid<dt_scanner_parse, Scanner, ParseCtx>::value,
                  "Specializations of scn::scanner must have a "
                  "parse(ParseContext&) member function.");
    static_assert(
        std::is_same_v<mp_eval_or<void, dt_scanner_parse, Scanner, ParseCtx>,
                       typename ParseCtx::iterator>,
        "scn::scanner::parse(ParseContext&) must return "
        "ParseContext::iterator. To report an error from scanner::parse, "
        "either throw an exception derived from scn::scan_format_string_error, "
        "or call ParseContext::on_error.");
    static_assert(mp_valid<dt_scanner_scan, Scanner, T, Ctx>::value,
                  "Specializations of scn::scanner must have a "
                  "scan(T&, Context&) const member function.");
    static_assert(
        std::is_same_v<mp_eval_or<void, dt_scanner_scan, Scanner, T, Ctx>,
                       scan_expected<typename Ctx::iterator>>,
        "scn::scanner::scan(T&, Context&) must be a const member function,"
        "that returns scan_expected<Context::iterator>.");

    auto s = Scanner{};
    return s.parse(parse_ctx);
}

template <typename T, typename Ctx, typename ParseCtx>
constexpr typename ParseCtx::iterator parse_format_specs(ParseCtx& parse_ctx)
{
    using char_type = typename Ctx::char_type;
    using map_result =
        std::remove_reference_t<decltype(arg_mapper<char_type>().map(
            SCN_DECLVAL(T&)))>;
    if constexpr (std::is_base_of_v<unscannable, map_result>) {
        // Error will be reported by static_assert in make_value(),
        // let's not muddy the compiler error by making more of them.
        return parse_ctx.begin();
    }
    else {
        using mapped_type =
            std::conditional_t<arg_type_constant<T, char_type>::value !=
                                   arg_type::custom_type,
                               map_result, T>;
        using scanner_type = typename Ctx::template scanner_type<mapped_type>;
        return parse_format_specs_impl<scanner_type, T, Ctx, ParseCtx>(
            parse_ctx);
    }
}

template <typename CharT, typename Source, typename... Args>
class format_string_checker {
public:
    using parse_context_type = compile_parse_context<CharT>;
    static constexpr auto num_args = sizeof...(Args);

    explicit constexpr format_string_checker(
        std::basic_string_view<CharT> format_str)
        : m_parse_context(source_tag<Source>, format_str, num_args, m_types),
          m_parse_funcs{&parse_format_specs<Args,
                                            default_context<CharT>,
                                            parse_context_type>...},
          m_types{arg_type_constant<Args, CharT>::value...}
    {
    }

    constexpr void on_literal_text(const CharT* begin, const CharT* end)
    {
        // TODO: Do we want to validate Unicode in format strings?
        // We're dealing with text, so we probably do.
        // We could do codeunit-to-codeunit matching,
        // but that could get messy wrt. whitespace matching.
        // It's simpler to not allow nonsense.
        while (begin != end) {
            const auto len = code_point_length_by_starting_code_unit(*begin);
            if (SCN_UNLIKELY(len == 0 ||
                             static_cast<size_t>(end - begin) < len)) {
                return on_error("Invalid encoding in format string");
            }

            const auto cp = decode_code_point_exhaustive(
                std::basic_string_view<CharT>{begin, len});
            if (SCN_UNLIKELY(cp >= invalid_code_point)) {
                return on_error("Invalid encoding in format string");
            }

            begin += len;
        }
    }

    constexpr auto on_arg_id()
    {
        return m_parse_context.next_arg_id();
    }
    constexpr auto on_arg_id(std::size_t id)
    {
        m_parse_context.check_arg_id(id);
        return id;
    }

    constexpr void on_replacement_field(size_t id, const CharT* begin)
    {
        set_arg_as_read(id);

        auto type = m_types[id];
        check_arg_can_be_read(type);

        if (type == arg_type::custom_type && id < num_args) {
            // Only call scanner::parse to check for errors,
            // we're discarding the result.
            // The advance_to dance is done to point the parse context to the
            // character after the `{`; right now, it points to that
            const auto beg = begin;
            m_parse_context.advance_to(begin);
            m_parse_funcs[id](m_parse_context);
            m_parse_context.advance_to(beg);
        }
    }

    constexpr const CharT* on_format_specs(std::size_t id,
                                           const CharT* begin,
                                           const CharT*)
    {
        set_arg_as_read(id);
        check_arg_can_be_read(m_types[id]);

        m_parse_context.advance_to(begin);
        return id < num_args ? m_parse_funcs[id](m_parse_context) : begin;
    }

    constexpr void check_args_exhausted()
    {
        if constexpr (num_args == 0) {
            return;
        }
        for (auto is_set : m_visited_args) {
            if (!is_set) {
                return on_error("Argument list not exhausted");
            }
        }
    }

    void on_error(const char* msg)
    {
        SCN_UNLIKELY_ATTR
        m_parse_context.on_error(msg);
    }

    // Only to satisfy the concept and eliminate compiler errors,
    // because errors are reported by failing to compile on_error above
    // (it's not constexpr)
    constexpr scan_expected<void> get_error() const
    {
        return {};
    }

private:
    constexpr void set_arg_as_read(size_t id)
    {
        if (id >= num_args) {
            return on_error("Invalid out-of-range argument ID");
        }
        if (m_visited_args[id]) {
            return on_error("Argument with this ID already scanned");
        }
        m_visited_args[id] = true;
    }

    constexpr void check_arg_can_be_read(arg_type type)
    {
        if (type == arg_type::string_view_type &&
            !m_parse_context.is_source_contiguous()) {
            // clang-format off
            return on_error("Cannot read a string_view from a non-contiguous source");
            // clang-format on
        }
        if (type == arg_type::string_view_type &&
            !m_parse_context.is_source_borrowed()) {
            // clang-format off
            return on_error("Cannot read a string_view from a non-borrowed source");
            // clang-format on
        }
    }

    using parse_func = const CharT* (*)(parse_context_type&);

    parse_context_type m_parse_context;
    parse_func m_parse_funcs[num_args > 0 ? num_args : 1];
    arg_type m_types[num_args > 0 ? num_args : 1];
    bool m_visited_args[num_args > 0 ? num_args : 1] = {false};
};

template <typename Source, typename... Args, typename Str>
constexpr auto check_format_string(const Str&)
    -> std::enable_if_t<!is_compile_string_v<Str>>
{
    // TODO: SCN_ENFORE_COMPILE_STRING?
#if 0  // SCN_ENFORE_COMPILE_STRING
    static_assert(dependent_false<Str>::value,
              "SCN_ENFORCE_COMPILE_STRING requires all format "
              "strings to use SCN_STRING.");
#endif
}

template <typename Source, typename... Args, typename Str>
constexpr auto check_format_string(Str format_str)
    -> std::enable_if_t<is_compile_string_v<Str>>
{
    using char_type = typename Str::char_type;

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wconversion")
    constexpr auto s = std::basic_string_view<char_type>{format_str};
    SCN_GCC_POP

    using checker = format_string_checker<char_type, Source, Args...>;
    constexpr bool invalid_format =
        (parse_format_string<true>(s, checker(s)), true);
    SCN_UNUSED(invalid_format);
}

template <typename CharT, std::size_t N>
constexpr std::basic_string_view<CharT> compile_string_to_view(
    const CharT (&s)[N])
{
    return {s, N - 1};
}
template <typename CharT>
constexpr std::basic_string_view<CharT> compile_string_to_view(
    std::basic_string_view<CharT> s)
{
    return s;
}
}  // namespace detail

#define SCN_STRING_IMPL(s, base, expl)                                       \
    [] {                                                                     \
        struct SCN_COMPILE_STRING : base {                                   \
            using char_type = ::scn::detail::remove_cvref_t<decltype(s[0])>; \
            SCN_MAYBE_UNUSED constexpr expl                                  \
            operator ::std::basic_string_view<char_type>() const             \
            {                                                                \
                return ::scn::detail::compile_string_to_view<char_type>(s);  \
            }                                                                \
        };                                                                   \
        return SCN_COMPILE_STRING{};                                         \
    }()

#define SCN_STRING(s) SCN_STRING_IMPL(s, ::scn::detail::compile_string, )

/**
 * Compile-time format string
 *
 * \ingroup format-string
 */
template <typename CharT, typename Source, typename... Args>
class basic_scan_format_string {
public:
    SCN_CLANG_PUSH
#if SCN_CLANG >= SCN_COMPILER(10, 0, 0)
    SCN_CLANG_IGNORE("-Wc++20-compat")  // false positive about consteval
#endif
    template <
        typename S,
        std::enable_if_t<
            std::is_convertible_v<const S&, std::basic_string_view<CharT>> &&
            detail::is_not_self<S, basic_scan_format_string>>* = nullptr>
    SCN_CONSTEVAL basic_scan_format_string(const S& s) : m_str(s)
    {
#if SCN_HAS_CONSTEVAL
        using checker = detail::format_string_checker<CharT, Source, Args...>;
        const auto e = detail::parse_format_string<true>(m_str, checker(s));
        SCN_UNUSED(e);
#else
        detail::check_format_string<Source, Args...>(s);
#endif
    }
    SCN_CLANG_POP

    template <
        typename OtherSource,
        std::enable_if_t<std::is_same_v<detail::remove_cvref_t<Source>,
                                        detail::remove_cvref_t<OtherSource>> &&
                         ranges::borrowed_range<Source> ==
                             ranges::borrowed_range<OtherSource>>* = nullptr>
    constexpr basic_scan_format_string(
        const basic_scan_format_string<CharT, OtherSource, Args...>& other)
        : m_str(other.get())
    {
    }

    basic_scan_format_string(detail::basic_runtime_format_string<CharT> r)
        : m_str(r.str)
    {
    }

    constexpr operator std::basic_string_view<CharT>() const
    {
        return m_str;
    }
    constexpr std::basic_string_view<CharT> get() const
    {
        return m_str;
    }

private:
    std::basic_string_view<CharT> m_str;
};

namespace detail {
class locale_ref {
#if !SCN_DISABLE_LOCALE
public:
    constexpr locale_ref() = default;

    template <typename Locale>
    SCN_PUBLIC explicit locale_ref(const Locale& loc);

    constexpr explicit operator bool() const noexcept
    {
        return m_locale != nullptr;
    }

    template <typename Locale>
    SCN_PUBLIC Locale get() const;

private:
    const void* m_locale{nullptr};
#else
public:
    constexpr locale_ref() = default;

    template <typename T>
    constexpr explicit locale_ref(T&&)
    {
    }

    constexpr explicit operator bool() const noexcept
    {
        return true;
    }
#endif
};
}  // namespace detail

/////////////////////////////////////////////////////////////////
// scan_context
/////////////////////////////////////////////////////////////////

namespace detail {
template <typename I>
using apply_cmp_with_nullptr = decltype(SCN_DECLVAL(const I&) == nullptr);
template <typename I>
inline constexpr bool is_comparable_with_nullptr =
    mp_valid_v<apply_cmp_with_nullptr, I>;

template <typename Args>
class scan_context_base {
public:
    /// Get argument at index `id`
    constexpr auto arg(size_t id) const noexcept
    {
        return m_args.get(id);
    }

    constexpr const Args& args() const
    {
        return m_args;
    }

    SCN_NODISCARD constexpr locale_ref locale() const
    {
        return m_locale;
    }

protected:
    scan_context_base(Args args, locale_ref loc) noexcept
        : m_args(SCN_MOVE(args)), m_locale(SCN_MOVE(loc))
    {
    }

    Args m_args;
    locale_ref m_locale;
};
}  // namespace detail

/**
 * \defgroup ctx Contexts and scanners
 *
 * \brief Lower-level APIs used for scanning individual values
 *
 * \section user-defined Scanning user-defined types
 *
 * User-defined types can be scanned by specializing the class template
 * `scn::scanner`.
 *
 * \code{.cpp}
 * struct mytype {
 *   int key;
 *   std::string value;
 * };
 *
 * template <>
 * struct scn::scanner<mytype> {
 *   template <typename ParseContext>
 *   constexpr auto parse(ParseCtx& pctx)
 *     -> typename ParseContext::iterator {
 *     // parse() implementation just returning begin():
 *     // only permits empty format specifiers
 *     return pctx.begin();
 *   }
 *
 *   template <typename Context>
 *   auto scan(mytype& val, Context& ctx)
 *     -> scan_expected<typename Context::iterator> {
 *     return scn::scan<int, std::string>(ctx.range(), "{}: {}")
 *       .transform([&](auto result) {
 *         std::tie(val.key, val.value) = std::move(result->values());
 *         return result.begin();
 *       });
 *   }
 * };
 * \endcode
 *
 * See also
 * \ref g-usertypes
 */

/**
 * Scanning context.
 *
 * \ingroup ctx
 */
template <typename Range, typename CharT>
class basic_scan_context
    : public detail::scan_context_base<
          basic_scan_args<basic_scan_context<Range, CharT>>> {
    using base = detail::scan_context_base<basic_scan_args<basic_scan_context>>;

    using args_type = basic_scan_args<basic_scan_context>;
    using arg_type = basic_scan_arg<basic_scan_context>;

    static_assert(std::is_same_v<Range, detail::buffer_range_tag> ||
                  ranges::borrowed_range<Range>);

public:
    /// Character type of the input
    using char_type = CharT;
    using range_type = std::conditional_t<
        std::is_same_v<Range, detail::buffer_range_tag>,
        typename detail::basic_scan_buffer<char_type>::range_type,
        Range>;
    using iterator = ranges::iterator_t<range_type>;
    using sentinel = ranges::sentinel_t<range_type>;
    using parse_context_type = basic_scan_parse_context<char_type>;

    /**
     * The scanner type associated with this scanning context.
     */
    template <typename T>
    using scanner_type = scanner<T, char_type>;

    [[deprecated(
        "Use a constructor that provides a range or the sentinel explicitly.")]]
    constexpr basic_scan_context(iterator curr,
                                 args_type a,
                                 detail::locale_ref loc = {}) noexcept
        : basic_scan_context(SCN_MOVE(curr), sentinel{}, SCN_MOVE(a), loc)
    {
    }

    constexpr basic_scan_context(range_type r,
                                 args_type a,
                                 detail::locale_ref loc = {}) noexcept
        : basic_scan_context(ranges::begin(r), ranges::end(r), SCN_MOVE(a), loc)
    {
    }

    constexpr basic_scan_context(iterator curr,
                                 sentinel end,
                                 args_type a,
                                 detail::locale_ref loc = {}) noexcept
        : base(SCN_MOVE(a), loc), m_current(curr), m_end(end)
    {
    }

    basic_scan_context(const basic_scan_context&) = delete;
    basic_scan_context& operator=(const basic_scan_context&) = delete;

    basic_scan_context(basic_scan_context&&) = default;
    basic_scan_context& operator=(basic_scan_context&&) = default;
    ~basic_scan_context() = default;

    /**
     * Returns an iterator pointing to the current position in the source
     * range.
     */
    constexpr iterator begin() const
    {
        return m_current;
    }

    /**
     * Returns a sentinel pointing to the end of the source range.
     */
    constexpr sentinel end() const
    {
        return m_end;
    }

    /**
     * Returns a subrange over `[begin(), end())`
     */
    constexpr auto range() const
    {
        return ranges::subrange{begin(), end()};
    }

    /// Advances the beginning of the source range to `it`
    void advance_to(iterator it)
    {
        m_current = SCN_MOVE(it);
    }

private:
    iterator m_current;
    SCN_NO_UNIQUE_ADDRESS sentinel m_end{};
};

namespace detail {
template <typename T, typename ParseCtx>
constexpr typename ParseCtx::iterator scanner_parse_for_builtin_type(
    ParseCtx& pctx,
    format_specs& specs);

template <typename T, typename Context>
SCN_PUBLIC scan_expected<typename Context::iterator>
scanner_scan_for_builtin_type(T& val, Context& ctx, const format_specs& specs);

template <typename T, typename CharT>
struct builtin_scanner {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return detail::scanner_parse_for_builtin_type<T>(pctx, m_specs);
    }

    template <typename Context>
    scan_expected<typename Context::iterator> scan(T& val, Context& ctx) const
    {
        return detail::scanner_scan_for_builtin_type(val, ctx, m_specs);
    }

protected:
    format_specs m_specs;
};
}  // namespace detail

/////////////////////////////////////////////////////////////////
// scanner
/////////////////////////////////////////////////////////////////

/**
 * `scanner` specialization for all built-in types
 *
 * \ingroup ctx
 */
template <typename T, typename CharT>
struct scanner<T,
               CharT,
               std::enable_if_t<detail::arg_type_constant<T, CharT>::value !=
                                    detail::arg_type::custom_type &&
                                !detail::is_type_disabled<T>>>
    : detail::builtin_scanner<T, CharT> {};

namespace detail {
template <typename T, typename ParseCtx>
constexpr typename ParseCtx::iterator scanner_parse_for_builtin_type(
    ParseCtx& pctx,
    format_specs& specs)
{
    using char_type = typename ParseCtx::char_type;

    auto begin = pctx.begin();
    const auto end = pctx.end();

    using handler_type = specs_setter;
    constexpr auto type = arg_type_constant<T, char_type>::value;
    auto checker =
        detail::specs_checker<handler_type>(handler_type(specs), type);

    const auto it =
        detail::parse_format_specs(to_address(begin), to_address(end), checker);

    switch (type) {
        case arg_type::none_type:
            SCN_FALLTHROUGH;
        case arg_type::custom_type:
            SCN_ENSURE(false);
            break;

        case arg_type::bool_type:
            check_bool_type_specs(specs, checker);
            break;

        case arg_type::schar_type:
        case arg_type::short_type:
        case arg_type::int_type:
        case arg_type::long_type:
        case arg_type::llong_type:
        case arg_type::int128_type:
        case arg_type::uchar_type:
        case arg_type::ushort_type:
        case arg_type::uint_type:
        case arg_type::ulong_type:
        case arg_type::ullong_type:
        case arg_type::uint128_type:
            check_int_type_specs(specs, checker);
            break;

        case arg_type::narrow_character_type:
        case arg_type::wide_character_type:
        case arg_type::code_point_type:
            check_char_type_specs(specs, checker);
            break;

        case arg_type::float_type:
        case arg_type::double_type:
        case arg_type::ldouble_type:
        case arg_type::float16_type:
        case arg_type::float32_type:
        case arg_type::float64_type:
        case arg_type::float128_type:
        case arg_type::bfloat16_type:
            check_float_type_specs(specs, checker);
            break;

        case arg_type::narrow_string_type:
        case arg_type::wide_string_type:
        case arg_type::string_view_type:
            check_string_type_specs(specs, checker);
            break;

        case arg_type::pointer_type:
            check_pointer_type_specs(specs, checker);
            break;

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wcovered-switch-default")

        default:
            SCN_ENSURE(false);
            SCN_UNREACHABLE;

            SCN_CLANG_POP
    }

#if !SCN_DISABLE_REGEX
    if (specs.type == presentation_type::regex ||
        specs.type == presentation_type::regex_escaped) {
        if (!pctx.is_source_contiguous()) {
            SCN_UNLIKELY_ATTR
            // clang-format off
            checker.on_error("Cannot read a regex from a non-contiguous source");
            // clang-format on
        }
        if (!pctx.is_source_borrowed()) {
            SCN_UNLIKELY_ATTR
            checker.on_error("Cannot read a regex from a non-borrowed source");
        }
    }
#endif

    return it;
}
}  // namespace detail

/**
 * Type for discarding any scanned value.
 * Example:
 *
 * \code{.cpp}
 * auto r = scn::scan<scn::discard<int>>("42", "{}");
 * // r.has_value() == true
 * // decltype(r->value()) is scn::discard<int>
 * \endcode
 *
 * \ingroup format-string
 */
template <typename T>
struct discard {
    constexpr discard() = default;

    constexpr discard(const T&) noexcept {}
    constexpr discard(T&&) noexcept {}

    constexpr discard& operator=(const T&) noexcept
    {
        return *this;
    }
    constexpr discard& operator=(T&&) noexcept
    {
        return *this;
    }
};

template <typename T, typename CharT>
struct scanner<discard<T>, CharT> : public scanner<T, CharT> {
    template <typename Context>
    auto scan(discard<T>&, Context& ctx) const
    {
        T val{};
        return scanner<T, CharT>::scan(val, ctx);
    }
};

namespace detail {
template <typename Range>
SCN_PUBLIC scan_expected<ranges::iterator_t<Range>>
internal_skip_classic_whitespace(Range r, bool allow_exhaustion);

#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(T, Context)    \
    extern template SCN_PUBLIC scan_expected<Context::iterator> \
    scanner_scan_for_builtin_type(T&, Context&, const format_specs&);

#if SCN_HAS_INT128
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_INT128(Context)   \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(int128, Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(uint128, Context)
#else
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_INT128(Context) /* int128 */
#endif

#if SCN_HAS_STD_F16
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F16(Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(std::float16_t, Context)
#else
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F16(Context) /* std::float16_t */
#endif

#if SCN_HAS_STD_F32
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F32(Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(std::float32_t, Context)
#else
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F32(Context) /* std::float32_t */
#endif

#if SCN_HAS_STD_F64
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F64(Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(std::float64_t, Context)
#else
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F64(Context) /* std::float64_t */
#endif

#if SCN_HAS_STD_F128
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F128(Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(std::float128_t, Context)
#else
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F128(Context)  /* std::float128_t \
                                                            */
#endif

#if SCN_HAS_STD_BF16
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_BF16(Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(std::bfloat16_t, Context)
#else
#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_BF16(Context)  /* std::bfloat16_t \
                                                            */
#endif

#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_EXT_FLOAT(Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F16(Context)           \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F32(Context)           \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F64(Context)           \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_F128(Context)          \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_BF16(Context)

#define SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(Context)                  \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(char, Context)               \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(wchar_t, Context)            \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(signed char, Context)        \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(signed char, Context)        \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(short, Context)              \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(int, Context)                \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(long, Context)               \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(long long, Context)          \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned char, Context)      \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned short, Context)     \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned int, Context)       \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned long, Context)      \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(unsigned long long, Context) \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(float, Context)              \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(double, Context)             \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(long double, Context)        \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(std::string, Context)        \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(std::wstring, Context)       \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(                             \
        std::basic_string_view<Context::char_type>, Context)              \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(regex_matches, Context)      \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE(wregex_matches, Context)     \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_INT128(Context)                   \
    SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_EXT_FLOAT(Context)                \
    extern template SCN_PUBLIC                                            \
        scan_expected<ranges::iterator_t<Context::range_type>>            \
        internal_skip_classic_whitespace(Context::range_type, bool);

SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scan_context)

}  // namespace detail

/////////////////////////////////////////////////////////////////
// visit_scan_arg
/////////////////////////////////////////////////////////////////

namespace detail {

template <typename Visitor, typename Ctx>
constexpr decltype(auto) visit_impl(Visitor&& vis, basic_scan_arg<Ctx>& arg)
{
#define SCN_VISIT(Type)                                                    \
    do {                                                                   \
        if constexpr (!detail::is_type_disabled<Type>) {                   \
            return vis(*static_cast<Type*>(get_arg_value(arg).ref_value)); \
        }                                                                  \
        else {                                                             \
            return vis(monostate_val);                                     \
        }                                                                  \
    } while (false)

    monostate monostate_val{};

    switch (get_arg_type(arg)) {
        case detail::arg_type::schar_type:
            SCN_VISIT(signed char);
        case detail::arg_type::short_type:
            SCN_VISIT(short);
        case detail::arg_type::int_type:
            SCN_VISIT(int);
        case detail::arg_type::long_type:
            SCN_VISIT(long);
        case detail::arg_type::llong_type:
            SCN_VISIT(long long);
        case detail::arg_type::int128_type:
#if SCN_HAS_INT128
            SCN_VISIT(int128);
#else
            return vis(monostate_val);
#endif
        case detail::arg_type::uchar_type:
            SCN_VISIT(unsigned char);
        case detail::arg_type::ushort_type:
            SCN_VISIT(unsigned short);
        case detail::arg_type::uint_type:
            SCN_VISIT(unsigned);
        case detail::arg_type::ulong_type:
            SCN_VISIT(unsigned long);
        case detail::arg_type::ullong_type:
            SCN_VISIT(unsigned long long);
        case detail::arg_type::uint128_type:
#if SCN_HAS_INT128
            SCN_VISIT(uint128);
#else
            return vis(monostate_val);
#endif
        case detail::arg_type::pointer_type:
            SCN_VISIT(void*);
        case detail::arg_type::bool_type:
            SCN_VISIT(bool);
        case detail::arg_type::narrow_character_type:
            SCN_VISIT(char);
        case detail::arg_type::wide_character_type:
            SCN_VISIT(wchar_t);
        case detail::arg_type::code_point_type:
            SCN_VISIT(char32_t);
        case detail::arg_type::float_type:
            SCN_VISIT(float);
        case detail::arg_type::double_type:
            SCN_VISIT(double);
        case detail::arg_type::ldouble_type:
            SCN_VISIT(long double);
        case detail::arg_type::float16_type:
#if SCN_HAS_STD_F16
            SCN_VISIT(std::float16_t);
#else
            return vis(monostate_val);
#endif
        case detail::arg_type::float32_type:
#if SCN_HAS_STD_F32
            SCN_VISIT(std::float32_t);
#else
            return vis(monostate_val);
#endif
        case detail::arg_type::float64_type:
#if SCN_HAS_STD_F64
            SCN_VISIT(std::float64_t);
#else
            return vis(monostate_val);
#endif
        case detail::arg_type::float128_type:
#if SCN_HAS_STD_F64
            SCN_VISIT(std::float128_t);
#else
            return vis(monostate_val);
#endif
        case detail::arg_type::bfloat16_type:
#if SCN_HAS_STD_BF16
            SCN_VISIT(std::bfloat16_t);
#else
            return vis(monostate_val);
#endif
        case detail::arg_type::narrow_string_type:
            SCN_VISIT(std::string);
        case detail::arg_type::wide_string_type:
            SCN_VISIT(std::wstring);
        case detail::arg_type::string_view_type: {
            if constexpr (std::is_same_v<typename Ctx::char_type, char>) {
                SCN_VISIT(std::string_view);
            }
            else {
                SCN_VISIT(std::wstring_view);
            }
        }

        case detail::arg_type::custom_type:
#if !SCN_DISABLE_TYPE_CUSTOM
            return vis(typename basic_scan_arg<Ctx>::handle(
                get_arg_value(arg).custom_value));
#else
            return vis(monostate_val);
#endif

            SCN_CLANG_PUSH
            SCN_CLANG_IGNORE("-Wcovered-switch-default")

            SCN_UNLIKELY_ATTR
        case detail::arg_type::none_type:
        default: {
            return vis(monostate_val);
        }

            SCN_CLANG_POP
    }

#undef SCN_VISIT

    SCN_ENSURE(false);
    SCN_UNREACHABLE;
}

}  // namespace detail

template <typename Visitor, typename Ctx>
[[deprecated("Use basic_scan_arg::visit instead")]] constexpr decltype(auto)
visit_scan_arg(Visitor&& vis, basic_scan_arg<Ctx>& arg)
{
    return detail::visit_impl(SCN_FWD(vis), arg);
}

template <typename Context>
template <typename Visitor>
constexpr decltype(auto) basic_scan_arg<Context>::visit(Visitor&& vis)
{
    return detail::visit_impl(SCN_FWD(vis), *this);
}

template <typename Context>
template <typename R, typename Visitor>
constexpr R basic_scan_arg<Context>::visit(Visitor&& vis)
{
    return detail::visit_impl(SCN_FWD(vis), *this);
}

/////////////////////////////////////////////////////////////////
// vscan
/////////////////////////////////////////////////////////////////

/**
 * \defgroup vscan Type-erased scanning API
 *
 * \brief Lower-level scanning API with type-erased arguments
 */

namespace detail {

template <typename Source>
struct invalid_scan_result_type {
    static_assert(
        detail::dependent_false<Source>::value,
        "Invalid source type given to scn::scan, can't construct result type");
    using type = void;
};

template <typename Source, typename = void>
struct custom_scan_result;

template <typename Source>
using custom_scan_result_t = typename custom_scan_result<Source>::type;

template <typename Source>
using scan_result_value_type = typename mp_cond<
    std::is_same<remove_cvref_t<Source>, std::FILE*>,
    mp_identity<std::FILE*>,
    std::is_same<remove_cvref_t<Source>, scan_file>,
    mp_identity<scan_file*>,
    mp_valid<custom_scan_result_t, Source>,
    mp_defer<custom_scan_result_t, Source>,
    mp_bool<ranges::forward_range<Source>>,
    mp_defer<borrowed_tail_subrange_t, Source>,
    is_specialization_of<remove_cvref_t<Source>, ranges::pair_concat_view>,
    mp_defer<borrowed_flattened_concat_tail_subrange_t, Source>,
    mp_bool<ranges::input_range<Source>>,
    mp_defer<borrowed_input_range_tail_subrange_t, Source>,
    std::true_type,
    mp_defer<invalid_scan_result_type, Source>>::type;
}  // namespace detail

/**
 * Result type returned by `vscan`.
 *
 * The value type of the `scan_expected` is `FILE*` if `Source` is `FILE*`,
 * `detail::borrowed_tail_subrange_t<Source>` otherwise.
 *
 * \ingroup vscan
 */
template <typename Source>
using vscan_result = scan_expected<detail::scan_result_value_type<Source>>;

namespace detail {

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(std::string_view source,
                                                    std::string_view format,
                                                    scan_args args);
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(
    scan_buffer::range_type source,
    std::string_view format,
    scan_args args);

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(std::wstring_view source,
                                                    std::wstring_view format,
                                                    wscan_args args);
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_impl(
    wscan_buffer::range_type source,
    std::wstring_view format,
    wscan_args args);

#if !SCN_DISABLE_LOCALE
template <typename Locale>
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_localized_impl(
    const Locale& loc,
    std::string_view source,
    std::string_view format,
    scan_args args);
template <typename Locale>
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_localized_impl(
    const Locale& loc,
    scan_buffer::range_type source,
    std::string_view format,
    scan_args args);

template <typename Locale>
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_localized_impl(
    const Locale& loc,
    std::wstring_view source,
    std::wstring_view format,
    wscan_args args);
template <typename Locale>
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_localized_impl(
    const Locale& loc,
    wscan_buffer::range_type source,
    std::wstring_view format,
    wscan_args args);
#endif

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    std::string_view source,
    basic_scan_arg<scan_context> arg);
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    scan_buffer::range_type source,
    basic_scan_arg<scan_context> arg);

SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    std::wstring_view source,
    basic_scan_arg<wscan_context> arg);
SCN_PUBLIC scan_expected<std::ptrdiff_t> vscan_value_impl(
    wscan_buffer::range_type source,
    basic_scan_arg<wscan_context> arg);

template <typename CharT>
auto vscan_range_type(basic_scan_buffer<CharT>& buffer)
{
    return buffer.get();
}
template <typename T, std::enable_if_t<ranges::forward_range<T>>* = nullptr>
auto vscan_range_type(T x)
{
    return x;
}

template <typename Range, typename CharT>
auto vscan_generic(Range&& range,
                   std::basic_string_view<CharT> format,
                   basic_scan_args<detail::default_context<CharT>> args)
    -> vscan_result<Range>
{
    auto&& buffer = make_scan_buffer(range);
    auto result = vscan_impl(vscan_range_type(buffer), format, args);
    if (SCN_UNLIKELY(!result)) {
        return unexpected(result.error());
    }
    return make_vscan_result(SCN_FWD(range), buffer, *result);
}

template <typename Locale, typename Range, typename CharT>
auto vscan_localized_generic(
    const Locale& loc,
    Range&& range,
    std::basic_string_view<CharT> format,
    basic_scan_args<detail::default_context<CharT>> args) -> vscan_result<Range>
{
#if !SCN_DISABLE_LOCALE
    auto&& buffer = detail::make_scan_buffer(range);

    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
    auto result = detail::vscan_localized_impl(loc, vscan_range_type(buffer),
                                               format, args);
    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

    if (SCN_UNLIKELY(!result)) {
        return unexpected(result.error());
    }
    return make_vscan_result(SCN_FWD(range), buffer, *result);
#else
    static_assert(dependent_false<Locale>::value,
                  "Can't use scan(locale, ...) with SCN_DISABLE_LOCALE on");

    return {};
#endif
}

template <typename Range, typename CharT>
auto vscan_value_generic(Range&& range,
                         basic_scan_arg<detail::default_context<CharT>> arg)
    -> vscan_result<Range>
{
    auto&& buffer = detail::make_scan_buffer(range);
    auto result = detail::vscan_value_impl(vscan_range_type(buffer), arg);
    if (SCN_UNLIKELY(!result)) {
        return unexpected(result.error());
    }
    return make_vscan_result(SCN_FWD(range), buffer, *result);
}
}  // namespace detail

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")

/**
 * Perform actual scanning from `source`, according to `format`, into the
 * type-erased arguments at `args`. Called by `scan`.
 *
 * \ingroup vscan
 */
template <typename Source>
auto vscan(Source&& source, std::string_view format, scan_args args)
    -> vscan_result<Source>
{
    return detail::vscan_generic(SCN_FWD(source), format, args);
}

/**
 * Perform actual scanning from `source`, according to `format`, into the
 * type-erased arguments at `args`, using `loc`, if requested. Called by
 * `scan`.
 *
 * \ingroup locale
 */
template <typename Source,
          typename Locale,
          typename = std::void_t<decltype(Locale::classic())>>
auto vscan(const Locale& loc,
           Source&& source,
           std::string_view format,
           scan_args args) -> vscan_result<Source>
{
    return detail::vscan_localized_generic(loc, SCN_FWD(source), format, args);
}

/**
 * Perform actual scanning from `source` into the type-erased argument at
 * `arg`. Called by `scan_value`.
 *
 * \ingroup vscan
 */
template <typename Source>
auto vscan_value(Source&& source, basic_scan_arg<scan_context> arg)
    -> vscan_result<Source>
{
    return detail::vscan_value_generic(SCN_FWD(source), arg);
}

/**
 * Perform actual scanning from `stdin`, according to `format`, into the
 * type-erased arguments at `args`. Called by `input`.
 *
 * \ingroup vscan
 */
[[deprecated(
    "Use scn::vscan with an explicit source parameter, "
    "or scn::input")]]
SCN_PUBLIC scan_expected<void> vinput(std::string_view format, scan_args args);

namespace detail {
template <typename T>
SCN_PUBLIC auto scan_int_impl(std::string_view source, T& value, int base)
    -> scan_expected<std::string_view::iterator>;

template <typename T>
SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view source) -> T;

#if !SCN_DISABLE_TYPE_SCHAR
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              signed char& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> signed char;
#endif
#if !SCN_DISABLE_TYPE_SHORT
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              short& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> short;
#endif
#if !SCN_DISABLE_TYPE_INT
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              int& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> int;
#endif
#if !SCN_DISABLE_TYPE_LONG
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              long& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> long;
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              long long& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              unsigned char& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned char;
#endif
#if !SCN_DISABLE_TYPE_USHORT
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              unsigned short& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned short;
#endif
#if !SCN_DISABLE_TYPE_UINT
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              unsigned int& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              unsigned long& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned long;
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              unsigned long long& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
extern template SCN_PUBLIC auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned long long;
#endif

#if SCN_HAS_INT128

#if !SCN_DISABLE_TYPE_INT128
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              int128& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
#endif

#if !SCN_DISABLE_TYPE_UINT128
extern template SCN_PUBLIC auto scan_int_impl(std::string_view source,
                                              uint128& value,
                                              int base)
    -> scan_expected<std::string_view::iterator>;
#endif

#endif  // SCN_HAS_INT128

}  // namespace detail

SCN_GCC_POP  // -Wnoexcept

    // dummy namespace to reset formatting
    namespace detail
{
}

/////////////////////////////////////////////////////////////////
// scan
/////////////////////////////////////////////////////////////////

/**
 * The return type of `scan`, based on the type of the source, and the
 * types of the scanned arguments.
 */
template <typename Source, typename... Args>
using scan_result_type =
    scan_expected<scan_result<detail::scan_result_value_type<Source>, Args...>>;

/**
 * If `in` contains a successful result as returned from `vscan`,
 * the range contained in `out` is set to `*in`.
 * Otherwise, `unexpected(in.error())` is stored in `out`.
 */
template <typename Result, typename Source>
auto fill_scan_result(scan_expected<Result>& out, scan_expected<Source>&& in)
    -> std::enable_if_t<
        detail::is_specialization_of<Result, scan_result>::value,
        std::void_t<decltype(detail::scan_result_source_access::
                                 set(*out, SCN_MOVE(*in)))>>
{
    if (SCN_UNLIKELY(!in)) {
        out = unexpected(in.error());
    }
    else {
        detail::scan_result_source_access::set(*out, SCN_MOVE(*in));
    }
}

/**
 * Returns an empty result type for a source of type `Source`, and arguments of
 * type `Args...`.
 */
template <typename Source, typename... Args>
auto make_scan_result()
{
    return scan_result_type<Source, Args...>();
}

template <typename Source, typename... Args>
auto make_scan_result(std::tuple<Args...>&& initial_values)
{
    using type = scan_result_type<Source, Args...>;
    using subrange_type = typename type::value_type::range_type;
    return type{std::in_place, subrange_type{}, SCN_MOVE(initial_values)};
}

/**
 * \defgroup scan Basic scanning API
 *
 * \brief The core public-facing interface of the library
 *
 * The following functions use a format string syntax similar to that of
 * `std::format`. See more at \ref format-string.
 *
 * When these functions take a `source` as input, it must
 * model the `scannable_source` concept. See more at \ref scannable.
 */

/**
 * Scans `Args...` from `source`, according to the
 * specifications given in the format string (`format`).
 * Returns the resulting values in an object of type `scan_result`,
 * alongside a `subrange` pointing to the unused input.
 *
 * Example:
 * \code{.cpp}
 * if (auto result = scn::scan<int>("123", "{}"))
 *     int value = result->value();
 * \endcode
 *
 * \ingroup scan
 */
template <typename... Args,
          typename Source,
          typename = std::enable_if_t<detail::is_narrow_source<Source>>>
SCN_NODISCARD auto scan(Source&& source,
                        scan_format_string<Source, Args...> format)
    -> scan_result_type<Source, Args...>
{
    auto result = make_scan_result<Source, Args...>();
    fill_scan_result(result, vscan(SCN_FWD(source), format,
                                   make_scan_args(result->values())));
    return result;
}

/**
 * `scan` with explicitly supplied default values
 *
 * Can be used, for example, for pre-allocating a scanned string:
 *
 * \code{.cpp}
 * std::string str;
 * str.reserve(64);
 *
 * // As long as the read string fits in `str`,
 * // does not allocate
 * auto result = scn::scan<std::string>(source, "{}",
 *                                      {std::move(str)});
 * // Access the read string with result->value()
 * \endcode
 *
 * \ingroup scan
 */
template <typename... Args,
          typename Source,
          typename = std::enable_if_t<detail::is_narrow_source<Source>>>
SCN_NODISCARD auto scan(Source&& source,
                        scan_format_string<Source, Args...> format,
                        std::tuple<Args...>&& initial_args)
    -> scan_result_type<Source, Args...>
{
    auto result = make_scan_result<Source>(SCN_MOVE(initial_args));
    fill_scan_result(result, vscan(SCN_FWD(source), format,
                                   make_scan_args(result->values())));
    return result;
}

/**
 * \defgroup locale Localization
 *
 * \brief Scanning APIs that allow passing in a locale
 */

/**
 * `scan` using an explicit locale.
 *
 * Has no effect on its own, locale-specific scanning still needs to be
 * opted-into on an argument-by-argument basis, with the `L` format string
 * specifier.
 *
 * \code{.cpp}
 * auto result = scn::scan<double>(
 *     std::locale{"fi_FI.UTF-8"}, "3,14, "{:L}");
 * // result->value() == 3.14
 * \endcode
 *
 * \ingroup locale
 */
template <typename... Args,
          typename Locale,
          typename Source,
          typename = std::enable_if_t<detail::is_narrow_source<Source>>,
          typename = std::void_t<decltype(Locale::classic())>>
SCN_NODISCARD auto scan(const Locale& loc,
                        Source&& source,
                        scan_format_string<Source, Args...> format)
    -> scan_result_type<Source, Args...>
{
    auto result = make_scan_result<Source, Args...>();
    fill_scan_result(result, vscan(loc, SCN_FWD(source), format,
                                   make_scan_args(result->values())));
    return result;
}

/**
 * `scan` with a locale and default values
 *
 * \ingroup locale
 */
template <typename... Args,
          typename Locale,
          typename Source,
          typename = std::enable_if_t<detail::is_narrow_source<Source>>,
          typename = std::void_t<decltype(Locale::classic())>>
SCN_NODISCARD auto scan(const Locale& loc,
                        Source&& source,
                        scan_format_string<Source, Args...> format,
                        std::tuple<Args...>&& initial_args)
    -> scan_result_type<Source, Args...>
{
    auto result = make_scan_result<Source>(SCN_MOVE(initial_args));
    fill_scan_result(result, vscan(loc, SCN_FWD(source), format,
                                   make_scan_args(result->values())));
    return result;
}

/**
 * `scan` a single value, with default options.
 *
 * Essentially equivalent to: `scn::scan<T>(source, "{}")`,
 * except it can skip parsing the format string, gaining performance.
 *
 * \ingroup scan
 */
template <typename T,
          typename Source,
          typename = std::enable_if_t<detail::is_narrow_source<Source>>>
SCN_NODISCARD auto scan_value(Source&& source) -> scan_result_type<Source, T>
{
    auto result = make_scan_result<Source, T>();
    fill_scan_result(
        result, vscan_value(SCN_FWD(source),
                            detail::make_arg<scan_context>(result->value())));
    return result;
}

/**
 * `scan` a single value, with default options, and a default value.
 *
 * \ingroup scan
 */
template <typename T,
          typename Source,
          std::enable_if_t<detail::is_narrow_source<Source>>* = nullptr>
SCN_NODISCARD auto scan_value(Source&& source, T initial_value)
    -> scan_result_type<Source, T>
{
    auto result =
        make_scan_result<Source>(std::tuple<T>{SCN_MOVE(initial_value)});
    fill_scan_result(
        result, vscan_value(SCN_FWD(source),
                            detail::make_arg<scan_context>(result->value())));
    return result;
}

/**
 * Scan from `stdin`.
 *
 * Equivalent to `scn::scan<...>(stdin, ...)`,
 * except it maintains a separate thread-safe putback buffer
 * in case a putback into `stdin` fails.
 *
 * \code{.cpp}
 * auto result = scn::input<int>("{}");
 * \endcode
 *
 * \ingroup scan
 */
template <typename... Args>
SCN_NODISCARD auto input(scan_format_string<scan_file&, Args...> format)
    -> scan_result_type<scan_file&, Args...>
{
    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wexit-time-destructors")

    static std::mutex stdin_mutex{};
    static scan_file stdin_file{stdin};
    std::lock_guard<std::mutex> lock(stdin_mutex);

    auto result = scan_result_type<scan_file&, Args...>(
        std::in_place, stdin_file, std::tuple<Args...>{});
    auto err = vscan(stdin_file, format, make_scan_args(result->values()));
    if (SCN_UNLIKELY(!err)) {
        result = unexpected(err.error());
    }
    return result;

    SCN_CLANG_POP
}

/**
 * Write msg to stdout, and call `input<Args...>(format)`
 *
 * \ingroup scan
 */
template <typename... Args>
SCN_NODISCARD auto prompt(const char* msg,
                          scan_format_string<scan_file&, Args...> format)
    -> scan_result_type<scan_file&, Args...>
{
    std::printf("%s", msg);
    std::fflush(stdout);
    return input<Args...>(format);
}

namespace detail {
template <typename T>
inline constexpr bool is_scan_int_type =
    (std::is_integral_v<T> && !std::is_same_v<T, char> &&
     !std::is_same_v<T, wchar_t> && !std::is_same_v<T, char32_t> &&
     !std::is_same_v<T, bool>)
#if SCN_HAS_INT128
    || std::is_same_v<T, int128> || std::is_same_v<T, uint128>
#endif
    ;
}  // namespace detail

/**
 * Fast integer reading.
 *
 * Quickly reads an integer from a `std::string_view`. Skips preceding
 * whitespace.
 *
 * Reads in the specified base,
 * allowing a base prefix. Set `base` to `0` to detect the base from the
 * input. `base` must either be `0`, or in range `[2, 36]`.
 *
 * \ingroup scan
 */
template <typename T, std::enable_if_t<detail::is_scan_int_type<T>>* = nullptr>
SCN_NODISCARD auto scan_int(std::string_view source, int base = 10)
    -> scan_result_type<std::string_view, T>
{
    auto result = scan_result_type<std::string_view, T>();
    if (auto r = detail::scan_int_impl(source, result->value(), base);
        SCN_LIKELY(r)) {
        detail::scan_result_source_access::set(
            *result, ranges::subrange{*r, source.end()});
    }
    else {
        result = unexpected(r.error());
    }
    return result;
}

namespace detail {
template <bool Val, typename T>
inline constexpr bool dependent_bool = Val;
}

/**
 * Very fast integer reading.
 *
 * Quickly reads an integer from a `std::string_view`.
 *
 * Be very careful when using this one!
 * Its speed comes from some very heavy assumptions about the validity of
 * the input:
 *  - `source` must not be empty.
 *  - `source` contains nothing but the integer: no leading or trailing
 *    whitespace, no extra junk. Leading `-` is allowed for signed types,
 *    no `+` is allowed.
 *  - The parsed value does not overflow.
 *  - The input is a valid base-10 integer.
 * Breaking these assumptions will lead to UB.
 *
 * \ingroup scan
 */
template <typename T, std::enable_if_t<detail::is_scan_int_type<T>>* = nullptr>
SCN_NODISCARD auto scan_int_exhaustive_valid(std::string_view source) -> T
{
    static_assert(
        detail::dependent_bool<!SCN_IS_BIG_ENDIAN, T>,
        "scan_int_exhaustive_valid requires a little endian environment");
    return detail::scan_int_exhaustive_valid_impl<T>(source);
}

SCN_END_NAMESPACE
}  // namespace scn
