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

#include <scn/util/memory.h>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wnoexcept")
#include <iterator>
SCN_GCC_POP

namespace scn {
    SCN_BEGIN_NAMESPACE

    /**
     * A view over a contiguous range.
     * Stripped-down version of `std::span`.
     */
    template <typename T>
    class SCN_TRIVIAL_ABI span {
    public:
        using element_type = T;
        using value_type = std::remove_cv_t<T>;
        using index_type = std::size_t;
        using ssize_type = std::ptrdiff_t;
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

        template <typename I,
                  typename = decltype(detail::to_address(SCN_DECLVAL(I)))>
        constexpr span(I begin, index_type count) SCN_NOEXCEPT
            : m_ptr(detail::to_address(begin)),
              m_end(detail::to_address(begin) + count)
        {
        }

        template <typename I,
                  typename S,
                  typename = decltype(detail::to_address(SCN_DECLVAL(I)),
                                      detail::to_address(SCN_DECLVAL(S)))>
        constexpr span(I first, S last) SCN_NOEXCEPT
            : m_ptr(detail::to_address(first)),
              m_end(detail::to_address_safe(last, first, last))
        {
        }

        template <typename U = std::add_const_t<T>,
                  typename E = element_type,
                  typename = std::enable_if_t<std::is_same_v<E, value_type>>>
        constexpr span(span<U> other) : m_ptr(other.m_ptr), m_end(other.m_end)
        {
        }

        template <size_t N>
        constexpr span(element_type (&arr)[N]) SCN_NOEXCEPT : m_ptr(&arr),
                                                              m_end(&arr + N)
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

        constexpr reference operator[](index_type i) const SCN_NOEXCEPT
        {
            SCN_EXPECT(size() > i);
            return *(m_ptr + i);
        }

        constexpr pointer data() const SCN_NOEXCEPT
        {
            return m_ptr;
        }
        SCN_NODISCARD constexpr index_type size() const SCN_NOEXCEPT
        {
            return static_cast<index_type>(m_end - m_ptr);
        }
        SCN_NODISCARD constexpr ssize_type ssize() const SCN_NOEXCEPT
        {
            return m_end - m_ptr;
        }

        constexpr span<T> first(index_type n) const
        {
            SCN_EXPECT(size() >= n);
            return span<T>(data(), data() + n);
        }
        constexpr span<T> last(index_type n) const
        {
            SCN_EXPECT(size() >= n);
            return span<T>(data() + size() - n, data() + size());
        }
        constexpr span<T> subspan(index_type off) const
        {
            SCN_EXPECT(size() >= off);
            return span<T>(data() + off, size() - off);
        }
        constexpr span<T> subspan(index_type off, difference_type count) const
        {
            SCN_EXPECT(size() > off + count);
            SCN_EXPECT(count > 0);
            return span<T>(data() + off, count);
        }

        constexpr operator span<std::add_const_t<T>>() const
        {
            return {m_ptr, m_end};
        }
        constexpr span<std::add_const_t<T>> as_const() const
        {
            return {m_ptr, m_end};
        }

    private:
        pointer m_ptr{nullptr};
        pointer m_end{nullptr};
    };

    template <
        typename I,
        typename S,
        typename Ptr = decltype(detail::to_address(SCN_DECLVAL(I))),
        typename SPtr = decltype(detail::to_address(SCN_DECLVAL(S))),
        typename ValueT = std::remove_reference_t<std::remove_pointer_t<Ptr>>>
    constexpr auto make_span(I first, S last) SCN_NOEXCEPT->span<ValueT>
    {
        return {first, last};
    }
    template <
        typename I,
        typename Ptr = decltype(detail::to_address(SCN_DECLVAL(I))),
        typename ValueT = std::remove_reference_t<std::remove_pointer_t<Ptr>>>
    constexpr auto make_span(I first,
                             std::size_t len) SCN_NOEXCEPT->span<ValueT>
    {
        return {first, len};
    }

    template <typename T>
    constexpr span<typename T::value_type> make_span(T& container) SCN_NOEXCEPT
    {
        using std::begin;
        using std::end;
        return span<typename T::value_type>(
            detail::to_address(begin(container)),
            detail::to_address_safe(end(container), begin(container),
                                    end(container)));
    }

    SCN_END_NAMESPACE
}  // namespace scn
