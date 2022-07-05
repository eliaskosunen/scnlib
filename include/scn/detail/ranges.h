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

#if SCN_STD_RANGES

#include <concepts>
#include <ranges>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace ranges = std::ranges;
    namespace ranges_std = std;

    namespace ranges_polyfill {
        template <typename Range>
        using owning_view = ranges::owning_view<Range>;

        namespace views {
            inline constexpr auto& all = ranges::views::all;

            template <typename Range>
            using all_t = ranges::views::all_t<Range>;
        }  // namespace views
    }      // namespace ranges_polyfill

    SCN_END_NAMESPACE
}  // namespace scn

namespace std::ranges {
    template <typename T>
    inline constexpr bool enable_view<scn::span<T>> = true;
    template <typename T>
    inline constexpr bool enable_borrowed_range<scn::span<T>> = true;
}  // namespace std::ranges

#else

#include <scn/util/meta.h>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
SCN_GCC_IGNORE("-Wconversion")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wshadow")
SCN_CLANG_IGNORE("-Wredundant-parens")
SCN_CLANG_IGNORE("-Wimplicit-int-conversion")
SCN_CLANG_IGNORE("-Wctad-maybe-unsupported")

#include <scn/external/nanorange/nanorange.hpp>

SCN_CLANG_POP

SCN_GCC_POP

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace ranges = nano;
    namespace ranges_std = nano;

    namespace ranges_polyfill {
        template <typename T>
        struct _is_initializer_list : std::false_type {};
        template <typename T>
        struct _is_initializer_list<std::initializer_list<T>> : std::true_type {
        };

        template <typename Range,
                  typename =
                      std::enable_if_t<scn::ranges::range<Range> &&
                                       scn::ranges_std::movable<Range> &&
                                       !_is_initializer_list<Range>::value>>
        class owning_view
            : public scn::ranges::view_interface<owning_view<Range>> {
        public:
            owning_view() = default;

            constexpr owning_view(Range&& r)
                SCN_NOEXCEPT_P(std::is_nothrow_move_constructible_v<Range>)
                : m_range(SCN_MOVE(r))
            {
            }

            owning_view(owning_view&&) = default;
            owning_view& operator=(owning_view&&) = default;

            owning_view(const owning_view&) = delete;
            owning_view& operator=(const owning_view&) = delete;

            ~owning_view() = default;

            constexpr Range& base() & SCN_NOEXCEPT
            {
                return m_range;
            }
            constexpr const Range& base() const& SCN_NOEXCEPT
            {
                return m_range;
            }
            constexpr Range&& base() && SCN_NOEXCEPT
            {
                return SCN_MOVE(m_range);
            }
            constexpr const Range&& base() const&& SCN_NOEXCEPT
            {
                return SCN_MOVE(m_range);
            }

            constexpr scn::ranges::iterator_t<Range> begin()
            {
                return scn::ranges::begin(m_range);
            }
            constexpr scn::ranges::sentinel_t<Range> end()
            {
                return scn::ranges::end(m_range);
            }

            template <typename R = Range,
                      std::enable_if_t<scn::ranges::range<const R>>* = nullptr>
            constexpr auto begin() const
            {
                return scn::ranges::begin(m_range);
            }
            template <typename R = Range,
                      std::enable_if_t<scn::ranges::range<const R>>* = nullptr>
            constexpr auto end() const
            {
                return scn::ranges::end(m_range);
            }

            template <typename R = Range,
                      std::void_t<decltype(scn::ranges::empty(
                          SCN_DECLVAL(R&)))>* = nullptr>
            constexpr bool empty()
            {
                return scn::ranges::empty(m_range);
            }
            template <typename R = Range,
                      std::void_t<decltype(scn::ranges::empty(
                          SCN_DECLVAL(const R&)))>* = nullptr>
            constexpr bool empty() const
            {
                return scn::ranges::empty(m_range);
            }

            template <typename R = Range,
                      std::void_t<decltype(scn::ranges::size(
                          SCN_DECLVAL(R&)))>* = nullptr>
            constexpr auto size()
            {
                return scn::ranges::size(m_range);
            }
            template <typename R = Range,
                      std::void_t<decltype(scn::ranges::size(
                          SCN_DECLVAL(const R&)))>* = nullptr>
            constexpr auto size() const
            {
                return scn::ranges::size(m_range);
            }

            template <typename R = Range,
                      std::void_t<decltype(scn::ranges::data(
                          SCN_DECLVAL(R&)))>* = nullptr>
            constexpr auto data()
            {
                return scn::ranges::data(m_range);
            }
            template <typename R = Range,
                      std::void_t<decltype(scn::ranges::data(
                          SCN_DECLVAL(const R&)))>* = nullptr>
            constexpr auto data() const
            {
                return scn::ranges::data(m_range);
            }

        private:
            Range m_range = Range();
        };

        template <typename Range>
        owning_view(Range) -> owning_view<Range>;

        namespace views {
            struct _all_fn {
            private:
                template <
                    typename Range,
                    std::enable_if_t<scn::ranges::view<std::decay_t<Range>>>* =
                        nullptr>
                static constexpr auto
                impl(Range&& r, scn::detail::priority_tag<2>) SCN_NOEXCEPT_P(
                    std::is_nothrow_constructible_v<std::decay_t<Range>, Range>)
                {
                    return SCN_FWD(r);
                }

                template <typename Range>
                static constexpr auto impl(Range&& r,
                                           scn::detail::priority_tag<1>)
                    SCN_NOEXCEPT->decltype(scn::ranges::ref_view{SCN_FWD(r)})
                {
                    return scn::ranges::ref_view{SCN_FWD(r)};
                }

                template <typename Range>
                static constexpr auto impl(Range&& r,
                                           scn::detail::priority_tag<0>)
                    SCN_NOEXCEPT_P(noexcept(scn::ranges_polyfill::owning_view{
                        SCN_FWD(r)}))
                        -> decltype(scn::ranges_polyfill::owning_view{
                            SCN_FWD(r)})
                {
                    return scn::ranges_polyfill::owning_view{SCN_FWD(r)};
                }

            public:
                template <typename Range>
                constexpr auto operator()(Range&& r) const SCN_NOEXCEPT_P(
                    noexcept(_all_fn::impl(SCN_FWD(r),
                                           scn::detail::priority_tag<2>{})))
                    -> decltype(_all_fn::impl(SCN_FWD(r),
                                              scn::detail::priority_tag<2>{}))
                {
                    return _all_fn::impl(SCN_FWD(r),
                                         scn::detail::priority_tag<2>{});
                }
            };

            inline constexpr _all_fn all{};

            template <typename Range>
            using all_t = decltype(all(SCN_DECLVAL(Range)));
        }  // namespace views
    }      // namespace ranges_polyfill

    SCN_END_NAMESPACE
}  // namespace scn

namespace nano {
    template <typename T>
    inline constexpr bool enable_view<scn::span<T>> = true;

    template <typename T>
    inline constexpr bool enable_borrowed_range<scn::span<T>> = true;
    template <typename T>
    inline constexpr bool
        enable_borrowed_range<scn::ranges_polyfill::owning_view<T>> =
            enable_borrowed_range<T>;
}  // namespace nano

#endif  // SCN_STD_RANGES

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename R>
        using borrowed_itsen_subrange_t = std::conditional_t<
            ranges::borrowed_range<R>,
            ranges::subrange<ranges::iterator_t<R>, ranges::sentinel_t<R>>,
            ranges::dangling>;
    }

    SCN_END_NAMESPACE
}  // namespace scn
