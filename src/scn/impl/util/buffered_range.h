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
#include <scn/detail/stdin_view.h>
#include <scn/util/span.h>

#include <algorithm>
#include <limits>
#include <utility>

// experimental

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        struct buffered_range_segment_impl_base : public ranges::view_base {
        public:
            using range_difference_t = std::ptrdiff_t;

            buffered_range_segment_impl_base() = default;

            buffered_range_segment_impl_base(
                const buffered_range_segment_impl_base&) = delete;
            buffered_range_segment_impl_base& operator=(
                const buffered_range_segment_impl_base&) = delete;

            buffered_range_segment_impl_base(
                buffered_range_segment_impl_base&&) = default;
            buffered_range_segment_impl_base& operator=(
                buffered_range_segment_impl_base&&) = default;

            ~buffered_range_segment_impl_base() = default;

            void set_amount_read(range_difference_t n)
            {
                m_read = n;
            }
            void increase_amount_read(range_difference_t n)
            {
                m_read += n;
            }

        protected:
            range_difference_t m_read{0};
        };

        template <typename CharT>
        class buffered_range_segment_null_impl
            : public buffered_range_segment_impl_base {
        public:
            buffered_range_segment_null_impl() = default;

            template <typename... A>
            explicit buffered_range_segment_null_impl(A&...)
            {
            }

            const CharT* begin() const
            {
                return nullptr;
            }
            const CharT* end() const
            {
                return nullptr;
            }

            void advance_iterator() {}

            SCN_NODISCARD range_difference_t potential_size() const
            {
                return 0;
            }
            void acquire(range_difference_t n =
                             std::numeric_limits<range_difference_t>::max())
            {
                // no-op
                SCN_UNUSED(n);
            }
        };

        template <typename Range>
        class buffered_range_segment_string_impl
            : public buffered_range_segment_impl_base {
        public:
            using range_type = Range;
            using char_type = detail::char_t<range_type>;
            using range_iterator_t = ranges::iterator_t<range_type>;

            buffered_range_segment_string_impl() = default;

            buffered_range_segment_string_impl(range_type& range,
                                               range_iterator_t& first)
                : m_range(&range), m_first(&first)
            {
            }

            ~buffered_range_segment_string_impl()
            {
                if (m_first) {
                    ranges::advance(*m_first, m_read);
                }
            }

            buffered_range_segment_string_impl(
                const buffered_range_segment_string_impl&) = delete;
            buffered_range_segment_string_impl& operator=(
                const buffered_range_segment_string_impl&) = delete;

            buffered_range_segment_string_impl(
                buffered_range_segment_string_impl&&) = default;
            buffered_range_segment_string_impl& operator=(
                buffered_range_segment_string_impl&&) = default;

            auto begin() const
            {
                SCN_EXPECT(m_first);
                return *m_first;
            }
            auto end() const
            {
                SCN_EXPECT(m_range);
                return m_range->end();
            }

            void advance_iterator()
            {
                SCN_EXPECT(m_first);
                ranges::advance(*m_first, m_read);
                m_read = 0;
            }

            SCN_NODISCARD range_difference_t potential_size() const
            {
                return ranges::distance(begin(), end());
            }
            void acquire(range_difference_t n =
                             std::numeric_limits<range_difference_t>::max())
            {
                // no-op
                SCN_EXPECT(m_range && m_range);
                SCN_UNUSED(n);
            }

        protected:
            range_type* m_range{nullptr};
            range_iterator_t* m_first{};
        };

        template <typename Range>
        class buffered_range_segment_stdin_impl
            : public buffered_range_segment_impl_base {
        public:
            using range_type = Range;
            using char_type = char;
            using range_iterator_t = ranges::iterator_t<range_type>;

            buffered_range_segment_stdin_impl() = default;

            buffered_range_segment_stdin_impl(range_type& range,
                                              range_iterator_t& first)
                : m_range(&range), m_first(&first)
            {
            }

            ~buffered_range_segment_stdin_impl()
            {
                if (m_first) {
                    ranges::advance(*m_first, m_read);
                }
            }

            buffered_range_segment_stdin_impl(
                const buffered_range_segment_stdin_impl&) = delete;
            buffered_range_segment_stdin_impl& operator=(
                const buffered_range_segment_stdin_impl&) = delete;

            buffered_range_segment_stdin_impl(
                buffered_range_segment_stdin_impl&&) = default;
            buffered_range_segment_stdin_impl& operator=(
                buffered_range_segment_stdin_impl&&) = default;

            auto begin() const
            {
                SCN_EXPECT(m_first);
                return m_avail_buf.begin();
            }
            auto end() const
            {
                SCN_EXPECT(m_first);
                return m_avail_buf.end();
            }

            void advance_iterator()
            {
                SCN_EXPECT(m_first);
                ranges::advance(*m_first, m_read);
                m_avail_buf = m_avail_buf.substr(m_read);
                m_read = 0;
            }

            SCN_NODISCARD range_difference_t potential_size() const
            {
                SCN_EXPECT(m_first);
                return m_first->manager()->in_avail(*m_first).size();
            }
            void acquire(range_difference_t n =
                             std::numeric_limits<range_difference_t>::max())
            {
                SCN_EXPECT(m_range && m_first);
                m_avail_buf =
                    m_first->manager()->in_avail(*m_first).substr(0, n);
            }

        protected:
            range_type* m_range{nullptr};
            range_iterator_t* m_first{};
            std::string_view m_avail_buf{};
        };

        template <typename Range, typename Iterator>
        struct buffered_range_segment_impl {
            using type =
                buffered_range_segment_null_impl<detail::char_t<Range>>;
        };

        template <typename Range>
        struct buffered_range_segment_impl<Range, std::string_view::iterator> {
            using type = buffered_range_segment_string_impl<Range>;
        };
        template <typename Range>
        struct buffered_range_segment_impl<Range, std::wstring_view::iterator> {
            using type = buffered_range_segment_string_impl<Range>;
        };
        template <typename Range, typename CharT>
        struct buffered_range_segment_impl<Range, const CharT*> {
            using type = buffered_range_segment_string_impl<Range>;
        };
        template <typename Range>
        struct buffered_range_segment_impl<Range, detail::stdin_iterator> {
            using type = buffered_range_segment_stdin_impl<Range>;
        };

        template <typename Range>
        class buffered_range_segment
            : public buffered_range_segment_impl<
                  Range,
                  ranges::iterator_t<Range>>::type,
              public ranges::view_interface<buffered_range_segment<Range>> {
            using base = typename buffered_range_segment_impl<
                Range,
                ranges::iterator_t<Range>>::type;

        public:
            buffered_range_segment(Range& range,
                                   ranges::iterator_t<Range>& first)
                : base(range, first)
            {
            }
        };

        template <typename R, typename I>
        buffered_range_segment(R&, I&) -> buffered_range_segment<R>;

        template <typename Range>
        inline constexpr bool range_supports_buffered_range_segments =
            !std::is_void_v<typename buffered_range_segment_impl<
                Range,
                ranges::iterator_t<Range>>::type>;
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
