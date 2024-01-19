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
simple_borrowed_iterator_t<Range> read_all(Range&& range)
{
    return ranges::next(ranges::begin(range), ranges::end(range));
}

template <typename Range>
eof_expected<simple_borrowed_iterator_t<Range>> read_code_unit(Range&& range)
{
    if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
        return unexpected(e);
    }

    return ranges::next(ranges::begin(range));
}

template <typename Range>
eof_expected<simple_borrowed_iterator_t<Range>> read_exactly_n_code_units(
    Range&& range,
    ranges::range_difference_t<Range> count)
{
    SCN_EXPECT(count >= 0);

    if constexpr (ranges::sized_range<Range>) {
        const auto sz = ranges::ssize(range);
        if (sz < count) {
            return unexpected(eof_error::eof);
        }

        return ranges::next(ranges::begin(range), count);
    }
    else {
        auto it = ranges::begin(range);
        if (guaranteed_minimum_size(range) >= count) {
            return ranges_polyfill::batch_next(it, count);
        }

        for (ranges::range_difference_t<Range> i = 0; i < count;
             ++i, (void)++it) {
            if (it == ranges::end(range)) {
                return unexpected(eof_error::eof);
            }
        }

        return it;
    }
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
