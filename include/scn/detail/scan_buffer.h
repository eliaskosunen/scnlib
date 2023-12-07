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

#include <variant>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        class contiguous_range_segment {
        public:
            using char_type = CharT;
            using string_type = std::basic_string<char_type>;
            using string_view_type = std::basic_string_view<char_type>;

            explicit contiguous_range_segment(string_view_type sv)
                : m_buffer(std::in_place_type<string_view_type>, sv)
            {
            }
            explicit contiguous_range_segment(string_type s)
                : m_buffer(std::in_place_type<string_type>, SCN_MOVE(s))
            {
            }

            auto& get_variant()
            {
                return m_buffer;
            }
            const auto& get_variant() const
            {
                return m_buffer;
            }

            string_view_type get_string_view() const
            {
                return std::visit(string_view_visitor{}, m_buffer);
            }

            SCN_NODISCARD bool holds_allocated_string() const
            {
                return std::holds_alternative<string_type>(m_buffer);
            }
            void into_allocated_string()
            {
                if (holds_allocated_string()) {
                    return;
                }
                string_type s{get_string_view()};
                m_buffer.template emplace<string_type>(SCN_MOVE(s));
            }
            string_type& get_allocated_string()
            {
                SCN_EXPECT(holds_allocated_string());
                return std::get<string_type>(m_buffer);
            }
            const string_type& get_allocated_string() const
            {
                SCN_EXPECT(holds_allocated_string());
                return std::get<string_type>(m_buffer);
            }

        private:
            struct string_view_visitor {
                string_view_type operator()(string_view_type sv) const
                {
                    return sv;
                }

                string_view_type operator()(const string_type& sv) const
                {
                    return sv;
                }
            };

            std::variant<string_view_type, string_type> m_buffer;
        };

        template <typename CharT>
        class basic_scan_buffer {
        public:
            class forward_iterator;

            using char_type = CharT;
            using range_type = ranges::subrange<forward_iterator,
                                                ranges_std::default_sentinel_t>;
            using contiguous_range_type = std::basic_string_view<char_type>;

            basic_scan_buffer(const basic_scan_buffer&) = delete;
            basic_scan_buffer& operator=(const basic_scan_buffer&) = delete;
            basic_scan_buffer(basic_scan_buffer&&) = delete;
            basic_scan_buffer& operator=(basic_scan_buffer&&) = delete;
            ~basic_scan_buffer() = default;

            virtual std::optional<CharT> read_single() = 0;

            SCN_NODISCARD std::ptrdiff_t characters_read() const
            {
                return get_contiguous_segment().size();
            }

            SCN_NODISCARD bool is_contiguous() const
            {
                return m_is_contiguous;
            }

            SCN_NODISCARD auto get_contiguous_segment() const
            {
                return m_buffer.get_string_view();
            }

            SCN_NODISCARD auto get_contiguous_buffer() const
            {
                SCN_EXPECT(is_contiguous());
                return get_contiguous_segment();
            }

            SCN_NODISCARD range_type get_forward_buffer();

        protected:
            friend class forward_iterator;

            using buffer_type = contiguous_range_segment<char_type>;

            basic_scan_buffer(std::false_type)
                : m_buffer(typename buffer_type::string_type{}),
                  m_is_contiguous(false)
            {
            }

            basic_scan_buffer(buffer_type buf, bool is_contiguous)
                : m_buffer(SCN_MOVE(buf)), m_is_contiguous(is_contiguous)
            {
            }

            buffer_type m_buffer;
            bool m_is_contiguous;
        };

        template <typename CharT>
        class basic_scan_string_buffer : public basic_scan_buffer<CharT> {
            using base = basic_scan_buffer<CharT>;

        public:
            basic_scan_string_buffer(std::basic_string_view<CharT> sv)
                : base(sv, true)
            {
            }

            std::optional<CharT> read_single() override
            {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
        };

        template <typename CharT>
        basic_scan_string_buffer(std::basic_string_view<CharT>)
            -> basic_scan_string_buffer<CharT>;

        template <typename CharT>
        class basic_scan_forward_buffer_base : public basic_scan_buffer<CharT> {
            using base = basic_scan_buffer<CharT>;

        protected:
            basic_scan_forward_buffer_base() : base(std::false_type{}) {}
        };

        template <typename Range>
        class basic_scan_forward_buffer_impl
            : public basic_scan_forward_buffer_base<
                  ranges::range_value_t<Range>> {
            using _char_type = ranges::range_value_t<Range>;
            using base = basic_scan_forward_buffer_base<_char_type>;

        public:
            using char_type = _char_type;
            using range_type = Range;
            using iterator = ranges::iterator_t<Range>;
            using sentinel = ranges::sentinel_t<Range>;

            basic_scan_forward_buffer_impl(Range r)
                : m_range(SCN_MOVE(r)), m_cursor(ranges::begin(m_range))
            {
            }

            std::optional<char_type> read_single() override
            {
                if (m_cursor == ranges::end(m_range)) {
                    return std::nullopt;
                }
                auto ch = *m_cursor;
                this->m_buffer.get_allocated_string().push_back(ch);
                ++m_cursor;
                return ch;
            }

        private:
            Range m_range;
            iterator m_cursor;
        };

        template <typename R>
        basic_scan_forward_buffer_impl(R&&)
            -> basic_scan_forward_buffer_impl<ranges_polyfill::views::all_t<R>>;

        template <typename CharT>
        class basic_scan_buffer<CharT>::forward_iterator {
        public:
            using value_type = CharT;
            using reference = CharT;
            using pointer = CharT*;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            forward_iterator() = default;

            std::ptrdiff_t position() const
            {
                return m_position;
            }

            auto to_contiguous_segment_iterator() const
            {
                SCN_EXPECT(m_parent);
                return m_parent->get_contiguous_segment().begin() + position();
            }

            forward_iterator& operator++()
            {
                SCN_EXPECT(m_parent);
                ++m_position;
                m_cached_current.reset();
                read_at_cursor();
                return *this;
            }

            forward_iterator operator++(int)
            {
                auto copy = *this;
                operator++();
                return copy;
            }

            CharT operator*() const
            {
                SCN_EXPECT(m_parent);
                read_at_cursor();
                SCN_EXPECT(m_cached_current);
                return *m_cached_current;
            }

            friend bool operator==(const forward_iterator& lhs,
                                   const forward_iterator& rhs)
            {
                if (lhs.m_position == rhs.m_position) {
                    return true;
                }
                const auto lhs_is_end = lhs.is_at_end();
                return lhs_is_end && lhs_is_end == rhs.is_at_end();
            }
            friend bool operator!=(const forward_iterator& lhs,
                                   const forward_iterator& rhs)
            {
                return !(lhs == rhs);
            }

            friend bool operator==(const forward_iterator& x,
                                   ranges_std::default_sentinel_t)
            {
                return x.is_at_end();
            }
            friend bool operator==(ranges_std::default_sentinel_t,
                                   const forward_iterator& x)
            {
                return x.is_at_end();
            }

            friend bool operator!=(const forward_iterator& x,
                                   ranges_std::default_sentinel_t)
            {
                return !x.is_at_end();
            }
            friend bool operator!=(ranges_std::default_sentinel_t,
                                   const forward_iterator& x)
            {
                return !x.is_at_end();
            }

        private:
            friend class basic_scan_buffer<CharT>;

            forward_iterator(basic_scan_buffer<CharT>& parent,
                             std::ptrdiff_t pos)
                : m_parent(&parent), m_position(pos)
            {
            }

            void read_at_cursor() const
            {
                SCN_EXPECT(m_parent);

                if (m_cached_current) {
                    return;
                }

                if (m_position < m_parent->characters_read()) {
                    m_cached_current =
                        m_parent->get_contiguous_segment()[m_position];
                    return;
                }

                m_cached_current = m_parent->read_single();
            }

            bool is_at_end() const
            {
                if (!m_parent) {
                    return true;
                }
                if (m_cached_current) {
                    return false;
                }
                read_at_cursor();
                return !m_cached_current;
            }

            basic_scan_buffer<CharT>* m_parent{nullptr};
            std::ptrdiff_t m_position{0};
            mutable std::optional<CharT> m_cached_current{std::nullopt};
        };

        template <typename CharT>
        SCN_NODISCARD auto basic_scan_buffer<CharT>::get_forward_buffer()
            -> range_type
        {
            SCN_EXPECT(!is_contiguous());
            return ranges::subrange{forward_iterator{*this, 0},
                                    ranges_std::default_sentinel};
        }

        template <typename Range>
        auto make_string_scan_buffer(const Range& range)
        {
            return basic_scan_string_buffer(std::basic_string_view{
                ranges::data(range), ranges::size(range)});
        }

        template <typename Range>
        auto make_forward_scan_buffer(Range&& range)
        {
            return basic_scan_forward_buffer_impl(SCN_FWD(range));
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
