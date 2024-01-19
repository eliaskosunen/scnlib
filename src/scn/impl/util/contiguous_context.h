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

#include <scn/detail/context.h>
#include <scn/detail/scan_buffer.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename CharT>
class basic_contiguous_scan_context
    : public detail::
          scan_context_base<CharT, basic_scan_args<basic_scan_context<CharT>>> {
    using base =
        detail::scan_context_base<CharT,
                                  basic_scan_args<basic_scan_context<CharT>>>;

public:
    using char_type = CharT;
    using buffer_type = detail::basic_scan_buffer<char_type>;
    using range_type = ranges::subrange<const char_type*, const char_type*>;
    using iterator = const char_type*;
    using sentinel = const char_type*;
    using parse_context_type = basic_scan_parse_context<char_type>;

    using parent_context_type = basic_scan_context<char_type>;
    using args_type = basic_scan_args<parent_context_type>;
    using arg_type = basic_scan_arg<parent_context_type>;

    template <typename Range,
              std::enable_if_t<ranges::contiguous_range<Range> &&
                               ranges::borrowed_range<Range>>* = nullptr>
    constexpr basic_contiguous_scan_context(Range&& r,
                                            args_type a,
                                            detail::locale_ref loc = {})
        : base(SCN_MOVE(a), loc),
          m_range(SCN_FWD(r)),
          m_current(m_range.begin())
    {
    }

    constexpr iterator begin() const
    {
        return m_current;
    }

    constexpr sentinel end() const
    {
        return m_range.end();
    }

    constexpr auto range() const
    {
        return ranges::subrange{begin(), end()};
    }

    constexpr auto underlying_range() const
    {
        return m_range;
    }

    void advance_to(iterator it)
    {
        SCN_EXPECT(it <= end());
        if constexpr (detail::is_comparable_with_nullptr<iterator>::value) {
            if (it == nullptr) {
                it = end();
            }
        }
        m_current = SCN_MOVE(it);
    }

    void advance_to(const typename parent_context_type::iterator& it)
    {
        SCN_EXPECT(it.position() <= m_range.size());
        m_current = m_range.begin() + it.position();
    }

    std::ptrdiff_t begin_position()
    {
        return ranges::distance(m_range.begin(), begin());
    }

private:
    range_type m_range;
    iterator m_current;
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
