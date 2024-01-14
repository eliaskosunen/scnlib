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

#include <scn/impl/algorithms/contiguous_range_factory.h>
#include <scn/impl/algorithms/read_simple.h>
#include <scn/impl/util/text_width.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
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
    using value_type = ranges_std::iter_value_t<It>;
    using pointer = value_type*;
    using reference = value_type&;
    using difference_type = ranges_std::iter_difference_t<It>;
    using iterator_category =
        std::conditional_t<ranges_std::bidirectional_iterator<It>,
                           std::bidirectional_iterator_tag,
                           std::forward_iterator_tag>;

    constexpr counted_width_iterator() = default;

    constexpr counted_width_iterator(iterator x, sentinel s, difference_type n)
        : m_current(x), m_end(s), m_count(n)
    {
    }

    template <typename OtherIt,
              typename OtherS,
              std::enable_if_t<std::is_convertible_v<OtherIt, It> &&
                               std::is_convertible_v<OtherS, S>>* = nullptr>
    constexpr counted_width_iterator(
        const counted_width_iterator<OtherIt, OtherS>& other)
        : m_current(other.m_current),
          m_end(other.m_end),
          m_count(other.m_count),
          m_multibyte_left(other.m_multibyte_left)
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
        m_end = other.m_end;
        m_count = other.m_count;
        m_multibyte_left = other.m_multibyte_left;
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
    constexpr difference_type multibyte_left() const
    {
        return m_multibyte_left;
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
        SCN_EXPECT(m_current != m_end);
        _increment_current();
        return *this;
    }

    constexpr counted_width_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    template <typename Iter = It>
    constexpr auto operator--()
        -> std::enable_if_t<ranges_std::bidirectional_iterator<Iter>,
                            counted_width_iterator&>
    {
        _decrement_current();
        return *this;
    }

    template <typename Iter = It>
    constexpr auto operator--(int)
        -> std::enable_if_t<ranges_std::bidirectional_iterator<Iter>,
                            counted_width_iterator>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    // TODO: optimize, make better than forward, if possible
