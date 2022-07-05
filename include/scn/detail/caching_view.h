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

    namespace detail {
        class caching_view_base {
        public:
            using sentinel = ranges_std::default_sentinel_t;
            using difference_type = std::ptrdiff_t;

            caching_view_base() = default;
            ~caching_view_base() = default;

            caching_view_base(const caching_view_base&) = delete;
            caching_view_base& operator=(const caching_view_base&) = delete;

            caching_view_base(caching_view_base&&) = default;
            caching_view_base& operator=(caching_view_base&&) = default;

            ranges_std::default_sentinel_t end() const SCN_NOEXCEPT
            {
                return ranges_std::default_sentinel;
            }

        protected:
            SCN_NODISCARD difference_type
            _convert_index_to_buffer(difference_type idx) const
            {
                return idx - m_buffer_begin_offset;
            }

            mutable difference_type m_iterator_offset{0};
            difference_type m_buffer_begin_offset{0};
        };

        template <typename CharT>
        class basic_caching_view_base : public caching_view_base {
        public:
            using char_type = CharT;
            using buffer_type = std::basic_string<char_type>;
            using difference_type = std::ptrdiff_t;

        protected:
            SCN_NODISCARD char_type
            get_cached_at_index(difference_type idx) const
            {
                const auto bufidx = _convert_index_to_buffer(idx);
                SCN_ENSURE(bufidx < buffer_size());
                return m_buffer[static_cast<std::size_t>(bufidx)];
            }

            difference_type buffer_size() const SCN_NOEXCEPT
            {
                return static_cast<difference_type>(m_buffer.size());
            }

            mutable buffer_type m_buffer{};
        };

        template <typename Range>
        class basic_caching_view
            : public basic_caching_view_base<ranges::range_value_t<Range>>,
              public ranges::view_interface<basic_caching_view<Range>> {
        public:
            using range_type = Range;
            using char_type = ranges::range_value_t<Range>;
            using difference_type = std::ptrdiff_t;

            class iterator;

            basic_caching_view(Range r)
                : m_range(SCN_FWD(r)), m_iterator(ranges::begin(m_range))
            {
            }

            iterator begin() const SCN_NOEXCEPT;

            void clear()
            {
                this->m_buffer_begin_offset = this->buffer_size();
                this->m_buffer.clear();
            }

        protected:
            SCN_NODISCARD char_type get_at_index(difference_type idx) const
            {
                const auto read_result = read_until_index(idx);
                SCN_ENSURE(read_result);
                return this->get_cached_at_index(idx);
            }

            void read_single_into_buffer() const
            {
                SCN_EXPECT(m_iterator != ranges::end(m_range));
                auto ch = *m_iterator;
                ++m_iterator;
                ++this->m_iterator_offset;
                this->m_buffer.push_back(ch);
            }

            SCN_NODISCARD
            bool read_multiple_into_buffer(difference_type n) const
            {
                SCN_EXPECT(n > 0);
                read_single_into_buffer();
                for (difference_type i = 1; i < n; ++i) {
                    if (m_iterator == ranges::end(m_range)) {
                        return false;
                    }
                    read_single_into_buffer();
                }
                return true;
            }

            SCN_NODISCARD bool read_until_index(difference_type idx) const
            {
                const auto bufidx = this->_convert_index_to_buffer(idx);
                if (bufidx >= this->buffer_size()) {
                    return read_multiple_into_buffer(bufidx -
                                                     this->buffer_size() + 1);
                }
                return true;
            }

            SCN_NODISCARD bool is_index_at_end(difference_type idx) const
            {
                const auto bufidx = this->_convert_index_to_buffer(idx);
                if (bufidx < this->buffer_size()) {
                    return false;
                }

                if (m_iterator == ranges::end(m_range)) {
                    return true;
                }

                return !this->read_until_index(idx);
            }

            ranges_polyfill::views::all_t<Range> m_range;
            mutable ranges::iterator_t<decltype(m_range)> m_iterator;
        };

        template <typename Range>
        class basic_caching_view<Range>::iterator {
        public:
            using view_type = basic_caching_view<Range>;

            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = typename view_type::char_type;
            using reference = value_type;
            using pointer = value_type*;

            friend view_type;

            iterator() = default;

            iterator& operator++()
            {
                SCN_EXPECT(m_view);
                ++m_index;
                return *this;
            }
            iterator operator++(int)
            {
                auto copy = *this;
                operator++();
                return copy;
            }

            iterator& operator--()
            {
                SCN_EXPECT(m_view);
                --m_index;
                return *this;
            }
            iterator operator--(int)
            {
                auto copy = *this;
                operator--();
                return copy;
            }

            value_type operator*() const
            {
                SCN_EXPECT(m_view);
                return m_view->get_at_index(m_index);
            }

            view_type& view()
            {
                SCN_EXPECT(m_view);
                return const_cast<view_type&>(*m_view);
            }
            const view_type& view() const
            {
                SCN_EXPECT(m_view);
                return *m_view;
            }

            difference_type index() const
            {
                SCN_EXPECT(m_view);
                return m_index;
            }

            friend bool operator==(const iterator& x,
                                   ranges_std::default_sentinel_t)
            {
                return x.is_at_end();
            }
            friend bool operator==(ranges_std::default_sentinel_t s,
                                   const iterator& x)
            {
                return x == s;
            }

            friend bool operator!=(const iterator& x,
                                   ranges_std::default_sentinel_t s)
            {
                return !(x == s);
            }
            friend bool operator!=(ranges_std::default_sentinel_t s,
                                   const iterator& x)
            {
                return !(x == s);
            }

            bool operator==(const iterator& o) const
            {
                return m_view == o.m_view && m_index == o.m_index;
            }
            bool operator!=(const iterator& o) const
            {
                return !(*this == o);
            }

        private:
            iterator(const view_type& view) : m_view(&view) {}

            bool is_at_end() const
            {
                if (!m_view) {
                    return true;
                }

                return m_view->is_index_at_end(m_index);
            }

            mutable const view_type* m_view;
            difference_type m_index{0};
        };

        template <typename Range>
        auto basic_caching_view<Range>::begin() const SCN_NOEXCEPT->iterator
        {
            return {*this};
        }

        template <typename Range>
        class basic_caching_subrange
            : public ranges::subrange<
                  ranges::iterator_t<basic_caching_view<Range>>,
                  ranges_std::default_sentinel_t,
                  ranges::subrange_kind::unsized> {
            using base =
                ranges::subrange<ranges::iterator_t<basic_caching_view<Range>>,
                                 ranges_std::default_sentinel_t,
                                 ranges::subrange_kind::unsized>;

        public:
            using base::base;

            basic_caching_subrange(const base& other)
                : base(other.begin(), other.end())
            {
            }
        };
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

#if SCN_STD_RANGES

namespace std::ranges {
    template <typename Range>
    inline constexpr bool enable_view<scn::detail::basic_caching_view<Range>> =
        true;
    template <typename Range>
    inline constexpr bool
        enable_view<scn::detail::basic_caching_subrange<Range>> = true;

    template <typename Range>
    inline constexpr bool
        enable_borrowed_range<scn::detail::basic_caching_subrange<Range>> =
            true;
}  // namespace std::ranges

#else

namespace nano {
    template <typename Range>
    inline constexpr bool enable_view<scn::detail::basic_caching_view<Range>> =
        true;
    template <typename Range>
    inline constexpr bool
        enable_view<scn::detail::basic_caching_subrange<Range>> = true;

    template <typename Range>
    inline constexpr bool
        enable_borrowed_range<scn::detail::basic_caching_subrange<Range>> =
            true;
}  // namespace nano

#endif
