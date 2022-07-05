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

    namespace impl::utf16 {
        template <typename U16>
        constexpr std::size_t code_point_length(U16 ch)
        {
            const auto lead = static_cast<uint16_t>(0xffff & ch);
            if (lead >= 0xd800 && lead <= 0xdbff) {
                return 2;
            }
            return 1;
        }

        inline scan_expected<std::u16string_view::iterator> decode_code_point(
            std::u16string_view str,
            code_point& cp)
        {
            SCN_EXPECT(!str.empty());
            char32_t output{0};
            const auto len = code_point_length(str[0]);
            auto result =
                simdutf::convert_utf16le_to_utf32(str.begin(), len, &output);
            if (SCN_UNLIKELY(result != 1)) {
                return unexpected_scan_error(
                    scan_error::invalid_encoding,
                    "Invalid UTF16, failed to decode single code point");
            }
            cp = static_cast<code_point>(output);
            return {str.begin() + len};
        }

        inline scan_expected<std::size_t> count_and_validate_code_points(
            std::u16string_view input)
        {
            SCN_EXPECT(!input.empty());
            if (const bool validation =
                    simdutf::validate_utf16le(input.data(), input.size());
                !validation) {
                return unexpected_scan_error(scan_error::invalid_encoding,
                                  "Invalid UTF16, failed to validate");
            }

            return {
                simdutf::utf32_length_from_utf16le(input.data(), input.size())};
        }

        inline span<code_point>::iterator decode_valid_code_points(
            std::u16string_view input,
            span<code_point>& output)
        {
            SCN_EXPECT(!input.empty());
            const auto offset = simdutf::convert_valid_utf16le_to_utf32(
                input.data(), input.size(),
                reinterpret_cast<char32_t*>(output.data()));
            return output.begin() + offset;
        }
    }  // namespace impl::utf16

    SCN_END_NAMESPACE
}  // namespace scn
