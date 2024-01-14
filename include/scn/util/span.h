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
#include <scn/util/memory.h>

#if SCN_STD_RANGES
namespace std::ranges {
template <typename T>
inline constexpr bool enable_view<scn::span<T>> = true;
template <typename T>
inline constexpr bool enable_borrowed_range<scn::span<T>> = true;
}  // namespace std::ranges
#else

NANO_BEGIN_NAMESPACE

template <typename T>
inline constexpr bool enable_view<scn::span<T>> = true;
template <typename T>
inline constexpr bool enable_borrowed_range<scn::span<T>> = true;

NANO_END_NAMESPACE

#endif

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
template <typename>
inline constexpr bool is_span = false;
template <typename T>
inline constexpr bool is_span<span<T>> = true;

template <typename>
inline constexpr bool is_std_array = false;
template <typename T, size_t N>
inline constexpr bool is_std_array<std::array<T, N>> = true;

template <typename It, typename T>
inline constexpr bool is_span_compatible_iterator =
    ranges_std::contiguous_iterator<It> &&
    std::is_convertible_v<
        std::remove_reference_t<ranges_std::iter_reference_t<It>> (*)[],
        T (*)[]>;

template <typename S, typename It>
inline constexpr bool is_span_compatible_sentinel =
    ranges_std::sized_sentinel_for<S, It> && !std::is_convertible_v<S, size_t>;

template <typename R, typename T>
inline constexpr bool is_span_compatible_range =
    !std::is_array_v<detail::remove_cvref_t<R>> &&
    !is_span<detail::remove_cvref_t<R>> &&
    !is_std_array<detail::remove_cvref_t<R>> && ranges::contiguous_range<R> &&
    ranges::sized_range<R> &&
    (ranges::borrowed_range<R> || std::is_const_v<T>)&&std::is_convertible_v<
        std::remove_reference_t<ranges::range_reference_t<R>> (*)[],
        T (*)[]>;

template <typename T, typename = size_t>
inline constexpr bool is_complete = false;
template <typename T>
inline constexpr bool is_complete<T, decltype(sizeof(T))> = true;
}  // namespace detail

/**
 * A view over a contiguous range.
 * C++23-compliant implementation of `std::span`, except it doesn't support
 * static extents.
 */
template <typename T>
class SCN_TRIVIAL_ABI span {
    static_assert(std::is_object_v<T>);
    static_assert(detail::is_complete<T>);
    static_assert(!std::is_abstract_v<T>);

public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr span() SCN_NOEXCEPT = default;

    template <typename Iterator,
              std::enable_if_t<
                  detail::is_span_compatible_iterator<Iterator,
                                                      element_type>>* = nullptr>
    constexpr span(Iterator first, size_type count) SCN_NOEXCEPT
        : m_ptr(detail::to_address(first)),
          m_end(detail::to_address(first) + count)
    {
    }
    template <
        typename Iterator,
        typename Sentinel,
        std::enable_if_t<
            detail::is_span_compatible_iterator<Iterator, element_type> &&
            detail::is_span_compatible_sentinel<Sentinel, Iterator>>* = nullptr>
    constexpr span(Iterator first, Sentinel last)
        SCN_NOEXCEPT_P(noexcept(last - first))
        : m_ptr(detail::to_address(first)), m_end(detail::to_address(last))
    {
    }

    template <size_t N>
    constexpr span(detail::type_identity_t<element_type> (&arr)[N]) SCN_NOEXCEPT
        : m_ptr(arr),
          m_end(arr + N)
    {
    }

    template <
        typename OtherT,
        size_t N,
        std::enable_if_t<
            std::is_convertible_v<OtherT (*)[], element_type (*)[]>>* = nullptr>
    constexpr span(std::array<OtherT, N>& arr) SCN_NOEXCEPT
        : m_ptr(arr.data()),
          m_end(arr.data() + N)
    {
    }
    template <
        typename OtherT,
        size_t N,
        std::enable_if_t<std::is_convertible_v<const OtherT (*)[],
                                               element_type (*)[]>>* = nullptr>
    constexpr span(const std::array<OtherT, N>& arr) SCN_NOEXCEPT
        : m_ptr(arr.data()),
          m_end(arr.data() + N)
    {
    }

