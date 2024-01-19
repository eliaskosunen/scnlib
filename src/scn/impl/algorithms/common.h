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
#include <scn/detail/scan_buffer.h>
#include <scn/util/meta.h>
#include <scn/util/string_view.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename Range>
bool is_entire_source_contiguous(const Range& r)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return true;
    }
    else if constexpr (std::is_same_v<
                           ranges::iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        auto beg = ranges::begin(r);
        if (!beg.stores_parent()) {
            return true;
        }
        return beg.parent()->is_contiguous();
    }
    else {
        return false;
    }
}

template <typename Range>
bool is_segment_contiguous(const Range& r)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return true;
    }
    else if constexpr (std::is_same_v<
                           ranges::iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        auto beg = ranges::begin(r);
        if (beg.contiguous_segment().empty()) {
            return false;
        }
        if constexpr (ranges::common_range<Range>) {
            return beg.contiguous_segment().end() ==
                   ranges::end(r).contiguous_segment().end();
        }
        else {
            if (beg.stores_parent()) {
                return beg.contiguous_segment().end() ==
                       beg.parent()->current_view().end();
            }
            return true;
        }
    }
    else {
        return false;
    }
}

template <typename Range>
std::size_t contiguous_beginning_size(const Range& r)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return ranges_polyfill::usize(r);
    }
    else if constexpr (std::is_same_v<
                           ranges::iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            auto seg = ranges::begin(r).contiguous_segment();
            auto dist = static_cast<size_t>(ranges_polyfill::pos_distance(
                ranges::begin(r), ranges::end(r)));
            return std::min(seg.size(), dist);
        }
        else {
            return ranges::begin(r).contiguous_segment().size();
        }
    }
    else {
        return false;
    }
}

template <typename Range>
auto get_contiguous_beginning(const Range& r)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return r;
    }
    else if constexpr (std::is_same_v<
                           ranges::iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            auto seg = ranges::begin(r).contiguous_segment();
            auto dist = static_cast<size_t>(ranges_polyfill::pos_distance(
                ranges::begin(r), ranges::end(r)));
            return seg.substr(0, std::min(seg.size(), dist));
        }
        else {
            return ranges::begin(r).contiguous_segment();
        }
    }
    else {
        return std::basic_string_view<detail::char_t<Range>>{};
    }
}

template <typename Range>
auto get_as_contiguous(Range&& r)
{
    SCN_EXPECT(is_segment_contiguous(r));
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return r;
    }
    else if constexpr (std::is_same_v<
                           ranges::iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            return detail::make_string_view_from_pointers(
                ranges::begin(r).to_contiguous_segment_iterator(),
                ranges::end(r).to_contiguous_segment_iterator());
        }
        else {
            return ranges::begin(r).contiguous_segment();
        }
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
        return std::basic_string_view<detail::char_t<Range>>{};
    }
}

template <typename Range>
std::size_t guaranteed_minimum_size(const Range& r)
{
    if constexpr (ranges::sized_range<Range>) {
        return ranges_polyfill::usize(r);
    }
    else if constexpr (std::is_same_v<
                           ranges::iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            return static_cast<size_t>(ranges::end(r).position() -
                                       ranges::begin(r).position());
        }
        else {
            if (ranges::begin(r).stores_parent()) {
                return static_cast<size_t>(
                    ranges::begin(r).parent()->chars_available() -
                    ranges::begin(r).position());
            }
            return ranges::begin(r).contiguous_segment().size();
        }
    }
    else {
        return 0;
    }
}

template <typename I, typename T>
struct iterator_value_result {
    SCN_NO_UNIQUE_ADDRESS I iterator;
    SCN_NO_UNIQUE_ADDRESS T value;
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
