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

namespace ranges = std::ranges;
namespace ranges_std = std;

namespace r_pf {
template <typename Range>
using owning_view = ranges::owning_view<Range>;

namespace views {
inline constexpr auto& all = ranges::views::all;

template <typename Range>
using all_t = ranges::views::all_t<Range>;
}  // namespace views

template <typename R>
concept simple_view =
    ranges::view<R> && ranges::range<const R> &&
    std::same_as<ranges::iterator_t<R>, ranges::iterator_t<const R>> &&
    std::same_as<ranges::sentinel_t<R>, ranges::sentinel_t<const R>>;
}  // namespace r_pf

SCN_END_NAMESPACE
}  // namespace scn

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

#include <scn/external/nanorange/nanorange.hpp>

SCN_CLANG_POP

SCN_GCC_POP

namespace scn {
SCN_BEGIN_NAMESPACE

namespace ranges = nano;
namespace ranges_std = nano;

namespace r_pf {
template <typename T>
struct _is_initializer_list : std::false_type {};
template <typename T>
struct _is_initializer_list<std::initializer_list<T>> : std::true_type {};

template <typename Range,
          typename = std::enable_if_t<scn::ranges::range<Range> &&
                                      scn::ranges_std::movable<Range> &&
                                      !_is_initializer_list<Range>::value>>
class owning_view : public scn::ranges::view_interface<owning_view<Range>> {
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

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wnoexcept")

    constexpr scn::ranges::iterator_t<Range> begin()
        SCN_NOEXCEPT_P(noexcept(scn::ranges::begin(SCN_DECLVAL(Range&))))
    {
        return scn::ranges::begin(m_range);
    }
    constexpr scn::ranges::sentinel_t<Range> end()
        SCN_NOEXCEPT_P(noexcept(scn::ranges::end(SCN_DECLVAL(Range&))))
    {
        return scn::ranges::end(m_range);
    }

    template <typename R = Range,
              std::enable_if_t<scn::ranges::range<const R>>* = nullptr>
    constexpr auto begin() const
        SCN_NOEXCEPT_P(noexcept(scn::ranges::begin(SCN_DECLVAL(const R&))))
    {
        return scn::ranges::begin(m_range);
    }
    template <typename R = Range,
              std::enable_if_t<scn::ranges::range<const R>>* = nullptr>
    constexpr auto end() const
        SCN_NOEXCEPT_P(noexcept(scn::ranges::end(SCN_DECLVAL(const R&))))
    {
        return scn::ranges::end(m_range);
    }

    template <
        typename R = Range,
        std::void_t<decltype(scn::ranges::empty(SCN_DECLVAL(R&)))>* = nullptr>
    constexpr bool empty()
        SCN_NOEXCEPT_P(noexcept(scn::ranges::empty(SCN_DECLVAL(R&))))
    {
        return scn::ranges::empty(m_range);
    }
    template <typename R = Range,
              std::void_t<decltype(scn::ranges::empty(
                  SCN_DECLVAL(const R&)))>* = nullptr>
    constexpr bool empty() const
        SCN_NOEXCEPT_P(noexcept(scn::ranges::empty(SCN_DECLVAL(const R&))))
    {
        return scn::ranges::empty(m_range);
    }

    template <
        typename R = Range,
        std::void_t<decltype(scn::ranges::size(SCN_DECLVAL(R&)))>* = nullptr>
    constexpr auto size()
        SCN_NOEXCEPT_P(noexcept(scn::ranges::size(SCN_DECLVAL(R&))))
    {
        return scn::ranges::size(m_range);
    }
    template <typename R = Range,
              std::void_t<decltype(scn::ranges::size(SCN_DECLVAL(const R&)))>* =
                  nullptr>
    constexpr auto size() const
        SCN_NOEXCEPT_P(noexcept(scn::ranges::size(SCN_DECLVAL(const R&))))
    {
        return scn::ranges::size(m_range);
    }

