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
#include <scn/impl/unicode/unicode.h>
#include <scn/impl/util/ascii_ctype.h>
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
        SCN_NODISCARD read_copying_result<InputR, OutputR> read_n_copying(
            InputR&& input,
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

        template <typename InputR, typename OutputR, typename Pred>
        SCN_NODISCARD read_copying_result<InputR, OutputR>
        read_until_classic_copying(InputR&& input,
                                   OutputR&& output,
                                   Pred&& until)
        {
            SCN_EXPECT(!ranges::empty(input));
            SCN_EXPECT(ranges::begin(output) != ranges::end(output));

            auto found = ranges::find_if(input, until);
            if (found == ranges::end(input)) {
                return read_all_copying(SCN_FWD(input), SCN_FWD(output));
            }

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
                [](typename ranges::range_value_t<InputR> ch)
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
                auto len = code_point_length(*src);
                if (!len) {
                    return unexpected(len.error());
                }

                if (*len == 1) {
                    if (until(static_cast<code_point>(*src))) {
                        return {{src, dst}};
                    }
                    *dst = *src;
                    ++src;
                    ++dst;
                    continue;
                }

                const auto src_copy = src;

                using char_type = ranges::range_value_t<InputR>;
                std::array<char_type, 4 / sizeof(char_type)> buffer{};
                buffer[0] = *src;
                --*len;
                ++src;
                auto buffer_it = buffer.begin() + 1;
                for (; *len != 0; --*len, (void)++buffer_it, (void)++src) {
                    // False positive
                    SCN_GCC_PUSH
                    SCN_GCC_IGNORE("-Wstringop-overflow")

                    *buffer_it = *src;

                    SCN_GCC_POP
                }

                code_point cp{};
                auto decode_sv = std::basic_string_view<char_type>{
                    buffer.data(),
                    static_cast<size_t>(buffer_it - buffer.begin())};
                auto decode_result = decode_code_point(decode_sv, cp);
                if (!decode_result) {
                    return unexpected(decode_result.error());
                }
                SCN_ENSURE(scn::detail::to_address_safe(*decode_result,
                                                        decode_sv.begin(),
                                                        decode_sv.end()) ==
                           scn::detail::to_address_safe(
                               buffer_it, buffer.begin(), buffer.end()));

                if (until(cp)) {
                    return {{src_copy, dst}};
                }

                auto [in, out] =
                    impl::copy(decode_sv, ranges::subrange{dst, output.end()});
                if (in != decode_sv.end()) {
                    return {{src_copy, dst}};
                }
                dst = out;
            }
            return {{src, dst}};
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
