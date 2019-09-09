// Copyright 2017-2019 Elias Kosunen
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
//
// The contents of this file are essentially stolen from NanoRange,
// except the code has been ported to C++11
//     https://github.com/tcbrindle/NanoRange
//     Copyright (c) 2018 Tristan Brindle
//     Distributed under the Boost Software License, Version 1.0

#ifndef SCN_DETAIL_RANGES_H
#define SCN_DETAIL_RANGES_H

#include "string_view.h"

#include <iterator>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        namespace ranges {
            // iterator_category
            using std::bidirectional_iterator_tag;
            using std::forward_iterator_tag;
            using std::input_iterator_tag;
            using std::output_iterator_tag;
            using std::random_access_iterator_tag;
            struct contiguous_iterator_tag : random_access_iterator_tag {
            };

            template <typename T>
            struct iterator_category;

            template <typename T, typename = void>
            struct _iterator_category {
            };
            template <typename T>
            struct _iterator_category<T*>
                : std::enable_if<std::is_object<T>::value,
                                 contiguous_iterator_tag> {
            };
            template <typename T>
            struct _iterator_category<const T> : iterator_category<T> {
            };
            template <typename T>
            struct _iterator_category<T,
                                      void_t<typename T::iterator_category>> {
                using type = typename T::iterator_category;
            };

            template <typename T>
            struct iterator_category : _iterator_category<T> {
            };
            template <typename T>
            using iterator_category_t = typename iterator_category<T>::type;

            template <typename T>
            using iter_reference_t = decltype(*std::declval<T&>());

            // iter_difference_t
            template <typename>
            struct incrementable_traits;

            struct _empty {
            };

            template <typename T>
            struct _with_difference_type {
                using difference_type = T;
            };
            template <typename, typename = void>
            struct _incrementable_traits_helper {
            };

            template <>
            struct _incrementable_traits_helper<void*> {
            };
            template <typename T>
            struct _incrementable_traits_helper<T*>
                : std::conditional<std::is_object<T>::value,
                                   _with_difference_type<std::ptrdiff_t>,
                                   _empty>::type {
            };
            template <typename I>
            struct _incrementable_traits_helper<const I>
                : incrementable_traits<typename std::decay<I>::type> {
            };

            template <typename, typename = void>
            struct _has_member_difference_type : std::false_type {
            };
            template <typename T>
            struct _has_member_difference_type<
                T,
                void_t<typename T::difference_type>> : std::true_type {
            };

            template <typename T>
            struct _incrementable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_difference_type<T>::value>::type> {
                using difference_type = typename T::difference_type;
            };
            template <typename T>
            struct _incrementable_traits_helper<
                T,
                typename std::enable_if<
                    !std::is_pointer<T>::value &&
                    !_has_member_difference_type<T>::value &&
                    std::is_integral<decltype(
                        std::declval<const T&>() -
                        std::declval<const T&>())>::value>::type>
                : _with_difference_type<typename std::make_signed<decltype(
                      std::declval<T>() - std::declval<T>())>::type> {
            };
            template <typename T>
            struct incrementable_traits : _incrementable_traits_helper<T> {
            };

            template <typename T>
            using iter_difference_t =
                typename incrementable_traits<T>::difference_type;

            // iter_value_t
            template <typename>
            struct readable_traits;

            template <typename T>
            struct _with_value_type {
                using value_type = T;
            };
            template <typename, typename = void>
            struct _readable_traits_helper {
            };

            template <typename T>
            struct _readable_traits_helper<T*>
                : std::conditional<
                      std::is_object<T>::value,
                      _with_value_type<typename std::remove_cv<T>::type>,
                      _empty>::type {
            };

            template <typename I>
            struct _readable_traits_helper<
                I,
                typename std::enable_if<std::is_array<I>::value>::type>
                : readable_traits<typename std::decay<I>::type> {
            };

            template <typename I>
            struct _readable_traits_helper<
                const I,
                typename std::enable_if<!std::is_array<I>::value>::type>
                : readable_traits<typename std::decay<I>::type> {
            };

            template <typename T, typename V = typename T::value_type>
            struct _member_value_type
                : std::conditional<std::is_object<V>::value,
                                   _with_value_type<V>,
                                   _empty>::type {
            };

            template <typename T, typename E = typename T::element_type>
            struct _member_element_type
                : std::conditional<
                      std::is_object<E>::value,
                      _with_value_type<typename std::remove_cv<E>::type>,
                      _empty>::type {
            };

            template <typename T>
            using _member_value_type_t = typename T::value_type;

            template <typename T>
            struct _has_member_value_type : exists<_member_value_type_t, T> {
            };

            template <typename T>
            using _member_element_type_t = typename T::element_type;

            template <typename T>
            struct _has_member_element_type
                : exists<_member_element_type_t, T> {
            };

            template <typename T>
            struct _readable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_value_type<T>::value &&
                    !_has_member_element_type<T>::value>::type>
                : _member_value_type<T> {
            };

            template <typename T>
            struct _readable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_element_type<T>::value &&
                    !_has_member_value_type<T>::value>::type>
                : _member_element_type<T> {
            };

            template <typename T>
            struct _readable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_element_type<T>::value &&
                    _has_member_value_type<T>::value>::type> {
            };

            template <typename T>
            struct readable_traits : _readable_traits_helper<T> {
            };

            template <typename T>
            using iter_value_t = typename readable_traits<T>::value_type;

            // sentinel_for
            struct _sentinel_for_concept {
                template <typename S, typename I>
                auto _test_requires(S s, I i)
                    -> decltype(scn::detail::valid_expr(*i, i == s, i != s));
            };
            template <typename S, typename I>
            struct sentinel_for
                : std::integral_constant<
                      bool,
                      std::is_default_constructible<S>::value &&
                          std::is_copy_constructible<S>::value &&
                          _requires<_sentinel_for_concept, S, I>::value> {
            };

            // sized_sentinel_for
            struct _sized_sentinel_for_concept {
                template <typename S, typename I>
                auto _test_requires(const S& s, const I& i) -> decltype(
                    requires_expr<std::is_same<decltype(s - i),
                                               iter_difference_t<I>>::value>{},
                    requires_expr<std::is_same<decltype(i - s),
                                               iter_difference_t<I>>::value>{});
            };
            template <typename S, typename I>
            struct sized_sentinel_for
                : std::integral_constant<
                      bool,
                      _requires<_sized_sentinel_for_concept, S, I>::value &&
                          sentinel_for<S, I>::value> {
            };
            template <typename S>
            struct sized_sentinel_for<S, void*> : std::false_type {
            };
            template <typename I>
            struct sized_sentinel_for<void*, I> : std::false_type {
            };
            template <>
            struct sized_sentinel_for<void*, void*> : std::false_type {
            };

            // begin
            namespace _begin {
                template <typename T>
                void begin(T&&) = delete;
                template <typename T>
                void begin(std::initializer_list<T>&&) = delete;

                struct fn {
                private:
                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR void impl(T(&&)[N],
                                                   priority_tag<2>) = delete;

                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR auto impl(T (&t)[N],
                                                   priority_tag<2>) noexcept
                        -> decltype((t) + 0)
                    {
                        return (t) + 0;
                    }

                    template <typename C>
                    static SCN_CONSTEXPR auto impl(basic_string_view<C> sv,
                                                   priority_tag<2>) noexcept
                        -> decltype(sv.begin())
                    {
                        return sv.begin();
                    }

                    template <typename T>
                    static SCN_CONSTEXPR auto
                    impl(T& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(t.begin())))
                        -> decltype(decay_copy(t.begin()))
                    {
                        return decay_copy(t.begin());
                    }

                    template <typename T>
                    static SCN_CONSTEXPR auto
                    impl(T& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(begin(std::forward<T>(t)))))
                        -> decltype(decay_copy(begin(std::forward<T>(t))))
                    {
                        return decay_copy(begin(std::forward<T>(t)));
                    }

                public:
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            }  // namespace _begin
            namespace {
                SCN_CONSTEXPR auto& begin = static_const<_begin::fn>::value;
            }

            // end
            namespace _end {
                template <typename T>
                void end(T&&) = delete;
                template <typename T>
                void end(std::initializer_list<T>&&) = delete;

                struct fn {
                private:
                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR void impl(T(&&)[N],
                                                   priority_tag<2>) = delete;

                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR auto impl(T (&t)[N],
                                                   priority_tag<2>) noexcept
                        -> decltype((t) + N)
                    {
                        return (t) + N;
                    }

                    template <typename C>
                    static SCN_CONSTEXPR auto impl(basic_string_view<C> sv,
                                                   priority_tag<2>) noexcept
                        -> decltype(sv.end())
                    {
                        return sv.end();
                    }

                    template <
                        typename T,
                        typename S =
                            decltype(decay_copy(std::declval<T&>().end())),
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T&>()))>
                    static SCN_CONSTEXPR auto impl(
                        T& t,
                        priority_tag<1>) noexcept(noexcept(decay_copy(t.end())))
                        -> decltype(decay_copy(t.end()))
                    {
                        return decay_copy(t.end());
                    }

                    template <
                        typename T,
                        typename S =
                            decltype(decay_copy(end(std::declval<T>()))),
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T>()))>
                    static SCN_CONSTEXPR auto
                    impl(T& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(end(std::forward<T>(t))))) -> S
                    {
                        return decay_copy(end(std::forward<T>(t)));
                    }

                public:
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            }  // namespace _end
            namespace {
                SCN_CONSTEXPR auto& end = static_const<_end::fn>::value;
            }

            // cbegin
            namespace _cbegin {
                struct fn {
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T& t) const
                        noexcept(noexcept(::scn::detail::ranges::begin(t)))
                            -> decltype(::scn::detail::ranges::begin(t))
                    {
                        return ::scn::detail::ranges::begin(t);
                    }

                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T&& t) const
                        noexcept(noexcept(::scn::detail::ranges::begin(
                            static_cast<const T&&>(t))))
                            -> decltype(::scn::detail::ranges::begin(
                                static_cast<const T&&>(t)))
                    {
                        return ::scn::detail::ranges::begin(
                            static_cast<const T&&>(t));
                    }
                };
            }  // namespace _cbegin
            namespace {
                SCN_CONSTEXPR auto& cbegin = static_const<_cbegin::fn>::value;
            }

            // cend
            namespace _cend {
                struct fn {
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T& t) const
                        noexcept(noexcept(::scn::detail::ranges::end(t)))
                            -> decltype(::scn::detail::ranges::end(t))
                    {
                        return ::scn::detail::ranges::end(t);
                    }

                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T&& t) const
                        noexcept(noexcept(::scn::detail::ranges::end(
                            static_cast<const T&&>(t))))
                            -> decltype(::scn::detail::ranges::end(
                                static_cast<const T&&>(t)))
                    {
                        return ::scn::detail::ranges::end(
                            static_cast<const T&&>(t));
                    }
                };
            }  // namespace _cend
            namespace {
                SCN_CONSTEXPR auto& cend = static_const<_cend::fn>::value;
            }

            // range
            struct _range_impl_concept {
                template <typename T>
                auto _test_requires(T&& t) -> decltype(
                    ::scn::detail::ranges::begin(std::forward<T>(t)),
                    ::scn::detail::ranges::end(std::forward<T>(t)));
            };
            template <typename T>
            struct _range_impl : _requires<_range_impl_concept, T> {
            };
            struct _range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) ->
                    typename std::enable_if<_range_impl<T&>::value,
                                            std::true_type>::type;
            };
            template <typename T>
            struct range : decltype(_range_concept::test<T>(0)) {
            };

            template <typename T>
            struct forwarding_range
                : std::integral_constant<bool,
                                         range<T>::value &&
                                             _range_impl<T>::value> {
            };

            // typedefs
            template <typename R>
            using iterator_t =
                typename std::enable_if<range<R>::value,
                                        decltype(::scn::detail::ranges::begin(
                                            std::declval<R&>()))>::type;
            template <typename R>
            using sentinel_t =
                typename std::enable_if<range<R>::value,
                                        decltype(::scn::detail::ranges::end(
                                            std::declval<R&>()))>::type;
            template <typename R>
            using range_difference_t =
                typename std::enable_if<range<R>::value,
                                        iter_difference_t<iterator_t<R>>>::type;
            template <typename R>
            using range_value_t =
                typename std::enable_if<range<R>::value,
                                        iter_value_t<iterator_t<R>>>::type;
            template <typename R>
            using range_reference_t =
                typename std::enable_if<range<R>::value,
                                        iter_reference_t<iterator_t<R>>>::type;

            // view
            struct view_base {
            };
            template <typename>
            struct _is_std_non_view : std::false_type {
            };
            template <typename T>
            struct _is_std_non_view<std::initializer_list<T>> : std::true_type {
            };
            template <typename T>
            struct _enable_view_helper
                : std::conditional<
                      std::is_base_of<view_base, T>::value,
                      std::true_type,
                      typename std::conditional<
                          _is_std_non_view<T>::value,
                          std::false_type,
                          typename std::conditional<
                              range<T>::value && range<const T>::value,
                              std::is_same<range_reference_t<T>,
                                           range_reference_t<const T>>,
                              std::true_type>::type>::type>::type {
            };
            template <typename T>
            struct view
                : std::integral_constant<
                      bool,
                      range<T>::value && std::is_copy_constructible<T>::value &&
                          std::is_default_constructible<T>::value &&
                          _enable_view_helper<T>::value> {
            };

            // data
            template <typename P>
            struct _is_object_pointer
                : std::integral_constant<
                      bool,
                      std::is_pointer<P>::value &&
                          std::is_object<_test_t<iter_value_t, P>>::value> {
            };

            namespace _data {
                struct fn {
                private:
                    template <typename T,
                              typename D = decltype(
                                  decay_copy(std::declval<T&>().data()))>
                    static constexpr auto impl(T& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(t.data()))) ->
                        typename std::enable_if<_is_object_pointer<D>::value,
                                                D>::type
                    {
                        return t.data();
                    }

                    template <typename T>
                    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
                        noexcept(
                            ::scn::detail::ranges::begin(std::forward<T>(t))))
                        -> typename std::enable_if<
                            _is_object_pointer<
                                decltype(::scn::detail::ranges::begin(
                                    std::forward<T>(t)))>::value,
                            decltype(::scn::detail::ranges::begin(
                                std::forward<T>(t)))>::type
                    {
                        return ::scn::detail::ranges::begin(std::forward<T>(t));
                    }

                public:
                    template <typename T>
                    constexpr auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<1>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<1>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<1>{});
                    }
                };
            }  // namespace _data
            namespace {
                SCN_CONSTEXPR auto& data = static_const<_data::fn>::value;
            }

            // size
            template <typename>
            struct disable_sized_range : std::false_type {
            };

            namespace _size {
                template <typename T>
                void size(T&&) = delete;
                template <typename T>
                void size(T&) = delete;

                struct fn {
                private:
                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR std::size_t impl(
                        const T(&&)[N],
                        priority_tag<3>) noexcept
                    {
                        return N;
                    }

                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR std::size_t impl(
                        const T (&)[N],
                        priority_tag<3>) noexcept
                    {
                        return N;
                    }

                    template <typename T,
                              typename I = decltype(
                                  decay_copy(std::declval<T>().size()))>
                    static constexpr auto impl(T&& t, priority_tag<2>) noexcept(
                        noexcept(decay_copy(std::forward<T>(t).size()))) ->
                        typename std::enable_if<
                            std::is_integral<I>::value &&
                                !disable_sized_range<remove_cvref_t<T>>::value,
                            I>::type
                    {
                        return decay_copy(std::forward<T>(t).size());
                    }

                    template <typename T,
                              typename I =
                                  decltype(decay_copy(size(std::declval<T>())))>
                    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(size(std::forward<T>(t))))) ->
                        typename std::enable_if<
                            std::is_integral<I>::value &&
                                !disable_sized_range<remove_cvref_t<T>>::value,
                            I>::type
                    {
                        return decay_copy(size(std::forward<T>(t)));
                    }

                    template <
                        typename T,
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T>())),
                        typename S = decltype(
                            ::scn::detail::ranges::end(std::declval<T>())),
                        typename D = decltype(decay_copy(std::declval<S>() -
                                                         std::declval<I>()))>
                    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(::scn::detail::ranges::end(t) -
                                            ::scn::detail::ranges::begin(t))))
                        -> typename std::enable_if<
                            !std::is_array<remove_cvref_t<T>>::value,
                            D>::type
                    {
                        return decay_copy(::scn::detail::ranges::end(t) -
                                          ::scn::detail::ranges::begin(t));
                    }

                public:
                    template <typename T>
                    constexpr auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<3>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<3>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<3>{});
                    }
                };
            }  // namespace _size
            namespace {
                SCN_CONSTEXPR auto& size = static_const<_size::fn>::value;
            }

            // empty
            namespace _empty_ns {
                struct fn {
                private:
                    template <typename T>
                    static SCN_CONSTEXPR auto
                    impl(T&& t, priority_tag<2>) noexcept(
                        noexcept((bool(std::forward<T>(t).empty()))))
                        -> decltype((bool(std::forward<T>(t).empty())))
                    {
                        return bool((std::forward<T>(t).empty()));
                    }
                    template <typename T>
                    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
                        noexcept(::scn::detail::ranges::size(
                                     std::forward<T>(t)) == 0))
                        -> decltype(::scn::detail::ranges::size(
                                        std::forward<T>(t)) == 0)
                    {
                        return ::scn::detail::ranges::size(
                                   std::forward<T>(t)) == 0;
                    }

                    template <
                        typename T,
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T>()))>
                    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
                        noexcept(::scn::detail::ranges::begin(t) ==
                                 ::scn::detail::ranges::end(t)))
                        -> decltype(::scn::detail::ranges::begin(t) ==
                                    ::scn::detail::ranges::end(t))
                    {
                        return ::scn::detail::ranges::begin(t) ==
                               ::scn::detail::ranges::end(t);
                    }

                public:
                    template <typename T>
                    constexpr auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            }  // namespace _empty_ns
            namespace {
                SCN_CONSTEXPR auto& empty = static_const<_empty_ns::fn>::value;
            }

            // sized_range
            struct _sized_range_concept {
                template <typename T>
                auto _test_requires(T& t)
                    -> decltype(::scn::detail::ranges::size(t));
            };
            template <typename T>
            struct sized_range
                : std::integral_constant<
                      bool,
                      range<T>::value &&
                          !disable_sized_range<
                              detail::remove_cvref_t<T>>::value &&
                          _requires<_sized_range_concept, T>::value> {
            };

            // contiguous_range
            struct _contiguous_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    _requires<_contiguous_range_concept, T>::value,
                    std::true_type>::type;

                template <typename T>
                auto _test_requires(T& t)
                    -> decltype(requires_expr<std::is_same<
                                    decltype(::scn::detail::ranges::data(t)),
                                    typename std::add_pointer<
                                        range_reference_t<T>>::type>::value>{});
            };
            template <typename T>
            struct contiguous_range
                : decltype(_contiguous_range_concept::test<T>(0)) {
            };

            // subrange
            template <typename D>
            class view_interface : public view_base {
                static_assert(std::is_class<D>::value, "");
                static_assert(
                    std::is_same<D, typename std::remove_cv<D>::type>::value,
                    "");

            private:
                SCN_CONSTEXPR14 D& derived() noexcept
                {
                    return static_cast<D&>(*this);
                }
                SCN_CONSTEXPR D& derived() const noexcept
                {
                    return static_cast<const D&>(*this);
                }

            public:
                SCN_NODISCARD SCN_CONSTEXPR14 bool empty()
                {
                    return ::scn::detail::ranges::begin(derived()) ==
                           ::scn::detail::ranges::end(derived());
                }
                SCN_NODISCARD SCN_CONSTEXPR bool empty() const
                {
                    return ::scn::detail::ranges::begin(derived()) ==
                           ::scn::detail::ranges::end(derived());
                }

                template <typename R = D,
                          typename = decltype(
                              ::scn::detail::ranges::empty(std::declval<R&>()))>
                SCN_CONSTEXPR14 explicit operator bool()
                {
                    return !::scn::detail::ranges::empty(derived());
                }
                template <typename R = D,
                          typename = decltype(::scn::detail::ranges::empty(
                              std::declval<const R&>()))>
                SCN_CONSTEXPR explicit operator bool() const
                {
                    return !::scn::detail::ranges::empty(derived());
                }

                SCN_CONSTEXPR14 auto data() -> decltype(
                    std::addressof(*::scn::detail::ranges::begin(derived())))
                {
                    return ::scn::detail::ranges::empty(derived())
                               ? nullptr
                               : std::addressof(
                                     *::scn::detail::ranges::begin(derived()));
                }
                SCN_CONSTEXPR auto data() const -> decltype(
                    std::addressof(*::scn::detail::ranges::begin(derived())))
                {
                    return ::scn::detail::ranges::empty(derived())
                               ? nullptr
                               : std::addressof(
                                     *::scn::detail::ranges::begin(derived()));
                }

                template <typename R = D,
                          typename std::enable_if<
                              range<R>::value &&
                              sized_sentinel_for<sentinel_t<R>, iterator_t<R>>::
                                  value>::type* = nullptr>
                SCN_CONSTEXPR14 auto size()
                    -> decltype(::scn::detail::ranges::end(derived()) -
                                ::scn::detail::ranges::begin(derived()))
                {
                    return ::scn::detail::ranges::end(derived()) -
                           ::scn::detail::ranges::begin(derived());
                }

                template <
                    typename R = D,
                    typename std::enable_if<
                        range<const R>::value &&
                        sized_sentinel_for<sentinel_t<const R>,
                                           iterator_t<const R>>::value>::type* =
                        nullptr>
                SCN_CONSTEXPR auto size() const
                    -> decltype(::scn::detail::ranges::end(derived()) -
                                ::scn::detail::ranges::begin(derived()))
                {
                    return ::scn::detail::ranges::end(derived()) -
                           ::scn::detail::ranges::begin(derived());
                }
            };

            enum class subrange_kind : bool { unsized, sized };

            template <typename I, typename S>
            struct _default_subrange_kind
                : std::integral_constant<subrange_kind,
                                         sized_sentinel_for<S, I>::value
                                             ? subrange_kind::sized
                                             : subrange_kind::unsized> {
            };

            namespace _subrange {
                template <typename I,
                          typename S = I,
                          subrange_kind = _default_subrange_kind<I, S>::value>
                class subrange;
            }  // namespace _subrange

            using _subrange::subrange;

            struct _pair_like_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T,
                          typename = typename std::tuple_size<T>::type>
                static auto test(int) -> typename std::enable_if<
                    _requires<_pair_like_concept, T>::value,
                    std::true_type>::type;

                template <typename T>
                auto _test_requires(T t) -> decltype(
                    requires_expr<
                        std::is_base_of<std::integral_constant<std::size_t, 2>,
                                        std::tuple_size<T>>::value>{},
                    std::declval<std::tuple_element<
                        0,
                        typename std::remove_const<T>::type>>(),
                    std::declval<std::tuple_element<
                        1,
                        typename std::remove_const<T>::type>>(),
                    requires_expr<std::is_convertible<
                        decltype(std::get<0>(t)),
                        const std::tuple_element<0, T>&>::value>{},
                    requires_expr<std::is_convertible<
                        decltype(std::get<1>(t)),
                        const std::tuple_element<1, T>&>::value>{});
            };
            template <typename T>
            struct _pair_like
                : std::integral_constant<
                      bool,
                      !std::is_reference<T>::value &&
                          decltype(_pair_like_concept::test<T>(0))::value> {
            };

            struct _pair_like_convertible_to_concept {
                template <typename T, typename U, typename V>
                auto _test_requires(T&& t)
                    -> decltype(requires_expr<std::is_convertible<
                                    decltype(std::get<0>(std::forward<T>(t))),
                                    U>::value>{},
                                requires_expr<std::is_convertible<
                                    decltype(std::get<1>(std::forward<T>(t))),
                                    V>::value>{});
            };
            template <typename T, typename U, typename V>
            struct _pair_like_convertible_to
                : std::integral_constant<
                      bool,
                      !range<T>::value &&
                          _pair_like<
                              typename std::remove_reference<T>::type>::value &&
                          _requires<_pair_like_convertible_to_concept,
                                    T,
                                    U,
                                    V>::value> {
            };
            template <typename T, typename U, typename V>
            struct _pair_like_convertible_from
                : std::integral_constant<
                      bool,
                      !range<T>::value &&
                          _pair_like<
                              typename std::remove_reference<T>::type>::value &&
                          std::is_constructible<T, U, V>::value> {
            };

            struct _iterator_sentinel_pair_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    !range<T>::value && _pair_like<T>::value &&
                        sentinel_for<
                            typename std::tuple_element<1, T>::type,
                            typename std::tuple_element<0, T>::type>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct _iterator_sentinel_pair
                : decltype(_iterator_sentinel_pair_concept::test<T>(0)) {
            };

            template <typename I, typename S, bool StoreSize = false>
            struct _subrange_data {
                SCN_CONSTEXPR _subrange_data() = default;
                SCN_CONSTEXPR _subrange_data(I&& b, S&& e)
                    : begin(std::move(b)), end(std::move(e))
                {
                }
                SCN_CONSTEXPR _subrange_data(I&& b, S&& e, iter_difference_t<I>)
                    : begin(std::move(b)), end(std::move(e))
                {
                }

                I begin{};
                S end{};
            };

            template <typename I, typename S>
            struct _subrange_data<I, S, true> {
                SCN_CONSTEXPR _subrange_data() = default;
                SCN_CONSTEXPR _subrange_data(I&& b,
                                             S&& e,
                                             iter_difference_t<I> s)
                    : begin(std::move(b)), end(std::move(e)), size(s)
                {
                }

                I begin{};
                S end{};
                iter_difference_t<I> size{0};
            };

            template <typename R, typename I, typename S, subrange_kind K>
            auto _subrange_range_constructor_constraint_helper_fn(long)
                -> std::false_type;

            template <typename R, typename I, typename S, subrange_kind K>
            auto _subrange_range_constructor_constraint_helper_fn(int) ->
                typename std::enable_if<
                    forwarding_range<R>::value &&
                        std::is_convertible<iterator_t<R>, I>::value &&
                        std::is_convertible<sentinel_t<R>, S>::value,
                    std::true_type>::type;

            template <typename R, typename I, typename S, subrange_kind K>
            struct _subrange_range_constructor_constraint_helper
                : decltype(
                      _subrange_range_constructor_constraint_helper_fn<R,
                                                                       I,
                                                                       S,
                                                                       K>(0)) {
            };

            template <typename R>
            SCN_CONSTEXPR subrange_kind _subrange_deduction_guide_helper()
            {
                return (sized_range<R>::value ||
                        sized_sentinel_for<sentinel_t<R>, iterator_t<R>>::value)
                           ? subrange_kind::sized
                           : subrange_kind::unsized;
            }

            template <typename T, typename U>
            struct _not_same_as : std::integral_constant<
                                      bool,
                                      !std::is_same<remove_cvref_t<T>,
                                                    remove_cvref_t<U>>::value> {
            };

            namespace _subrange {
                template <typename I, typename S, subrange_kind K>
                class subrange : public view_interface<subrange<I, S, K>> {
                    static_assert(requires_expr<*std::declval<const I&>()>{},
                                  "");
                    static_assert(requires_expr<++std::declval<const I&>()>{},
                                  "");
                    static_assert(sentinel_for<S, I>::value, "");
                    static_assert(K == subrange_kind::sized ||
                                      !sized_sentinel_for<S, I>::value,
                                  "");

                    static SCN_CONSTEXPR bool _store_size()
                    {
                        return K == subrange_kind::sized &&
                               !sized_sentinel_for<S, I>::value;
                    }

                public:
                    using iterator = I;
                    using sentinel = S;

                    subrange() = default;

                    template <bool SS = _store_size(),
                              typename std::enable_if<!SS>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(I i, S s)
                        : m_data{std::move(i), std::move(s)}
                    {
                    }
                    template <subrange_kind KK = K,
                              typename std::enable_if<
                                  KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(I i, S s, iter_difference_t<I> n)
                        : m_data{std::move(i), std::move(s), n}
                    {
                    }

                    template <
                        typename R,
                        bool SS = _store_size(),
                        typename std::enable_if<
                            _not_same_as<R, subrange>::value>::type* = nullptr,
                        typename std::enable_if<
                            _subrange_range_constructor_constraint_helper<
                                R,
                                I,
                                S,
                                K>::value &&
                            SS && sized_range<R>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(R&& r)
                        : subrange(::scn::detail::ranges::begin(r),
                                   ::scn::detail::ranges::end(r),
                                   ::scn::detail::ranges::size(r))
                    {
                    }

                    template <
                        typename R,
                        bool SS = _store_size(),
                        typename std::enable_if<
                            _not_same_as<R, subrange>::value>::type* = nullptr,
                        typename std::enable_if<
                            _subrange_range_constructor_constraint_helper<
                                R,
                                I,
                                S,
                                K>::value &&
                            !SS>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(R&& r)
                        : subrange(::scn::detail::ranges::begin(r),
                                   ::scn::detail::ranges::end(r))
                    {
                    }

                    template <
                        typename R,
                        subrange_kind KK = K,
                        typename std::enable_if<
                            forwarding_range<R>::value &&
                            std::is_convertible<iterator_t<R>, I>::value &&
                            std::is_convertible<sentinel_t<R>, S>::value &&
                            KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(R&& r, iter_difference_t<I> n)
                        : subrange(::scn::detail::ranges::begin(r),
                                   ::scn::detail::ranges::end(r),
                                   n)
                    {
                    }

                    template <
                        typename PairLike,
                        bool SS = _store_size(),
                        typename std::enable_if<
                            _not_same_as<PairLike, subrange>::value>::type* =
                            nullptr,
                        typename std::enable_if<
                            _pair_like_convertible_to<PairLike, I, S>::value &&
                            !SS>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(PairLike&& r)
                        : subrange{std::get<0>(std::forward<PairLike>(r)),
                                   std::get<1>(std::forward<PairLike>(r))}
                    {
                    }

                    template <
                        typename PairLike,
                        subrange_kind KK = K,
                        typename std::enable_if<
                            _pair_like_convertible_to<PairLike, I, S>::value &&
                            KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(PairLike&& r,
                                             iter_difference_t<I> n)
                        : subrange{std::get<0>(std::forward<PairLike>(r)),
                                   std::get<1>(std::forward<PairLike>(r)), n}
                    {
                    }

                    template <
                        typename PairLike,
                        typename std::enable_if<
                            _not_same_as<PairLike, subrange>::value>::type* =
                            nullptr,
                        typename std::enable_if<_pair_like_convertible_from<
                            PairLike,
                            const I&,
                            const S&>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 operator PairLike() const
                    {
                        return PairLike(begin(), end());
                    }

                    SCN_CONSTEXPR I begin() const
                    {
                        return m_data.begin;
                    }

                    SCN_CONSTEXPR S end() const
                    {
                        return m_data.end;
                    }

                    SCN_NODISCARD SCN_CONSTEXPR bool empty() const
                    {
                        return m_data.begin == m_data.end;
                    }

                    template <subrange_kind KK = K,
                              typename std::enable_if<
                                  KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR iter_difference_t<I> size() const
                    {
                        return _store_size() ? m_data.size
                                             : m_data.end - m_data.begin;
                    }

                private:
                    _subrange_data<I, S, _store_size()> m_data{};
                };
            }  // namespace _subrange

            // reconstructible_range
            template <typename R>
            struct pair_reconstructible_range
                : std::integral_constant<
                      bool,
                      range<R>::value &&
                          forwarding_range<
                              typename std::remove_reference<R>::type>::value &&
                          std::is_constructible<R,
                                                iterator_t<R>,
                                                sentinel_t<R>>::value> {
            };
            template <typename R>
            struct reconstructible_range
                : std::integral_constant<
                      bool,
                      range<R>::value &&
                          forwarding_range<
                              typename std::remove_reference<R>::type>::value &&
                          std::is_constructible<
                              R,
                              subrange<iterator_t<R>, sentinel_t<R>>>::value> {
            };

            // bidir iterator
            struct _bidirectional_iterator_concept {
                template <typename I>
                auto _test_requires(I i) -> decltype(
                    requires_expr<std::is_same<decltype(i--), I>::value>{});
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename I>
                static auto test(int) -> typename std::enable_if<
                    std::is_base_of<bidirectional_iterator_tag,
                                    iterator_category_t<I>>::value &&
                        _requires<_bidirectional_iterator_concept, I>::value,
                    std::true_type>::type;
            };
            template <typename I>
            struct bidirectional_iterator
                : decltype(_bidirectional_iterator_concept::test<I>(0)) {
            };

            // random access iterator
            struct _random_access_iterator_concept {
                template <typename I>
                auto _test_requires(I i,
                                    const I j,
                                    const iter_difference_t<I> n)
                    -> decltype(valid_expr(
                        j + n,
                        requires_expr<
                            std::is_same<decltype(j + n), I>::value>{},
                        n + j,
#ifndef _MSC_VER
                        requires_expr<
                            std::is_same<decltype(n + j), I>::value>{},
#endif
                        j - n,
                        requires_expr<
                            std::is_same<decltype(j - n), I>::value>{},
                        j[n],
                        requires_expr<
                            std::is_same<decltype(j[n]),
                                         iter_reference_t<I>>::value>{},
                        requires_expr<std::is_convertible<decltype(i < j),
                                                          bool>::value>{}));
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename I>
                static auto test(int) -> typename std::enable_if<
                    bidirectional_iterator<I>::value &&
                        std::is_base_of<random_access_iterator_tag,
                                        iterator_category_t<I>>::value &&
                        sized_sentinel_for<I, I>::value &&
                        _requires<_random_access_iterator_concept, I>::value,
                    std::true_type>::type;
            };
            template <typename I>
            struct random_access_iterator
                : decltype(_random_access_iterator_concept::test<I>(0)) {
            };

            // advance
            namespace _advance {
                struct fn {
                private:
                    template <typename T>
                    static SCN_CONSTEXPR T abs(T t)
                    {
                        return t < T{0} ? -t : t;
                    }

                    template <typename R,
                              typename std::enable_if<random_access_iterator<
                                  R>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(R& r,
                                                     iter_difference_t<R> n)
                    {
                        r += n;
                    }

                    template <
                        typename I,
                        typename std::enable_if<
                            bidirectional_iterator<I>::value &&
                            !random_access_iterator<I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     iter_difference_t<I> n)
                    {
                        constexpr auto zero = iter_difference_t<I>{0};

                        if (n > zero) {
                            while (n-- > zero) {
                                ++i;
                            }
                        }
                        else {
                            while (n++ < zero) {
                                --i;
                            }
                        }
                    }

                    template <typename I,
                              typename std::enable_if<!bidirectional_iterator<
                                  I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     iter_difference_t<I> n)
                    {
                        while (n-- > iter_difference_t<I>{0}) {
                            ++i;
                        }
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            std::is_assignable<I&, S>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     S bound,
                                                     priority_tag<2>)
                    {
                        i = std::move(bound);
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     S bound,
                                                     priority_tag<1>)
                    {
                        fn::impl(i, bound - i);
                    }

                    template <typename I, typename S>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     S bound,
                                                     priority_tag<0>)
                    {
                        while (i != bound) {
                            ++i;
                        }
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 auto impl(I& i,
                                                     iter_difference_t<I> n,
                                                     S bound)
                        -> iter_difference_t<I>
                    {
                        if (fn::abs(n) >= fn::abs(bound - i)) {
                            auto dist = bound - i;
                            fn::impl(i, bound, priority_tag<2>{});
                            return dist;
                        }
                        else {
                            fn::impl(i, n);
                            return n;
                        }
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            bidirectional_iterator<I>::value &&
                            !sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 auto impl(I& i,
                                                     iter_difference_t<I> n,
                                                     S bound)
                        -> iter_difference_t<I>
                    {
                        constexpr iter_difference_t<I> zero{0};
                        iter_difference_t<I> counter{0};

                        if (n < zero) {
                            do {
                                --i;
                                --counter;
                            } while (++n < zero && i != bound);
                        }
                        else {
                            while (n-- > zero && i != bound) {
                                ++i;
                                ++counter;
                            }
                        }

                        return counter;
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            !bidirectional_iterator<I>::value &&
                            !sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 auto impl(I& i,
                                                     iter_difference_t<I> n,
                                                     S bound)
                        -> iter_difference_t<I>
                    {
                        constexpr iter_difference_t<I> zero{0};
                        iter_difference_t<I> counter{0};

                        while (n-- > zero && i != bound) {
                            ++i;
                            ++counter;
                        }

                        return counter;
                    }

                public:
                    template <typename I>
                    SCN_CONSTEXPR14 void operator()(
                        I& i,
                        iter_difference_t<I> n) const
                    {
                        fn::impl(i, n);
                    }

                    template <typename I,
                              typename S,
                              typename std::enable_if<
                                  sentinel_for<S, I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 void operator()(I& i, S bound) const
                    {
                        fn::impl(i, bound, priority_tag<2>{});
                    }

                    template <typename I,
                              typename S,
                              typename std::enable_if<
                                  sentinel_for<S, I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 iter_difference_t<I>
                    operator()(I& i, iter_difference_t<I> n, S bound) const
                    {
                        return n - fn::impl(i, n, bound);
                    }
                };
            }  // namespace _advance
            namespace {
                SCN_CONSTEXPR auto& advance = static_const<_advance::fn>::value;
            }

            // distance
            namespace _distance {
                struct fn {
                private:
                    template <typename I, typename S>
                    static constexpr auto impl(I i, S s) ->
                        typename std::enable_if<sized_sentinel_for<S, I>::value,
                                                iter_difference_t<I>>::type
                    {
                        return s - i;
                    }

                    template <typename I, typename S>
                    static constexpr auto impl(I i, S s) ->
                        typename std::enable_if<
                            !sized_sentinel_for<S, I>::value,
                            iter_difference_t<I>>::type
                    {
                        iter_difference_t<I> counter{0};
                        while (i != s) {
                            ++i;
                            ++counter;
                        }
                        return counter;
                    }

                    template <typename R>
                    static constexpr auto impl(R&& r) ->
                        typename std::enable_if<
                            sized_range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return static_cast<iter_difference_t<iterator_t<R>>>(
                            ranges::size(r));
                    }

                    template <typename R>
                    static constexpr auto impl(R&& r) ->
                        typename std::enable_if<
                            !sized_range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return fn::impl(ranges::begin(r), ranges::end(r));
                    }

                public:
                    template <typename I, typename S>
                    constexpr auto operator()(I first, S last) const ->
                        typename std::enable_if<sentinel_for<S, I>::value,
                                                iter_difference_t<I>>::type
                    {
                        return fn::impl(std::move(first), std::move(last));
                    }

                    template <typename R>
                    constexpr auto operator()(R&& r) const ->
                        typename std::enable_if<
                            range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return fn::impl(std::forward<R>(r));
                    }
                };
            }  // namespace _distance
            namespace {
                SCN_CONSTEXPR auto& distance =
                    static_const<_distance::fn>::value;
            }
        }  // namespace ranges
    }      // namespace detail

