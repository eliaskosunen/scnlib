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
        template <typename... T>
        void valid_expr(T&&...);

        template <std::size_t I>
        struct priority_tag : priority_tag<I - 1> {};
        template <>
        struct priority_tag<0> {};

        template <typename T>
        using integer_type_for_char = typename std::
            conditional<std::is_signed<T>::value, int, unsigned>::type;

        template <typename T>
        using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

        template <typename T, template <typename...> class Templ>
        struct is_specialization_of_impl : std::false_type {};
        template <typename... T, template <typename...> class Templ>
        struct is_specialization_of_impl<Templ<T...>, Templ> : std::true_type {
        };

        template <typename T, template <typename...> class Templ>
        using is_specialization_of =
            is_specialization_of_impl<remove_cvref_t<T>, Templ>;
        template <typename T, template <typename...> class Templ>
        inline constexpr bool is_specialization_of_v =
            is_specialization_of<T, Templ>::value;

        // tag_invoke

        namespace tginv {
            namespace adl {
                template <typename Tag, typename... Args>
                constexpr auto adl_tag_invoke(Tag&& tag, Args&&... args)
                    SCN_NOEXCEPT_P(noexcept(tag_invoke(SCN_FWD(tag),
                                                       SCN_FWD(args)...)))
                        -> decltype(tag_invoke(SCN_FWD(tag), SCN_FWD(args)...))
                {
                    return tag_invoke(SCN_FWD(tag), SCN_FWD(args)...);
                }
            }  // namespace adl

            struct fn {
                template <typename Tag, typename... Args>
                constexpr auto operator()(Tag&& tag, Args&&... args) const
                    SCN_NOEXCEPT_P(noexcept(
                        adl::adl_tag_invoke(SCN_FWD(tag), SCN_FWD(args)...)))
                        -> decltype(adl::adl_tag_invoke(SCN_FWD(tag),
                                                        SCN_FWD(args)...))
                {
                    return adl::adl_tag_invoke(SCN_FWD(tag), SCN_FWD(args)...);
                }
            };
        }  // namespace tginv

        inline constexpr tginv::fn tag_invoke{};

        template <bool, typename Tag, typename... Args>
        struct is_nothrow_tag_invocable_t : std::false_type {};
        template <typename Tag, typename... Args>
        struct is_nothrow_tag_invocable_t<true, Tag, Args...>
            : std::integral_constant<
                  bool,
                  std::is_nothrow_invocable_v<decltype(tag_invoke),
                                              Tag,
                                              Args...>> {};

        template <typename Tag, typename... Args>
        struct is_tag_invocable
            : std::is_invocable<decltype(tag_invoke), Tag, Args...> {};
        template <typename Tag, typename... Args>
        inline constexpr bool is_tag_invocable_v =
            is_tag_invocable<Tag, Args...>::value;

        template <typename Tag, typename... Args>
        struct is_nothrow_tag_invocable
            : is_nothrow_tag_invocable_t<is_tag_invocable_v<Tag, Args...>,
                                         Tag,
                                         Args...> {};
        template <typename Tag, typename... Args>
        inline constexpr bool is_nothrow_tag_invocable_v =
            is_nothrow_tag_invocable<Tag, Args...>::value;

        template <auto& Name>
        using tag_t = remove_cvref_t<decltype(Name)>;

        struct nonesuch {
            nonesuch() = delete;
            nonesuch(const nonesuch&) = delete;
            nonesuch(nonesuch&&) = delete;
            nonesuch& operator=(const nonesuch&) = delete;
            nonesuch& operator=(nonesuch&&) = delete;
            ~nonesuch() = delete;
        };

        template <class Default,
                  class AlwaysVoid,
                  template <class...>
                  class Op,
                  class... Args>
        struct detector {
            using value_t = std::false_type;
            using type = Default;
        };

        template <class Default, template <class...> class Op, class... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
            using value_t = std::true_type;
            using type = Op<Args...>;
        };

        template <template <class...> class Op, class... Args>
        using is_detected =
            typename detector<nonesuch, void, Op, Args...>::value_t;

        template <template <class...> class Op, class... Args>
        using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

        template <class Default, template <class...> class Op, class... Args>
        using detected_or = detector<Default, void, Op, Args...>;

        template <template <class...> class Op, class... Args>
        constexpr inline bool is_detected_v = is_detected<Op, Args...>::value;

        template <class Default, template <class...> class Op, class... Args>
        using detected_or_t = typename detected_or<Default, Op, Args...>::type;

        template <class Expected, template <class...> class Op, class... Args>
        using is_detected_exact =
            std::is_same<Expected, detected_t<Op, Args...>>;

        template <class Expected, template <class...> class Op, class... Args>
        constexpr inline bool is_detected_exact_v =
            is_detected_exact<Expected, Op, Args...>::value;

        template <class To, template <class...> class Op, class... Args>
        using is_detected_convertible =
            std::is_convertible<detected_t<Op, Args...>, To>;

        template <class To, template <class...> class Op, class... Args>
        constexpr inline bool is_detected_convertible_v =
            is_detected_convertible<To, Op, Args...>::value;
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
