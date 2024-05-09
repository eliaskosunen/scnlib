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

#include <scn/detail/ranges.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace ranges {

template <typename R>
using const_iterator_t = iterator_t<std::add_const_t<R>>;

// Like std::ranges::distance, utilizing .position if available
namespace detail::distance_ {
struct fn {
private:
    template <typename I, typename S>
    static constexpr auto impl(I i, S s, priority_tag<1>)
        -> decltype(s.position() - i.position())
    {
        return s.position() - i.position();
    }

    template <typename I, typename S>
    static constexpr auto impl(I i, S s, priority_tag<0>)
        -> std::enable_if_t<sized_sentinel_for<S, I>, iter_difference_t<I>>
    {
        return s - i;
    }

    template <typename I, typename S>
    static constexpr auto impl(I i, S s, priority_tag<0>)
        -> std::enable_if_t<!sized_sentinel_for<S, I>, iter_difference_t<I>>
    {
        iter_difference_t<I> counter{0};
        while (i != s) {
            ++i;
            ++counter;
        }
        return counter;
    }

public:
    template <typename I, typename S>
    constexpr auto operator()(I first, S last) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            iter_difference_t<I>>
    {
        return fn::impl(std::move(first), std::move(last), priority_tag<0>{});
    }
};
}  // namespace detail::distance_

inline constexpr auto distance = detail::distance_::fn{};

namespace detail {
template <typename I, typename = void>
struct has_batch_advance : std::false_type {};
template <typename I>
struct has_batch_advance<I,
                         std::void_t<decltype(SCN_DECLVAL(I&).batch_advance(
                             SCN_DECLVAL(std::ptrdiff_t)))>> : std::true_type {
};
}  // namespace detail

// std::advance, utilizing .batch_advance if available
namespace detail::advance_ {
struct fn {
private:
    template <typename T>
    static constexpr T abs(T t)
    {
        if (t < T{0}) {
            return -t;
        }
        return t;
    }

    template <typename I>
    static constexpr auto impl(I& i, iter_difference_t<I> n, priority_tag<1>)
        -> std::enable_if_t<has_batch_advance<I>::value>
    {
        i.batch_advance(n);
    }

    template <typename I>
    static constexpr auto impl_i_n(I& i,
                                   iter_difference_t<I> n,
                                   priority_tag<0>)
        -> std::enable_if_t<random_access_iterator<I>>
    {
        i += n;
    }

    template <typename I>
    static constexpr auto impl_i_n(I& i,
                                   iter_difference_t<I> n,
                                   priority_tag<0>)
        -> std::enable_if_t<bidirectional_iterator<I> &&
                            !random_access_iterator<I>>
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

    template <typename I>
    static constexpr auto impl_i_n(I& i,
                                   iter_difference_t<I> n,
                                   priority_tag<0>)
        -> std::enable_if_t<!bidirectional_iterator<I>>
    {
        while (n-- > iter_difference_t<I>{0}) {
            ++i;
        }
    }

    template <typename I, typename S>
    static constexpr auto impl_i_s(I& i, S bound, priority_tag<2>)
        -> std::enable_if_t<std::is_assignable_v<I&, S>>
    {
        i = std::move(bound);
    }

    template <typename I, typename S>
    static constexpr auto impl_i_s(I& i, S bound, priority_tag<1>)
        -> std::enable_if_t<sized_sentinel_for<S, I>>
    {
        fn::impl_i_n(i, bound - i);
    }

    template <typename I, typename S>
    static constexpr void impl_i_s(I& i, S bound, priority_tag<0>)
    {
        while (i != bound) {
            ++i;
        }
    }

    template <typename I, typename S>
    static constexpr auto impl_i_n_s(I& i, iter_difference_t<I> n, S bound)
        -> std::enable_if_t<sized_sentinel_for<S, I>, iter_difference_t<I>>
    {
        if (fn::abs(n) >= fn::abs(bound - i)) {
            auto dist = bound - i;
            fn::impl_i_s(i, bound, priority_tag<2>{});
            return dist;
        }
        fn::impl_i_n(i, n, priority_tag<1>{});
        return n;
    }

