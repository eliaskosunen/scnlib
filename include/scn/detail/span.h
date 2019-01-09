// Copyright 2017-2018 Elias Kosunen
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

#ifndef SCN_SPAN_H
#define SCN_SPAN_H

#include "config.h"

#include <iterator>
#include <type_traits>

namespace scn {
    template <typename T>
    class span {
    public:
        using element_type = T;
        using value_type = typename std::remove_cv<T>::type;
        using index_type = std::ptrdiff_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = T*;
        using reference = T&;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        SCN_CONSTEXPR span() = default;
        SCN_CONSTEXPR span(pointer ptr, index_type count)
            : m_ptr(ptr), m_size(count)
        {
        }
        SCN_CONSTEXPR span(pointer first, pointer last)
            : span(first, last - first)
        {
        }

        SCN_CONSTEXPR iterator begin() const noexcept
        {
            return m_ptr;
        }
        SCN_CONSTEXPR iterator end() const noexcept
        {
            return m_ptr + m_size;
        }
        SCN_CONSTEXPR reverse_iterator rbegin() const noexcept
        {
            return reverse_iterator{end()};
        }
        SCN_CONSTEXPR reverse_iterator rend() const noexcept
        {
            return reverse_iterator{begin()};
        }

        SCN_CONSTEXPR const_iterator cbegin() const noexcept
        {
            return m_ptr;
        }
        SCN_CONSTEXPR const_iterator cend() const noexcept
        {
            return m_ptr + m_size;
        }
        SCN_CONSTEXPR const_reverse_iterator crbegin() const noexcept
        {
            return reverse_iterator{cend()};
        }
        SCN_CONSTEXPR const_reverse_iterator crend() const noexcept
        {
            return reverse_iterator{cbegin()};
        }

        SCN_CONSTEXPR14 reference operator[](index_type i) const noexcept
        {
            return *(m_ptr + i);
        }

        SCN_CONSTEXPR pointer data() const noexcept
        {
            return m_ptr;
        }
        SCN_CONSTEXPR index_type size() const noexcept
        {
            return m_size;
        }

        SCN_CONSTEXPR14 span<T> subspan(index_type off) const
        {
            return span<T>(data() + off, size() - off);
        }
        SCN_CONSTEXPR14 span<T> subspan(index_type off,
                                        difference_type count) const
        {
            return span<T>(data() + off, count);
        }

    private:
        pointer m_ptr{nullptr};
        index_type m_size{0};
    };

    template <typename T>
    span<T> make_span(T* ptr, std::ptrdiff_t count)
    {
        return span<T>(ptr, count);
    }
    template <typename T>
    span<T> make_span(T* first, T* last)
    {
        return span<T>(first, last);
    }
    template <typename T>
    span<typename T::value_type> make_span(T& container)
    {
        return span<typename T::value_type>(std::begin(container),
                                            std::end(container));
    }
}  // namespace scn

#endif  // SCN_SPAN_H

