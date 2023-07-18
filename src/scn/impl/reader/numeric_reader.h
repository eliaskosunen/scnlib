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

#include <scn/impl/algorithms/read_nocopy.h>
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
            scan_expected<ranges::borrowed_iterator_t<Range>> read_sign(
                Range&& range,
                sign& p_sign)
            {
                return read_one_of_code_unit(range, "+-")
                    .and_then(
                        [&](auto it) -> scan_expected<
                                         ranges::borrowed_iterator_t<Range>> {
                            if (*ranges::begin(range) == '-') {
                                p_sign = sign::minus_sign;
                            }
                            else {
                                p_sign = sign::plus_sign;
                            }
                            return it;
                        })
                    .or_else(
                        [&](auto err) -> scan_expected<
                                          ranges::borrowed_iterator_t<Range>> {
                            if (err.code() ==
                                scan_error::invalid_scanned_value) {
                                return ranges::begin(range);
                            }
                            return unexpected(err);
                        });
            }

            template <typename Range>
            scan_expected<ranges::borrowed_iterator_t<Range>>
            check_thsep_grouping(Range&& range,
                                 std::string& thsep_indices,
                                 std::string_view grouping)
            {
                SCN_EXPECT(!thsep_indices.empty());

                return ranges::end(range);
            }
        };

        template <typename Derived, typename CharT>
        class numeric_reader : public reader_base<Derived, CharT>,
                               public numeric_reader_base {
        protected:
            contiguous_range_factory<CharT> m_buffer{};
        };
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
