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

#include <scn/impl/algorithms/read.h>
#include <scn/impl/reader/common.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        struct numeric_reader_base {
            enum class sign {
                default_sign = -1,
                minus_sign = 0,
                plus_sign = 1
            };

            SCN_NODISCARD static uint8_t char_to_int(char ch)
            {
                static constexpr std::array<uint8_t, 256> digits_arr = {
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
                    255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,
                    17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
                    29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255,
                    255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
                    21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
                    33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255};
                return digits_arr[static_cast<unsigned char>(ch)];
            }
            SCN_NODISCARD uint8_t static char_to_int(wchar_t ch)
            {
#if WCHAR_MIN < 0
                if (ch >= 0 && ch <= 255) {
#else
                if (ch <= 255) {
#endif
                    return char_to_int(static_cast<char>(ch));
                }
                return 255;
            }

        protected:
            template <typename Range>
            eof_expected<simple_borrowed_iterator_t<Range>> read_sign(
                Range&& range,
                sign& p_sign)
            {
                auto r = read_one_of_code_unit(range, "+-");
                if (SCN_UNLIKELY(!r)) {
                    if (r.error() == parse_error::error) {
                        return ranges::begin(range);
                    }
                    return unexpected(eof_error::eof);
                }

                auto& it = *r;
                if (*ranges::begin(range) == '-') {
                    p_sign = sign::minus_sign;
                }
                else {
                    p_sign = sign::plus_sign;
                }
                return it;
            }

            template <typename Range,
                      std::enable_if_t<ranges::view<Range>>* = nullptr>
            scan_error check_thsep_grouping(Range&& range,
                                            std::string thsep_indices,
                                            std::string_view grouping)
            {
                SCN_EXPECT(!thsep_indices.empty());

                if (!check_thsep_grouping_impl(range, thsep_indices,
                                               grouping)) {
                    SCN_UNLIKELY_ATTR
                    return {scan_error::invalid_scanned_value,
                            "Invalid thousands separator grouping"};
                }

                return {};
            }

        private:
            template <typename Range>
            bool check_thsep_grouping_impl(Range& range,
                                           std::string& thsep_indices,
                                           std::string_view grouping)
            {
                transform_thsep_indices(
                    thsep_indices,
                    ranges::distance(ranges::begin(range), ranges::end(range)));

                auto thsep_it = thsep_indices.rbegin();
                for (auto grouping_it = grouping.begin();
                     grouping_it != grouping.end() &&
                     thsep_it != thsep_indices.rend() - 1;
                     ++grouping_it, (void)++thsep_it) {
                    if (*thsep_it != *grouping_it) {
                        return false;
                    }
                }

                SCN_CLANG_PUSH
                // false positive
                SCN_CLANG_IGNORE("-Wzero-as-null-pointer-constant")

                for (; thsep_it < thsep_indices.rend() - 1; ++thsep_it) {
                    if (*thsep_it != grouping.back()) {
                        return false;
                    }
                }

                if (thsep_it == thsep_indices.rend() - 1) {
                    if (*thsep_it > grouping.back()) {
                        return false;
                    }
                }

                SCN_CLANG_POP

                return true;
            }

            static void transform_thsep_indices(std::string& indices,
                                                std::ptrdiff_t last_thsep_index)
            {
                for (auto thsep_it = indices.rbegin();
                     thsep_it != indices.rend(); ++thsep_it) {
                    const auto tmp = *thsep_it;
                    *thsep_it = static_cast<char>(last_thsep_index - tmp - 1);
                    last_thsep_index = static_cast<std::ptrdiff_t>(tmp);
                }
                indices.insert(indices.begin(),
                               static_cast<char>(last_thsep_index));
            }
        };

        template <typename CharT>
        class numeric_reader : public numeric_reader_base {
        protected:
            contiguous_range_factory<CharT> m_buffer{};
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
