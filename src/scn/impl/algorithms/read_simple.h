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
#include <scn/impl/util/buffered_range.h>
#include <scn/util/expected.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename Range>
        simple_borrowed_iterator_t<Range> read_all(Range&& range)
        {
            return ranges::next(ranges::begin(range), ranges::end(range));
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>> read_code_unit(
            Range&& range)
        {
            if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
                return unexpected(e);
            }

            return ranges::next(ranges::begin(range));
        }

        template <typename Range>
        scan_expected<simple_borrowed_iterator_t<Range>>
        read_exactly_n_code_units(Range&& range,
                                  ranges::range_difference_t<Range> count)
        {
            SCN_EXPECT(count >= 0);

            if constexpr (ranges::sized_range<Range>) {
                const auto sz = ranges::ssize(range);
                if (sz < count) {
                    return unexpected_scan_error(scan_error::end_of_range,
                                                 "EOF");
                }

                return ranges::next(ranges::begin(range), count);
            }
            else {
                auto it = ranges::begin(range);

                if constexpr (range_supports_buffered_range_segments<Range>) {
                    auto buf = buffered_range_segment(range, it);
                    if (buf.potential_size() >= count) {
                        buf.set_amount_read(count);
                        return it;
                    }
                }

                for (ranges::range_difference_t<Range> i = 0; i < count;
                     ++i, (void)++it) {
                    if (it == ranges::end(range)) {
                        return unexpected_scan_error(scan_error::end_of_range,
                                                     "EOF");
                    }
                }

                return it;
            }
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
