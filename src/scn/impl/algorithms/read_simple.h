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

#include <scn/impl/algorithms/common.h>
#include <scn/impl/algorithms/eof_check.h>

namespace scn {
SCN_BEGIN_NAMESPACE

namespace impl {
template <typename Range>
auto read_all(Range&& range) -> detail::simple_borrowed_iterator_t<Range>
{
    return ranges_impl::next(ranges_impl::begin(range), ranges_impl::end(range));
}

template <typename Range>
auto read_code_unit(Range&& range)
    -> eof_expected<detail::simple_borrowed_iterator_t<Range>>
{
    if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
        return unexpected(e);
    }

    return ranges_impl::next(ranges_impl::begin(range));
}

template <typename Range>
auto read_exactly_n_code_units(Range&& range,
                               ranges_impl::range_difference_t<Range> count)
    -> eof_expected<detail::simple_borrowed_iterator_t<Range>>
{
    SCN_EXPECT(count >= 0);

    if constexpr (ranges_impl::sized_range<Range>) {
        const auto sz = ranges_impl::ssize(range);
        if (sz < count) {
            return unexpected(eof_error::eof);
        }

        return ranges_impl::next(ranges_impl::begin(range), count);
    }
    else {
        auto it = ranges_impl::begin(range);
        if (guaranteed_minimum_size(range) >= count) {
            return ranges_polyfill::batch_next(it, count);
        }

        for (ranges_impl::range_difference_t<Range> i = 0; i < count;
             ++i, (void)++it) {
            if (it == ranges_impl::end(range)) {
                return unexpected(eof_error::eof);
            }
        }

        return it;
    }
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
