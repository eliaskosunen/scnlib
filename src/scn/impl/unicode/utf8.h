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

#include <scn/detail/error.h>
#include <scn/impl/unicode/common.h>
#include <scn/util/expected.h>
#include <scn/util/span.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl::utf8 {
        constexpr std::size_t code_point_length(char ch)
        {
            SCN_GCC_COMPAT_PUSH
            SCN_GCC_COMPAT_IGNORE("-Wsign-conversion")
            const auto lengths =
                "\1\1\1\1\1\1\1\1"  // highest bit is 0 -> single-byte
                "\1\1\1\1\1\1\1\1"
                "\0\0\0\0\0\0\0\0"  // highest bits 10 -> error, non-initial
                                    // byte
                "\2\2\2\2"          // highest bits 110 -> 2-byte cp
                "\3\3"              // highest bits 1110 -> 3-byte cp
                "\4";               // highest bits 11110 -> 4-byte cp
            const int len = lengths[static_cast<unsigned char>(ch) >> 3];
            return len;
            SCN_GCC_COMPAT_POP
        }

        inline scan_expected<std::string_view::iterator> decode_code_point(
            std::string_view str,
            code_point& cp)
        {
            SCN_EXPECT(!str.empty());
            char32_t output{0};
            const auto len = code_point_length(str[0]);
            if (len == 0) {
                return unexpected_scan_error(
                    scan_error::invalid_encoding,
                    "Invalid UTF8, failed to decode single code point");
            }
            auto result =
                simdutf::convert_utf8_to_utf32(str.begin(), len, &output);
            if (SCN_UNLIKELY(result != 1)) {
                return unexpected_scan_error(
                    scan_error::invalid_encoding,
                    "Invalid UTF8, failed to decode single code point");
            }
            cp = static_cast<code_point>(output);
            return {str.begin() + len};
        }

        inline scan_expected<std::size_t> count_and_validate_code_points(
            std::string_view input)
        {
            SCN_EXPECT(!input.empty());
            if (const bool validation =
                    simdutf::validate_utf8(input.data(), input.size());
                !validation) {
                return unexpected_scan_error(
                    scan_error::invalid_encoding,
                    "Invalid UTF8, failed to validate");
            }

            return {
                simdutf::utf32_length_from_utf8(input.data(), input.size())};
        }

        inline span<code_point>::iterator decode_valid_code_points(
            std::string_view input,
            span<code_point> output)
        {
            SCN_EXPECT(!input.empty());
            const auto offset = simdutf::convert_valid_utf8_to_utf32(
                input.data(), input.size(),
                reinterpret_cast<char32_t*>(output.data()));
            return output.begin() + offset;
        }
    }  // namespace impl::utf8

    SCN_END_NAMESPACE
}  // namespace scn
