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

#ifndef SCN_DETAIL_TUPLE_RETURN_H
#define SCN_DETAIL_TUPLE_RETURN_H

#include "scan.h"

#include <functional>
#include <tuple>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        // From cppreference
        template <typename Fn,
                  typename... Args,
                  typename std::enable_if<std::is_member_pointer<
                      typename std::decay<Fn>::type>::value>::type* = nullptr,
                  int = 0>
        constexpr auto invoke(Fn&& f, Args&&... args) noexcept(
            noexcept(std::mem_fn(f)(std::forward<Args>(args)...)))
            -> decltype(std::mem_fn(f)(std::forward<Args>(args)...))
        {
            return std::mem_fn(f)(std::forward<Args>(args)...);
        }

        template <typename Fn,
                  typename... Args,
                  typename std::enable_if<!std::is_member_pointer<
                      typename std::decay<Fn>::type>::value>::type* = nullptr>
        constexpr auto invoke(Fn&& f, Args&&... args) noexcept(
            noexcept(std::forward<Fn>(f)(std::forward<Args>(args)...)))
            -> decltype(std::forward<Fn>(f)(std::forward<Args>(args)...))
        {
            return std::forward<Fn>(f)(std::forward<Args>(args)...);
        }

        // From Boost.mp11
        template <typename T, T... I>
        struct integer_sequence {
        };

        // iseq_if_c
        template <bool C, typename T, typename E>
        struct iseq_if_c_impl;

        template <typename T, typename E>
        struct iseq_if_c_impl<true, T, E> {
            using type = T;
        };

        template <typename T, typename E>
        struct iseq_if_c_impl<false, T, E> {
            using type = E;
        };

        template <bool C, typename T, typename E>
        using iseq_if_c = typename iseq_if_c_impl<C, T, E>::type;

        // iseq_identity
        template <typename T>
        struct iseq_identity {
            using type = T;
        };

        template <typename S1, typename S2>
        struct append_integer_sequence;

        template <typename T, T... I, T... J>
        struct append_integer_sequence<integer_sequence<T, I...>,
                                       integer_sequence<T, J...>> {
            using type = integer_sequence<T, I..., (J + sizeof...(I))...>;
        };

        template <typename T, T N>
        struct make_integer_sequence_impl;

        template <typename T, T N>
        struct make_integer_sequence_impl_ {
        private:
            static_assert(
                N >= 0,
                "make_integer_sequence<T, N>: N must not be negative");

            static T const M = N / 2;
            static T const R = N % 2;

            using S1 = typename make_integer_sequence_impl<T, M>::type;
            using S2 = typename append_integer_sequence<S1, S1>::type;
            using S3 = typename make_integer_sequence_impl<T, R>::type;
            using S4 = typename append_integer_sequence<S2, S3>::type;

        public:
            using type = S4;
        };

        template <typename T, T N>
        struct make_integer_sequence_impl
            : iseq_if_c<N == 0,
                        iseq_identity<integer_sequence<T>>,
                        iseq_if_c<N == 1,
                                  iseq_identity<integer_sequence<T, 0>>,
                                  make_integer_sequence_impl_<T, N>>> {
        };

        // make_integer_sequence
        template <typename T, T N>
        using make_integer_sequence =
            typename detail::make_integer_sequence_impl<T, N>::type;

        // index_sequence
        template <std::size_t... I>
        using index_sequence = integer_sequence<std::size_t, I...>;

        // make_index_sequence
        template <std::size_t N>
        using make_index_sequence = make_integer_sequence<std::size_t, N>;

        // index_sequence_for
        template <typename... T>
        using index_sequence_for =
            make_integer_sequence<std::size_t, sizeof...(T)>;

        // From cppreference
        template <class F, class Tuple, std::size_t... I>
        constexpr auto
        apply_impl(F&& f, Tuple&& t, index_sequence<I...>) noexcept(
            noexcept(detail::invoke(std::forward<F>(f),
                                    std::get<I>(std::forward<Tuple>(t))...)))
            -> decltype(detail::invoke(std::forward<F>(f),
                                       std::get<I>(std::forward<Tuple>(t))...))
        {
            return detail::invoke(std::forward<F>(f),
                                  std::get<I>(std::forward<Tuple>(t))...);
        }  // namespace detail

        template <class F, class Tuple>
        constexpr auto apply(F&& f, Tuple&& t) noexcept(
            noexcept(detail::apply_impl(
                std::forward<F>(f),
                std::forward<Tuple>(t),
                make_index_sequence<std::tuple_size<
                    typename std::remove_reference<Tuple>::type>::value>{})))
            -> decltype(detail::apply_impl(
                std::forward<F>(f),
                std::forward<Tuple>(t),
                make_index_sequence<std::tuple_size<
                    typename std::remove_reference<Tuple>::type>::value>{}))
        {
            return detail::apply_impl(
                std::forward<F>(f), std::forward<Tuple>(t),
                make_index_sequence<std::tuple_size<
                    typename std::remove_reference<Tuple>::type>::value>{});
        }
    }  // namespace detail

    /**
     * Alternative interface for scanning, returning values as a tuple, instead
     * of taking them by reference.
     *
     * It's highly recommended to use this interface only with C++17 or later,
     * as structured bindings make it way more ergonomic.
     *
     * Compared to the regular scan interface, the performance of this interface
     * is the same (generated code is virtually identical with optimizations
     * enabled), but the compile time is slower.
     *
     * Values scanned by this function still need to be default-constructible.
     * To scan a non-default-constructible value, use \c scn::optional
     *
     * @param r Input range
     * @return Tuple, where the first element is the scan result, and the
     * remaining elements are the scanned values.
     */
    template <typename... Args, typename Range, typename Format>
    SCN_NODISCARD auto scan_tuple(Range&& r, Format f)
        -> std::tuple<detail::scan_result_for_range<Range>, Args...>
    {
        using result = detail::scan_result_for_range<Range>;
        using range_type = typename result::wrapped_range_type;

        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;
        using char_type = typename range_type::char_type;

        auto range = wrap(SCN_FWD(r));
        auto scanfn = [&range, &f](Args&... a) {
            auto args = make_args<context_type, parse_context_type>(a...);
            return vscan(SCN_MOVE(range), basic_string_view<char_type>(f),
                         {args});
        };

        std::tuple<Args...> values{Args{}...};
        auto ret = detail::apply(scanfn, values);
        return std::tuple_cat(
            std::tuple<result>{detail::wrap_result(wrapped_error{ret.err},
                                                   detail::range_tag<Range>{},
                                                   SCN_MOVE(ret.range))},
            SCN_MOVE(values));
    }

    template <typename... Args, typename Range>
    SCN_NODISCARD auto scan_tuple_default(Range&& r)
        -> std::tuple<detail::scan_result_for_range<Range>, Args...>
    {
        using result = detail::scan_result_for_range<Range>;
        using range_type = typename result::wrapped_range_type;

        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_empty_parse_context<typename context_type::locale_type>;

        auto range = wrap(SCN_FWD(r));
        auto scanfn = [&range](Args&... a) {
            auto args = make_args<context_type, parse_context_type>(a...);
            return vscan_default(SCN_MOVE(range),
                                 static_cast<int>(sizeof...(Args)), {args});
        };

        std::tuple<Args...> values{Args{}...};
        auto ret = detail::apply(scanfn, values);
        return std::tuple_cat(
            std::tuple<result>{detail::wrap_result(wrapped_error{ret.err},
                                                   detail::range_tag<Range>{},
                                                   SCN_MOVE(ret.range))},
            SCN_MOVE(values));
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif
