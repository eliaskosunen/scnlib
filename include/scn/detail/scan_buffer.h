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
#include <scn/util/string_view.h>

#include <variant>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        class basic_scan_buffer {
        public:
            class forward_iterator;

            using char_type = CharT;
            using range_type = ranges::subrange<forward_iterator,
                                                ranges_std::default_sentinel_t>;

            basic_scan_buffer(const basic_scan_buffer&) = delete;
            basic_scan_buffer& operator=(const basic_scan_buffer&) = delete;
            basic_scan_buffer(basic_scan_buffer&&) = delete;
            basic_scan_buffer& operator=(basic_scan_buffer&&) = delete;
            virtual ~basic_scan_buffer() = default;

            virtual bool fill() = 0;

            virtual void sync(std::ptrdiff_t position)
            {
                SCN_UNUSED(position);
            }

            SCN_NODISCARD std::ptrdiff_t chars_available() const
            {
                return m_putback_buffer.size() + m_current_view.size();
            }

            SCN_NODISCARD std::basic_string_view<CharT> current_view() const
            {
                return m_current_view;
            }

            SCN_NODISCARD std::basic_string<CharT>& putback_buffer()
            {
                return m_putback_buffer;
            }
            SCN_NODISCARD const std::basic_string<CharT>& putback_buffer() const
            {
                return m_putback_buffer;
            }

            SCN_NODISCARD std::basic_string_view<CharT> get_segment_starting_at(
                std::ptrdiff_t pos) const
            {
                if (pos < m_putback_buffer.size()) {
                    return std::basic_string_view<CharT>(m_putback_buffer)
                        .substr(pos);
                }
                return m_current_view.substr(pos - m_putback_buffer.size());
            }

            SCN_NODISCARD bool is_contiguous() const
            {
                return m_is_contiguous;
            }

            SCN_NODISCARD std::basic_string_view<CharT> get_contiguous() const
            {
                SCN_EXPECT(is_contiguous());
                return current_view();
            }

            SCN_NODISCARD range_type get();

        protected:
            friend class forward_iterator;

            struct contiguous_tag {};
            struct non_contiguous_tag {};

            basic_scan_buffer(contiguous_tag,
                              std::basic_string_view<char_type> sv)
                : m_current_view(sv), m_is_contiguous(true)
            {
            }

            basic_scan_buffer(non_contiguous_tag,
                              std::basic_string_view<char_type> sv = {})
                : m_current_view(sv), m_is_contiguous(false)
            {
            }

            basic_scan_buffer(bool is_contiguous,
                              std::basic_string_view<char_type> sv)
                : m_current_view(sv), m_is_contiguous(is_contiguous)
            {
            }

            std::basic_string_view<char_type> m_current_view{};
            std::basic_string<char_type> m_putback_buffer{};
            bool m_is_contiguous{false};
        };

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

            basic_scan_buffer<CharT>* parent()
            {
                return m_parent;
            }
            const basic_scan_buffer<CharT>* parent() const
            {
                return m_parent;
            }

            std::basic_string_view<CharT> contiguous_segment() const
            {
                SCN_EXPECT(m_parent);
                return m_parent->get_segment_starting_at(position());
            }
            auto to_contiguous_segment_iterator() const
            {
                return contiguous_segment().data();
            }

            forward_iterator& operator++()
            {
                SCN_EXPECT(m_parent);
                ++m_position;
                if (SCN_LIKELY(!m_current_view.empty())) {
                    m_current_view = m_current_view.substr(1);
                }
                read_at_position();
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
                if (SCN_LIKELY(!m_current_view.empty())) {
                    return m_current_view.front();
                }
                auto view = m_parent->get_segment_starting_at(m_position);
                SCN_EXPECT(!view.empty());
                return view.front();
            }

            forward_iterator& batch_advance(std::ptrdiff_t n)
            {
                SCN_EXPECT(m_parent);
                SCN_EXPECT(n >= 0);
                m_position += n;

                m_current_view = m_current_view.substr(
                    std::min(static_cast<size_t>(n), m_current_view.size()));

                SCN_ENSURE(m_position <= m_parent->chars_available());
                return *this;
            }

            friend bool operator==(const forward_iterator& lhs,
                                   const forward_iterator& rhs)
            {
                if (!lhs.m_parent && !rhs.m_parent) {
                    return true;
                }
                if (!lhs.m_parent) {
                    return ranges_std::default_sentinel == rhs;
                }
                if (!rhs.m_parent) {
                    return lhs == ranges_std::default_sentinel;
                }
                SCN_EXPECT(lhs.m_parent == rhs.m_parent);
                return lhs.m_position == rhs.m_position;
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
                             std::basic_string_view<CharT> view,
                             std::ptrdiff_t pos)
                : m_parent(&parent), m_current_view(view), m_position(pos)
            {
            }

            bool read_at_position() const
            {
                if (!m_current_view.empty()) {
                    return true;
                }

                if (m_position >= m_parent->chars_available()) {
                    if (m_parent->is_contiguous()) {
                        return false;
                    }
                    if (!m_parent->fill()) {
                        return false;
                    }
                }
                m_current_view = m_parent->get_segment_starting_at(m_position);
                SCN_ENSURE(!m_current_view.empty());
                return true;
            }

            bool is_at_end() const
            {
                if (!m_parent) {
                    return true;
                }
                return !read_at_position();
            }

            basic_scan_buffer<CharT>* m_parent{nullptr};
            mutable std::basic_string_view<CharT> m_current_view{};
            std::ptrdiff_t m_position{0};
        };

        template <typename CharT>
        SCN_NODISCARD auto basic_scan_buffer<CharT>::get() -> range_type
        {
            if (m_putback_buffer.empty()) {
                return ranges::subrange{
                    forward_iterator{*this, m_current_view, 0},
                    ranges_std::default_sentinel};
            }
            return ranges::subrange{
                forward_iterator{*this, std::basic_string_view<CharT>{}, 0},
                ranges_std::default_sentinel};
        }

        static_assert(ranges::forward_range<scan_buffer::range_type>);

        template <typename CharT>
        class basic_scan_string_buffer : public basic_scan_buffer<CharT> {
            using base = basic_scan_buffer<CharT>;

        public:
            basic_scan_string_buffer(std::basic_string_view<CharT> sv)
                : base(typename base::contiguous_tag{}, sv)
            {
            }

            bool fill() override
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
            basic_scan_forward_buffer_base()
                : base(typename base::non_contiguous_tag{})
            {
            }
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

            bool fill() override
            {
                if (m_cursor == ranges::end(m_range)) {
                    return false;
                }
                if (auto prev = m_latest) {
                    this->m_putback_buffer.push_back(*prev);
                }
                m_latest = *m_cursor;
                ++m_cursor;
                this->m_current_view =
                    std::basic_string_view<char_type>{&*m_latest, 1};
                return true;
            }

        private:
            Range m_range;
            iterator m_cursor;
            std::optional<char_type> m_latest{std::nullopt};
        };

        template <typename R>
        basic_scan_forward_buffer_impl(R&&)
            -> basic_scan_forward_buffer_impl<ranges_polyfill::views::all_t<R>>;

        class scan_file_buffer : public basic_scan_buffer<char> {
            using base = basic_scan_buffer<char>;

        public:
            scan_file_buffer(std::FILE* file);

            bool fill() override;
            void sync(std::ptrdiff_t position) override;

        private:
            std::FILE* m_file;
            std::optional<char_type> m_latest{std::nullopt};
        };

        template <typename CharT>
        class basic_scan_ref_buffer : public basic_scan_buffer<CharT> {
            using base = basic_scan_buffer<CharT>;

        public:
            basic_scan_ref_buffer(base& other, std::ptrdiff_t starting_pos)
                : base(other.is_contiguous(), std::basic_string_view<CharT>{}),
                  m_other(other),
                  m_starting_pos(starting_pos)
            {
                this->m_current_view =
                    other.get_segment_starting_at(starting_pos);
                m_fill_needs_to_propagate =
                    other.get_segment_starting_at(0).end() ==
                    this->m_current_view.end();
            }

            bool fill() override
            {
                if (m_fill_needs_to_propagate) {
                    auto ret = m_other.fill();
                    this->m_current_view = m_other.current_view();
                    this->m_putback_buffer =
                        m_other.putback_buffer().substr(m_starting_pos);
                    return ret;
                }

                m_fill_needs_to_propagate = true;
                this->m_putback_buffer =
                    std::basic_string<CharT>{this->m_current_view};
                this->m_current_view = m_other.current_view();
                return true;
            }

        private:
            base& m_other;
            std::ptrdiff_t m_starting_pos;
            bool m_fill_needs_to_propagate{false};
        };

        template <typename CharT>
        basic_scan_ref_buffer(basic_scan_buffer<CharT>&)
            -> basic_scan_ref_buffer<CharT>;

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

        inline auto make_file_scan_buffer(std::FILE* file)
        {
            return scan_file_buffer(file);
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
