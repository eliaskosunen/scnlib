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
auto read_all(const Range& range) -> ranges::const_iterator_t<Range>
{
    return ranges::next(range.begin(), range.end());
}

template <typename Range>
auto read_code_unit(const Range& range)
    -> eof_expected<ranges::const_iterator_t<Range>>
{
    if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
        return unexpected(e);
    }

    return ranges::next(range.begin());
}

template <typename Range>
auto read_exactly_n_code_units(const Range& range, std::ptrdiff_t count)
    -> eof_expected<ranges::const_iterator_t<Range>>
{
    SCN_EXPECT(count >= 0);

    if constexpr (ranges::sized_range<Range>) {
        const auto sz = static_cast<std::ptrdiff_t>(range.size());
        if (sz < count) {
            return unexpected(eof_error::eof);
        }

        return ranges::next(range.begin(), count);
    }
    else {
        auto it = range.begin();
        if (guaranteed_minimum_size(range) >= count) {
            return ranges::next(it, count);
        }

        for (std::ptrdiff_t i = 0; i < count; ++i, (void)++it) {
            if (it == range.end()) {
                return unexpected(eof_error::eof);
            }
        }

        return it;
    }
}
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn
