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

#include <scn/detail/scan_buffer.h>
#include <scn/impl/util/internal_error.h>
#include <scn/impl/util/ranges_impl.h>
#include <scn/util/string_view.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
#if SCN_MSVC_DEBUG_ITERATORS
#define SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND 1
#else
#define SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND 0
#endif

template <typename T>
constexpr bool range_supports_nocopy() noexcept
{
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    return ranges::contiguous_range<T> ||
           (ranges::random_access_range<T> &&
            detail::can_make_address_from_iterator<ranges::iterator_t<T>>);
#else
    return ranges::contiguous_range<T>;
#endif
}

template <typename R>
constexpr auto range_nocopy_data(const R& r) noexcept
{
    static_assert(range_supports_nocopy<R>());
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    return detail::to_address(ranges::begin(r));
#else
    return ranges::data(r);
#endif
}

template <typename R>
constexpr auto range_nocopy_size(const R& r) noexcept
{
    static_assert(range_supports_nocopy<R>());
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    return static_cast<size_t>(ranges::distance(detail::to_address(r.begin()),
                                                detail::to_address(r.end())));
#else
    return r.size();
#endif
}

template <typename I, typename S>
SCN_NODISCARD constexpr bool is_range_eof(I begin, S end)
{
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    if constexpr (ranges::contiguous_iterator<I> ||
                  (ranges::random_access_iterator<I> &&
                   detail::can_make_address_from_iterator<I>)) {
        return detail::to_address(begin) == detail::to_address(end);
    }
    else
#endif
    {
        return begin == end;
    }
}

template <typename Range>
SCN_NODISCARD constexpr bool is_range_eof(const Range& r)
{
    return is_range_eof(r.begin(), r.end());
}

template <typename Range>
SCN_NODISCARD constexpr eof_error eof_check(const Range& range)
{
    if (SCN_UNLIKELY(is_range_eof(range))) {
        return eof_error::eof;
    }
    return eof_error::good;
}

template <typename Range>
bool is_entire_source_contiguous(const Range& r)
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return true;
    }
    else if constexpr (std::is_same_v<
                           ranges::const_iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        auto beg = r.begin();
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
                           ranges::const_iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        auto beg = r.begin();
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
        return r.size();
    }
    else if constexpr (std::is_same_v<
                           ranges::const_iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            auto seg = r.begin().contiguous_segment();
            auto dist =
                static_cast<size_t>(ranges::distance(r.begin(), r.end()));
            return std::min(seg.size(), dist);
        }
        else {
            return r.begin().contiguous_segment().size();
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
                           ranges::const_iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            auto seg = r.begin().contiguous_segment();
            auto dist =
                static_cast<size_t>(ranges::distance(r.begin(), r.end()));
            return seg.substr(0, std::min(seg.size(), dist));
        }
        else {
            return r.begin().contiguous_segment();
        }
    }
    else {
        return std::basic_string_view<detail::char_t<Range>>{};
    }
}

template <typename Range>
auto get_as_contiguous(const Range& r)
{
    SCN_EXPECT(is_segment_contiguous(r));

    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return r;
    }
    else if constexpr (std::is_same_v<
                           ranges::const_iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            return detail::make_string_view_from_pointers(
                r.begin().to_contiguous_segment_iterator(),
                r.end().to_contiguous_segment_iterator());
        }
        else {
            return r.begin().contiguous_segment();
        }
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
        // for return type deduction
        return std::basic_string_view<detail::char_t<Range>>{};
    }
}

template <typename Range>
std::size_t guaranteed_minimum_size(const Range& r)
{
    if constexpr (ranges::sized_range<Range>) {
        return r.size();
    }
    else if constexpr (std::is_same_v<
                           ranges::const_iterator_t<Range>,
                           typename detail::basic_scan_buffer<
                               detail::char_t<Range>>::forward_iterator>) {
        if constexpr (ranges::common_range<Range>) {
            return static_cast<size_t>(ranges::distance(r.begin(), r.end()));
        }
        else {
            if (r.begin().stores_parent()) {
                return static_cast<size_t>(
                    r.begin().parent()->chars_available() -
                    r.begin().position());
            }
            return r.begin().contiguous_segment().size();
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