    template <typename I, typename S>
    static constexpr auto impl_i_n_s(I& i, iter_difference_t<I> n, S bound)
        -> std::enable_if_t<bidirectional_iterator<I> &&
                                !sized_sentinel_for<S, I>,
                            iter_difference_t<I>>
    {
        constexpr iter_difference_t<I> zero{0};
        iter_difference_t<I> counter{0};

        if (n < zero) {
            do {
                --i;
                --counter;  // Yes, really
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

    template <typename I, typename S>
    static constexpr auto impl_i_n_s(I& i, iter_difference_t<I> n, S bound)
        -> std::enable_if_t<!bidirectional_iterator<I> &&
                                !sized_sentinel_for<S, I>,
                            iter_difference_t<I>>
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
    constexpr auto operator()(I& i, iter_difference_t<I> n) const
        -> std::enable_if_t<input_or_output_iterator<I>>
    {
        fn::impl_i_n(i, n, detail::priority_tag<1>{});
    }

    template <typename I, typename S>
    constexpr auto operator()(I& i, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>>
    {
        fn::impl_i_s(i, bound, priority_tag<2>{});
    }

    template <typename I, typename S>
    constexpr auto operator()(I& i, iter_difference_t<I> n, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            iter_difference_t<I>>
    {
        return n - fn::impl_i_n_s(i, n, bound);
    }
};
}  // namespace detail::advance_

inline constexpr auto advance = detail::advance_::fn{};

namespace next_impl {
struct fn {
    template <typename I>
    constexpr auto operator()(I x) const
        -> std::enable_if_t<input_or_output_iterator<I>, I>
    {
        ++x;
        return x;
    }

    template <typename I>
    constexpr auto operator()(I x, iter_difference_t<I> n) const
        -> std::enable_if_t<input_or_output_iterator<I>, I>
    {
        ranges::advance(x, n);
        return x;
    }

    template <typename I, typename S>
    constexpr auto operator()(I x, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            I>
    {
        ranges::advance(x, bound);
        return x;
    }

    template <typename I, typename S>
    constexpr auto operator()(I x, iter_difference_t<I> n, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            I>
    {
        ranges::advance(x, n, bound);
        return x;
    }
};
}  // namespace next_impl

inline constexpr next_impl::fn next{};

// prev, for forward_iterators
namespace detail::prev_backtrack_ {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It it, It, priority_tag<2>)
        -> std::enable_if_t<bidirectional_iterator<It>, It>
    {
        --it;
        return it;
    }

    template <typename It>
    static constexpr auto impl(It it, It beg, priority_tag<1>)
        -> remove_cvref_t<decltype((void)beg.batch_advance(42), it)>
    {
        return beg.batch_advance(it.position() - 1);
    }

    template <typename It>
    static constexpr auto impl(It it, It beg, priority_tag<0>)
        -> std::enable_if_t<forward_iterator<It>, It>
    {
        SCN_EXPECT(it != beg);

        while (true) {
            auto tmp = beg;
            ++beg;
            if (beg == it) {
                return tmp;
            }
        }
    }

public:
    template <typename It>
    constexpr auto operator()(It it, It beg) const
        -> decltype(fn::impl(it, beg, priority_tag<2>{}))
    {
        return fn::impl(it, beg, priority_tag<2>{});
    }
};
}  // namespace detail::prev_backtrack_

inline constexpr auto prev_backtrack = detail::prev_backtrack_::fn{};

// operator<, for forward_iterators
namespace detail::less_backtrack_ {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It, priority_tag<2>)
        -> decltype(static_cast<void>(lhs < rhs), true)
    {
        return lhs < rhs;
    }

    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It, priority_tag<1>)
        -> decltype(static_cast<void>(lhs.position() < rhs.position()), true)
    {
        return lhs.position() < rhs.position();
    }

    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It beg, priority_tag<0>)
        -> std::enable_if_t<ranges::forward_iterator<It>, bool>
    {
        while (true) {
            if (beg == rhs) {
                return false;
            }
            if (beg == lhs) {
                return true;
            }
            ++beg;
        }
    }

public:
    template <typename It>
    constexpr auto operator()(It lhs, It rhs, It beg) const
        -> decltype(fn::impl(lhs, rhs, beg, priority_tag<2>{}))
    {
        return fn::impl(lhs, rhs, beg, priority_tag<2>{});
    }
};
}  // namespace detail::less_backtrack_

inline constexpr auto less_backtrack = detail::less_backtrack_::fn{};

}  // namespace ranges

SCN_END_NAMESPACE
}  // namespace scn
