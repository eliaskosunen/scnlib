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
    static_assert(ranges::forward_iterator<It>);
    static_assert(ranges::sentinel_for<S, It>);

    template <typename OtherIt, typename OtherS>
    friend class counted_width_iterator;

public:
    using iterator = It;
    using sentinel = S;
    using value_type = ranges::iter_value_t<It>;
    using pointer = value_type*;
    using reference = value_type&;
    using difference_type = ranges::iter_difference_t<It>;
    using iterator_category =
        std::conditional_t<ranges::bidirectional_iterator<It>,
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
        -> std::enable_if_t<ranges::bidirectional_iterator<Iter>,
                            counted_width_iterator&>
    {
        _decrement_current();
        return *this;
    }

    template <typename Iter = It>
    constexpr auto operator--(int)
        -> std::enable_if_t<ranges::bidirectional_iterator<Iter>,
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
        -> decltype(SCN_DECLVAL(const It&) == SCN_DECLVAL(const OtherIt&))
    {
        return a.m_current == b.m_current;
    }
    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator!=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) == SCN_DECLVAL(const OtherIt&))
    {
        return !(a == b);
    }

    friend constexpr bool operator==(const counted_width_iterator& x,
                                     ranges::default_sentinel_t)
    {
        return x.count() == 0 && x.multibyte_left() == 0;
    }
    friend constexpr bool operator==(ranges::default_sentinel_t,
                                     const counted_width_iterator& x)
    {
        return x.count() == 0 && x.multibyte_left() == 0;
    }

    friend constexpr bool operator!=(const counted_width_iterator& a,
                                     ranges::default_sentinel_t b)
    {
        return !(a == b);
    }
    friend constexpr bool operator!=(ranges::default_sentinel_t a,
                                     const counted_width_iterator& b)
    {
        return !(a == b);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator<(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
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
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
    {
        return !(b < a);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator<=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
    {
        return !(b < a);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator>=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
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

#if 0
    friend constexpr ranges::iter_rvalue_reference_t<It>
    iter_move(const counted_width_iterator& i) noexcept(
        noexcept(ranges_impl::iter_move(i.m_current)))
    {
        return ranges_impl::iter_move(i.m_current);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto iter_swap(
        const counted_width_iterator<It, S>& a,
        const counted_width_iterator<OtherIt, OtherS>&
            b) noexcept(noexcept(ranges_impl::iter_swap(a.m_current, b.m_current)))
        -> std::enable_if_t<ranges_std::indirectly_swappable<OtherIt, It>>
    {
        ranges_impl::iter_swap(a.m_current, b.m_current);
    }
#endif

private:
    difference_type _get_cp_length_at_current() const
    {
        return static_cast<difference_type>(
            detail::code_point_length_by_starting_code_unit(*m_current));
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

        auto cp_str = std::basic_string<value_type>{m_current, *r};
        return static_cast<difference_type>(
            calculate_text_width(std::basic_string_view<value_type>{cp_str}));
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
counted_width_iterator(I, S, ranges::iter_difference_t<I>)
    -> counted_width_iterator<I, S>;
}  // namespace counted_width_iterator_impl

using counted_width_iterator_impl::counted_width_iterator;

template <typename View, typename = void>
struct take_width_view_storage;

template <typename View>
struct take_width_view_storage<View,
                               std::enable_if_t<ranges::borrowed_range<View>>> {
    take_width_view_storage(const View& v) : view(v) {}

    const View& get() const
    {
        return view;
    }

    View view;
};

template <typename View>
struct take_width_view_storage<
    View,
    std::enable_if_t<!ranges::borrowed_range<View>>> {
    take_width_view_storage(const View& v) : view(&v) {}

    const View& get() const
    {
        return *view;
    }

    const View* view;
};

template <typename View>
class take_width_view : public ranges::view_interface<take_width_view<View>> {
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

        template <
            typename S,
            std::enable_if_t<std::is_same_v<S, sentinel<!IsConst>>>* = nullptr,
            bool C = IsConst,
            typename VV = View,
            std::enable_if_t<C && std::is_convertible_v<ranges::sentinel_t<VV>,
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

    constexpr take_width_view(const View& base, std::ptrdiff_t count)
        : m_base(base), m_count(count)
    {
    }

    constexpr View base() const
    {
        return m_base;
    }

    constexpr auto begin() const
    {
        return counted_width_iterator{m_base.get().begin(), m_base.get().end(),
                                      m_count};
    }

    constexpr auto end() const
    {
        return sentinel<true>{m_base.get().end()};
    }

private:
    take_width_view_storage<View> m_base{};
    std::ptrdiff_t m_count{0};
};

template <typename R>
take_width_view(R&&, std::ptrdiff_t) -> take_width_view<R>;

struct _take_width_fn {
    template <typename R>
    constexpr auto operator()(const R& r, std::ptrdiff_t n) const
        -> decltype(take_width_view{r, n})
    {
        return take_width_view{r, n};
    }
};

inline constexpr _take_width_fn take_width{};
}  // namespace impl

namespace ranges {
template <typename R>
inline constexpr bool enable_borrowed_range<::scn::impl::take_width_view<R>> =
    enable_borrowed_range<R>;
}

SCN_END_NAMESPACE
}  // namespace scn