    template <
        typename R = Range,
        std::void_t<decltype(scn::ranges::data(SCN_DECLVAL(R&)))>* = nullptr>
    constexpr auto data()
        SCN_NOEXCEPT_P(noexcept(scn::ranges::data(SCN_DECLVAL(R&))))
    {
        return scn::ranges::data(m_range);
    }
    template <typename R = Range,
              std::void_t<decltype(scn::ranges::data(SCN_DECLVAL(const R&)))>* =
                  nullptr>
    constexpr auto data() const
        SCN_NOEXCEPT_P(noexcept(scn::ranges::data(SCN_DECLVAL(const R&))))
    {
        return scn::ranges::data(m_range);
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
    template <
        typename Range,
        std::enable_if_t<scn::ranges::view<std::decay_t<Range>>>* = nullptr>
    static constexpr auto impl(Range&& r, scn::detail::priority_tag<2>)
        SCN_NOEXCEPT_P(
            std::is_nothrow_constructible_v<std::decay_t<Range>, Range>)
    {
        return SCN_FWD(r);
    }

    template <typename Range>
    static constexpr auto impl(Range&& r, scn::detail::priority_tag<1>)
        SCN_NOEXCEPT->decltype(scn::ranges::ref_view{SCN_FWD(r)})
    {
        return scn::ranges::ref_view{SCN_FWD(r)};
    }

    template <typename Range>
    static constexpr auto impl(Range&& r, scn::detail::priority_tag<0>)
        SCN_NOEXCEPT_P(noexcept(scn::ranges_polyfill::owning_view{SCN_FWD(r)}))
            -> decltype(scn::ranges_polyfill::owning_view{SCN_FWD(r)})
    {
        return scn::ranges_polyfill::owning_view{SCN_FWD(r)};
    }

public:
    template <typename Range>
    constexpr auto operator()(Range&& r) const SCN_NOEXCEPT_P(
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

SCN_END_NAMESPACE
}  // namespace scn

NANO_BEGIN_NAMESPACE

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

namespace detail {
namespace simple_iterator_t_fn {
template <typename T>
constexpr auto impl()
{
    return tag_type<decltype(ranges::begin(SCN_DECLVAL(T&)))>{};
}

template <typename Range>
using result = decltype(impl<Range>());
}  // namespace simple_iterator_t_fn

template <typename Range>
using simple_iterator_t = typename simple_iterator_t_fn::result<Range>::type;

namespace char_t_fn {
template <typename T, typename = typename T::value_type>
constexpr auto impl_nonarray(priority_tag<2>)
{
    return tag_type<typename T::value_type>{};
}
template <typename T, typename = decltype(SCN_DECLVAL(T&).begin())>
constexpr auto impl_nonarray(priority_tag<1>)
{
    return tag_type<remove_cvref_t<decltype(*(SCN_DECLVAL(T&).begin()))>>{};
}
template <typename T>
constexpr auto impl_nonarray(priority_tag<0>)
{
    return tag_type<remove_cvref_t<decltype(*begin(SCN_DECLVAL(T&)))>>{};
}

template <typename T,
          typename = std::enable_if_t<ranges::range<remove_cvref_t<T>>>>
constexpr auto impl()
{
    using T_nocvref = remove_cvref_t<T>;
    if constexpr (std::is_array_v<T_nocvref>) {
        return tag_type<std::remove_all_extents_t<T_nocvref>>{};
    }
    else {
        return impl_nonarray<T_nocvref>(priority_tag<2>{});
    }
}

template <typename Range>
using result = decltype(impl<Range>());
}  // namespace char_t_fn

template <typename Range>
using char_t = typename char_t_fn::result<Range>::type;

template <typename Range, typename = void>
inline constexpr bool is_file_or_narrow_range_impl = false;
template <>
inline constexpr bool is_file_or_narrow_range_impl<std::FILE*, void> = true;
template <typename Range>
inline constexpr bool
    is_file_or_narrow_range_impl<Range,
                                 std::enable_if_t<ranges::range<Range>>> =
        std::is_same_v<char_t<Range>, char>;

template <typename Range>
inline constexpr bool is_file_or_narrow_range =
    is_file_or_narrow_range_impl<remove_cvref_t<Range>>;

template <typename Range, typename = void>
inline constexpr bool is_wide_range = false;
template <typename Range>
inline constexpr bool
    is_wide_range<Range,
                  std::enable_if_t<ranges::range<remove_cvref_t<Range>>>> =
        std::is_same_v<char_t<Range>, wchar_t>;
}  // namespace detail

// borrowed_iterator_t and borrowed_subrange_t, but shorter template
// names

template <typename R, bool Borrowed = ranges::borrowed_range<R>>
struct simple_borrowed_iterator {
    using type = detail::simple_iterator_t<R>;
};
template <typename R>
struct simple_borrowed_iterator<R, false> {
    using type = std::conditional_t<
        std::is_same_v<detail::remove_cvref_t<R>, std::FILE*>,
        std::FILE*,
        ranges::dangling>;
};

template <typename R>
using simple_borrowed_iterator_t = typename simple_borrowed_iterator<R>::type;

template <typename R, bool Borrowed = ranges::borrowed_range<R>>
struct simple_borrowed_subrange {
    using type = ranges::subrange<detail::simple_iterator_t<R>>;
};
template <typename R>
struct simple_borrowed_subrange<R, false> {
    using type = std::conditional_t<
        std::is_same_v<detail::remove_cvref_t<R>, std::FILE*>,
        std::FILE*,
        ranges::dangling>;
};

template <typename R>
using simple_borrowed_subrange_t = typename simple_borrowed_subrange<R>::type;

template <typename R, bool Borrowed = ranges::borrowed_range<R>>
struct borrowed_subrange_with_sentinel {
    using type = ranges::subrange<ranges::iterator_t<R>, ranges::sentinel_t<R>>;
};
template <typename R>
struct borrowed_subrange_with_sentinel<R, false> {
    using type = ranges::dangling;
};

/// Equivalent to
/// `ranges::subrange<ranges::iterator_t<R>, ranges::sentinel_t<R>>`
/// if `R` is a `borrowed_range`, and `ranges::dangling` otherwise.
///
/// Similar to `ranges::borrowed_subrange_t<R>`, expect this preserves
/// the range sentinel.
template <typename R>
using borrowed_subrange_with_sentinel_t =
    typename borrowed_subrange_with_sentinel<R>::type;

namespace r_pf {

// Necessary, because nanorange subrange::size() returns a signed type
namespace usize_impl {
SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
struct fn {
private:
    template <typename T>
    using usize_return_t =
        std::conditional_t<sizeof(ranges::range_difference_t<T>) <
                               sizeof(std::ptrdiff_t),
                           std::size_t,
                           std::make_unsigned_t<ranges::range_difference_t<T>>>;

    template <typename T>
    static constexpr auto impl(T&& t)
        SCN_NOEXCEPT_P(noexcept(ranges::size(SCN_FWD(t))))
            -> decltype(ranges::size(SCN_FWD(t)), usize_return_t<T>())
    {
        return static_cast<usize_return_t<T>>(ranges::size(SCN_FWD(t)));
    }

public:
    template <typename T>
    constexpr auto operator()(T&& t) const
        SCN_NOEXCEPT_P(noexcept(fn::impl(SCN_FWD(t))))
            -> decltype(fn::impl(SCN_FWD(t)))
    {
        return fn::impl(SCN_FWD(t));
    }
};
SCN_GCC_POP
}  // namespace usize_impl

inline constexpr usize_impl::fn usize{};

// ranges::next, utilizing .batch_advance if available
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
        return ranges::next(it, n);
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

// ranges::distance, utilizing .position if available
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
        -> decltype(ranges::distance(lhs, rhs))
    {
        return ranges::distance(lhs, rhs);
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
        return ranges::prev(it);
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
