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
            // iterator_category is span.h

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
                    static SCN_CONSTEXPR14 void impl(T(&&)[N],
                                                     priority_tag<3>) = delete;

                    template <typename T, std::size_t N>
                    static SCN_CONSTEXPR14 auto impl(T (&t)[N],
                                                     priority_tag<3>) noexcept
                        -> decltype((t) + 0)
                    {
                        return (t) + 0;
                    }

                    template <typename C>
                    static SCN_CONSTEXPR14 auto impl(basic_string_view<C> sv,
                                                     priority_tag<2>) noexcept
                        -> decltype(sv.begin())
                    {
                        return sv.begin();
                    }

                    template <typename T>
                    static SCN_CONSTEXPR14 auto
                    impl(T& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(t.begin())))
                        -> decltype(decay_copy(t.begin()))
                    {
                        return decay_copy(t.begin());
                    }

                    template <typename T>
                    static SCN_CONSTEXPR14 auto
                    impl(T&& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(begin(std::forward<T>(t)))))
                        -> decltype(decay_copy(begin(std::forward<T>(t))))
                    {
                        return decay_copy(begin(std::forward<T>(t)));
                    }

                public:
                    template <typename T>
                    SCN_CONSTEXPR14 auto operator()(T&& t) const
                        noexcept(noexcept(fn::impl(std::forward<T>(t),
                                                   priority_tag<3>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<3>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<3>{});
                    }
                };
            }  // namespace _begin
            namespace {
                constexpr auto& begin = static_const<_begin::fn>::value;
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
                    static constexpr void impl(T(&&)[N],
                                               priority_tag<2>) = delete;

                    template <typename T, std::size_t N>
                    static constexpr auto impl(T (&t)[N],
                                               priority_tag<2>) noexcept
                        -> decltype((t) + N)
                    {
                        return (t) + N;
                    }

                    template <typename C>
                    static constexpr auto impl(basic_string_view<C> sv,
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
                    static constexpr auto impl(T& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(t.end())))
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
                    static constexpr auto impl(T& t, priority_tag<0>) noexcept(
                        noexcept(decay_copy(end(std::forward<T>(t))))) -> S
                    {
                        return decay_copy(end(std::forward<T>(t)));
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
            }  // namespace _end
            namespace {
                constexpr auto& end = static_const<_end::fn>::value;
            }

            // cbegin
            namespace _cbegin {
                struct fn {
                    template <typename T>
                    constexpr auto operator()(const T& t) const
                        noexcept(noexcept(::scn::detail::ranges::begin(t)))
                            -> decltype(::scn::detail::ranges::begin(t))
                    {
                        return ::scn::detail::ranges::begin(t);
                    }

                    template <typename T>
                    constexpr auto operator()(const T&& t) const
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
                constexpr auto& cbegin = static_const<_cbegin::fn>::value;
            }

            // cend
            namespace _cend {
                struct fn {
                    template <typename T>
                    constexpr auto operator()(const T& t) const
                        noexcept(noexcept(::scn::detail::ranges::end(t)))
                            -> decltype(::scn::detail::ranges::end(t))
                    {
                        return ::scn::detail::ranges::end(t);
                    }

                    template <typename T>
                    constexpr auto operator()(const T&& t) const
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
                constexpr auto& cend = static_const<_cend::fn>::value;
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
                    template <typename CharT,
                              typename Traits,
                              typename Allocator>
                    static constexpr auto impl(
                        std::basic_string<CharT, Traits, Allocator>& str,
                        priority_tag<2>) noexcept -> typename std::
                        basic_string<CharT, Traits, Allocator>::pointer
                    {
                        return std::addressof(*str.begin());
                    }
                    template <typename CharT,
                              typename Traits,
                              typename Allocator>
                    static constexpr auto impl(
                        const std::basic_string<CharT, Traits, Allocator>& str,
                        priority_tag<2>) noexcept -> typename std::
                        basic_string<CharT, Traits, Allocator>::const_pointer
                    {
                        return std::addressof(*str.begin());
                    }
                    template <typename CharT,
                              typename Traits,
                              typename Allocator>
                    static constexpr auto impl(
                        std::basic_string<CharT, Traits, Allocator>&& str,
                        priority_tag<2>) noexcept -> typename std::
                        basic_string<CharT, Traits, Allocator>::pointer
                    {
                        return std::addressof(*str.begin());
                    }

                    template <typename T,
                              typename D = decltype(
                                  decay_copy(std::declval<T&>().data()))>
                    static constexpr auto impl(T& t, priority_tag<1>) noexcept(
                        noexcept(decay_copy(t.data()))) ->
                        typename std::enable_if<_is_object_pointer<D>::value,
                                                D>::type
                    {
                        return decay_copy(t.data());
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
                                                   priority_tag<2>{})))
                            -> decltype(fn::impl(std::forward<T>(t),
                                                 priority_tag<2>{}))
                    {
                        return fn::impl(std::forward<T>(t), priority_tag<2>{});
                    }
                };
            }  // namespace _data
            namespace {
                constexpr auto& data = static_const<_data::fn>::value;
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
                    static constexpr std::size_t impl(const T(&&)[N],
                                                      priority_tag<3>) noexcept
                    {
                        return N;
                    }

                    template <typename T, std::size_t N>
                    static constexpr std::size_t impl(const T (&)[N],
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
                constexpr auto& size = static_const<_size::fn>::value;
            }

            // empty
            namespace _empty_ns {
                struct fn {
                private:
                    template <typename T>
                    static constexpr auto impl(T&& t, priority_tag<2>) noexcept(
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
                constexpr auto& empty = static_const<_empty_ns::fn>::value;
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
                constexpr D& derived() const noexcept
                {
                    return static_cast<const D&>(*this);
                }

            public:
                SCN_NODISCARD SCN_CONSTEXPR14 bool empty()
                {
                    return ::scn::detail::ranges::begin(derived()) ==
                           ::scn::detail::ranges::end(derived());
                }
                SCN_NODISCARD constexpr bool empty() const
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
                constexpr explicit operator bool() const
                {
                    return !::scn::detail::ranges::empty(derived());
                }

                template <typename R = D,
                          typename std::enable_if<
                              contiguous_range<R>::value>::type* = nullptr>
                auto data() -> decltype(std::addressof(
                    *::scn::detail::ranges::begin(static_cast<R&>(*this))))
                {
                    return ::scn::detail::ranges::empty(derived())
                               ? nullptr
                               : std::addressof(
                                     *::scn::detail::ranges::begin(derived()));
                }
                template <typename R = D,
                          typename std::enable_if<contiguous_range<
                              const R>::value>::type* = nullptr>
                auto data() const
                    -> decltype(std::addressof(*::scn::detail::ranges::begin(
                        static_cast<const R&>(*this))))
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
                SCN_CONSTEXPR14 auto size() -> decltype(
                    ::scn::detail::ranges::end(static_cast<R&>(*this)) -
                    ::scn::detail::ranges::begin(static_cast<R&>(*this)))
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
                constexpr auto size() const -> decltype(
                    ::scn::detail::ranges::end(static_cast<const R&>(*this)) -
                    ::scn::detail::ranges::begin(static_cast<const R&>(*this)))
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
                constexpr _subrange_data() = default;
                constexpr _subrange_data(I&& b, S&& e)
                    : begin(std::move(b)), end(std::move(e))
                {
                }
                template <bool Dependent = true>
                constexpr _subrange_data(
                    I&& b,
                    S&& e,
                    typename std::enable_if<Dependent,
                                            iter_difference_t<I>>::type)
                    : begin(std::move(b)), end(std::move(e))
                {
                }

                constexpr iter_difference_t<I> get_size() const
                {
                    return distance(begin, end);
                }

                I begin{};
                S end{};
            };

            template <typename I, typename S>
            struct _subrange_data<I, S, true> {
                constexpr _subrange_data() = default;
                constexpr _subrange_data(I&& b, S&& e, iter_difference_t<I> s)
                    : begin(std::move(b)), end(std::move(e)), size(s)
                {
                }

                constexpr iter_difference_t<I> get_size() const
                {
                    return size;
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
            constexpr subrange_kind _subrange_deduction_guide_helper()
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
                    static_assert(sentinel_for<S, I>::value, "");
                    static_assert(K == subrange_kind::sized ||
                                      !sized_sentinel_for<S, I>::value,
                                  "");

                    static constexpr bool _store_size =
                        K == subrange_kind::sized &&
                        !sized_sentinel_for<S, I>::value;

                public:
                    using iterator = I;
                    using sentinel = S;

                    subrange() = default;

                    template <bool SS = _store_size,
                              typename std::enable_if<!SS>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(I i, S s)
                        : m_data{std::move(i), std::move(s)}
                    {
                    }
                    template <bool Dependent = true,
                              subrange_kind KK = K,
                              typename std::enable_if<
                                  KK == subrange_kind::sized>::type* = nullptr>
                    SCN_CONSTEXPR14 subrange(
                        I i,
                        S s,
                        typename std::enable_if<Dependent,
                                                iter_difference_t<I>>::type n)
                        : m_data{std::move(i), std::move(s), n}
                    {
                    }

                    constexpr I begin() const noexcept
                    {
                        return m_data.begin;
                    }

                    constexpr S end() const noexcept
                    {
                        return m_data.end;
                    }

                    SCN_NODISCARD constexpr bool empty() const noexcept
                    {
                        return m_data.begin == m_data.end;
                    }

                    template <subrange_kind KK = K,
                              typename std::enable_if<
                                  KK == subrange_kind::sized>::type* = nullptr>
                    constexpr iter_difference_t<I> size() const noexcept
                    {
                        return m_data.get_size();
                    }

                private:
                    _subrange_data<I, S, _store_size> m_data{};
                };

                template <typename I, typename S, subrange_kind K>
                I begin(subrange<I, S, K>&& r) noexcept
                {
                    return r.begin();
                }
                template <typename I, typename S, subrange_kind K>
                S end(subrange<I, S, K>&& r) noexcept
                {
                    return r.end();
                }
            }  // namespace _subrange

            template <std::size_t N>
            struct _subrange_get_impl;
            template <>
            struct _subrange_get_impl<0> {
                template <typename I, typename S, subrange_kind K>
                static auto get(const subrange<I, S, K>& s)
                    -> decltype(s.begin())
                {
                    return s.begin();
                }
            };
            template <>
            struct _subrange_get_impl<1> {
                template <typename I, typename S, subrange_kind K>
                static auto get(const subrange<I, S, K>& s) -> decltype(s.end())
                {
                    return s.end();
                }
            };

            template <std::size_t N,
                      typename I,
                      typename S,
                      subrange_kind K,
                      typename std::enable_if<(N < 2)>::type* = nullptr>
            auto get(const subrange<I, S, K>& s)
                -> decltype(_subrange_get_impl<N>::get(s))
            {
                return _subrange_get_impl<N>::get(s);
            }

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
                    static constexpr T abs(T t)
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
                constexpr auto& advance = static_const<_advance::fn>::value;
            }

            // distance
            namespace _distance {
                struct fn {
                private:
                    template <typename I, typename S>
                    static SCN_CONSTEXPR14 auto impl(I i, S s) ->
                        typename std::enable_if<sized_sentinel_for<S, I>::value,
                                                iter_difference_t<I>>::type
                    {
                        return s - i;
                    }

                    template <typename I, typename S>
                    static SCN_CONSTEXPR14 auto impl(I i, S s) ->
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
                    static SCN_CONSTEXPR14 auto impl(R&& r) ->
                        typename std::enable_if<
                            sized_range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return static_cast<iter_difference_t<iterator_t<R>>>(
                            ranges::size(r));
                    }

                    template <typename R>
                    static SCN_CONSTEXPR14 auto impl(R&& r) ->
                        typename std::enable_if<
                            !sized_range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return fn::impl(ranges::begin(r), ranges::end(r));
                    }

                public:
                    template <typename I, typename S>
                    SCN_CONSTEXPR14 auto operator()(I first, S last) const ->
                        typename std::enable_if<sentinel_for<S, I>::value,
                                                iter_difference_t<I>>::type
                    {
                        return fn::impl(std::move(first), std::move(last));
                    }

                    template <typename R>
                    SCN_CONSTEXPR14 auto operator()(R&& r) const ->
                        typename std::enable_if<
                            range<R>::value,
                            iter_difference_t<iterator_t<R>>>::type
                    {
                        return fn::impl(std::forward<R>(r));
                    }
                };
            }  // namespace _distance
            namespace {
                constexpr auto& distance = static_const<_distance::fn>::value;
            }
        }  // namespace ranges
    }      // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

namespace std {
    template <typename I, typename S, ::scn::detail::ranges::subrange_kind K>
    class tuple_size<::scn::detail::ranges::subrange<I, S, K>>
        : public integral_constant<size_t, 2> {
    };

    template <typename I, typename S, ::scn::detail::ranges::subrange_kind K>
    class tuple_element<0, ::scn::detail::ranges::subrange<I, S, K>> {
    public:
        using type = I;
    };
    template <typename I, typename S, ::scn::detail::ranges::subrange_kind K>
    class tuple_element<1, ::scn::detail::ranges::subrange<I, S, K>> {
    public:
        using type = S;
    };

    using ::scn::detail::ranges::get;
}  // namespace std

#endif  // SCN_DETAIL_RANGES_H
