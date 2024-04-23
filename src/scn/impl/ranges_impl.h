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

namespace r_pf {
}
namespace ranges_polyfill = r_pf;

SCN_END_NAMESPACE
}  // namespace scn

#if SCN_STD_RANGES

#include <concepts>
#include <ranges>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace ranges_impl = std::ranges;
namespace ranges_std = std;

namespace r_pf {
template <typename Range>
using owning_view = ranges_impl::owning_view<Range>;

namespace views {
inline constexpr auto& all = ranges_impl::views::all;

template <typename Range>
using all_t = ranges_impl::views::all_t<Range>;
}  // namespace views

template <typename R>
concept simple_view =
    ranges_impl::view<R> && ranges_impl::range<const R> &&
    std::same_as<ranges_impl::iterator_t<R>,
                 ranges_impl::iterator_t<const R>> &&
    std::same_as<ranges_impl::sentinel_t<R>, ranges_impl::sentinel_t<const R>>;
}  // namespace r_pf

namespace ranges {
template <typename I, typename S>
inline constexpr bool enable_borrowed_range<std::ranges::subrange<I, S>> = true;
}

SCN_END_NAMESPACE
}  // namespace scn

namespace std::ranges {
template <typename I, typename S>
inline constexpr bool enable_borrowed_range<scn::ranges::subrange<I, S>> = true;
}

#else

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
SCN_GCC_IGNORE("-Wconversion")
SCN_GCC_IGNORE("-Wsign-conversion")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wshadow")
SCN_CLANG_IGNORE("-Wredundant-parens")
SCN_CLANG_IGNORE("-Wzero-as-null-pointer-constant")
SCN_CLANG_IGNORE("-Wmismatched-tags")
SCN_CLANG_IGNORE("-Wmissing-variable-declarations")

#if SCN_CLANG >= SCN_COMPILER(16, 0, 0)
SCN_CLANG_IGNORE("-Wunsafe-buffer-usage")
#endif

#if SCN_CLANG >= SCN_COMPILER(9, 0, 0)
SCN_CLANG_IGNORE("-Wctad-maybe-unsupported")
#endif

#if SCN_CLANG >= SCN_COMPILER(8, 0, 0)
SCN_CLANG_IGNORE("-Wimplicit-int-conversion")
#endif

#define NANORANGE_NO_STD_FORWARD_DECLARATIONS

#include <scn/impl/external/nanorange/nanorange.hpp>

SCN_CLANG_POP

SCN_GCC_POP