    template <
        typename Range,
        std::enable_if_t<
            detail::is_span_compatible_range<Range, element_type>>* = nullptr>
    constexpr span(Range&& r)
        : m_ptr(ranges::data(r)), m_end(ranges::data(r) + ranges::size(r))
    {
    }

    template <typename OtherT,
              std::enable_if_t<std::is_convertible_v<OtherT (*)[], T (*)[]>>* =
                  nullptr>
    constexpr span(const span<OtherT>& other) SCN_NOEXCEPT
        : m_ptr(other.begin()),
          m_end(other.end())
    {
    }

    constexpr iterator begin() SCN_NOEXCEPT
    {
        return m_ptr;
    }
    constexpr iterator end() SCN_NOEXCEPT
    {
        return m_end;
    }
    constexpr reverse_iterator rbegin() SCN_NOEXCEPT
    {
        return reverse_iterator{end()};
    }
    constexpr reverse_iterator rend() SCN_NOEXCEPT
    {
        return reverse_iterator{begin()};
    }

    constexpr const_iterator begin() const SCN_NOEXCEPT
    {
        return m_ptr;
    }
    constexpr const_iterator end() const SCN_NOEXCEPT
    {
        return m_end;
    }
    constexpr const_reverse_iterator rbegin() const SCN_NOEXCEPT
    {
        return reverse_iterator{end()};
    }
    constexpr const_reverse_iterator rend() const SCN_NOEXCEPT
    {
        return reverse_iterator{begin()};
    }

    constexpr const_iterator cbegin() const SCN_NOEXCEPT
    {
        return m_ptr;
    }
    constexpr const_iterator cend() const SCN_NOEXCEPT
    {
        return m_end;
    }
    constexpr const_reverse_iterator crbegin() const SCN_NOEXCEPT
    {
        return reverse_iterator{cend()};
    }
    constexpr const_reverse_iterator crend() const SCN_NOEXCEPT
    {
        return reverse_iterator{cbegin()};
    }

    constexpr reference operator[](size_type i) const SCN_NOEXCEPT
    {
        SCN_EXPECT(size() > i);
        return *(m_ptr + i);
    }

    constexpr pointer data() const SCN_NOEXCEPT
    {
        return m_ptr;
    }
    SCN_NODISCARD constexpr size_type size() const SCN_NOEXCEPT
    {
        return static_cast<size_type>(m_end - m_ptr);
    }
    SCN_NODISCARD constexpr size_type size_bytes() const SCN_NOEXCEPT
    {
        return size() * sizeof(element_type);
    }

    constexpr span<T> first(size_t n) const
    {
        SCN_EXPECT(size() >= n);
        return span<T>(data(), data() + n);
    }
    constexpr span<T> last(size_t n) const
    {
        SCN_EXPECT(size() >= n);
        return span<T>(data() + size() - n, data() + size());
    }
    constexpr span<T> subspan(size_t off) const
    {
        SCN_EXPECT(size() >= off);
        return span<T>(data() + off, size() - off);
    }
    constexpr span<T> subspan(size_t off, difference_type count) const
    {
        SCN_EXPECT(size() > off + count);
        SCN_EXPECT(count > 0);
        return span<T>(data() + off, count);
    }

private:
    pointer m_ptr{nullptr};
    pointer m_end{nullptr};
};

template <typename T, size_t N>
span(T (&)[N]) -> span<T>;
template <typename T, size_t N>
span(std::array<T, N>&) -> span<T>;
template <typename T, size_t N>
span(const std::array<T, N>&) -> span<T>;

template <
    typename Iterator,
    typename End,
    std::enable_if_t<ranges_std::contiguous_iterator<Iterator>>* = nullptr>
span(Iterator, End)
    -> span<std::remove_reference_t<ranges_std::iter_reference_t<Iterator>>>;

template <typename Range,
          std::enable_if_t<ranges::contiguous_range<Range>>* = nullptr>
span(Range&&)
    -> span<std::remove_reference_t<ranges::range_reference_t<Range>>>;

SCN_END_NAMESPACE
}  // namespace scn
