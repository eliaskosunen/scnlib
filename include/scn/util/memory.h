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

#if SCN_MSVC && SCN_HAS_STRING_VIEW
#include <string_view>
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename T>
        struct pointer_traits;

        template <typename T>
        struct pointer_traits<T*> {
            using pointer = T*;
            using element_type = T;
            using difference_type = std::ptrdiff_t;

            template <typename U>
            using rebind = U*;

            template <typename U = T,
                      typename std::enable_if<!std::is_void<U>::value>::type* =
                          nullptr>
            static constexpr pointer pointer_to(U& r) SCN_NOEXCEPT
            {
                return &r;
            }
        };

        template <typename T>
        constexpr T* to_address_impl(T* p, priority_tag<2>) SCN_NOEXCEPT
        {
            return p;
        }
        template <typename Ptr>
        constexpr auto to_address_impl(const Ptr& p,
                                       priority_tag<1>) SCN_NOEXCEPT
            ->decltype(::scn::detail::pointer_traits<Ptr>::to_address(p))
        {
            return ::scn::detail::pointer_traits<Ptr>::to_address(p);
        }
        template <typename Ptr>
        constexpr auto to_address_impl(const Ptr& p,
                                       priority_tag<0>) SCN_NOEXCEPT
            ->decltype(::scn::detail::to_address_impl(p.operator->(),
                                                      priority_tag<2>{}))
        {
            return ::scn::detail::to_address_impl(p.operator->(),
                                                  priority_tag<2>{});
        }

        template <typename Ptr>
        constexpr auto to_address(Ptr&& p) SCN_NOEXCEPT
            ->decltype(::scn::detail::to_address_impl(SCN_FWD(p),
                                                      priority_tag<2>{}))
        {
            return ::scn::detail::to_address_impl(SCN_FWD(p),
                                                  priority_tag<2>{});
        }

        template <typename It, typename = void>
        struct can_make_address_from_iterator : std::false_type {};
        template <typename It>
        struct can_make_address_from_iterator<
            It,
            std::enable_if_t<
                std::is_pointer_v<It> ||
                std::is_pointer_v<
                    typename pointer_traits<It>::pointer> ||
                std::is_pointer_v<decltype(SCN_DECLVAL(It&).operator->())>>>
            : std::true_type {};

#if 0
#if SCN_WINDOWS
        template <typename I, typename B, typename E>
        constexpr auto to_address_safe(I&& p, B begin, E end)
            SCN_NOEXCEPT->decltype(to_address(SCN_FWD(p)))
        {
            if (p >= begin && p < end) {
                return to_address(SCN_FWD(p));
            }
            if (begin == end) {
                return to_address(SCN_FWD(p));
            }
            if (p == end) {
                return to_address(SCN_FWD(p) - 1) + 1;
            }
            SCN_ENSURE(false);
            SCN_UNREACHABLE;
        }
#else
        template <typename I, typename B, typename E>
        constexpr auto to_address_safe(I&& p, B, E)
            SCN_NOEXCEPT->decltype(to_address(SCN_FWD(p)))
        {
            return to_address(SCN_FWD(p));
        }
#endif
#endif

// Workaround for MSVC _String_view_iterator
#if SCN_MSVC
        template <typename Traits>
        struct pointer_traits<std::_String_view_iterator<Traits>> {
            using iterator = std::_String_view_iterator<Traits>;
            using pointer = typename iterator::pointer;
            using element_type = typename iterator::value_type;
            using difference_type = typename iterator::difference_type;

            static constexpr pointer to_address(const iterator& it) SCN_NOEXCEPT
            {
                // operator-> of _String_view_iterator
                // is checked for past-the-end dereference,
                // even though operator-> isn't dereferencing anything :)))
                return it._Unwrapped();
            }
        };

        template <typename Traits>
        struct pointer_traits<std::_String_const_iterator<Traits>> {
            using iterator = std::_String_const_iterator<Traits>;
            using pointer = typename iterator::pointer;
            using element_type = typename iterator::value_type;
            using difference_type = typename iterator::difference_type;

            static constexpr pointer to_address(const iterator& it) SCN_NOEXCEPT
            {
                return it._Unwrapped();
            }
        };
#endif
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
