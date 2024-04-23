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

#include <iterator>

namespace scn {
SCN_BEGIN_NAMESPACE

// Very miniomal ranges implementation,
// copied from nanorange

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
struct incrementable_traits_helper {
};

// Workaround for GCC silliness: void* has no difference_type
// FIXME: This is required to stop WeaklyIncrementable<void*> being a hard
// error Can we formulate the concept differently to avoid the need for this
// hack?
template <>
struct incrementable_traits_helper<void*> {
};

template <typename T>
struct incrementable_traits_helper<T*>
    : detail::conditional_t<std::is_object_v<T>,
                            with_difference_type<std::ptrdiff_t>,
                            empty> {
};

template <class I>
struct incrementable_traits_helper<const I>
    : incrementable_traits<std::decay_t<I>> {
};

template <typename, typename = void>
struct has_member_difference_type : std::false_type {
};

template <typename T>
struct has_member_difference_type<T, std::void_t<typename T::difference_type>>
    : std::true_type {
};

template <typename T>
constexpr bool has_member_difference_type_v =
    has_member_difference_type<T>::value;

template <typename T>
struct incrementable_traits_helper<
    T,
    std::enable_if_t<has_member_difference_type_v<T>>> {
    using difference_type = typename T::difference_type;
};

template <typename T>
struct incrementable_traits_helper<
    T,
    std::enable_if_t<!std::is_pointer_v<T> &&
                     !has_member_difference_type_v<T> &&
                     std::is_integral_v<decltype(std::declval<const T&>() -
                                                 std::declval<const T&>())>>>
    : with_difference_type<
          std::make_signed_t<decltype(std::declval<T>() - std::declval<T>())>> {
};
}  // namespace detail

template <typename T>
struct incrementable_traits : detail::incrementable_traits_helper<T> {
};

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
struct readable_traits_helper {
};

template <typename T>
struct readable_traits_helper<T*>
    : detail::conditional_t<std::is_object_v<T>,
                            with_value_type<std::remove_cv_t<T>>,
                            empty> {
};

template <typename I>
struct readable_traits_helper<I, std::enable_if_t<std::is_array_v<I>>>
    : readable_traits<std::decay_t<I>> {
};

template <typename I>
struct readable_traits_helper<const I, std::enable_if_t<!std::is_array_v<I>>>
    : readable_traits<std::decay_t<I>> {
};

template <typename T, typename V = typename T::value_type>
struct member_value_type
    : detail::conditional_t<std::is_object_v<V>, with_value_type<V>, empty> {
};

template <typename T, typename E = typename T::element_type>
struct member_element_type
    : detail::conditional_t<std::is_object_v<E>,
                            with_value_type<std::remove_cv_t<E>>,
                            empty> {
};

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
    : member_value_type<T> {
};

template <typename T>
struct readable_traits_helper<T,
                              std::enable_if_t<has_member_element_type_v<T> &&
                                               !has_member_value_type_v<T>>>
    : member_element_type<T> {
};

// A type which has both value_type and element_type members must specialise
// readable_traits to tell us which one to prefer -- see
// https://github.com/ericniebler/stl2/issues/562
template <typename T>
struct readable_traits_helper<T,
                              std::enable_if_t<has_member_element_type_v<T> &&
                                               has_member_value_type_v<T>>> {
};
}  // namespace detail

template <typename T>
struct readable_traits : detail::readable_traits_helper<T> {
};

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

#if SCN_HAS_CONCEPTS && SCN_HAS_RANGES
using std::contiguous_iterator_tag;
#else
struct contiguous_iterator_tag : random_access_iterator_tag {};
#endif

template <typename T>
struct iterator_category;

namespace detail {
template <typename T, typename = void>
struct iterator_category_ {
};
template <typename T>
struct iterator_category_<T*>
    : std::enable_if<std::is_object_v<T>, contiguous_iterator_tag> {
};
template <typename T>
struct iterator_category_<const T> : iterator_category<T> {
};
template <typename T>
struct iterator_category_<T, std::void_t<typename T::iterator_category>> {
    using type = typename T::iterator_category;
};
}  // namespace detail

template <typename T>
struct iterator_category : detail::iterator_category_<T> {
};
template <typename T>
using iterator_category_t = typename iterator_category<T>::type;

