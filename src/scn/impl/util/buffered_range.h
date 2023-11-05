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

#include <scn/detail/istream_range.h>
#include <scn/detail/ranges.h>
#include <scn/util/span.h>

#include <algorithm>
#include <limits>

// experimental

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        struct buffered_range_segment_impl_base {
        public:
            using range_difference_t = std::ptrdiff_t;

            buffered_range_segment_impl_base() = default;

            buffered_range_segment_impl_base(
                const buffered_range_segment_impl_base&) = delete;
            buffered_range_segment_impl_base(
                buffered_range_segment_impl_base&&) = delete;
            buffered_range_segment_impl_base& operator=(
                const buffered_range_segment_impl_base&) = delete;
            buffered_range_segment_impl_base& operator=(
                buffered_range_segment_impl_base&&) = delete;
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

        template <typename Range>
        class buffered_range_segment_string_impl
            : public buffered_range_segment_impl_base {
        public:
            using range_type = Range;
            using char_type = detail::char_t<range_type>;
            using range_iterator_t = ranges::iterator_t<range_type>;

            buffered_range_segment_string_impl(range_type& range,
                                               range_iterator_t& first)
                : m_range(&range), m_first(&first)
            {
            }

            ~buffered_range_segment_string_impl()
            {
                ranges::advance(*m_first, m_read);
            }

            auto begin() const
            {
                return *m_first;
            }
            auto end() const
            {
                return m_range->end();
            }

            SCN_NODISCARD range_difference_t potential_size() const
            {
                return ranges::distance(begin(), end());
            }
            void acquire(range_difference_t n =
                             std::numeric_limits<range_difference_t>::max())
            {
                // no-op
                SCN_UNUSED(n);
            }

        protected:
            range_type* m_range{nullptr};
            range_iterator_t* m_first{};
        };

        template <typename Range>
        class buffered_range_segment_iostream_impl
            : public buffered_range_segment_impl_base {
        public:
            using range_type = Range;
            using char_type = detail::char_t<range_type>;
            using range_iterator_t = ranges::iterator_t<range_type>;

            buffered_range_segment_iostream_impl(range_type& range,
                                                 range_iterator_t& first)
                : m_range(&range),
                  m_first(&first),
                  m_streambuf_view(&m_first->view().base())
            {
            }

            ~buffered_range_segment_iostream_impl()
            {
                ranges::advance(*m_first, m_read);
            }

            auto begin() const
            {
                return m_buffer.begin();
            }
            auto end() const
            {
                return m_buffer.end();
            }

            SCN_NODISCARD range_difference_t potential_size() const
            {
                auto* sb = m_streambuf_view->rdbuf();
                auto sb_count = m_streambuf_view->count();
                auto first_idx = m_first->index();

                SCN_EXPECT(sb_count >= first_idx);
                if (sb_count != first_idx) {
                    return sb_count - first_idx;
                }
                return sb->in_avail();
            }

            void acquire(range_difference_t n =
                             std::numeric_limits<range_difference_t>::max())
            {
                auto* sb = m_streambuf_view->rdbuf();
                auto sb_count = m_streambuf_view->count();
                auto first_idx = m_first->index();

                SCN_EXPECT(sb_count >= first_idx);
                if (sb_count != first_idx) {
                    m_buffer = m_first->view().buffer().subspan(first_idx);
                    auto n_to_read = std::min(ranges::ssize(m_buffer), n);
                    m_buffer = m_buffer.first(n);
                    return;
                }

                const auto avail = sb->in_avail();
                const auto n_to_read = std::min(avail, n);
                m_buffer_memory.resize(n);
                auto read_n = sb->sgetn(m_buffer_memory.data(), n);
                SCN_ENSURE(read_n == n);
                m_buffer = {m_buffer_memory};
            }

        protected:
            range_type* m_range{nullptr};
            range_iterator_t* m_first{};
            detail::basic_input_istreambuf_view<char_type>* m_streambuf_view{
                nullptr};
            std::basic_string<char_type> m_buffer_memory{};
            span<const char_type> m_buffer{};
        };

        template <typename Range, typename Iterator>
        struct buffered_range_segment_impl {
            using type = void;
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
        struct buffered_range_segment_impl<Range,
                                           istreambuf_subrange::iterator> {
            using type = buffered_range_segment_iostream_impl<Range>;
        };
        template <typename Range>
        struct buffered_range_segment_impl<Range,
                                           wistreambuf_subrange::iterator> {
            using type = buffered_range_segment_iostream_impl<Range>;
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