namespace scn {
SCN_BEGIN_NAMESPACE

namespace ranges_impl = nano;
namespace ranges_std = nano;

namespace r_pf {
template <typename T>
struct _is_initializer_list : std::false_type {
};
template <typename T>
struct _is_initializer_list<std::initializer_list<T>> : std::true_type {
};

template <typename Range,
          typename = std::enable_if_t<scn::ranges_impl::range<Range> &&
                                      scn::ranges_std::movable<Range> &&
                                      !_is_initializer_list<Range>::value>>
class owning_view
    : public scn::ranges_impl::view_interface<owning_view<Range>> {
public:
    owning_view() = default;

    constexpr owning_view(Range&& r) noexcept(
        std::is_nothrow_move_constructible_v<Range>)
        : m_range(SCN_MOVE(r))
    {
    }

    owning_view(owning_view&&) = default;
    owning_view& operator=(owning_view&&) = default;

    owning_view(const owning_view&) = delete;
    owning_view& operator=(const owning_view&) = delete;

    ~owning_view() = default;

    constexpr Range& base() & noexcept
    {
        return m_range;
    }
    constexpr const Range& base() const& noexcept
    {
        return m_range;
    }
    constexpr Range&& base() && noexcept
    {
        return SCN_MOVE(m_range);
    }
    constexpr const Range&& base() const&& noexcept
    {
        return SCN_MOVE(m_range);
    }

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wnoexcept")

    constexpr scn::ranges_impl::iterator_t<Range> begin() noexcept(
        noexcept(scn::ranges_impl::begin(SCN_DECLVAL(Range&))))
    {
        return scn::ranges_impl::begin(m_range);
    }
    constexpr scn::ranges_impl::sentinel_t<Range> end() noexcept(
        noexcept(scn::ranges_impl::end(SCN_DECLVAL(Range&))))
    {
        return scn::ranges_impl::end(m_range);
    }

    template <typename R = Range,
              std::enable_if_t<scn::ranges_impl::range<const R>>* = nullptr>
    constexpr auto begin() const
        noexcept(noexcept(scn::ranges_impl::begin(SCN_DECLVAL(const R&))))
    {
        return scn::ranges_impl::begin(m_range);
    }
    template <typename R = Range,
              std::enable_if_t<scn::ranges_impl::range<const R>>* = nullptr>
    constexpr auto end() const
        noexcept(noexcept(scn::ranges_impl::end(SCN_DECLVAL(const R&))))
    {
        return scn::ranges_impl::end(m_range);
    }

    template <typename R = Range,
              std::void_t<decltype(scn::ranges_impl::empty(SCN_DECLVAL(R&)))>* =
                  nullptr>
    constexpr bool empty() noexcept(
        noexcept(scn::ranges_impl::empty(SCN_DECLVAL(R&))))
    {
        return scn::ranges_impl::empty(m_range);
    }
    template <typename R = Range,
              std::void_t<decltype(scn::ranges_impl::empty(
                  SCN_DECLVAL(const R&)))>* = nullptr>
    constexpr bool empty() const
        noexcept(noexcept(scn::ranges_impl::empty(SCN_DECLVAL(const R&))))
    {
        return scn::ranges_impl::empty(m_range);
    }

    template <typename R = Range,
              std::void_t<decltype(scn::ranges_impl::size(SCN_DECLVAL(R&)))>* =
                  nullptr>
    constexpr auto size() noexcept(
        noexcept(scn::ranges_impl::size(SCN_DECLVAL(R&))))
    {
        return scn::ranges_impl::size(m_range);
    }
    template <typename R = Range,
              std::void_t<decltype(scn::ranges_impl::size(
                  SCN_DECLVAL(const R&)))>* = nullptr>
    constexpr auto size() const
        noexcept(noexcept(scn::ranges_impl::size(SCN_DECLVAL(const R&))))
    {
        return scn::ranges_impl::size(m_range);
    }

    template <typename R = Range,
              std::void_t<decltype(scn::ranges_impl::data(SCN_DECLVAL(R&)))>* =
                  nullptr>
    constexpr auto data() noexcept(
        noexcept(scn::ranges_impl::data(SCN_DECLVAL(R&))))
    {
        return scn::ranges_impl::data(m_range);
    }
    template <typename R = Range,
              std::void_t<decltype(scn::ranges_impl::data(
                  SCN_DECLVAL(const R&)))>* = nullptr>
    constexpr auto data() const
        noexcept(noexcept(scn::ranges_impl::data(SCN_DECLVAL(const R&))))
    {
        return scn::ranges_impl::data(m_range);
    }

private:
    Range m_range = Range();

    SCN_GCC_POP  // -Wnoexcept
};

template <typename Range>
owning_view(Range) -> owning_view<Range>;

namespace views {
struct _all_fn {
private:
    template <typename Range,
              std::enable_if_t<scn::ranges_impl::view<std::decay_t<Range>>>* =
                  nullptr>
    static constexpr auto
    impl(Range&& r, scn::detail::priority_tag<2>) noexcept(
        std::is_nothrow_constructible_v<std::decay_t<Range>, Range>)
    {
        return SCN_FWD(r);
    }

    template <typename Range>
    static constexpr auto impl(Range&& r, scn::detail::priority_tag<1>) noexcept
        -> decltype(scn::ranges_impl::ref_view{SCN_FWD(r)})
    {
        return scn::ranges_impl::ref_view{SCN_FWD(r)};
    }

    template <typename Range>
    static constexpr auto
    impl(Range&& r, scn::detail::priority_tag<0>) noexcept(
        noexcept(scn::ranges_polyfill::owning_view{SCN_FWD(r)}))
        -> decltype(scn::ranges_polyfill::owning_view{SCN_FWD(r)})
    {
        return scn::ranges_polyfill::owning_view{SCN_FWD(r)};
    }

public:
    template <typename Range>
    constexpr auto operator()(Range&& r) const noexcept(
        noexcept(_all_fn::impl(SCN_FWD(r), scn::detail::priority_tag<2>{})))
        -> decltype(_all_fn::impl(SCN_FWD(r), scn::detail::priority_tag<2>{}))
    {
        return _all_fn::impl(SCN_FWD(r), scn::detail::priority_tag<2>{});
    }
};

inline constexpr _all_fn all{};

template <typename Range>
using all_t = decltype(all(SCN_DECLVAL(Range)));
}  // namespace views

template <typename R>
inline constexpr bool simple_view = nano::detail::simple_view<R>;
}  // namespace r_pf

namespace ranges {
template <typename I, typename S>
inline constexpr bool enable_borrowed_range<nano::subrange<I, S>> = true;
}

SCN_END_NAMESPACE
}  // namespace scn

NANO_BEGIN_NAMESPACE

template <typename I, typename S>
inline constexpr bool enable_borrowed_range<scn::ranges::subrange<I, S>> = true;

template <typename T>
inline constexpr bool
    enable_borrowed_range<scn::ranges_polyfill::owning_view<T>> =
        enable_borrowed_range<T>;

namespace detail {
template <>
struct iterator_category_<std::string::iterator, void> {
    using type = contiguous_iterator_tag;
};
template <>
struct iterator_category_<std::wstring::iterator, void>
    : contiguous_iterator_tag {
    using type = contiguous_iterator_tag;
};

template <>
struct iterator_category_<std::string_view::iterator, void>
    : contiguous_iterator_tag {
    using type = contiguous_iterator_tag;
};
template <>
struct iterator_category_<std::wstring_view::iterator, void>
    : contiguous_iterator_tag {
    using type = contiguous_iterator_tag;
};
}  // namespace detail

