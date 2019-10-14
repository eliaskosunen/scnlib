// Copyright 2017-2019 Elias Kosunen
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

#ifndef SCN_DETAIL_SPAN_H
#define SCN_DETAIL_SPAN_H

#include "config.h"

#include <iterator>

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename T>
    class span;

    namespace detail {
        namespace ranges {
            // iterator_category
            using std::bidirectional_iterator_tag;
            using std::forward_iterator_tag;
            using std::input_iterator_tag;
            using std::output_iterator_tag;
            using std::random_access_iterator_tag;
            struct contiguous_iterator_tag : random_access_iterator_tag {
            };
        }  // namespace ranges
#if 0
        template <typename Element>
        class span_iterator {
        public:
            using element_type = Element;
            using value_type = typename std::remove_cv<element_type>::type;
            using pointer = Element*;
            using const_pointer = const Element*;
            using reference = Element&;
            using const_reference = const Element&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = ranges::contiguous_iterator_tag;

            span_iterator() = default;

            template <typename S>
            span_iterator(const S& s, pointer p)
                : m_begin(s.data()), m_end(s.data() + s.size()), m_elem(p)
            {
            }
            template <typename S>
            span_iterator(const S& s, difference_type n)
                : m_begin(s.data()),
                  m_end(s.data() + s.size()),
                  m_elem(s.data() + n)
            {
            }

            reference operator*()
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(m_elem >= m_begin && m_elem < m_end);
                return *m_elem;
            }
            const_reference operator*() const
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(m_elem >= m_begin && m_elem < m_end);
                return *m_elem;
            }

            span_iterator& operator++()
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(m_elem != m_end);
                ++m_elem;
                return *this;
            }
            span_iterator operator++(int)
            {
                auto tmp = *this;
                operator++();
                return tmp;
            }

            span_iterator& operator--()
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(m_elem != m_begin);
                --m_elem;
                return *this;
            }
            span_iterator operator--(int)
            {
                auto tmp = *this;
                operator--();
                return tmp;
            }

            span_iterator& operator+=(difference_type n)
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(m_end - *this >= n);
                m_elem += n;
                return *this;
            }
            span_iterator& operator-=(difference_type n)
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(*this - m_begin >= n);
                m_elem += n;
                return *this;
            }

            reference operator[](difference_type i)
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(m_elem + i >= m_begin && m_elem + i < m_end);
                return *(m_elem + i);
            }
            const_reference operator[](difference_type i) const
            {
                SCN_EXPECT(m_begin && m_end);
                SCN_EXPECT(m_elem + i >= m_begin && m_elem + i < m_end);
                return *(m_elem + i);
            }

        private:
            const_pointer m_begin{nullptr}, m_end{nullptr};
            pointer m_elem{nullptr};
        };

        template <typename ElemL, typename ElemR>
        bool operator==(const span_iterator<ElemL>& l,
                        const span_iterator<ElemR>& r)
        {
            return l.m_elem == r.m_elem;
        }
        template <typename ElemL, typename ElemR>
        bool operator!=(const span_iterator<ElemL>& l,
                        const span_iterator<ElemR>& r)
        {
            return !operator==(l, r);
        }

        template <typename ElemL, typename ElemR>
        bool operator<(const span_iterator<ElemL>& l,
                       const span_iterator<ElemR>& r)
        {
            return l.m_elem < r.m_elem;
        }
        template <typename ElemL, typename ElemR>
        bool operator>(const span_iterator<ElemL>& l,
                       const span_iterator<ElemR>& r)
        {
            return operator<(r, l);
        }
        template <typename ElemL, typename ElemR>
        bool operator<=(const span_iterator<ElemL>& l,
                        const span_iterator<ElemR>& r)
        {
            return !operator>(l, r);
        }
        template <typename ElemL, typename ElemR>
        bool operator>=(const span_iterator<ElemL>& l,
                        const span_iterator<ElemR>& r)
        {
            return !operator<(l, r);
        }

        template <typename Elem>
        span_iterator<Elem> operator+(
            span_iterator<Elem> l,
            typename span_iterator<Elem>::difference_type r)
        {
            l += r;
            return l;
        }
        template <typename Elem>
        span_iterator<Elem> operator+(
            typename span_iterator<Elem>::difference_type r,
            span_iterator<Elem> l)
        {
            l += r;
            return l;
        }

        template <typename Elem>
        span_iterator<Elem> operator-(
            span_iterator<Elem> l,
            typename span_iterator<Elem>::difference_type r)
        {
            l -= r;
            return l;
        }

        template <typename ElemL, typename ElemR>
        typename span_iterator<ElemL>::difference_type operator-(
            const span_iterator<ElemL>& l,
            const span_iterator<ElemR>& r)
        {
            SCN_EXPECT(l.m_span && r.m_span);
            SCN_EXPECT(l.m_span == r.m_span);
            return l.m_elem - r.m_elem;
        }