namespace detail {

template <typename T, typename = void>
struct legacy_iterator_category : iterator_category<T> {
};

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
    std::is_default_constructible_v<I> && movable<I> &&
    detail::requires_<detail::weakly_incrementable_concept, I>;

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
            std::is_base_of_v<contiguous_iterator_tag,
                              iterator_category_t<I>> &&
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
        noexcept(decay_copy(SCN_FWD(t).begin())))
        -> std::enable_if_t<
            input_or_output_iterator<decltype(decay_copy(SCN_FWD(t).begin()))>,
            decltype(decay_copy(SCN_FWD(t).begin()))>
    {
        return decay_copy(t.begin());
    }

    template <typename T>
    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
        noexcept(decay_copy(begin(SCN_FWD(t)))))
        -> std::enable_if_t<
            input_or_output_iterator<decltype(decay_copy(begin(SCN_FWD(t))))>,
            decltype(decay_copy(begin(SCN_FWD(t))))>
    {
        return decay_copy(begin(SCN_FWD(t)));
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
              typename S = decltype(decay_copy(SCN_DECLVAL(T).end())),
              typename I = decltype(::scn::ranges::begin(SCN_DECLVAL(T)))>
    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
        noexcept(decay_copy(SCN_FWD(t).end())))
        -> std::enable_if_t<sentinel_for<S, I>,
                            decltype(decay_copy(SCN_FWD(t).end()))>
    {
        return decay_copy(SCN_FWD(t).end());
    }

    template <typename T,
              typename S = decltype(decay_copy(end(SCN_DECLVAL(T)))),
              typename I = decltype(::scn::ranges::begin(SCN_DECLVAL(T)))>
    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(noexcept(
        decay_copy(end(SCN_FWD(t))))) -> std::enable_if_t<sentinel_for<S, I>, S>
    {
        return decay_copy(end(SCN_FWD(t)));
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
              typename D = decltype(decay_copy(SCN_DECLVAL(T&).data()))>
    static constexpr auto impl(T& t, priority_tag<1>) noexcept(noexcept(
        decay_copy(t.data()))) -> std::enable_if_t<is_object_pointer_v<D>, D>
    {
        return t.data();
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

    template <typename T,
              typename I = decltype(decay_copy(std::declval<T>().size()))>
    static constexpr auto impl(T&& t, priority_tag<2>) noexcept(
        noexcept(decay_copy(SCN_FWD(t).size())))
        -> std::enable_if_t<std::is_integral_v<I> &&
                                !disable_sized_range<remove_cvref_t<T>>,
                            I>
    {
        return decay_copy(SCN_FWD(t).size());
    }

    template <typename T,
              typename I = decltype(decay_copy(size(std::declval<T>())))>
    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
        noexcept(decay_copy(size(SCN_FWD(t)))))
        -> std::enable_if_t<std::is_integral_v<I> &&
                                !disable_sized_range<remove_cvref_t<T>>,
                            I>
    {
        return decay_copy(size(SCN_FWD(t)));
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

struct dangling {
    constexpr dangling() noexcept = default;

    template <typename... Args>
    constexpr dangling(Args&&...) noexcept
    {
    }
};

template <typename R>
using borrowed_iterator_t =
    std::conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;

template <typename D>
class view_interface {
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
    [[nodiscard]] constexpr auto empty()
        -> std::enable_if_t<forward_range<R>, bool>
    {
        return ranges::begin(derived()) == ranges::end(derived());
    }

    template <typename R = D>
    [[nodiscard]] constexpr auto empty() const
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
        return ranges::empty(derived())
                   ? nullptr
                   : std::addressof(*ranges::begin(derived()));
    }

    template <typename R = D,
              typename = std::enable_if_t<
                  range<const R> && contiguous_iterator<iterator_t<const R>>>>
    constexpr auto data() const
    {
        return ranges::empty(derived())
                   ? nullptr
                   : std::addressof(*ranges::begin(derived()));
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

template <typename I, typename S = I>
class subrange : public view_interface<subrange<I, S>> {
    static_assert(input_or_output_iterator<I>);
    static_assert(sentinel_for<S, I>);

public:
    subrange() = default;

    template <typename I_,
              std::enable_if_t<std::is_convertible_v<I_, I>>* = nullptr>
    constexpr subrange(I_ i, S s)
        : m_iterator(SCN_MOVE(i)), m_sentinel(SCN_MOVE(s))
    {
    }

    template <
        typename R,
        std::enable_if_t<is_not_self<R, subrange> && borrowed_range<R> &&
                         std::is_convertible_v<iterator_t<R>, I> &&
                         std::is_convertible_v<sentinel_t<R>, S>>* = nullptr>
    constexpr subrange(R&& r)
        : m_iterator(scn::ranges::begin(r)), m_sentinel(scn::ranges::end(r))
    {
    }

    constexpr I begin() const
    {
        return m_iterator;
    }
    constexpr S end() const
    {
        return m_sentinel;
    }

    SCN_NODISCARD constexpr bool empty() const
    {
        return m_iterator == m_sentinel;
    }

    template <typename I_ = I,
              std::enable_if_t<sized_sentinel_for<S, I_>>* = nullptr>
    constexpr std::size_t size() const
    {
        return static_cast<size_t>(m_sentinel - m_iterator);
    }

private:
    SCN_NO_UNIQUE_ADDRESS I m_iterator{};
    SCN_NO_UNIQUE_ADDRESS S m_sentinel{};
};

template <typename I,
          typename S,
          std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>>* =
              nullptr>
subrange(I, S) -> subrange<I, S>;

template <typename R, std::enable_if_t<borrowed_range<R>>* = nullptr>
subrange(R&&) -> subrange<iterator_t<R>, sentinel_t<R>>;

}  // namespace detail::subrange_

using detail::subrange_::subrange;

template <typename I, typename S>
inline constexpr bool enable_borrowed_range<subrange<I, S>> = true;

struct default_sentinel_t {};
inline constexpr default_sentinel_t default_sentinel{};

}  // namespace ranges

namespace detail {
namespace simple_iterator_t_fn {
template <typename T>
constexpr auto impl()
{
    return tag_type<decltype(ranges::begin(SCN_DECLVAL(T&)))>{};
}

template <typename Range>
using result = decltype(impl<Range>());
}  // namespace simple_iterator_t_fn

template <typename Range>
using simple_iterator_t = typename simple_iterator_t_fn::result<Range>::type;

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

template <typename Range, typename = void>
inline constexpr bool is_file_or_narrow_range_impl = false;
template <>
inline constexpr bool is_file_or_narrow_range_impl<std::FILE*, void> = true;
template <typename Range>
inline constexpr bool
    is_file_or_narrow_range_impl<Range,
                                 std::enable_if_t<ranges::range<Range>>> =
        std::is_same_v<char_t<Range>, char>;

template <typename Range>
inline constexpr bool is_file_or_narrow_range =
    is_file_or_narrow_range_impl<remove_cvref_t<Range>>;

template <typename Range, typename = void>
inline constexpr bool is_wide_range = false;
template <typename Range>
inline constexpr bool
    is_wide_range<Range,
                  std::enable_if_t<ranges::range<remove_cvref_t<Range>>>> =
        std::is_same_v<char_t<Range>, wchar_t>;

// borrowed_iterator_t and borrowed_subrange_t, but shorter template
// names

template <typename R, bool Borrowed = ranges::borrowed_range<R>>
struct simple_borrowed_iterator {
    using type = simple_iterator_t<R>;
};
template <typename R>
struct simple_borrowed_iterator<R, false> {
    using type =
        std::conditional_t<std::is_same_v<remove_cvref_t<R>, std::FILE*>,
                           std::FILE*,
                           ranges::dangling>;
};

template <typename R>
using simple_borrowed_iterator_t = typename simple_borrowed_iterator<R>::type;

template <typename R, bool Borrowed = ranges::borrowed_range<R>>
struct simple_borrowed_subrange {
    using type = ranges::subrange<simple_iterator_t<R>>;
};
template <typename R>
struct simple_borrowed_subrange<R, false> {
    using type =
        std::conditional_t<std::is_same_v<remove_cvref_t<R>, std::FILE*>,
                           std::FILE*,
                           ranges::dangling>;
};

template <typename R>
using simple_borrowed_subrange_t = typename simple_borrowed_subrange<R>::type;

template <typename R, bool Borrowed = ranges::borrowed_range<R>>
struct borrowed_tail_subrange {
    using type = ranges::subrange<ranges::iterator_t<R>, ranges::sentinel_t<R>>;
};
template <typename R>
struct borrowed_tail_subrange<R, false> {
    using type = ranges::dangling;
};

/// Equivalent to
/// `ranges_impl::subrange<ranges_impl::iterator_t<R>,
/// ranges_impl::sentinel_t<R>>` if `R` is a `borrowed_range`, and
/// `ranges_impl::dangling` otherwise.
///
/// Similar to `ranges_impl::borrowed_subrange_t<R>`, expect this preserves
/// the range sentinel.
template <typename R>
using borrowed_tail_subrange_t = typename borrowed_tail_subrange<R>::type;

}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn

#if SCN_STDLIB_LIBCPP
#define SCN_RANGES_HAS_NAMESPACE_STD 1
#define SCN_BEGIN_NAMESPACE_STD      _LIBCPP_BEGIN_NAMESPACE_STD
#define SCN_END_NAMESPACE_STD        _LIBCPP_END_NAMESPACE_STD
#elif SCN_STDLIB_MS_STL
#define SCN_RANGES_HAS_NAMESPACE_STD 1
#define SCN_BEGIN_NAMESPACE_STD      _STD_BEGIN
#define SCN_END_NAMESPACE_STD        _STD_END
#elif defined(_GLIBCXX_DEBUG)
#define SCN_RANGES_HAS_NAMESPACE_STD 0
#else
#define SCN_RANGES_HAS_NAMESPACE_STD 1
#define SCN_BEGIN_NAMESPACE_STD      namespace std {
#define SCN_END_NAMESPACE_STD        }
#endif

#if SCN_RANGES_HAS_NAMESPACE_STD
SCN_BEGIN_NAMESPACE_STD
template <typename, typename>
class basic_string_view;
SCN_END_NAMESPACE_STD
#else
#include <string_view>
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace ranges {
template <typename CharT, typename Traits>
inline constexpr bool
    enable_borrowed_range<std::basic_string_view<CharT, Traits>> = true;
}

SCN_END_NAMESPACE
}  // namespace scn