#if 0
    namespace detail {
        namespace ranges {
            template <typename T>
            using _with_reference_t = T&;

            struct _can_reference_concept {
                template <typename T>
                auto _test_requires() -> _with_reference_t<T>;
            };
            template <typename T>
            struct can_reference : _requires<_can_reference_concept, T> {
            };

            struct _dereferenceable_concept {
                template <typename T>
                auto _test_requires(T& t) -> decltype(
                    requires_expr<can_reference<decltype(*t)>::value>{});
            };
            template <typename T>
            struct dereferenceable : _requires<_dereferenceable_concept, T> {
            };
            template <>
            struct dereferenceable<void*> : std::false_type {
            };

            namespace _iter_move {
                void iter_move();
                struct fn {
                private:
                    template <typename T>
                    static SCN_CONSTEXPR14 auto impl(
                        T&& t,
                        priority_tag<2>) noexcept(noexcept(iter_move(t)))
                        -> decltype(iter_move(t))
                    {
                        return iter_move(t);
                    }

                    template <typename T>
                    static SCN_CONSTEXPR14 auto
                    impl(T&& t, priority_tag<1>) noexcept(
                        noexcept(std::move(*std::declval<T&&>()))) ->
                        typename std::enable_if<
                            std::is_lvalue_reference<
                                decltype(*std::forward<T>(t))>::value,
                            decltype(std::move(*std::forward<T>(t)))>::type
                    {
                        return std::move(*std::forward<T>(t));
                    }

                    template <typename T>
                    static SCN_CONSTEXPR14 auto impl(
                        T&& t,
                        priority_tag<0>) noexcept(noexcept(*std::forward<T>(t)))
                        -> decltype(*std::forward<T>(t))
                    {
                        return *std::forward<T>(t);
                    }

                public:
                    template <typename T>
                    SCN_CONSTEXPR14 auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            };  // namespace _iter_move
            namespace {
                SCN_CONSTEXPR auto& iter_move =
                    static_const<_iter_move::fn>::value;
            }

            using std::bidirectional_iterator_tag;
            using std::forward_iterator_tag;
            using std::input_iterator_tag;
            using std::output_iterator_tag;
            using std::random_access_iterator_tag;
            struct contiguous_iterator_tag : random_access_iterator_tag {
            };

            template <typename T>
            struct iterator_category;

            template <typename T, typename = void>
            struct _iterator_category {
            };
            template <typename T>
            struct _iterator_category<T*>
                : std::enable_if<std::is_object<T>::value,
                                 contiguous_iterator_tag> {
            };
            template <typename T>
            struct _iterator_category<const T> : iterator_category<T> {
            };
            template <typename T>
            struct _iterator_category<T,
                                      void_t<typename T::iterator_category>> {
                using type = typename T::iterator_category;
            };

            template <typename T>
            struct iterator_category : _iterator_category<T> {
            };
            template <typename T>
            using iterator_category_t = typename iterator_category<T>::type;

            template <typename T>
            using iter_reference_t =
                typename std::enable_if<dereferenceable<T>::value,
                                        decltype(*std::declval<T&>())>::type;

            struct _iter_rvalue_reference_concept {
                template <typename T>
                auto _test_requires(T& t) -> decltype(
                    ::scn::detail::ranges::iter_move(t),
                    requires_expr<can_reference<decltype(
                        ::scn::detail::ranges::iter_move(t))>::value>{});
            };
            template <typename T>
            using iter_rvalue_reference_t = typename std::enable_if<
                _requires<_iter_rvalue_reference_concept, T>::value,
                decltype(::scn::detail::ranges::iter_move(
                    std::declval<T&>()))>::type;

            template <typename>
            struct incrementable_traits;

            struct _empty {
            };

            template <typename T>
            struct _with_difference_type {
                using difference_type = T;
            };
            template <typename, typename = void>
            struct _incrementable_traits_helper {
            };

            template <>
            struct _incrementable_traits_helper<void*> {
            };
            template <typename T>
            struct _incrementable_traits_helper<T*>
                : std::conditional<std::is_object<T>::value,
                                   _with_difference_type<std::ptrdiff_t>,
                                   _empty>::type {
            };
            template <typename I>
            struct _incrementable_traits_helper<const I>
                : incrementable_traits<typename std::decay<I>::type> {
            };

            template <typename, typename = void>
            struct _has_member_difference_type : std::false_type {
            };
            template <typename T>
            struct _has_member_difference_type<
                T,
                void_t<typename T::difference_type>> : std::true_type {
            };

            template <typename T>
            struct _incrementable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_difference_type<T>::value>::type> {
                using difference_type = typename T::difference_type;
            };
            template <typename T>
            struct _incrementable_traits_helper<
                T,
                typename std::enable_if<
                    !std::is_pointer<T>::value &&
                    !_has_member_difference_type<T>::value &&
                    std::is_integral<decltype(
                        std::declval<const T&>() -
                        std::declval<const T&>())>::value>::type>
                : _with_difference_type<typename std::make_signed<decltype(
                      std::declval<T>() - std::declval<T>())>::type> {
            };
            template <typename T>
            struct incrementable_traits : _incrementable_traits_helper<T> {
            };

            template <typename T>
            using iter_difference_t =
                typename incrementable_traits<T>::difference_type;

            template <typename>
            struct readable_traits;

            template <typename T>
            struct _with_value_type {
                using value_type = T;
            };
            template <typename, typename = void>
            struct _readable_traits_helper {
            };

            template <typename T>
            struct _readable_traits_helper<T*>
                : std::conditional<
                      std::is_object<T>::value,
                      _with_value_type<typename std::remove_cv<T>::type>,
                      _empty>::type {
            };

            template <typename I>
            struct _readable_traits_helper<
                I,
                typename std::enable_if<std::is_array<I>::value>::type>
                : readable_traits<typename std::decay<I>::type> {
            };

            template <typename I>
            struct _readable_traits_helper<
                const I,
                typename std::enable_if<!std::is_array<I>::value>::type>
                : readable_traits<typename std::decay<I>::type> {
            };

            template <typename T, typename V = typename T::value_type>
            struct _member_value_type
                : std::conditional<std::is_object<V>::value,
                                   _with_value_type<V>,
                                   _empty>::type {
            };

            template <typename T, typename E = typename T::element_type>
            struct _member_element_type
                : std::conditional<
                      std::is_object<E>::value,
                      _with_value_type<typename std::remove_cv<E>::type>,
                      _empty>::type {
            };

            template <typename T>
            using _member_value_type_t = typename T::value_type;

            template <typename T>
            struct _has_member_value_type : exists<_member_value_type_t, T> {
            };

            template <typename T>
            using _member_element_type_t = typename T::element_type;

            template <typename T>
            struct _has_member_element_type
                : exists<_member_element_type_t, T> {
            };

            template <typename T>
            struct _readable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_value_type<T>::value &&
                    !_has_member_element_type<T>::value>::type>
                : _member_value_type<T> {
            };

            template <typename T>
            struct _readable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_element_type<T>::value &&
                    !_has_member_value_type<T>::value>::type>
                : _member_element_type<T> {
            };

            template <typename T>
            struct _readable_traits_helper<
                T,
                typename std::enable_if<
                    _has_member_element_type<T>::value &&
                    _has_member_value_type<T>::value>::type> {
            };

            template <typename T>
            struct readable_traits : _readable_traits_helper<T> {
            };

            template <typename T>
            using iter_value_t = typename readable_traits<T>::value_type;

            struct _derived_from_concept {
                template <typename, typename>
                static auto test(long) -> std::false_type;
                template <typename Derived, typename Base>
                static auto test(int) -> typename std::enable_if<
                    std::is_base_of<Base, Derived>::value &&
                        std::is_convertible<const volatile Derived*,
                                            const volatile Base*>::value,
                    std::true_type>::type;
            };
            template <typename Derived, typename Base>
            struct derived_from
                : decltype(_derived_from_concept::test<Derived, Base>(0)) {
            };

            struct _convertible_to_concept {
                template <typename From, typename To>
                auto _test_requires(From (&f)())
                    -> decltype(static_cast<To>(f()));
            };

            template <typename From, typename To>
            struct convertible_to
                : std::integral_constant<
                      bool,
                      std::is_convertible<From, To>::value &&
                          _requires<_convertible_to_concept, From, To>::value> {
            };

            struct _common_reference_with_concept {
                template <typename T, typename U>
                static auto test(long) -> std::false_type;
                template <typename T, typename U>
                static auto test(int) -> typename std::enable_if<
                    std::is_same<get_common_reference_t<T, U>,
                                 get_common_reference_t<U, T>>::value &&
                        convertible_to<T,
                                       get_common_reference_t<T, U>>::value &&
                        convertible_to<U, get_common_reference_t<T, U>>::value,
                    std::true_type>::type;
            };

            template <typename T, typename U>
            struct common_reference_with
                : decltype(_common_reference_with_concept::test<T, U>(0)) {
            };

            template <typename T>
            struct destructible : std::is_nothrow_destructible<T> {
            };
            template <typename T, typename... Args>
            struct constructible_from
                : std::integral_constant<
                      bool,
                      destructible<T>::value &&
                          std::is_constructible<T, Args...>::value> {
            };
            template <typename T>
            struct default_constructible : constructible_from<T> {
            };
            template <typename T>
            struct move_constructible
                : std::integral_constant<bool,
                                         constructible_from<T, T>::value &&
                                             convertible_to<T, T>::value> {
            };

            struct _copy_constructible_concept {
                template <typename T>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    move_constructible<T>::value &&
                        constructible_from<T, T&>::value &&
                        convertible_to<T&, T>::value &&
                        constructible_from<T, const T&>::value &&
                        convertible_to<const T&, T>::value &&
                        constructible_from<T, const T>::value &&
                        convertible_to<const T, T>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct copy_constructible
                : decltype(_copy_constructible_concept::test<T>(0)) {
            };

            struct _assignable_from_concept {
                template <typename L, typename R>
                auto _test_requires(L l, R&& r) -> decltype(
                    requires_expr<std::is_same<decltype(l = std::forward<R>(r)),
                                               L>::value>{});

                template <typename L, typename R>
                static auto test(long) -> std::false_type;
                template <typename L, typename R>
                static auto test(int) -> typename std::enable_if<
                    std::is_lvalue_reference<L>::value &&
                        common_reference_with<
                            const typename std::remove_reference<L>::type&,
                            const typename std::remove_reference<R>::type&>::
                            value &&
                        _requires<_assignable_from_concept, L, R>::value,
                    std::true_type>::type;
            };
            template <typename L, typename R>
            struct assignable_from
                : decltype(_assignable_from_concept::test<L, R>(0)) {
            };

            namespace _swap {
                template <typename T>
                void swap(T&, T&) = delete;
                template <typename T, std::size_t N>
                void swap(T (&)[N], T (&)[N]) = delete;

                struct fn {
                private:
                    template <typename T, typename U>
                    static SCN_CONSTEXPR14 auto
                    impl(T&& t, U&& u, priority_tag<2>) noexcept(
                        noexcept(swap(std::forward<T>(t), std::forward<U>(u))))
                        -> decltype(static_cast<void>(swap(std::forward<T>(t),
                                                           std::forward<U>(u))))
                    {
                        (void)swap(std::forward<T>(t), std::forward<U>(u));
                    }

                    template <typename T,
                              typename U,
                              std::size_t N,
                              typename F = fn>
                    static SCN_CONSTEXPR14 auto
                        impl(T (&t)[N], U (&u)[N], priority_tag<1>) noexcept(
                            noexcept(std::declval<F&>()(*t, *u)))
                            -> decltype(std::declval<F&>()(*t, *u))
                    {
                        for (std::size_t i = 0; i < N; ++i) {
                            fn::impl(t[i], u[i], priority_tag<2>{});
                        }
                    }

                    template <typename T>
                    static SCN_CONSTEXPR14 auto
                    impl(T& a, T& b, priority_tag<0>) noexcept(
                        std::is_nothrow_move_constructible<T>::value&&
                            std::is_nothrow_assignable<T&, T>::value) ->
                        typename std::enable_if<
                            move_constructible<T>::value &&
                            assignable_from<T&, T>::value>::type
                    {
                        T temp = std::move(a);
                        a = std::move(b);
                        b = std::move(temp);
                    }

                public:
                    template <typename T, typename U>
                    SCN_CONSTEXPR14 auto operator()(T&& t, U&& u) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   std::forward<U>(u),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 std::forward<U>(u),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), std::forward<U>(u),
                                        priority_tag<2>{});
                    }
                };
            }  // namespace _swap
            namespace {
                SCN_CONSTEXPR auto& swap = static_const<_swap::fn>::value;
            }

            struct _swappable_concept {
                template <typename T>
                auto _test_requires(T& a, T& b)
                    -> decltype(::scn::detail::ranges::swap(a, b));
            };
            template <typename T>
            struct swappable : _requires<_swappable_concept, T> {
            };

            struct _movable_concept {
                template <typename T>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    std::is_object<T>::value && move_constructible<T>::value &&
                        assignable_from<T&, T>::value && swappable<T>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct movable : decltype(_movable_concept::test<T>(0)) {
            };

            struct _copyable_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    copy_constructible<T>::value && movable<T>::value &&
                        assignable_from<T&, const T&>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct copyable : decltype(_copyable_concept::test<T>(0)) {
            };

            template <typename T>
            struct semiregular
                : std::integral_constant<bool,
                                         copyable<T>::value &&
                                             default_constructible<T>::value> {
            };

            struct _weakly_equality_comparable_with_concept {
                template <typename T, typename U>
                auto _test_requires(
                    const typename std::remove_reference<T>::type& t,
                    const typename std::remove_reference<U>& u)
                    -> decltype(valid_expr(
                        t == u,
                        requires_expr<
                            convertible_to<decltype(t == u), bool>::value>{},
                        t != u,
                        requires_expr<
                            convertible_to<decltype(t != u), bool>::value>{},
                        u == t,
                        requires_expr<
                            convertible_to<decltype(u == t), bool>::value>{},
                        u != t,
                        requires_expr<
                            convertible_to<decltype(u != t), bool>::value>{}));
            };
            template <typename T, typename U>
            struct weakly_equality_comparable_with
                : _requires<_weakly_equality_comparable_with_concept, T, U> {
            };

            template <typename T>
            struct equality_comparable : weakly_equality_comparable_with<T, T> {
            };

            template <typename T>
            struct regular
                : std::integral_constant<bool,
                                         semiregular<T>::value &&
                                             equality_comparable<T>::value> {
            };

            struct _equality_comparable_with_concept {
                template <typename, typename>
                static auto test(long) -> std::false_type;
                template <typename T, typename U>
                static auto test(int) -> typename std::enable_if<
                    equality_comparable<T>::value &&
                        equality_comparable<U>::value &&
                        common_reference_with<
                            const typename std::remove_reference<T>::type&,
                            const typename std::remove_reference<U>::type>::
                            value &&
                        equality_comparable<get_common_reference_t<
                            const typename std::remove_reference<T>::type&,
                            const typename std::remove_reference<T>::type&>>::
                            value &&
                        weakly_equality_comparable_with<T, U>::value,
                    std::true_type>::type;
            };
            template <typename T, typename U>
            struct equality_comparable_with
                : decltype(_equality_comparable_with_concept::test<T, U>(0)) {
            };

            struct _totally_ordered_concept {
                template <typename T>
                auto _test_requires(
                    const typename std::remove_reference<T>::type& a,
                    const typename std::remove_reference<T>::type& b)
                    -> decltype(
                        requires_expr<
                            convertible_to<decltype(a < b), bool>::value>{},
                        requires_expr<
                            convertible_to<decltype(a > b), bool>::value>{},
                        requires_expr<
                            convertible_to<decltype(a <= b), bool>::value>{},
                        requires_expr<
                            convertible_to<decltype(a >= b), bool>::value>{});
            };
            template <typename T>
            struct totally_ordered
                : std::integral_constant<
                      bool,
                      equality_comparable<T>::value &&
                          _requires<_totally_ordered_concept, T>::value> {
            };

            struct _readable_concept {
                template <typename In>
                auto _test_requires()
                    -> decltype(std::declval<iter_value_t<In>>(),
                                std::declval<iter_reference_t<In>>(),
                                std::declval<iter_rvalue_reference_t<In>>());

                template <typename>
                static auto test(long) -> std::false_type;
                template <typename In>
                static auto test(int) -> typename std::enable_if<
                    _requires<_readable_concept, In>::value &&
                        common_reference_with<iter_reference_t<In>&&,
                                              iter_value_t<In>&>::value &&
                        common_reference_with<
                            iter_reference_t<In>&&,
                            iter_rvalue_reference_t<In>&&>::value &&
                        common_reference_with<iter_rvalue_reference_t<In>&&,
                                              const iter_value_t<In>&>::value,
                    std::true_type>::type;
            };

            template <typename In>
            struct readable : decltype(_readable_concept::test<In>(0)) {
            };

            template <typename T, typename Deduced>
            auto same_lv(Deduced&) ->
                typename std::enable_if<std::is_same<T, Deduced>::value,
                                        int>::type;

            struct _weakly_incrementable_concept {
                template <typename I>
                auto _test_requires(I i) -> decltype(
                    std::declval<iter_difference_t<I>>(),
                    requires_expr<
                        std::is_integral<iter_difference_t<I>>::value &&
                        std::is_signed<iter_difference_t<I>>::value>{},
                    requires_expr<std::is_same<decltype(++i), I&>::value>{},
                    i++);
            };
            template <typename I>
            struct weakly_incrementable
                : std::integral_constant<
                      bool,
                      default_constructible<I>::value && movable<I>::value &&
                          _requires<_weakly_incrementable_concept, I>::value> {
            };

            struct _incrementable_concept {
                template <typename I>
                auto _test_requires(I i) -> decltype(
                    requires_expr<std::is_same<decltype(i++), I>::value>{});
            };
            template <typename I>
            struct incrementable
                : std::integral_constant<
                      bool,
                      regular<I>::value && weakly_incrementable<I>::value &&
                          _requires<_incrementable_concept, I>::value> {
            };

            struct _input_or_output_iterator_concept {
                template <typename I>
                auto _test_requires(I i) -> decltype(
                    requires_expr<can_reference<decltype(*i)>::value>{});
            };
            template <typename I>
            struct input_or_output_iterator
                : std::integral_constant<
                      bool,
                      _requires<_input_or_output_iterator_concept, I>::value &&
                          weakly_incrementable<I>::value> {
            };

            template <typename S, typename I>
            struct sentinel_for
                : std::integral_constant<
                      bool,
                      semiregular<S>::value &&
                          input_or_output_iterator<I>::value &&
                          weakly_equality_comparable_with<S, I>::value> {
            };

            struct _sized_sentinel_for_concept {
                template <typename S, typename I>
                auto _test_requires(const S& s, const I& i) -> decltype(
                    requires_expr<std::is_same<decltype(s - i),
                                               iter_difference_t<I>>::value>{},
                    requires_expr<std::is_same<decltype(i - s),
                                               iter_difference_t<I>>::value>{});
            };
            template <typename S, typename I>
            struct sized_sentinel_for
                : std::integral_constant<
                      bool,
                      sentinel_for<S, I>::value &&
                          _requires<_sized_sentinel_for_concept, S, I>::value> {
            };
            template <typename S>
            struct sized_sentinel_for<S, void*> : std::false_type {
            };
            template <typename I>
            struct sized_sentinel_for<void*, I> : std::false_type {
            };
            template <>
            struct sized_sentinel_for<void*, void*> : std::false_type {
            };

            struct _input_iterator_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename I>
                static auto test(int) -> typename std::enable_if<
                    input_or_output_iterator<I>::value && readable<I>::value &&
                        exists<iterator_category_t, I>::value &&
                        derived_from<iterator_category_t<I>,
                                     input_iterator_tag>::value,
                    std::true_type>::type;
            };
            template <typename I>
            struct input_iterator
                : decltype(_input_iterator_concept::test<I>(0)) {
            };

            struct _forward_iterator_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename I>
                static auto test(int) -> typename std::enable_if<
                    input_iterator<I>::value &&
                        derived_from<iterator_category_t<I>,
                                     forward_iterator_tag>::value &&
                        incrementable<I>::value && sentinel_for<I, I>::value,
                    std::true_type>::type;
            };
            template <typename I>
            struct forward_iterator
                : decltype(_forward_iterator_concept::test<I>(0)) {
            };

            struct _bidirectional_iterator_concept {
                template <typename I>
                auto _test_requires(I i) -> decltype(
                    same_lv<I>(--i),
                    requires_expr<std::is_same<decltype(i--), I>::value>{});
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename I>
                static auto test(int) -> typename std::enable_if<
                    forward_iterator<I>::value &&
                        derived_from<iterator_category_t<I>,
                                     bidirectional_iterator_tag>::value &&
                        _requires<_bidirectional_iterator_concept, I>::value,
                    std::true_type>::type;
            };
            template <typename I>
            struct bidirectional_iterator
                : decltype(_bidirectional_iterator_concept::test<I>(0)) {
            };

            struct _random_access_iterator_concept {
                template <typename I>
                auto _test_requires(I i,
                                    const I j,
                                    const iter_difference_t<I> n)
                    -> decltype(valid_expr(
                        same_lv<I>(i += n),
                        j + n,
                        requires_expr<
                            std::is_same<decltype(j + n), I>::value>{},
                        n + j,
#ifndef _MSC_VER
                        requires_expr<
                            std::is_same<decltype(n + j), I>::value>{},
#endif
                        same_lv<I>(i -= n),
                        j - n,
                        requires_expr<
                            std::is_same<decltype(j - n), I>::value>{},
                        j[n],
                        requires_expr<
                            std::is_same<decltype(j[n]),
                                         iter_reference_t<I>>::value>{}));
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename I>
                static auto test(int) -> typename std::enable_if<
                    bidirectional_iterator<I>::value &&
                        derived_from<iterator_category_t<I>,
                                     random_access_iterator_tag>::value &&
                        totally_ordered<I>::value &&
                        sized_sentinel_for<I, I>::value &&
                        _requires<_random_access_iterator_concept, I>::value,
                    std::true_type>::type;
            };
            template <typename I>
            struct random_access_iterator
                : decltype(_random_access_iterator_concept::test<I>(0)) {
            };

            struct _contiguous_iterator_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename I>
                static auto test(int) -> typename std::enable_if<
                    random_access_iterator<I>::value &&
                        derived_from<iterator_category_t<I>,
                                     contiguous_iterator_tag>::value &&
                        std::is_lvalue_reference<iter_reference_t<I>>::value &&
                        std::is_same<
                            iter_value_t<I>,
                            remove_cvref_t<iter_reference_t<I>>>::value,
                    std::true_type>::type;
            };

            template <typename I>
            struct contiguous_iterator
                : decltype(_contiguous_iterator_concept::test<I>(0)) {
            };

            namespace _begin {
                template <typename T>
                void begin(T&&) = delete;
                template <typename T>
                void begin(std::initializer_list<T>&&) = delete;

                struct fn {
                private:
                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR void impl(T(&&)[N],
                                                   priority_tag<2>) = delete;

                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR auto impl(T (&t)[N],
                                                   priority_tag<2>) noexcept
                        -> decltype((t) + 0)
                    {
                        return (t) + 0;
                    }

                    template <typename C>
                    static SCN_CONSTEXPR auto impl(basic_string_view<C> sv,
                                                   priority_tag<2>) noexcept
                        -> decltype(sv.begin())
                    {
                        return sv.begin();
                    }

                    template <typename T>
                    static SCN_CONSTEXPR auto
                    impl(T& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(t.begin()))) ->
                        typename std::enable_if<
                            input_or_output_iterator<
                                decltype(decay_copy(t.begin()))>::value,
                            decltype(decay_copy(t.begin()))>::type
                    {
                        return decay_copy(t.begin());
                    }

                    template <typename T>
                    static SCN_CONSTEXPR auto
                    impl(T& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(begin(std::forward<T>(t))))) ->
                        typename std::enable_if<
                            input_or_output_iterator<decltype(
                                decay_copy(begin(std::forward<T>(t))))>::value,
                            decltype(
                                decay_copy(begin(std::forward<T>(t))))>::type
                    {
                        return decay_copy(begin(std::forward<T>(t)));
                    }

                public:
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            }  // namespace _begin
            namespace {
                SCN_CONSTEXPR auto& begin = static_const<_begin::fn>::value;
            }

            namespace _end {
                template <typename T>
                void end(T&&) = delete;
                template <typename T>
                void end(std::initializer_list<T>&&) = delete;

                struct fn {
                private:
                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR void impl(T(&&)[N],
                                                   priority_tag<2>) = delete;

                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR auto impl(T (&t)[N],
                                                   priority_tag<2>) noexcept
                        -> decltype((t) + N)
                    {
                        return (t) + N;
                    }

                    template <typename C>
                    static SCN_CONSTEXPR auto impl(basic_string_view<C> sv,
                                                   priority_tag<2>) noexcept
                        -> decltype(sv.end())
                    {
                        return sv.end();
                    }

                    template <
                        typename T,
                        typename S =
                            decltype(decay_copy(std::declval<T&>().end())),
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T&>()))>
                    static SCN_CONSTEXPR auto impl(
                        T& t,
                        priority_tag<1>) noexcept(noexcept(decay_copy(t.end())))
                        -> typename std::enable_if<
                            sentinel_for<S, I>::value,
                            decltype(decay_copy(t.end()))>::type
                    {
                        return decay_copy(t.end());
                    }

                    template <
                        typename T,
                        typename S =
                            decltype(decay_copy(end(std::declval<T>()))),
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T>()))>
                    static SCN_CONSTEXPR auto
                    impl(T& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(end(std::forward<T>(t))))) ->
                        typename std::enable_if<sentinel_for<S, I>::value,
                                                S>::type
                    {
                        return decay_copy(end(std::forward<T>(t)));
                    }

                public:
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            }  // namespace _end
            namespace {
                SCN_CONSTEXPR auto& end = static_const<_end::fn>::value;
            }

            namespace _cbegin {
                struct fn {
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T& t) const
                        noexcept(noexcept(::scn::detail::ranges::begin(t)))
                            -> decltype(::scn::detail::ranges::begin(t))
                    {
                        return ::scn::detail::ranges::begin(t);
                    }

                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T&& t) const
                        noexcept(noexcept(::scn::detail::ranges::begin(
                            static_cast<const T&&>(t))))
                            -> decltype(::scn::detail::ranges::begin(
                                static_cast<const T&&>(t)))
                    {
                        return ::scn::detail::ranges::begin(
                            static_cast<const T&&>(t));
                    }
                };
            }  // namespace _cbegin
            namespace {
                SCN_CONSTEXPR auto& cbegin = static_const<_cbegin::fn>::value;
            }

            namespace _cend {
                struct fn {
                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T& t) const
                        noexcept(noexcept(::scn::detail::ranges::end(t)))
                            -> decltype(::scn::detail::ranges::end(t))
                    {
                        return ::scn::detail::ranges::end(t);
                    }

                    template <typename T>
                    SCN_CONSTEXPR auto operator()(const T&& t) const
                        noexcept(noexcept(::scn::detail::ranges::end(
                            static_cast<const T&&>(t))))
                            -> decltype(::scn::detail::ranges::end(
                                static_cast<const T&&>(t)))
                    {
                        return ::scn::detail::ranges::end(
                            static_cast<const T&&>(t));
                    }
                };
            }  // namespace _cend
            namespace {
                SCN_CONSTEXPR auto& cend = static_const<_cend::fn>::value;
            }

            struct _range_impl_concept {
                template <typename T>
                auto _test_requires(T&& t) -> decltype(
                    ::scn::detail::ranges::begin(std::forward<T>(t)),
                    ::scn::detail::ranges::end(std::forward<T>(t)));
            };
            template <typename T>
            struct _range_impl : _requires<_range_impl_concept, T> {
            };
            struct _range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) ->
                    typename std::enable_if<_range_impl<T&>::value,
                                            std::true_type>::type;
            };
            template <typename T>
            struct range : decltype(_range_concept::test<T>(0)) {
            };

            template <typename T>
            struct forwarding_range
                : std::integral_constant<bool,
                                         range<T>::value &&
                                             _range_impl<T>::value> {
            };

            template <typename R>
            using iterator_t =
                typename std::enable_if<range<R>::value,
                                        decltype(::scn::detail::ranges::begin(
                                            std::declval<R&>()))>::type;
            template <typename R>
            using sentinel_t =
                typename std::enable_if<range<R>::value,
                                        decltype(::scn::detail::ranges::end(
                                            std::declval<R&>()))>::type;
            template <typename R>
            using range_difference_t =
                typename std::enable_if<range<R>::value,
                                        iter_difference_t<iterator_t<R>>>::type;
            template <typename R>
            using range_value_t =
                typename std::enable_if<range<R>::value,
                                        iter_value_t<iterator_t<R>>>::type;
            template <typename R>
            using range_reference_t =
                typename std::enable_if<range<R>::value,
                                        iter_reference_t<iterator_t<R>>>::type;
            template <typename R>
            using range_rvalue_reference_t = typename std::enable_if<
                range<R>::value,
                iter_rvalue_reference_t<iterator_t<R>>>::type;

            template <typename>
            struct disable_sized_range : std::false_type {
            };

            namespace _size {
                template <typename T>
                void size(T&&) = delete;
                template <typename T>
                void size(T&) = delete;

                struct fn {
                private:
                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR std::size_t impl(
                        const T(&&)[N],
                        priority_tag<3>) noexcept
                    {
                        return N;
                    }

                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR std::size_t impl(
                        const T (&)[N],
                        priority_tag<3>) noexcept
                    {
                        return N;
                    }

                    template <typename T,
                              typename I = decltype(
                                  decay_copy(std::declval<T>().size()))>
                    static constexpr auto impl(T&& t, priority_tag<2>) noexcept(
                        noexcept(decay_copy(std::forward<T>(t).size()))) ->
                        typename std::enable_if<
                            std::is_integral<I>::value &&
                                !disable_sized_range<remove_cvref_t<T>>::value,
                            I>::type
                    {
                        return decay_copy(std::forward<T>(t).size());
                    }

                    template <typename T,
                              typename I =
                                  decltype(decay_copy(size(std::declval<T>())))>
                    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(size(std::forward<T>(t))))) ->
                        typename std::enable_if<
                            std::is_integral<I>::value &&
                                !disable_sized_range<remove_cvref_t<T>>::value,
                            I>::type
                    {
                        return decay_copy(size(std::forward<T>(t)));
                    }

                    template <
                        typename T,
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T>())),
                        typename S = decltype(
                            ::scn::detail::ranges::end(std::declval<T>())),
                        typename D = decltype(decay_copy(std::declval<S>() -
                                                         std::declval<I>()))>
                    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(::scn::detail::ranges::end(t) -
                                            ::scn::detail::ranges::begin(t))))
                        -> typename std::enable_if<
                            !std::is_array<remove_cvref_t<T>>::value &&
                                sized_sentinel_for<S, I>::value &&
                                forward_iterator<I>::value,
                            D>::type
                    {
                        return decay_copy(::scn::detail::ranges::end(t) -
                                          ::scn::detail::ranges::begin(t));
                    }

                public:
                    template <typename T>
                    constexpr auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<3>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<3>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<3>{});
                    }
                };
            }  // namespace _size
            namespace {
                SCN_CONSTEXPR auto& size = static_const<_size::fn>::value;
            }

            namespace _empty_ns {
                struct fn {
                private:
                    template <typename T>
                    static SCN_CONSTEXPR auto
                    impl(T&& t, priority_tag<2>) noexcept(
                        noexcept((bool(std::forward<T>(t).empty()))))
                        -> decltype((bool(std::forward<T>(t).empty())))
                    {
                        return bool((std::forward<T>(t).empty()));
                    }
                    template <typename T>
                    static constexpr auto impl(T&& t, priority_tag<1>) noexcept(
                        noexcept(::scn::detail::ranges::size(
                                     std::forward<T>(t)) == 0))
                        -> decltype(::scn::detail::ranges::size(
                                        std::forward<T>(t)) == 0)
                    {
                        return ::scn::detail::ranges::size(
                                   std::forward<T>(t)) == 0;
                    }

                    template <
                        typename T,
                        typename I = decltype(
                            ::scn::detail::ranges::begin(std::declval<T>()))>
                    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
                        noexcept(::scn::detail::ranges::begin(t) ==
                                 ::scn::detail::ranges::end(t))) ->
                        typename std::enable_if<
                            forward_iterator<I>::value,
                            decltype(::scn::detail::ranges::begin(t) ==
                                     ::scn::detail::ranges::end(t))>::type
                    {
                        return ::scn::detail::ranges::begin(t) ==
                               ::scn::detail::ranges::end(t);
                    }

                public:
                    template <typename T>
                    constexpr auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            }  // namespace _empty_ns
            namespace {
                SCN_CONSTEXPR auto& empty = static_const<_empty_ns::fn>::value;
            }

            template <typename P>
            struct _is_object_pointer
                : std::integral_constant<
                      bool,
                      std::is_pointer<P>::value &&
                          std::is_object<_test_t<iter_value_t, P>>::value> {
            };

            namespace _data {
                struct fn {
                private:
                    template <typename T,
                              typename D = decltype(
                                  decay_copy(std::declval<T&>().data()))>
                    static constexpr auto impl(T& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(t.data()))) ->
                        typename std::enable_if<_is_object_pointer<D>::value,
                                                D>::type
                    {
                        return t.data();
                    }

                    template <typename T>
                    static constexpr auto impl(T&& t, priority_tag<0>) noexcept(
                        noexcept(
                            ::scn::detail::ranges::begin(std::forward<T>(t))))
                        -> typename std::enable_if<
                            _is_object_pointer<
                                decltype(::scn::detail::ranges::begin(
                                    std::forward<T>(t)))>::value,
                            decltype(::scn::detail::ranges::begin(
                                std::forward<T>(t)))>::type
                    {
                        return ::scn::detail::ranges::begin(std::forward<T>(t));
                    }

                public:
                    template <typename T>
                    constexpr auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<1>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<1>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<1>{});
                    }
                };
            }  // namespace _data
            namespace {
                SCN_CONSTEXPR auto& data = static_const<_data::fn>::value;
            }

            struct _sized_range_concept {
                template <typename T>
                auto _test_requires(T& t)
                    -> decltype(::scn::detail::ranges::size(t));
            };
            template <typename T>
            struct sized_range
                : std::integral_constant<
                      bool,
                      range<T>::value &&
                          !disable_sized_range<
                              detail::remove_cvref_t<T>>::value &&
                          _requires<_sized_range_concept, T>::value> {
            };

            struct view_base {
            };
            template <typename>
            struct _is_std_non_view : std::false_type {
            };
            template <typename T>
            struct _is_std_non_view<std::initializer_list<T>> : std::true_type {
            };
            template <typename T>
            struct _enable_view_helper
                : std::conditional<
                      derived_from<T, view_base>::value,
                      std::true_type,
                      typename std::conditional<
                          _is_std_non_view<T>::value,
                          std::false_type,
                          typename std::conditional<
                              range<T>::value && range<const T>::value,
                              std::is_same<range_reference_t<T>,
                                           range_reference_t<const T>>,
                              std::true_type>::type>::type>::type {
            };
            template <typename T>
            struct view
                : std::integral_constant<bool,
                                         range<T>::value &&
                                             semiregular<T>::value &&
                                             _enable_view_helper<T>::value> {
            };

            struct _input_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    range<T>::value && input_iterator<iterator_t<T>>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct input_range : decltype(_input_range_concept::test<T>(0)) {
            };

            struct _forward_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    input_range<T>::value &&
                        forward_iterator<iterator_t<T>>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct forward_range
                : decltype(_forward_range_concept::test<T>(0)) {
            };

            struct _bidirectional_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    forward_range<T>::value &&
                        bidirectional_iterator<iterator_t<T>>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct bidirectional_range
                : decltype(_bidirectional_range_concept::test<T>(0)) {
            };

            struct _random_access_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    bidirectional_range<T>::value &&
                        random_access_iterator<iterator_t<T>>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct random_access_range
                : decltype(_random_access_range_concept::test<T>(0)) {
            };

            struct _contiguous_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    random_access_range<T>::value &&
                        _requires<_contiguous_range_concept, T>::value,
                    std::true_type>::type;

                template <typename T>
                auto _test_requires(T& t)
                    -> decltype(requires_expr<std::is_same<
                                    decltype(::scn::detail::ranges::data(t)),
                                    typename std::add_pointer<
                                        range_reference_t<T>>::type>::value>{});
            };
            template <typename T>
            struct contiguous_range
                : decltype(_contiguous_range_concept::test<T>(0)) {
            };

            struct _common_range_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    range<T>::value &&
                        std::is_same<iterator_t<T>, sentinel_t<T>>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct common_range : decltype(_common_range_concept::test<T>(0)) {
            };
            template <typename T>
            struct viewable_range
                : std::integral_constant<
                      bool,
                      range<T>::value &&
                          (forward_range<T>::value ||
                           view<typename std::decay<T>::type>::value)> {
            };

            struct dangling {
                SCN_CONSTEXPR dangling() noexcept = default;
                template <typename... Args>
                SCN_CONSTEXPR14 dangling(Args&&...) noexcept
                {
                }
            };
            template <typename R>
            using safe_iterator_t =
                typename std::conditional<forwarding_range<R>::value,
                                          iterator_t<R>,
                                          dangling>::type;

            template <typename R>
            struct _simple_view
                : std::integral_constant<
                      bool,
                      view<R>::value && range<const R>::value &&
                          std::is_same<iterator_t<R>,
                                       iterator_t<const R>>::value &&
                          std::is_same<sentinel_t<R>,
                                       sentinel_t<const R>>::value> {
            };

            struct _has_arrow_concept {
                template <typename I>
                auto _test_requires(I i) -> decltype(i.operator->());
            };
            template <typename I>
            struct _has_arrow
                : std::integral_constant<
                      bool,
                      input_iterator<I>::value &&
                          (std::is_pointer<I>::value ||
                           _requires<_has_arrow_concept, I>::value)> {
            };

            template <typename T, typename U>
            struct _not_same_as : std::integral_constant<
                                      bool,
                                      !std::is_same<remove_cvref_t<T>,
                                                    remove_cvref_t<U>>::value> {
            };

            namespace _advance {
                struct fn {
                private:
                    template <typename T>
                    static SCN_CONSTEXPR T abs(T t)
                    {
                        return t < T{0} ? -t : t;
                    }

                    template <typename R,
                              typename std::enable_if<random_access_iterator<
                                  R>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(R& r,
                                                     iter_difference_t<R> n)
                    {
                        r += n;
                    }

                    template <
                        typename I,
                        typename std::enable_if<
                            bidirectional_iterator<I>::value &&
                            !random_access_iterator<I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     iter_difference_t<I> n)
                    {
                        constexpr auto zero = iter_difference_t<I>{0};

                        if (n > zero) {
                            while (n-- > zero) {
                                ++i;
                            }
                        }
                        else {
                            while (n++ < zero) {
                                --i;
                            }
                        }
                    }

                    template <typename I,
                              typename std::enable_if<!bidirectional_iterator<
                                  I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     iter_difference_t<I> n)
                    {
                        while (n-- > iter_difference_t<I>{0}) {
                            ++i;
                        }
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            assignable_from<I&, S>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     S bound,
                                                     priority_tag<2>)
                    {
                        i = std::move(bound);
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     S bound,
                                                     priority_tag<1>)
                    {
                        fn::impl(i, bound - i);
                    }

                    template <typename I, typename S>
                    static SCN_CONSTEXPR14 void impl(I& i,
                                                     S bound,
                                                     priority_tag<0>)
                    {
                        while (i != bound) {
                            ++i;
                        }
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 auto impl(I& i,
                                                     iter_difference_t<I> n,
                                                     S bound)
                        -> iter_difference_t<I>
                    {
                        if (fn::abs(n) >= fn::abs(bound - i)) {
                            auto dist = bound - i;
                            fn::impl(i, bound, priority_tag<2>{});
                            return dist;
                        }
                        else {
                            fn::impl(i, n);
                            return n;
                        }
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            bidirectional_iterator<I>::value &&
                            !sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 auto impl(I& i,
                                                     iter_difference_t<I> n,
                                                     S bound)
                        -> iter_difference_t<I>
                    {
                        constexpr iter_difference_t<I> zero{0};
                        iter_difference_t<I> counter{0};

                        if (n < zero) {
                            do {
                                --i;
                                --counter;
                            } while (++n < zero && i != bound);
                        }
                        else {
                            while (n-- > zero && i != bound) {
                                ++i;
                                ++counter;
                            }
                        }

                        return counter;
                    }

                    template <
                        typename I,
                        typename S,
                        typename std::enable_if<
                            !bidirectional_iterator<I>::value &&
                            !sized_sentinel_for<S, I>::value>::type* = nullptr>
                    static SCN_CONSTEXPR14 auto impl(I& i,
                                                     iter_difference_t<I> n,
                                                     S bound)
                        -> iter_difference_t<I>
                    {
                        constexpr iter_difference_t<I> zero{0};
                        iter_difference_t<I> counter{0};

                        while (n-- > zero && i != bound) {
                            ++i;
                            ++counter;
                        }

                        return counter;
                    }

                public:
                    template <typename I,
                              typename std::enable_if<input_or_output_iterator<
                                  I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 void operator()(
                        I& i,
                        iter_difference_t<I> n) const
                    {
                        fn::impl(i, n);
                    }

                    template <typename I,
                              typename S,
                              typename std::enable_if<
                                  input_or_output_iterator<I>::value &&
                                  sentinel_for<S, I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 void operator()(I& i, S bound) const
                    {
                        fn::impl(i, bound, priority_tag<2>{});
                    }

                    template <typename I,
                              typename S,
                              typename std::enable_if<
                                  input_or_output_iterator<I>::value &&
                                  sentinel_for<S, I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 iter_difference_t<I>
                    operator()(I& i, iter_difference_t<I> n, S bound) const
                    {
                        return n - fn::impl(i, n, bound);
                    }
                };
            }  // namespace _advance
            namespace {
                SCN_CONSTEXPR auto& advance = static_const<_advance::fn>::value;
            }

            namespace _distance {
                struct fn {
                private:
                    template <typename I, typename S>
                    static constexpr auto impl(I i, S s) ->
                        typename std::enable_if<sized_sentinel_for<S, I>::value,
                                                iter_difference_t<I>>::type
                    {
                        return s - i;
                    }

                    template <typename I, typename S>
                    static constexpr auto impl(I i, S s) ->
                        typename std::enable_if<
                            !sized_sentinel_for<S, I>::value,
                            iter_difference_t<I>>::type
                    {
                        iter_difference_t<I> counter{0};
                        while (i != s) {
                            ++i;
                            ++counter;
                        }
                        return counter;
                    }

                    template <typename R>
                    static constexpr auto impl(R&& r) ->
                        typename std::enable_if<
                            sized_range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return static_cast<iter_difference_t<iterator_t<R>>>(
                            ranges::size(r));
                    }

                    template <typename R>
                    static constexpr auto impl(R&& r) ->
                        typename std::enable_if<
                            !sized_range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return fn::impl(ranges::begin(r), ranges::end(r));
                    }

                public:
                    template <typename I, typename S>
                    constexpr auto operator()(I first, S last) const ->
                        typename std::enable_if<
                            input_or_output_iterator<I>::value &&
                                sentinel_for<S, I>::value,
                            iter_difference_t<I>>::type
                    {
                        return fn::impl(std::move(first), std::move(last));
                    }

                    template <typename R>
                    constexpr auto operator()(R&& r) const ->
                        typename std::enable_if<
                            range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return fn::impl(std::forward<R>(r));
                    }
                };
            }  // namespace _distance
            namespace {
                SCN_CONSTEXPR auto& distance =
                    static_const<_distance::fn>::value;
            }

            namespace _next {
                struct fn {
                    template <typename I,
                              typename std::enable_if<input_or_output_iterator<
                                  I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 I operator()(I x) const
                    {
                        ++x;
                        return x;
                    }

                    template <typename I,
                              typename std::enable_if<input_or_output_iterator<
                                  I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 I operator()(I x,
                                                 iter_difference_t<I> n) const
                    {
                        ::scn::detail::ranges::advance(x, n);
                        return x;
                    }

                    template <typename I,
                              typename S,
                              typename std::enable_if<
                                  input_or_output_iterator<I>::value &&
                                  sentinel_for<S, I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 I operator()(I x, S bound) const
                    {
                        ranges::advance(x, bound);
                        return x;
                    }

                    template <typename I,
                              typename S,
                              typename std::enable_if<
                                  input_or_output_iterator<I>::value &&
                                  sentinel_for<S, I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 I operator()(I x,
                                                 iter_difference_t<I> n,
                                                 S bound) const
                    {
                        ::scn::detail::ranges::advance(x, n, bound);
                        return x;
                    }
                };
            }  // namespace _next
            namespace {
                SCN_CONSTEXPR auto& next = static_const<_next::fn>::value;
            }

            namespace _prev {
                struct fn {
                    template <typename I,
                              typename std::enable_if<bidirectional_iterator<
                                  I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 I operator()(I x) const
                    {
                        --x;
                        return x;
                    }

                    template <typename I,
                              typename std::enable_if<bidirectional_iterator<
                                  I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 I operator()(I x,
                                                 iter_difference_t<I> n) const
                    {
                        ::scn::detail::ranges::advance(x, -n);
                        return x;
                    }

                    template <typename I,
                              typename S,
                              typename std::enable_if<
                                  bidirectional_iterator<I>::value &&
                                  sentinel_for<S, I>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 I operator()(I x,
                                                 iter_difference_t<I> n,
                                                 S bound) const
                    {
                        ::scn::detail::ranges::advance(x, -n, bound);
                        return x;
                    }
                };
            }  // namespace _prev
            namespace {
                SCN_CONSTEXPR auto& prev = static_const<_prev::fn>::value;
            }

            template <typename D>
            class view_interface : public view_base {
                static_assert(std::is_class<D>::value, "");
                static_assert(
                    std::is_same<D, typename std::remove_cv<D>::type>::value,
                    "");

            private:
                SCN_CONSTEXPR14 D& derived() noexcept
                {
                    return static_cast<D&>(*this);
                }
                SCN_CONSTEXPR D& derived() const noexcept
                {
                    return static_cast<const D&>(*this);
                }

            public:
                template <typename R = D,
                          typename std::enable_if<
                              forward_range<R>::value>::type* = nullptr>
                SCN_NODISCARD SCN_CONSTEXPR14 bool empty()
                {
                    return ::scn::detail::ranges::begin(derived()) ==
                           ::scn::detail::ranges::end(derived());
                }
                template <typename R = D,
                          typename std::enable_if<
                              forward_range<const R>::value>::type* = nullptr>
                SCN_NODISCARD SCN_CONSTEXPR bool empty() const
                {
                    return ::scn::detail::ranges::begin(derived()) ==
                           ::scn::detail::ranges::end(derived());
                }

                template <typename R = D,
                          typename = decltype(
                              ::scn::detail::ranges::empty(std::declval<R&>()))>
                SCN_CONSTEXPR14 explicit operator bool()
                {
                    return !::scn::detail::ranges::empty(derived());
                }
                template <typename R = D,
                          typename = decltype(::scn::detail::ranges::empty(
                              std::declval<const R&>()))>
                SCN_CONSTEXPR explicit operator bool() const
                {
                    return !::scn::detail::ranges::empty(derived());
                }

                template <typename R = D,
                          typename std::enable_if<contiguous_iterator<
                              iterator_t<R>>::value>::type* = nullptr>
                SCN_CONSTEXPR14 auto data() -> decltype(
                    std::addressof(*::scn::detail::ranges::begin(derived())))
                {
                    return ::scn::detail::ranges::empty(derived())
                               ? nullptr
                               : std::addressof(
                                     *::scn::detail::ranges::begin(derived()));
                }
                template <typename R = D,
                          typename std::enable_if<contiguous_iterator<
                              iterator_t<const R>>::value>::type* = nullptr>
                SCN_CONSTEXPR auto data() const -> decltype(
                    std::addressof(*::scn::detail::ranges::begin(derived())))
                {
                    return ::scn::detail::ranges::empty(derived())
                               ? nullptr
                               : std::addressof(
                                     *::scn::detail::ranges::begin(derived()));
                }

                template <typename R = D,
                          typename std::enable_if<
                              forward_range<R>::value &&
                              sized_sentinel_for<sentinel_t<R>, iterator_t<R>>::
                                  value>::type* = nullptr>
                SCN_CONSTEXPR14 auto size()
                    -> decltype(::scn::detail::ranges::end(derived()) -
                                ::scn::detail::ranges::begin(derived()))
                {
                    return ::scn::detail::ranges::end(derived()) -
                           ::scn::detail::ranges::begin(derived());
                }

                template <
                    typename R = D,
                    typename std::enable_if<
                        forward_range<const R>::value &&
                        sized_sentinel_for<sentinel_t<const R>,
                                           iterator_t<const R>>::value>::type* =
                        nullptr>
                SCN_CONSTEXPR auto size() const
                    -> decltype(::scn::detail::ranges::end(derived()) -
                                ::scn::detail::ranges::begin(derived()))
                {
                    return ::scn::detail::ranges::end(derived()) -
                           ::scn::detail::ranges::begin(derived());
                }

                template <typename R = D,
                          typename std::enable_if<
                              forward_range<R>::value>::type* = nullptr>
                SCN_CONSTEXPR14 auto front()
                    -> decltype(*::scn::detail::ranges::begin(derived()))
                {
                    return *::scn::detail::ranges::begin(derived());
                }

                template <typename R = D,
                          typename std::enable_if<
                              forward_range<const R>::value>::type* = nullptr>
                SCN_CONSTEXPR auto front() const
                    -> decltype(*::scn::detail::ranges::begin(derived()))
                {
                    return *::scn::detail::ranges::begin(derived());
                }

                template <typename R = D,
                          typename std::enable_if<
                              bidirectional_range<R>::value &&
                              common_range<R>::value>::type* = nullptr>
                SCN_CONSTEXPR14 auto back()
                    -> decltype(*::scn::detail::ranges::prev(
                        ::scn::detail::ranges::end(derived())))
                {
                    return *::scn::detail::ranges::prev(
                        ::scn::detail::ranges::end(derived()));
                }

                template <typename R = D,
                          typename std::enable_if<
                              bidirectional_range<const R>::value &&
                              common_range<const R>::value>::type* = nullptr>
                SCN_CONSTEXPR14 auto back() const
                    -> decltype(*::scn::detail::ranges::prev(
                        ::scn::detail::ranges::end(derived())))
                {
                    return *::scn::detail::ranges::prev(
                        ::scn::detail::ranges::end(derived()));
                }

                template <typename R = D,
                          typename std::enable_if<
                              random_access_range<R>::value>::type* = nullptr>
                SCN_CONSTEXPR14 auto operator[](
                    iter_difference_t<iterator_t<R>> n)
                    -> decltype(::scn::detail::ranges::begin(derived())[n])
                {
                    return ::scn::detail::ranges::begin(derived())[n];
                }

                template <typename R = D,
                          typename std::enable_if<random_access_range<
                              const R>::value>::type* = nullptr>
                SCN_CONSTEXPR auto operator[](
                    iter_difference_t<iterator_t<const R>> n) const
                    -> decltype(::scn::detail::ranges::begin(derived())[n])
                {
                    return ::scn::detail::ranges::begin(derived())[n];
                }
            };

            enum class subrange_kind : bool { unsized, sized };

            template <typename I, typename S>
            struct _default_subrange_kind
                : std::integral_constant<subrange_kind,
                                         sized_sentinel_for<S, I>::value
                                             ? subrange_kind::sized
                                             : subrange_kind::unsized> {
            };

            namespace _subrange {
                template <typename I,
                          typename S = I,
                          subrange_kind = _default_subrange_kind<I, S>::value>
                class subrange;
            }  // namespace _subrange

            using _subrange::subrange;

            struct _pair_like_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T,
                          typename = typename std::tuple_size<T>::type>
                static auto test(int) -> typename std::enable_if<
                    _requires<_pair_like_concept, T>::value,
                    std::true_type>::type;

                template <typename T>
                auto _test_requires(T t) -> decltype(
                    requires_expr<derived_from<
                        std::tuple_size<T>,
                        std::integral_constant<std::size_t, 2>>::value>{},
                    std::declval<std::tuple_element<
                        0,
                        typename std::remove_const<T>::type>>(),
                    std::declval<std::tuple_element<
                        1,
                        typename std::remove_const<T>::type>>(),
                    requires_expr<convertible_to<
                        decltype(std::get<0>(t)),
                        const std::tuple_element<0, T>&>::value>{},
                    requires_expr<convertible_to<
                        decltype(std::get<1>(t)),
                        const std::tuple_element<1, T>&>::value>{});
            };
            template <typename T>
            struct _pair_like
                : std::integral_constant<
                      bool,
                      !std::is_reference<T>::value &&
                          decltype(_pair_like_concept::test<T>(0))::value> {
            };

            struct _pair_like_convertible_to_concept {
                template <typename T, typename U, typename V>
                auto _test_requires(T&& t)
                    -> decltype(requires_expr<convertible_to<
                                    decltype(std::get<0>(std::forward<T>(t))),
                                    U>::value>{},
                                requires_expr<convertible_to<
                                    decltype(std::get<1>(std::forward<T>(t))),
                                    V>::value>{});
            };
            template <typename T, typename U, typename V>
            struct _pair_like_convertible_to
                : std::integral_constant<
                      bool,
                      !range<T>::value &&
                          _pair_like<
                              typename std::remove_reference<T>::type>::value &&
                          _requires<_pair_like_convertible_to_concept,
                                    T,
                                    U,
                                    V>::value> {
            };
            template <typename T, typename U, typename V>
            struct _pair_like_convertible_from
                : std::integral_constant<
                      bool,
                      !range<T>::value &&
                          _pair_like<
                              typename std::remove_reference<T>::type>::value &&
                          constructible_from<T, U, V>::value> {
            };

            struct _iterator_sentinel_pair_concept {
                template <typename>
                static auto test(long) -> std::false_type;
                template <typename T>
                static auto test(int) -> typename std::enable_if<
                    !range<T>::value && _pair_like<T>::value &&
                        sentinel_for<
                            typename std::tuple_element<1, T>::type,
                            typename std::tuple_element<0, T>::type>::value,
                    std::true_type>::type;
            };
            template <typename T>
            struct _iterator_sentinel_pair
                : decltype(_iterator_sentinel_pair_concept::test<T>(0)) {
            };

            template <typename I, typename S, bool StoreSize = false>
            struct _subrange_data {
                SCN_CONSTEXPR _subrange_data() = default;
                SCN_CONSTEXPR _subrange_data(I&& b, S&& e)
                    : begin(std::move(b)), end(std::move(e))
                {
                }
                SCN_CONSTEXPR _subrange_data(I&& b, S&& e, iter_difference_t<I>)
                    : begin(std::move(b)), end(std::move(e))
                {
                }

                I begin{};
                S end{};
            };

            template <typename I, typename S>
            struct _subrange_data<I, S, true> {
                SCN_CONSTEXPR _subrange_data() = default;
                SCN_CONSTEXPR _subrange_data(I&& b,
                                             S&& e,
                                             iter_difference_t<I> s)
                    : begin(std::move(b)), end(std::move(e)), size(s)
                {
                }

                I begin{};
                S end{};
                iter_difference_t<I> size{0};
            };

            template <typename R, typename I, typename S, subrange_kind K>
            auto _subrange_range_constructor_constraint_helper_fn(long)
                -> std::false_type;

            template <typename R, typename I, typename S, subrange_kind K>
            auto _subrange_range_constructor_constraint_helper_fn(int) ->
                typename std::enable_if<
                    forwarding_range<R>::value &&
                        convertible_to<iterator_t<R>, I>::value &&
                        convertible_to<sentinel_t<R>, S>::value,
                    std::true_type>::type;

            template <typename R, typename I, typename S, subrange_kind K>
            struct _subrange_range_constructor_constraint_helper
                : decltype(
                      _subrange_range_constructor_constraint_helper_fn<R,
                                                                       I,
                                                                       S,
                                                                       K>(0)) {
            };

            template <typename R>
            SCN_CONSTEXPR subrange_kind _subrange_deduction_guide_helper()
            {
                return (sized_range<R>::value ||
                        sized_sentinel_for<sentinel_t<R>, iterator_t<R>>::value)
                           ? subrange_kind::sized
                           : subrange_kind::unsized;
            }

            namespace _subrange {
                template <typename I, typename S, subrange_kind K>
                class subrange : public view_interface<subrange<I, S, K>> {
                    static_assert(input_or_output_iterator<I>::value, "");
                    static_assert(sentinel_for<S, I>::value, "");
                    static_assert(K == subrange_kind::sized ||
                                      !sized_sentinel_for<S, I>::value,
                                  "");

                    static SCN_CONSTEXPR bool _store_size()
                    {
                        return K == subrange_kind::sized &&
                               !sized_sentinel_for<S, I>::value;
                    }

                public:
                    using iterator = I;
                    using sentinel = S;

                    subrange() = default;

                    template <bool SS = _store_size(),
                              typename std::enable_if<!SS>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(I i, S s)
                        : m_data{std::move(i), std::move(s)}
                    {
                    }
                    template <subrange_kind KK = K,
                              typename std::enable_if<
                                  KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(I i, S s, iter_difference_t<I> n)
                        : m_data{std::move(i), std::move(s), n}
                    {
                    }

                    template <
                        typename R,
                        bool SS = _store_size(),
                        typename std::enable_if<
                            _not_same_as<R, subrange>::value>::type* = nullptr,
                        typename std::enable_if<
                            _subrange_range_constructor_constraint_helper<
                                R,
                                I,
                                S,
                                K>::value &&
                            SS && sized_range<R>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(R&& r)
                        : subrange(::scn::detail::ranges::begin(r),
                                   ::scn::detail::ranges::end(r),
                                   ::scn::detail::ranges::size(r))
                    {
                    }

                    template <
                        typename R,
                        bool SS = _store_size(),
                        typename std::enable_if<
                            _not_same_as<R, subrange>::value>::type* = nullptr,
                        typename std::enable_if<
                            _subrange_range_constructor_constraint_helper<
                                R,
                                I,
                                S,
                                K>::value &&
                            !SS>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(R&& r)
                        : subrange(::scn::detail::ranges::begin(r),
                                   ::scn::detail::ranges::end(r))
                    {
                    }

                    template <typename R,
                              subrange_kind KK = K,
                              typename std::enable_if<
                                  forwarding_range<R>::value &&
                                  convertible_to<iterator_t<R>, I>::value &&
                                  convertible_to<sentinel_t<R>, S>::value &&
                                  KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(R&& r, iter_difference_t<I> n)
                        : subrange(::scn::detail::ranges::begin(r),
                                   ::scn::detail::ranges::end(r),
                                   n)
                    {
                    }

                    template <
                        typename PairLike,
                        bool SS = _store_size(),
                        typename std::enable_if<
                            _not_same_as<PairLike, subrange>::value>::type* =
                            nullptr,
                        typename std::enable_if<
                            _pair_like_convertible_to<PairLike, I, S>::value &&
                            !SS>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(PairLike&& r)
                        : subrange{std::get<0>(std::forward<PairLike>(r)),
                                   std::get<1>(std::forward<PairLike>(r))}
                    {
                    }

                    template <
                        typename PairLike,
                        subrange_kind KK = K,
                        typename std::enable_if<
                            _pair_like_convertible_to<PairLike, I, S>::value &&
                            KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(PairLike&& r,
                                             iter_difference_t<I> n)
                        : subrange{std::get<0>(std::forward<PairLike>(r)),
                                   std::get<1>(std::forward<PairLike>(r)), n}
                    {
                    }

                    template <
                        typename PairLike,
                        typename std::enable_if<
                            _not_same_as<PairLike, subrange>::value>::type* =
                            nullptr,
                        typename std::enable_if<_pair_like_convertible_from<
                            PairLike,
                            const I&,
                            const S&>::value>::type* = nullptr>
                    SCN_CONSTEXPR14 operator PairLike() const
                    {
                        return PairLike(begin(), end());
                    }

                    SCN_CONSTEXPR I begin() const
                    {
                        return m_data.begin;
                    }

                    SCN_CONSTEXPR S end() const
                    {
                        return m_data.end;
                    }

                    SCN_NODISCARD SCN_CONSTEXPR bool empty() const
                    {
                        return m_data.begin == m_data.end;
                    }

                    template <subrange_kind KK = K,
                              typename std::enable_if<
                                  KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR iter_difference_t<I> size() const
                    {
                        return _store_size() ? m_data.size
                                             : m_data.end - m_data.begin;
                    }

                    SCN_NODISCARD SCN_CONSTEXPR14 subrange
                    next(iter_difference_t<I> n = 1) const
                    {
                        auto tmp = *this;
                        tmp.advance(n);
                        return tmp;
                    }

                    template <typename II = I,
                              typename std::enable_if<bidirectional_iterator<
                                  II>::value>::type* = nullptr>
                    SCN_NODISCARD SCN_CONSTEXPR14 subrange
                    prev(iter_difference_t<I> n = 1) const
                    {
                        auto tmp = *this;
                        tmp.advance(-n);
                        return tmp;
                    }

                    SCN_CONSTEXPR14 subrange& advance(iter_difference_t<I> n)
                    {
                        if (_store_size()) {
                            m_data.size -= n - ranges::advance(m_data.begin, n,
                                                               m_data.end);
                        }
                        else {
                            ranges::advance(m_data.begin, n, m_data.end);
                        }
                        return *this;
                    }

                private:
                    _subrange_data<I, S, _store_size()> m_data{};
                };
            }  // namespace _subrange

            template <typename R>
            struct pair_reconstructible_range
                : std::integral_constant<
                      bool,
                      range<R>::value &&
                          forwarding_range<
                              typename std::remove_reference<R>::type>::value &&
                          constructible_from<R, iterator_t<R>, sentinel_t<R>>::
                              value> {
            };
            template <typename R>
            struct reconstructible_range
                : std::integral_constant<
                      bool,
                      range<R>::value &&
                          forwarding_range<
                              typename std::remove_reference<R>::type>::value &&
                          constructible_from<
                              R,
                              subrange<iterator_t<R>, sentinel_t<R>>>::value> {
            };
        }  // namespace ranges
    }      // namespace detail
#endif
    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_RANGES_H
