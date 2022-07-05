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

#include <scn/impl/algorithms/read_code_point.h>
#include <scn/impl/algorithms/read_copying.h>
#include <scn/impl/algorithms/read_nocopy.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename Range>
        scan_expected<read_nocopy_result<Range>> read_n_code_points_nocopy(
            Range&& range,
            ranges::range_difference_t<Range> cp_count)
        {
            static_assert(range_supports_nocopy<Range>());
            // TODO: optimize with a binary search?

            SCN_EXPECT(!ranges::empty(range));

            auto it = ranges::begin(range);
            for (ranges::range_difference_t<Range> i = 0; i < cp_count; ++i) {
                if (it == ranges::end(range)) {
                    break;
                }

                auto rng = ranges::subrange{it, ranges::end(range)};

                using char_type = ranges::range_value_t<Range>;
                char_type buffer[4 / sizeof(char_type)]{};
                auto e = read_code_point(
                    rng, span<char_type>{buffer, 4 / sizeof(char_type)});
                if (!e) {
                    return unexpected(e.error());
                }

                it = e->iterator;
            }

            using char_type = ranges::range_value_t<Range>;
            return read_nocopy_result<Range>{
                it, std::basic_string_view<char_type>{
                        ranges::data(range),
                        static_cast<std::size_t>(
                            ranges::distance(ranges::begin(range), it))}};
        }

        template <typename InputR, typename OutputR>
        scan_expected<read_copying_result<InputR, OutputR>>
        read_n_code_points_copying(InputR&& input,
                                   OutputR&& output,
                                   ranges::range_difference_t<InputR> cp_count)
        {
            SCN_EXPECT(!ranges::empty(input));
            SCN_EXPECT(ranges::begin(output) != ranges::end(output));

            auto in = ranges::begin(input);
            auto out = ranges::begin(output);

            for (ranges::range_difference_t<InputR> i = 0; i < cp_count; ++i) {
                if (in == ranges::end(input) || out == ranges::end(output)) {
                    break;
                }

                auto rng = ranges::subrange{in, ranges::end(input)};

                using char_type = ranges::range_value_t<InputR>;
                char_type buffer[4 / sizeof(char_type)]{};
                auto e = read_code_point(
                    rng, span<char_type>{buffer, 4 / sizeof(char_type)});
                if (!e) {
                    return unexpected(e.error());
                }

                auto out_before_write = out;
                for (char_type ch : e->value) {
                    if (out == ranges::end(output)) {
                        return read_copying_result<InputR, OutputR>{
                            in, out_before_write};
                    }
                    *out++ = ch;
                }
                in = e->iterator;
            }

            return read_copying_result<InputR, OutputR>{in, out};
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