NANO_END_NAMESPACE

#endif  // SCN_STD_RANGES

namespace scn {
SCN_BEGIN_NAMESPACE

namespace r_pf {

// Necessary, because nanorange subrange::size() returns a signed type
namespace usize_impl {
SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
struct fn {
private:
    template <typename T>
    using usize_return_t = std::conditional_t<
        sizeof(ranges_impl::range_difference_t<T>) < sizeof(std::ptrdiff_t),
        std::size_t,
        std::make_unsigned_t<ranges_impl::range_difference_t<T>>>;

    template <typename T>
    static constexpr auto impl(T&& t) noexcept(
        noexcept(ranges_impl::size(SCN_FWD(t))))
        -> decltype(ranges_impl::size(SCN_FWD(t)), usize_return_t<T>())
    {
        return static_cast<usize_return_t<T>>(ranges_impl::size(SCN_FWD(t)));
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        noexcept(noexcept(fn::impl(SCN_FWD(t))))
            -> decltype(fn::impl(SCN_FWD(t)))
    {
        return fn::impl(SCN_FWD(t));
    }
};
SCN_GCC_POP
}  // namespace usize_impl

inline constexpr usize_impl::fn usize{};

// ranges_impl::next, utilizing .batch_advance if available
namespace batch_next_impl {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It it, std::ptrdiff_t n, detail::priority_tag<1>)
        -> detail::remove_cvref_t<decltype(it.batch_advance(n), it)>
    {
        it.batch_advance(n);
        return it;
    }

    template <typename It>
    static constexpr auto impl(It it, std::ptrdiff_t n, detail::priority_tag<0>)
    {
        return ranges_impl::next(it, n);
    }

public:
    template <typename It>
    constexpr auto operator()(It it, std::ptrdiff_t n) const
        -> decltype(fn::impl(it, n, detail::priority_tag<1>{}))
    {
        return fn::impl(it, n, detail::priority_tag<1>{});
    }
};
}  // namespace batch_next_impl

inline constexpr batch_next_impl::fn batch_next{};

template <typename It>
void batch_advance(It& it, std::ptrdiff_t n)
{
    it = batch_next(it, n);
}

// ranges_impl::distance, utilizing .position if available
namespace pos_distance_impl {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It lhs, It rhs, detail::priority_tag<1>)
        -> detail::remove_cvref_t<decltype(rhs.position() - lhs.position())>
    {
        return rhs.position() - lhs.position();
    }

    template <typename Lhs, typename Rhs>
    static constexpr auto impl(Lhs lhs, Rhs rhs, detail::priority_tag<0>)
        -> decltype(ranges_impl::distance(lhs, rhs))
    {
        return ranges_impl::distance(lhs, rhs);
    }

public:
    template <typename Lhs, typename Rhs>
    constexpr auto operator()(Lhs lhs, Rhs rhs) const
        -> decltype(fn::impl(lhs, rhs, detail::priority_tag<1>{}))
    {
        return fn::impl(lhs, rhs, detail::priority_tag<1>{});
    }
};
}  // namespace pos_distance_impl

inline constexpr pos_distance_impl::fn pos_distance{};

// prev, for forward_iterators
namespace prev_backtrack_impl {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It it, It, detail::priority_tag<2>)
        -> std::enable_if_t<ranges_std::bidirectional_iterator<It>, It>
    {
        return ranges_impl::prev(it);
    }

    template <typename It>
    static constexpr auto impl(It it, It beg, detail::priority_tag<1>)
        -> detail::remove_cvref_t<decltype((void)beg.batch_advance(42), it)>
    {
        return beg.batch_advance(it.position() - 1);
    }

    template <typename It>
    static constexpr auto impl(It it, It beg, detail::priority_tag<0>)
        -> std::enable_if_t<ranges_std::forward_iterator<It>, It>
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
        -> decltype(fn::impl(it, beg, detail::priority_tag<2>{}))
    {
        return fn::impl(it, beg, detail::priority_tag<2>{});
    }
};
}  // namespace prev_backtrack_impl

inline constexpr prev_backtrack_impl::fn prev_backtrack{};

// operator<, for forward_iterators
namespace less_backtrack_impl {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It, detail::priority_tag<2>)
        -> decltype(static_cast<void>(lhs < rhs), true)
    {
        return lhs < rhs;
    }

    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It, detail::priority_tag<1>)
        -> decltype(static_cast<void>(lhs.position() < rhs.position()), true)
    {
        return lhs.position() < rhs.position();
    }

    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It beg, detail::priority_tag<0>)
        -> std::enable_if_t<ranges_std::forward_iterator<It>, bool>
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
        -> decltype(fn::impl(lhs, rhs, beg, detail::priority_tag<2>{}))
    {
        return fn::impl(lhs, rhs, beg, detail::priority_tag<2>{});
    }
};
}  // namespace less_backtrack_impl

inline constexpr less_backtrack_impl::fn less_backtrack{};
}  // namespace r_pf

SCN_END_NAMESPACE
}  // namespace scn