#if 0
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
        -> std::enable_if_t<ranges_std::common_with<OtherIt, It>, bool>
    {
        return a.m_current == b.m_current;
    }
    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator!=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> std::enable_if_t<ranges_std::common_with<OtherIt, It>, bool>
    {
        return !(a == b);
    }

    friend constexpr bool operator==(const counted_width_iterator& x,
                                     ranges_std::default_sentinel_t)
    {
        return x.count() == 0 && x.multibyte_left() == 0;
    }
    friend constexpr bool operator==(ranges_std::default_sentinel_t,
                                     const counted_width_iterator& x)
    {
        return x.count() == 0 && x.multibyte_left() == 0;
    }

    friend constexpr bool operator!=(const counted_width_iterator& a,
                                     ranges_std::default_sentinel_t b)
    {
        return !(a == b);
    }
    friend constexpr bool operator!=(ranges_std::default_sentinel_t a,
                                     const counted_width_iterator& b)
    {
        return !(a == b);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator<(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> std::enable_if_t<ranges_std::common_with<OtherIt, It>, bool>
    {
        if (a.count() == b.count()) {
            return a.multibyte_left() > b.multibyte_left();
        }

        return a.count() > b.count();
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator>(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> std::enable_if_t<ranges_std::common_with<OtherIt, It>, bool>
    {
        return !(b < a);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator<=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> std::enable_if_t<ranges_std::common_with<OtherIt, It>, bool>
    {
        return !(b < a);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator>=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> std::enable_if_t<ranges_std::common_with<OtherIt, It>, bool>
    {
        return !(a < b);
    }

#if 0
                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator-(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        ranges_std::iter_difference_t<OtherIt>>
                {
                    // TODO
                }

                friend constexpr ranges_std::iter_difference_t<It> operator-(
                    const counted_width_iterator& x,
                    ranges_std::default_sentinel_t)
                {
                    // TODO
                }

                friend constexpr ranges_std::iter_difference_t<It> operator-(
                    ranges_std::default_sentinel_t,
                    const counted_width_iterator& x)
                {
                    // TODO
                }
#endif

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

    friend constexpr ranges_std::iter_rvalue_reference_t<It> iter_move(
        const counted_width_iterator& i)
        SCN_NOEXCEPT_P(noexcept(ranges::iter_move(i.m_current)))
    {
        return ranges::iter_move(i.m_current);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto iter_swap(
        const counted_width_iterator<It, S>& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        SCN_NOEXCEPT_P(noexcept(ranges::iter_swap(a.m_current, b.m_current)))
            -> std::enable_if_t<ranges_std::indirectly_swappable<OtherIt, It>>
    {
        ranges::iter_swap(a.m_current, b.m_current);
    }

private:
    difference_type _get_cp_length_at_current() const
    {
        return static_cast<difference_type>(
            code_point_length_by_starting_code_unit(*m_current));
    }

    difference_type _get_width_at_current_cp_start(difference_type cplen) const
    {
        if (SCN_UNLIKELY(cplen == 0)) {
            return 0;
        }

        if (cplen == 1) {
            SCN_EXPECT(m_current != m_end);
            auto cp = static_cast<char32_t>(*m_current);
            return static_cast<difference_type>(calculate_valid_text_width(cp));
        }

        auto r = read_exactly_n_code_units(ranges::subrange{m_current, m_end},
                                           cplen);
        if (SCN_UNLIKELY(!r)) {
            return 0;
        }

        auto cp_view = make_contiguous_buffer(ranges::subrange{m_current, *r});
        if (SCN_UNLIKELY(!validate_unicode(cp_view.view()))) {
            return 0;
        }

        return static_cast<difference_type>(
            calculate_valid_text_width(cp_view.view()));
    }

    void _increment_current()
    {
        if (m_multibyte_left == 0) {
            auto cplen = _get_cp_length_at_current();
            m_multibyte_left = cplen - 1;
            m_count -= _get_width_at_current_cp_start(cplen);
        }
        else {
            --m_multibyte_left;
        }

        ++m_current;
    }

    void _decrement_current()
    {
        --m_current;

        auto cplen = _get_cp_length_at_current();
        if (cplen == 0) {
            ++m_multibyte_left;
        }
        else {
            m_count += _get_width_at_current_cp_start(cplen);
            m_multibyte_left = cplen - 1;
        }
    }

    It m_current{};
    S m_end{};
    difference_type m_count{0};
    difference_type m_multibyte_left{0};
};

template <typename I, typename S>
counted_width_iterator(I, S, ranges_std::iter_difference_t<I>)
    -> counted_width_iterator<I, S>;
}  // namespace counted_width_iterator_impl

using counted_width_iterator_impl::counted_width_iterator;

template <typename View>
class take_width_view : public ranges::view_interface<take_width_view<View>> {
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

        constexpr explicit sentinel(underlying s) : m_end(SCN_MOVE(s)) {}

        template <typename S,
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

        friend constexpr bool operator==(const CWI& y, const sentinel& x)
        {
            return (y.count() == 0 && y.multibyte_left() == 0) ||
                   y.base() == x.m_end;
        }

        friend constexpr bool operator==(const sentinel& x, const CWI& y)
        {
            return y == x;
        }

        friend constexpr bool operator!=(const CWI& y, const sentinel& x)
        {
            return !(y == x);
        }

        friend constexpr bool operator!=(const sentinel& x, const CWI& y)
        {
            return !(y == x);
        }

    private:
        SCN_NO_UNIQUE_ADDRESS underlying m_end{};
    };

public:
    using value_type = ranges::range_value_t<View>;

    take_width_view() = default;

    constexpr take_width_view(View base, ranges::range_difference_t<View> count)
        : m_base(SCN_MOVE(base)), m_count(count)
    {
    }

    constexpr View base() const
    {
        return m_base;
    }

    template <typename V = View,
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

    template <typename V = View,
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

static_assert(
    ranges::input_range<
        take_width_view<ranges_polyfill::views::all_t<std::string_view>>>);
static_assert(
    ranges::forward_range<
        take_width_view<ranges_polyfill::views::all_t<std::string_view>>>);
static_assert(
    ranges::bidirectional_range<
        take_width_view<ranges_polyfill::views::all_t<std::string_view>>>);

template <typename R, std::enable_if_t<ranges::range<R>>* = nullptr>
take_width_view(R&&, ranges::range_difference_t<R>)
    -> take_width_view<ranges_polyfill::views::all_t<R>>;

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

#if SCN_STD_RANGES

namespace std::ranges {
template <typename R>
inline constexpr bool enable_view<::scn::impl::take_width_view<R>> = true;
template <typename R>
inline constexpr bool enable_borrowed_range<::scn::impl::take_width_view<R>> =
    enable_borrowed_range<R>;
}  // namespace std::ranges

#else

NANO_BEGIN_NAMESPACE

template <typename R>
inline constexpr bool enable_view<::scn::impl::take_width_view<R>> = true;
template <typename R>
inline constexpr bool enable_borrowed_range<::scn::impl::take_width_view<R>> =
    enable_borrowed_range<R>;

NANO_END_NAMESPACE

#endif  // SCN_STD_RANGES
