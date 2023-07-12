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
#include <scn/impl/util/ascii_ctype.h>
#include <scn/impl/util/text_width.h>
#include <scn/util/span.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename In, typename Out>
        using read_copying_result =
            ranges::in_out_result<ranges::borrowed_iterator_t<In>,
                                  ranges::borrowed_iterator_t<Out>>;

        template <typename InputR, typename OutputR>
        SCN_NODISCARD read_copying_result<InputR, OutputR> read_all_copying(
            InputR&& input,
            OutputR&& output)
        {
            SCN_EXPECT(!ranges::empty(input));
            SCN_EXPECT(ranges::begin(output) != ranges::end(output));

            auto [in, out] = impl::copy(SCN_FWD(input), SCN_FWD(output));
            return {SCN_MOVE(in), SCN_MOVE(out)};
        }

        template <typename InputR, typename OutputR>
        SCN_NODISCARD read_copying_result<InputR, OutputR>
        read_n_code_units_copying(InputR&& input,
                                  OutputR&& output,
                                  ranges::range_difference_t<InputR> n)
        {
            SCN_EXPECT(n >= 0);
            auto in_view = ranges::take_view{SCN_FWD(input), n};
            auto [in, out] = read_all_copying(in_view, SCN_FWD(output));
            if constexpr (detail::is_specialization_of_v<
                              decltype(in), ranges_std::counted_iterator>) {
                return {SCN_MOVE(in).base(), SCN_MOVE(out)};
            }
            else {
                return {SCN_MOVE(in), SCN_MOVE(out)};
            }
        }

        template <typename InputR, typename OutputR, typename Predicate>
        SCN_NODISCARD scan_expected<read_copying_result<InputR, OutputR>>
        read_until_with_max_n_width_units_copying(InputR&& input,
                                                  OutputR&& output,
                                                  std::ptrdiff_t width,
                                                  Predicate&& pred)
        {
            SCN_EXPECT(!ranges::empty(input));
            SCN_EXPECT(ranges::begin(output) != ranges::end(output));
            SCN_EXPECT(width >= 0);

            std::ptrdiff_t acc_width = 0;
            auto src = ranges::begin(input);
            auto dst = ranges::begin(output);

            while (src != ranges::end(input) && dst != ranges::end(output)) {
                using char_type = detail::char_t<InputR>;
                std::array<char_type, 4 / sizeof(char_type)> buffer{};
                auto read_result = read_code_point(
                    ranges::subrange{src, ranges::end(input)},
                    span<char_type>{buffer.data(), buffer.size()});
                if (SCN_UNLIKELY(!read_result)) {
                    return unexpected(read_result.error());
                }

                auto code_point_encoded = std::basic_string_view<char_type>{
                    read_result->value.data(), read_result->value.size()};
                auto decode_result = get_next_code_point(code_point_encoded);
                if (SCN_UNLIKELY(!decode_result)) {
                    return unexpected(decode_result.error());
                }

                if (!pred(decode_result->value)) {
                    break;
                }

                acc_width += static_cast<std::ptrdiff_t>(
                    calculate_valid_text_width(decode_result->value));
                if (acc_width > width) {
                    break;
                }

                auto src_copy = src;
                auto [in, out] = impl::copy(
                    code_point_encoded, ranges::subrange{dst, output.end()});
                if (in != code_point_encoded.end()) {
                    return {{src_copy, dst}};
                }
                dst = out;
            }
            return {{src, dst}};
        }

        template <typename InputR, typename OutputR>
        SCN_NODISCARD scan_expected<read_copying_result<InputR, OutputR>>
        read_n_width_units_copying(InputR&& input,
                                   OutputR&& output,
                                   std::ptrdiff_t width)
        {
            return read_until_with_max_n_width_units_copying(
                SCN_FWD(input), SCN_FWD(output), width,
                [](code_point) { return true; });
        }

        template <typename InputR, typename OutputR, typename Pred>
        SCN_NODISCARD read_copying_result<InputR, OutputR>
        read_until_classic_copying(InputR&& input,
                                   OutputR&& output,
                                   Pred&& until)
        {
            SCN_EXPECT(!ranges::empty(input));
            SCN_EXPECT(ranges::begin(output) != ranges::end(output));

            auto found = ranges::find_if(input, until);
            auto [in, out] = impl::copy(
                ranges::subrange{ranges::begin(input), found}, SCN_FWD(output));
            return {SCN_MOVE(in), SCN_MOVE(out)};
        }

        template <typename InputR, typename OutputR>
        SCN_NODISCARD read_copying_result<InputR, OutputR>
        read_until_classic_space_copying(InputR&& input, OutputR&& output)
        {
            return read_until_classic_copying(
                SCN_FWD(input), SCN_FWD(output),
                [](typename detail::char_t<InputR> ch)
                    SCN_NOEXCEPT { return is_ascii_space(ch); });
        }

        template <typename InputR, typename OutputR, typename Pred>
        SCN_NODISCARD scan_expected<read_copying_result<InputR, OutputR>>
        read_until_code_point_copying(InputR&& input,
                                      OutputR&& output,
                                      Pred&& until)
        {
            SCN_EXPECT(!ranges::empty(input));
            SCN_EXPECT(ranges::begin(output) != ranges::end(output));

            auto src = ranges::begin(input);
            auto dst = ranges::begin(output);

            while (src != ranges::end(input) && dst != ranges::end(output)) {
                using char_type = detail::char_t<InputR>;
                std::array<char_type, 4 / sizeof(char_type)> buffer{};
                auto read_result = read_code_point(
                    ranges::subrange{src, ranges::end(input)},
                    span<char_type>{buffer.data(), buffer.size()});
                if (SCN_UNLIKELY(!read_result)) {
                    return unexpected(read_result.error());
                }

                auto code_point_encoded = std::basic_string_view<char_type>{
                    read_result->value.data(), read_result->value.size()};
                auto decode_result = get_next_code_point(code_point_encoded);
                if (SCN_UNLIKELY(!decode_result)) {
                    return unexpected(decode_result.error());
                }

                if (until(decode_result->value)) {
                    break;
                }

                auto [in, out] = impl::copy(
                    code_point_encoded, ranges::subrange{dst, output.end()});
                if (in != code_point_encoded.end()) {
                    break;
                }

                ranges::advance(src,
                                static_cast<ranges::range_difference_t<InputR>>(
                                    code_point_encoded.size()));
                dst = out;
            }
            return {{src, dst}};
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