#endif
    }  // namespace detail

    /**
     * A view over a contiguous range.
     * Stripped-down version of `std::span`.
     */
    template <typename T>
    class span {
    public:
        using element_type = T;
        using value_type = typename std::remove_cv<T>::type;
        using index_type = std::size_t;
        using ssize_type = std::ptrdiff_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;

        // using iterator = detail::span_iterator<element_type>;
        // using const_iterator = detail::span_iterator<const element_type>;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        constexpr span() noexcept = default;
        constexpr span(pointer ptr, index_type count) noexcept
            : m_ptr(ptr), m_end(ptr + count)
        {
        }
        constexpr span(pointer first, pointer last) noexcept
            : m_ptr(first), m_end(last)
        {
        }

        SCN_CONSTEXPR14 iterator begin() noexcept
        {
            return _make_begin();
        }
        SCN_CONSTEXPR14 iterator end() noexcept
        {
            return _make_end();
        }
        SCN_CONSTEXPR14 reverse_iterator rbegin() noexcept
        {
            return reverse_iterator{end()};
        }
        SCN_CONSTEXPR14 reverse_iterator rend() noexcept
        {
            return reverse_iterator{begin()};
        }

        constexpr const_iterator begin() const noexcept
        {
            return _make_begin();
        }
        constexpr const_iterator end() const noexcept
        {
            return _make_end();
        }
        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return reverse_iterator{end()};
        }
        constexpr const_reverse_iterator rend() const noexcept
        {
            return reverse_iterator{begin()};
        }

        constexpr const_iterator cbegin() const noexcept
        {
            return _make_begin();
        }
        constexpr const_iterator cend() const noexcept
        {
            return _make_end();
        }
        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return reverse_iterator{cend()};
        }
        constexpr const_reverse_iterator crend() const noexcept
        {
            return reverse_iterator{cbegin()};
        }

        constexpr reference operator[](index_type i) const noexcept
        {
            SCN_EXPECT(size() > i);
            return *(m_ptr + i);
        }

        constexpr pointer data() const noexcept
        {
            return m_ptr;
        }
        constexpr index_type size() const noexcept
        {
            return static_cast<index_type>(m_end - m_ptr);
        }
        constexpr ssize_type ssize() const noexcept
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

        constexpr span<typename std::add_const<T>::type> as_const() const
        {
            return {m_ptr, m_end};
        }

    private:
        SCN_CONSTEXPR14 iterator _make_begin()
        {
            SCN_EXPECT(m_ptr);
            // return {*this, m_ptr};
            return m_ptr;
        }
        constexpr const_iterator _make_begin() const
        {
            SCN_EXPECT(m_ptr);
            // return {*this, m_ptr};
            return m_ptr;
        }

        SCN_CONSTEXPR14 iterator _make_end()
        {
            SCN_EXPECT(m_ptr);
            // return {*this, m_ptr + m_size};
            return m_end;
        }
        constexpr const_iterator _make_end() const
        {
            SCN_EXPECT(m_ptr);
            // return {*this, m_ptr + m_size};
            return m_end;
        }

        pointer m_ptr{nullptr};
        pointer m_end{nullptr};
    };

    template <typename T>
    constexpr span<T> make_span(T* ptr, std::size_t count) noexcept
    {
        return span<T>(ptr, count);
    }
    template <typename T>
    constexpr span<T> make_span(T* first, T* last) noexcept
    {
        return span<T>(first, last);
    }
    template <typename T>
    constexpr span<typename T::value_type> make_span(T& container) noexcept
    {
        using std::begin;
        using std::end;
        return span<typename T::value_type>(
            std::addressof(*begin(container)),
            std::addressof(*(end(container) - 1)) + 1);
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_SPAN_H
