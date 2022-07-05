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

#include <scn/impl/unicode/utf16.h>
#include <scn/impl/unicode/utf8.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        scan_expected<std::size_t> code_point_length(CharT ch)
        {
            constexpr auto enc = get_encoding<CharT>();

            if constexpr (enc == encoding::utf8) {
                auto len = utf8::code_point_length(ch);
                if (len == 0) {
                    return unexpected_scan_error(scan_error::invalid_encoding,
                                                 "Invalid UTF-8 code point");
                }
                return len;
            }
            else if constexpr (enc == encoding::utf16) {
                return utf16::code_point_length(ch);
            }
            else if constexpr (enc == encoding::utf32) {
                return size_t{1};
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        auto decode_code_point_impl(std::basic_string_view<CharT> mapped_input,
                                    code_point& cp)
        {
            constexpr auto enc = get_encoding<CharT>();

            if constexpr (enc == encoding::utf8) {
                return utf8::decode_code_point(mapped_input, cp);
            }
            else if constexpr (enc == encoding::utf16) {
                return utf16::decode_code_point(mapped_input, cp);
            }
            else if constexpr (enc == encoding::utf32) {
                cp = static_cast<code_point>(mapped_input[0]);
                return detail::always_success_expected{
                    ranges::next(mapped_input.begin())};
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        scan_expected<typename std::basic_string_view<CharT>::iterator>
        decode_code_point(std::basic_string_view<CharT> input, code_point& cp)
        {
            const auto mapped_input = string_view_to_encoding<CharT>(input);

            return decode_code_point_impl(mapped_input, cp)
                .transform([&](auto it) SCN_NOEXCEPT {
                    return ranges::next(
                        input.begin(),
                        ranges::distance(mapped_input.begin(), it));
                });
        }

        template <typename CharT>
        scan_expected<std::size_t> count_and_validate_code_points(
            std::basic_string_view<CharT> input)
        {
            constexpr auto enc = get_encoding<CharT>();
            const auto mapped_input = string_view_to_encoding<CharT>(input);

            if constexpr (enc == encoding::utf8) {
                return utf8::count_and_validate_code_points(mapped_input);
            }
            else if constexpr (enc == encoding::utf16) {
                return utf16::count_and_validate_code_points(mapped_input);
            }
            else if constexpr (enc == encoding::utf32) {
                SCN_EXPECT(!mapped_input.empty());
                if (!simdutf::validate_utf32(mapped_input.data(),
                                             mapped_input.size())) {
                    return unexpected_scan_error(
                        scan_error::invalid_encoding,
                        "Invalid UTF-32, failed to validate");
                }
                return {input.size()};
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        scan_expected<std::size_t> count_and_validate_utf8_code_units(
            std::basic_string_view<CharT> input)
        {
            constexpr auto enc = get_encoding<CharT>();
            const auto mapped_input = string_view_to_encoding<CharT>(input);

            SCN_EXPECT(!mapped_input.empty());
            if constexpr (enc == encoding::utf8) {
                if (!simdutf::validate_utf8(mapped_input.data(),
                                            mapped_input.size())) {
                    return unexpected_scan_error(
                        scan_error::invalid_encoding,
                        "Invalid UTF-8, failed to validate");
                }
                return input.size();
            }
            else if constexpr (enc == encoding::utf16) {
                if (!simdutf::validate_utf16le(mapped_input.data(),
                                               mapped_input.size())) {
                    return unexpected_scan_error(
                        scan_error::invalid_encoding,
                        "Invalid UTF-16, failed to validate");
                }
                return simdutf::utf8_length_from_utf16le(mapped_input.data(),
                                                         mapped_input.size());
            }
            else if constexpr (enc == encoding::utf32) {
                if (!simdutf::validate_utf32(mapped_input.data(),
                                             mapped_input.size())) {
                    return unexpected_scan_error(
                        scan_error::invalid_encoding,
                        "Invalid UTF-32, failed to validate");
                }
                return simdutf::utf8_length_from_utf32(mapped_input.data(),
                                                       mapped_input.size());
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        span<code_point>::iterator decode_valid_code_points(
            std::basic_string_view<CharT> input,
            span<code_point> output)
        {
            constexpr auto enc = get_encoding<CharT>();
            const auto mapped_input = string_view_to_encoding<CharT>(input);

            if constexpr (enc == encoding::utf8) {
                return utf8::decode_valid_code_points(mapped_input, output);
            }
            else if constexpr (enc == encoding::utf16) {
                return utf8::decode_valid_code_points(mapped_input, output);
            }
            else if constexpr (enc == encoding::utf32) {
                SCN_EXPECT(output.size() >= input.size());
                SCN_UNUSED(mapped_input);
                std::memcpy(output.data(), input.data(),
                            input.size() * sizeof(CharT));
                return output.begin() + input.size();
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        span<char>::iterator encode_to_utf8(std::basic_string_view<CharT> input,
                                            span<char> output)
        {
            SCN_EXPECT(!input.empty());

            constexpr auto enc = get_encoding<CharT>();
            const auto mapped_input = string_view_to_encoding<CharT>(input);

            if constexpr (enc == encoding::utf8) {
                SCN_EXPECT(output.size() >= input.size());
                std::copy(input.begin(), input.end(), output.begin());
                return output.begin() + input.size();
            }
            else if constexpr (enc == encoding::utf16) {
                SCN_EXPECT(simdutf::validate_utf16le(mapped_input.data(),
                                                     mapped_input.size()));
                SCN_EXPECT(simdutf::utf8_length_from_utf16le(
                               mapped_input.data(), mapped_input.size()) <=
                           output.size());
                const auto offset = simdutf::convert_valid_utf16le_to_utf8(
                    mapped_input.data(), mapped_input.size(), output.data());
                return output.begin() + offset;
            }
            else if constexpr (enc == encoding::utf32) {
                SCN_EXPECT(simdutf::validate_utf32(mapped_input.data(),
                                                   mapped_input.size()));
                {
                    const auto len = simdutf::utf8_length_from_utf32(
                        mapped_input.data(), mapped_input.size());
                    SCN_EXPECT(len <= output.size());
                }
                const auto offset = simdutf::convert_valid_utf32_to_utf8(
                    mapped_input.data(), mapped_input.size(), output.data());
                return output.begin() + offset;
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        std::size_t count_code_units_in_valid_utf8(std::string_view input)
        {
            SCN_EXPECT(!input.empty());

            constexpr auto enc = get_encoding<CharT>();

            if constexpr (enc == encoding::utf8) {
                return input.size();
            }
            else if constexpr (enc == encoding::utf16) {
                return simdutf::utf16_length_from_utf8(input.data(),
                                                       input.size());
            }
            else if constexpr (enc == encoding::utf32) {
                return simdutf::utf32_length_from_utf8(input.data(),
                                                       input.size());
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        inline scan_error transcode_to_string(std::string_view source,
                                              std::wstring& dest)
        {
            constexpr auto source_enc = get_encoding<char>();
            constexpr auto dest_enc = get_encoding<wchar_t>();
            static_assert(source_enc != dest_enc);
            static_assert(source_enc == encoding::utf8);

            if (!simdutf::validate_utf8(source.data(), source.size())) {
                return {scan_error::invalid_encoding, "Invalid UTF-8"};
            }

            if constexpr (dest_enc == encoding::utf16) {
                const auto len = simdutf::utf16_length_from_utf8(source.data(),
                                                                 source.size());
                dest.resize(len);
                const auto utf16_chars_written =
                    simdutf::convert_valid_utf8_to_utf16le(
                        source.data(), source.size(),
                        reinterpret_cast<char16_t*>(dest.data()));
                SCN_ENSURE(utf16_chars_written == dest.size());
            }
            else if (dest_enc == encoding::utf32) {
                const auto len = simdutf::utf32_length_from_utf8(source.data(),
                                                                 source.size());
                dest.resize(len);
                const auto utf32_chars_written =
                    simdutf::convert_valid_utf8_to_utf32(
                        source.data(), source.size(),
                        reinterpret_cast<char32_t*>(dest.data()));
                SCN_ENSURE(utf32_chars_written == dest.size());
            }
            else {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
            return {};
        }

        inline scan_error transcode_to_string(std::wstring_view source,
                                              std::string& dest)
        {
            constexpr auto source_enc = get_encoding<wchar_t>();
            constexpr auto dest_enc = get_encoding<char>();
            static_assert(source_enc != dest_enc);
            static_assert(dest_enc == encoding::utf8);

            if constexpr (source_enc == encoding::utf16) {
                const auto source_data =
                    reinterpret_cast<const char16_t*>(source.data());

                if (!simdutf::validate_utf16le(source_data, source.size())) {
                    return {scan_error::invalid_encoding, "Invalid UTF-16"};
                }

                const auto len = simdutf::utf8_length_from_utf16le(
                    source_data, source.size());
                dest.resize(len);
                const auto utf8_chars_written =
                    simdutf::convert_valid_utf16le_to_utf8(
                        source_data, source.size(), dest.data());
                SCN_ENSURE(utf8_chars_written == dest.size());
            }
            else if (source_enc == encoding::utf32) {
                const auto source_data =
                    reinterpret_cast<const char32_t*>(source.data());

                if (!simdutf::validate_utf32(source_data, source.size())) {
                    return {scan_error::invalid_encoding, "Invalid UTF-32"};
                }

                const auto len =
                    simdutf::utf8_length_from_utf32(source_data, source.size());
                dest.resize(len);
                const auto utf8_chars_written =
                    simdutf::convert_valid_utf32_to_utf8(
                        source_data, source.size(), dest.data());
                SCN_ENSURE(utf8_chars_written == dest.size());
            }
            else {
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }
            return {};
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
