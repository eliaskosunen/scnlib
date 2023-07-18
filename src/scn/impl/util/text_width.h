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

#include <scn/detail/pp.h>
#include <array>
#include <iterator>
#include "scn/external/nanorange/nanorange.hpp"
#include "scn/impl/algorithms/common.h"

#if SCN_POSIX
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <cwchar>
#endif

#include <scn/impl/locale.h>
#include <scn/impl/unicode/unicode.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        enum class text_width_algorithm {
            // Use POSIX wcswidth
            // Only on POSIX
            wcswidth,

            // 1 code unit = 1 width unit
            code_units,

            // 1 code point = 1 width unit
            code_points,

            // 1 (extended) grapheme cluster = 1 width unit
            // grapheme_clusters,  // TODO

            // 1 code point = 1 width unit, except some are 2
            // {fmt} uses this in v10.0.0
            fmt_v10,

            // Whatever {fmt} uses in its latest version
            fmt_latest = fmt_v10,

            // 1 (extended) grapheme cluster = 1 width unit, except some are 2
            // std::format uses this, in C++23
            // std_format_23,  // TODO

            // Whatever std::format uses in the latest C++ WD
            // std_format_latest = std_format_23,

            // Width according to UAX #11,
            // with the "ambiguous" category having a width of 1
            // uax11,  // TODO

            // Width according to UAX #11,
            // with the "ambiguous" category having a width of 2
            // uax11_cjk,  // TODO
        };

        inline constexpr auto default_text_width_algorithm =
            text_width_algorithm::fmt_latest;

        constexpr std::size_t calculate_valid_text_width_for_fmt_v10(
            code_point cp)
        {
            if (cp >= 0x1100 &&
                (cp <= 0x115f ||  // Hangul Jamo init. consonants
                 cp == 0x2329 ||  // LEFT-POINTING ANGLE BRACKET
                 cp == 0x232a ||  // RIGHT-POINTING ANGLE BRACKET
                 // CJK ... Yi except IDEOGRAPHIC HALF FILL SPACE:
                 (cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f) ||
                 (cp >= 0xac00 && cp <= 0xd7a3) ||  // Hangul Syllables
                 (cp >= 0xf900 &&
                  cp <= 0xfaff) ||  // CJK Compatibility Ideographs
                 (cp >= 0xfe10 && cp <= 0xfe19) ||    // Vertical Forms
                 (cp >= 0xfe30 && cp <= 0xfe6f) ||    // CJK Compatibility Forms
                 (cp >= 0xff00 && cp <= 0xff60) ||    // Fullwidth Forms
                 (cp >= 0xffe0 && cp <= 0xffe6) ||    // Fullwidth Forms
                 (cp >= 0x20000 && cp <= 0x2fffd) ||  // CJK
                 (cp >= 0x30000 && cp <= 0x3fffd) ||
                 // Miscellaneous Symbols and Pictographs + Emoticons:
                 (cp >= 0x1f300 && cp <= 0x1f64f) ||
                 // Supplemental Symbols and Pictographs:
                 (cp >= 0x1f900 && cp <= 0x1f9ff))) {
                return 2;
            }
            return 1;
        }

        template <typename Dependent = void>
        std::size_t calculate_valid_text_width(
            code_point cp,
            text_width_algorithm algo = default_text_width_algorithm)
        {
            SCN_GCC_COMPAT_PUSH
            SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")
            switch (algo) {
                case text_width_algorithm::wcswidth: {
#if SCN_POSIX
                    set_clocale_classic_guard clocale_guard{LC_CTYPE};

                    auto input_u32 = static_cast<char32_t>(cp);
                    std::wstring winput;
                    transcode_valid_to_string(
                        std::u32string_view{&input_u32, 1}, winput);
                    const auto n = ::wcswidth(winput.data(), winput.size());
                    SCN_ENSURE(n != -1);
                    return static_cast<size_t>(n);
#else
                    SCN_ASSERT(false, "No wcswidth");
                    SCN_UNREACHABLE;
#endif
                }

                case text_width_algorithm::code_units: {
                    auto input_u32 = static_cast<char32_t>(cp);
                    std::wstring winput;
                    transcode_valid_to_string(
                        std::u32string_view{&input_u32, 1}, winput);
                    return winput.size();
                }

                case text_width_algorithm::code_points: {
                    return 1;
                }

                case text_width_algorithm::fmt_v10: {
                    return calculate_valid_text_width_for_fmt_v10(cp);
                }

                default:
                    SCN_ASSERT(false, "Not implemented");
                    SCN_UNREACHABLE;
            }
            SCN_GCC_COMPAT_POP  // -Wswitch-enum
        }

        template <typename CharT>
        std::size_t calculate_valid_text_width(
            std::basic_string_view<CharT> input,
            text_width_algorithm algo = default_text_width_algorithm)
        {
            SCN_GCC_COMPAT_PUSH
            SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")
            switch (algo) {
                case text_width_algorithm::wcswidth: {
#if SCN_POSIX
                    set_clocale_classic_guard clocale_guard{LC_CTYPE};

                    std::wstring winput;
                    transcode_valid_to_string(input, winput);
                    const auto n = ::wcswidth(winput.data(), winput.size());
                    SCN_ENSURE(n != -1);
                    return static_cast<size_t>(n);
#else
                    SCN_ASSERT(false, "No wcswidth");
                    SCN_UNREACHABLE;
#endif
                }

                case text_width_algorithm::code_units: {
                    return input.size();
                }

                case text_width_algorithm::code_points: {
                    return count_valid_code_points(input);
                }

                case text_width_algorithm::fmt_v10: {
                    size_t count{0};
                    for_each_code_point(input, [&count](code_point cp) {
                        count += calculate_valid_text_width_for_fmt_v10(cp);
                    });
                    return count;
                }

                default:
                    SCN_ASSERT(false, "Not implemented");
                    SCN_UNREACHABLE;
            }
            SCN_GCC_COMPAT_POP  // -Wswitch-enum
        }

        namespace counted_width_iterator_impl {
            template <typename It, typename S>
            class counted_width_iterator {
                static_assert(ranges_std::forward_iterator<It>);
                static_assert(ranges_std::sentinel_for<S, It>);

                template <typename OtherIt, typename OtherS>
                friend class counted_width_iterator;

            public:
                using iterator = It;
                using sentinel = S;
                using difference_type = std::iter_difference_t<It>;
                using iterator_category =
                    typename std::iterator_traits<iterator>::iterator_category;

                constexpr counted_width_iterator() = default;

                constexpr counted_width_iterator(iterator x,
                                                 sentinel s,
                                                 difference_type n)
                    : m_current(x), m_end(s), m_count(n)
                {
                }

                template <typename OtherIt,
                          typename OtherS,
                          std::enable_if_t<std::is_convertible_v<OtherIt, It> &&
                                           std::is_convertible_v<OtherS, S>>* =
                              nullptr>
                constexpr counted_width_iterator(
                    const counted_width_iterator<OtherIt, OtherS>& other)
                    : m_current(other.m_current),
                      m_end(other.m_end),
                      m_count(other.m_count)
                {
                }

                template <typename OtherIt, typename OtherS>
                constexpr auto operator=(
                    const counted_width_iterator<OtherIt, OtherS>& other)
                    -> std::enable_if_t<std::is_convertible_v<OtherIt, It> &&
                                            std::is_convertible_v<OtherS, S>,
                                        counted_width_iterator&>
                {
                    m_current = other.m_current;
                    m_count = other.m_count;
                    return *this;
                }

                constexpr It base() const
                {
                    return m_current;
                }
                constexpr difference_type count() const
                {
                    return m_count;
                }

                constexpr decltype(auto) operator*()
                {
                    return *m_current;
                }
                constexpr decltype(auto) operator*() const
                {
                    return *m_current;
                }

                constexpr counted_width_iterator& operator++()
                {
                    m_count -= _get_width_at_current();
                    ++m_current;
                    return *this;
                }

                constexpr counted_width_iterator operator++(int)
                {
                    auto tmp = *this;
                    ++*this;
                    return tmp;
                }

                // TODO: optimize, make better than forward, if possible
#if 0
                template <typename Iter = It>
                constexpr auto operator--() -> std::enable_if_t<
                    ranges_std::bidirectional_iterator<Iter>,
                    counted_width_iterator&>
                {
                    --m_current;
                    ++m_count;  // TODO
                    return *this;
                }

                template <typename Iter = It>
                constexpr auto operator--(int) -> std::enable_if_t<
                    ranges_std::bidirectional_iterator<Iter>,
                    counted_width_iterator>
                {
                    auto tmp = *this;
                    --*this;
                    return tmp;
                }

                template <typename Iter = It>
                constexpr auto operator+(difference_type n) -> std::enable_if_t<
                    ranges_std::random_access_iterator<Iter>,
                    counted_width_iterator>
                {
                    // TODO
                    return counted_width_iterator(m_current + n, m_count - n);
                }

                template <typename Iter = It,
                          std::enable_if_t<ranges_std::random_access_iterator<
                              Iter>>* = nullptr>
                friend constexpr counted_width_iterator operator+(
                    ranges_std::iter_difference_t<Iter> n,
                    const counted_width_iterator<Iter>& x)
                {
                    return x + n;
                }

                template <typename Iter = It>
                constexpr auto operator+=(difference_type n)
                    -> std::enable_if_t<
                        ranges_std::random_access_iterator<Iter>,
                        counted_width_iterator&>
                {
                    // TODO
                    m_current += n;
                    m_count -= n;
                    return *this;
                }

                template <typename Iter = It>
                constexpr auto operator-(difference_type n) -> std::enable_if_t<
                    ranges_std::random_access_iterator<Iter>,
                    counted_width_iterator>
                {
                    // TODO
                    return counted_width_iterator(m_current - n, m_count + n);
                }

                template <typename Iter = It,
                          std::enable_if_t<ranges_std::random_access_iterator<
                              Iter>>* = nullptr>
                constexpr decltype(auto) operator[](difference_type n) const
                {
                    return m_current[n];
                }
#endif

                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator==(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        bool>
                {
                    return a.count() == b.count();
                }
                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator!=(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        bool>
                {
                    return !(a == b);
                }

                friend constexpr bool operator==(
                    const counted_width_iterator& x,
                    ranges_std::default_sentinel_t)
                {
                    return x.count() == 0;
                }
                friend constexpr bool operator==(
                    ranges_std::default_sentinel_t,
                    const counted_width_iterator& x)
                {
                    return x.count() == 0;
                }

                friend constexpr bool operator!=(
                    const counted_width_iterator& a,
                    ranges_std::default_sentinel_t b)
                {
                    return !(a == b);
                }
                friend constexpr bool operator!=(
                    ranges_std::default_sentinel_t a,
                    const counted_width_iterator& b)
                {
                    return !(a == b);
                }

                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator<(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        bool>
                {
                    return a.count() < b.count();
                }

                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator>(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        bool>
                {
                    return !(b < a);
                }

                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator<=(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        bool>
                {
                    return !(b < a);
                }

                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator>=(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        bool>
                {
                    return !(a < b);
                }

                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator-(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        ranges_std::iter_difference_t<OtherIt>>
                {
                    return b.count() - a.count();
                }

                friend constexpr ranges_std::iter_difference_t<It> operator-(
                    const counted_width_iterator& x,
                    ranges_std::default_sentinel_t)
                {
                    return -x.m_count;
                }

                friend constexpr ranges_std::iter_difference_t<It> operator-(
                    ranges_std::default_sentinel_t,
                    const counted_width_iterator& x)
                {
                    return -x.m_count;
                }

#if 0
                template <typename Iter = It>
                constexpr auto operator-=(difference_type n)
                    -> std::enable_if_t<
                        ranges_std::random_access_iterator<Iter>,
                        counted_width_iterator&>
                {
                    // TODO
                    m_current -= n;
                    m_count += n;
                    return *this;
                }
#endif

                friend constexpr ranges_std::iter_rvalue_reference_t<It>
                iter_move(const counted_width_iterator& i)
                    SCN_NOEXCEPT_P(noexcept(ranges::iter_move(i.m_current)))
                {
                    return ranges::iter_move(i.m_current);
                }

                template <typename OtherIt, typename OtherS>
                friend constexpr auto iter_swap(
                    const counted_width_iterator<It, S>& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    SCN_NOEXCEPT_P(noexcept(ranges::iter_swap(a.m_current,
                                                              b.m_current)))
                        -> std::enable_if_t<
                            ranges_std::indirectly_swappable<OtherIt, It>>
                {
                    ranges::iter_swap(a.m_current, b.m_current);
                }

            private:
                difference_type _get_cp_length_at_current() const
                {
                    auto r =
                        code_point_length_by_starting_code_unit(*m_current);
                    if (!r) {
                        return 0;
                    }
                    return static_cast<difference_type>(*r);
                }

                difference_type _get_width_at_current() const
                {
                    auto cplen = _get_cp_length_at_current();
                    if (cplen == 0) {
                        return 0;
                    }

                    auto r = read_exactly_n_code_units(
                        ranges::subrange{m_current, m_end}, cplen);
                    if (SCN_UNLIKELY(!r)) {
                        return 0;
                    }

                    auto cp_view =
                        make_contiguous_buffer(ranges::subrange{m_current, *r});
                    if (SCN_UNLIEKLY(!validate_unicode(cp_view.view()))) {
                        return 0;
                    }

                    return calculate_valid_text_width(cp_view.view());
                }

                It m_current{};
                S m_end{};
                difference_type m_count{0};
            };
        }  // namespace counted_width_iterator_impl

        using counted_width_iterator_impl::counted_width_iterator;

        template <typename View>
        class take_width_view
            : public ranges::view_interface<take_width_view<View>> {
            static_assert(ranges::view<View>);

            template <bool IsConst>
            class sentinel {
                friend class sentinel<!IsConst>;
                using Base = std::conditional_t<IsConst, const View, View>;
                using CWI = counted_width_iterator<ranges::iterator_t<Base>,
                                                   ranges::sentinel_t<Base>>;
                using underlying = ranges::sentinel_t<Base>;

            public:
                constexpr sentinel() = default;

                constexpr explicit sentinel(underlying s) : m_end(SCN_MOVE(s))
                {
                }

                template <
                    typename S,
                    std::enable_if_t<
                        ranges_std::same_as<S, sentinel<!IsConst>>>* = nullptr,
                    bool C = IsConst,
                    typename VV = View,
                    std::enable_if_t<
                        C && ranges_std::convertible_to<ranges::sentinel_t<VV>,
                                                        underlying>>* = nullptr>
                constexpr explicit sentinel(S s) : m_end(SCN_MOVE(s.m_end))
                {
                }

                constexpr underlying base() const
                {
                    return m_end;
                }

                friend constexpr bool operator==(const CWI& y,
                                                 const sentinel& x)
                {
                    return y.count() == 0 || y.base() == x.end_;
                }

                friend constexpr bool operator==(const sentinel& x,
                                                 const CWI& y)
                {
                    return y == x;
                }

                friend constexpr bool operator!=(const CWI& y,
                                                 const sentinel& x)
                {
                    return !(y == x);
                }

                friend constexpr bool operator!=(const sentinel& x,
                                                 const CWI& y)
                {
                    return !(y == x);
                }

            private:
                SCN_NO_UNIQUE_ADDRESS underlying m_end{};
            };

        public:
            take_width_view() = default;

            constexpr take_width_view(View base,
                                      ranges::range_difference_t<View> count)
                : m_base(SCN_MOVE(base)), m_count(count)
            {
            }

            constexpr View base() const
            {
                return m_base;
            }

            template <
                typename V = View,
                std::enable_if_t<!ranges_polyfill::simple_view<V>, int> = 0>
            constexpr auto begin()
            {
                return counted_width_iterator{ranges::begin(m_base),
                                              ranges::end(m_base), m_count};
            }

            template <typename V = View,
                      std::enable_if_t<ranges::range<const V>, int> = 0>
            constexpr auto begin() const
            {
                return counted_width_iterator{ranges::begin(m_base),
                                              ranges::end(m_base), m_count};
            }

            template <
                typename V = View,
                std::enable_if_t<!ranges_polyfill::simple_view<V>, int> = 0>
            constexpr auto end()
            {
                return sentinel<false>{ranges::end(m_base)};
            }

            template <typename V = View,
                      std::enable_if_t<ranges::range<const V>, int> = 0>
            constexpr auto end() const
            {
                return sentinel<true>{ranges::end(m_base)};
            }

            // We don't want to be sized
#if 0
            template <typename V = View,
                      std::enable_if_t<sized_range<VV>, int> = 0>
            constexpr auto size()
            {
                auto n = ranges::size(base_);
                return ranges::min(n, static_cast<decltype(n)>(count_));
            }

            template <typename VV = V,
                      std::enable_if_t<sized_range<const VV>, int> = 0>
            constexpr auto size() const
            {
                auto n = ranges::size(base_);
                return ranges::min(n, static_cast<decltype(n)>(count_));
            }
#endif

        private:
            View m_base{};
            ranges::range_difference_t<View> m_count{0};
        };

        template <typename R, std::enable_if_t<ranges::range<R>>* = nullptr>
        take_width_view(R&&, ranges::range_difference_t<R>)
            -> take_width_view<ranges::all_view<R>>;

        struct _take_width_fn {
            template <typename R, typename N>
            constexpr auto operator()(R&& r, N&& n) const
                -> decltype(take_width_view{SCN_FWD(r), SCN_FWD(n)})
            {
                return take_width_view{SCN_FWD(r), SCN_FWD(n)};
            }
        };

        inline constexpr _take_width_fn take_width{};
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
