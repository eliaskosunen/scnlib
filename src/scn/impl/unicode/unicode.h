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

#include <scn/detail/unicode.h>
#include <scn/util/expected.h>
#include <scn/util/meta.h>
#include <scn/util/span.h>

#include <cstdint>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wdocumentation")
SCN_CLANG_IGNORE("-Wdocumentation-unknown-command")
SCN_CLANG_IGNORE("-Wnewline-eof")
SCN_CLANG_IGNORE("-Wextra-semi")

SCN_MSVC_PUSH
SCN_MSVC_IGNORE(4146)  // unary minus applied to unsigned

#include <simdutf.h>

SCN_MSVC_POP

SCN_CLANG_POP

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        enum class encoding : char { utf8, utf16, utf32, other };

        template <typename CharT>
        constexpr encoding get_encoding()
        {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wswitch-default")
            switch (sizeof(CharT)) {
                case 1:
                    return encoding::utf8;
                case 2:
                    return encoding::utf16;
                case 4:
                    return encoding::utf32;
            }
            return encoding::other;
            SCN_GCC_POP  // -Wswitch-default
        }

        template <typename U8>
        constexpr std::size_t utf8_code_point_length_by_starting_code_unit(
            U8 ch)
        {
            SCN_GCC_COMPAT_PUSH
            SCN_GCC_COMPAT_IGNORE("-Wsign-conversion")
            const auto lengths =
                "\1\1\1\1\1\1\1\1"  // highest bit is 0 -> single-byte
                "\1\1\1\1\1\1\1\1"
                "\0\0\0\0\0\0\0\0"  // highest bits 10 -> error, non-initial
                // byte
                "\2\2\2\2"  // highest bits 110 -> 2-byte cp
                "\3\3"      // highest bits 1110 -> 3-byte cp
                "\4";       // highest bits 11110 -> 4-byte cp
            const int len = lengths[static_cast<unsigned char>(ch) >> 3];
            return len;
            SCN_GCC_COMPAT_POP
        }

        template <typename U16>
        constexpr std::size_t utf16_code_point_length_by_starting_code_unit(
            U16 ch)
        {
            const auto lead = static_cast<uint16_t>(0xffff & ch);
            if (lead >= 0xd800 && lead <= 0xdbff) {
                // high surrogate
                return 2;
            }
            if (lead >= 0xdc00 && lead <= 0xdfff) {
                // unpaired low surrogate
                return 0;
            }
            return 1;
        }

        template <typename CharT>
        scan_expected<std::size_t> code_point_length_by_starting_code_unit(
            CharT ch)
        {
            constexpr auto enc = get_encoding<CharT>();

            if constexpr (enc == encoding::utf8) {
                auto len = utf8_code_point_length_by_starting_code_unit(ch);
                if (len == 0) {
                    return unexpected_scan_error(scan_error::invalid_encoding,
                                                 "Invalid UTF-8 code point");
                }
                return len;
            }
            else if constexpr (enc == encoding::utf16) {
                auto len = utf16_code_point_length_by_starting_code_unit(ch);
                if (len == 0) {
                    return unexpected_scan_error(scan_error::invalid_encoding,
                                                 "Invalid UTF-16 code point");
                }
                return len;
            }
            else if constexpr (enc == encoding::utf32) {
                return size_t{1};
            }
            else {
                SCN_UNUSED(ch);
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        auto get_next_code_point(std::basic_string_view<CharT> input)
            -> scan_expected<iterator_value_result<
                ranges::iterator_t<std::basic_string_view<CharT>>,
                code_point>>
        {
            SCN_EXPECT(!input.empty());

            const auto len = code_point_length_by_starting_code_unit(input[0]);
            if (!len) {
                return unexpected(len.error());
            }
            if (*len == 0) {
                return unexpected_scan_error(scan_error::invalid_encoding,
                                             "Invalid encoding");
            }

            constexpr auto enc = get_encoding<CharT>();
            std::size_t result{1};
            char32_t output{};
            if constexpr (enc == encoding::utf8) {
                result = simdutf::convert_utf8_to_utf32(
                    reinterpret_cast<const char*>(input.data()), *len, &output);
            }
            else if constexpr (enc == encoding::utf16) {
                result = simdutf::convert_utf16le_to_utf32(
                    reinterpret_cast<const char16_t*>(input.data()), *len,
                    &output);
            }
            else if constexpr (enc == encoding::utf32) {
                output = static_cast<char32_t>(input[0]);
            }

            if (result != 1) {
                return unexpected_scan_error(scan_error::invalid_encoding,
                                             "Invalid encoding");
            }

            return iterator_value_result<
                ranges::iterator_t<std::basic_string_view<CharT>>, code_point>{
                input.begin() + *len, static_cast<code_point>(output)};
        }

        template <typename CharT>
        bool validate_unicode(std::basic_string_view<CharT> input)
        {
            if (input.empty()) {
                return true;
            }

            constexpr auto enc = get_encoding<CharT>();
            if constexpr (enc == encoding::utf8) {
                return simdutf::validate_utf8(input.data(), input.size());
            }
            else if constexpr (enc == encoding::utf16) {
                return simdutf::validate_utf16le(
                    reinterpret_cast<const char16_t*>(input.data()),
                    input.size());
            }
            else if constexpr (enc == encoding::utf32) {
                return simdutf::validate_utf32(
                    reinterpret_cast<const char32_t*>(input.data()),
                    input.size());
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        std::size_t count_valid_code_points(std::basic_string_view<CharT> input)
        {
            if (input.empty()) {
                return 0;
            }

            SCN_EXPECT(validate_unicode(input));

            constexpr auto enc = get_encoding<CharT>();
            if constexpr (enc == encoding::utf8) {
                return simdutf::utf32_length_from_utf8(input.data(),
                                                       input.size());
            }
            else if constexpr (enc == encoding::utf16) {
                return simdutf::utf32_length_from_utf16le(
                    reinterpret_cast<char16_t*>(input.data()), input.size());
            }
            else if constexpr (enc == encoding::utf32) {
                return input.size();
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }

        template <typename CharT>
        scan_expected<std::size_t> validate_and_count_code_points(
            std::basic_string_view<CharT> input)
        {
            if (!validate_unicode(input)) {
                return unexpected_scan_error(scan_error::invalid_encoding,
                                             "Invalid encoding");
            }

            return count_valid_code_ponts(input);
        }

        template <typename DestCharT, typename SourceCharT>
        std::size_t count_valid_transcoded_code_units(
            std::basic_string_view<SourceCharT> input)
        {
            if (input.empty()) {
                return 0;
            }

            SCN_EXPECT(validate_unicode(input));

            constexpr auto src_enc = get_encoding<SourceCharT>();
            constexpr auto dest_enc = get_encoding<DestCharT>();

            if (src_enc == dest_enc) {
                return input.size();
            }

            if constexpr (src_enc == encoding::utf8) {
                if constexpr (dest_enc == encoding::utf16) {
                    return simdutf::utf16_length_from_utf8(input.data(),
                                                           input.size());
                }
                else {
                    return simdutf::utf32_length_from_utf8(input.data(),
                                                           input.size());
                }
            }
            else if constexpr (src_enc == encoding::utf16) {
                if constexpr (dest_enc == encoding::utf8) {
                    return simdutf::utf8_length_from_utf16le(
                        reinterpret_cast<const char16_t*>(input.data()),
                        input.size());
                }
                else {
                    return simdutf::utf32_length_from_utf16le(
                        reinterpret_cast<const char16_t*>(input.data()),
                        input.size());
                }
            }
            else if constexpr (src_enc == encoding::utf32) {
                if constexpr (dest_enc == encoding::utf8) {
                    return simdutf::utf8_length_from_utf32(
                        reinterpret_cast<const char32_t*>(input.data()),
                        input.size());
                }
                else {
                    return simdutf::utf16_length_from_utf32(
                        reinterpret_cast<const char32_t*>(input.data()),
                        input.size());
                }
            }
            else {
                static_assert(detail::dependent_false<SourceCharT>::type);
            }
        }

        template <typename DestCharT, typename SourceCharT>
        scan_expected<std::size_t> validate_and_count_transcoded_code_units(
            std::basic_string_view<SourceCharT> input)
        {
            if (!validate_unicode(input)) {
                return unexpected_scan_error(scan_error::invalid_encoding,
                                             "Invalid encoding");
            }

            return count_valid_transcoded_code_units<DestCharT>(input);
        }

        template <typename CharT>
        span<code_point>::iterator get_valid_code_points(
            std::basic_string_view<CharT> input,
            span<code_point> output)
        {
            if (input.empty()) {
                return output.begin();
            }

            SCN_EXPECT(validate_unicode(input));
            SCN_EXPECT(count_valid_code_ponts(input));

            constexpr auto enc = get_encoding<CharT>();
            std::size_t offset{0};
            if constexpr (enc == encoding::utf8) {
                offset = simdutf::convert_valid_utf8_to_utf32(
                    input.data(), input.size(),
                    reinterpret_cast<char32_t*>(output.data()));
            }
            else if constexpr (enc == encoding::utf16) {
                offset = simdutf::convert_valid_utf16le_to_utf32(
                    reinterpret_cast<const char16_t*>(input.data()),
                    input.size(), reinterpret_cast<char32_t*>(output.data()));
            }
            else if constexpr (enc == encoding::utf32) {
                SCN_EXPECT(output.size() >= input.size());
                std::memcpy(output.data(), input.size(),
                            input.size() * sizeof(CharT));
                offset = input.size();
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }

            return input.begin() + offset;
        }

        template <typename SourceCharT, typename DestCharT>
        std::size_t transcode_valid(std::basic_string_view<SourceCharT> input,
                                    span<DestCharT> output)
        {
            if (input.empty()) {
                return 0;
            }

            SCN_EXPECT(validate_unicode(input));
            SCN_EXPECT(count_valid_transcoded_code_units<DestCharT>(input) <=
                       output.size());

            constexpr auto src_enc = get_encoding<SourceCharT>();
            constexpr auto dest_enc = get_encoding<DestCharT>();

            if constexpr (src_enc == dest_enc) {
                std::memcpy(output.data(), input.data(), input.size());
                return input.size();
            }

            if constexpr (src_enc == encoding::utf8) {
                if constexpr (dest_enc == encoding::utf16) {
                    return simdutf::convert_valid_utf8_to_utf16le(
                        input.data(), input.size(),
                        reinterpret_cast<char16_t*>(output.data()));
                }
                else {
                    return simdutf::convert_valid_utf8_to_utf32(
                        input.data(), input.size(),
                        reinterpret_cast<char32_t*>(output.data()));
                }
            }
            else if constexpr (src_enc == encoding::utf16) {
                if constexpr (dest_enc == encoding::utf8) {
                    return simdutf::convert_valid_utf16le_to_utf8(
                        reinterpret_cast<const char16_t*>(input.data()),
                        input.size(), output.data());
                }
                else {
                    return simdutf::convert_valid_utf16le_to_utf32(
                        reinterpret_cast<const char16_t*>(input.data()),
                        input.size(),
                        reinterpret_cast<char32_t*>(output.data()));
                }
            }
            else if constexpr (src_enc == encoding::utf32) {
                if constexpr (dest_enc == encoding::utf8) {
                    return simdutf::convert_valid_utf32_to_utf8(
                        reinterpret_cast<const char32_t*>(input.data()),
                        input.size(), output.data());
                }
                else {
                    return simdutf::convert_valid_utf32_to_utf16le(
                        reinterpret_cast<const char32_t*>(input.data()),
                        input.size(),
                        reinterpret_cast<char16_t*>(output.data()));
                }
            }
            else {
                static_assert(detail::dependent_false<SourceCharT>::type);
            }
        }

        template <typename SourceCharT, typename DestCharT>
        void transcode_valid_to_string(
            std::basic_string_view<SourceCharT> source,
            std::basic_string<DestCharT>& dest)
        {
            SCN_EXPECT(validate_unicode(source));

            auto transcoded_length =
                count_valid_transcoded_code_units<DestCharT>(source);
            dest.resize(transcoded_length);

            const auto n = transcode_valid(
                source, span<DestCharT>{dest.data(), dest.size()});
            SCN_ENSURE(n == dest.size());
        }

        template <typename SourceCharT, typename DestCharT>
        bool transcode_to_string(std::basic_string_view<SourceCharT> source,
                                 std::basic_string<DestCharT>& dest)
        {
            if (!validate_unicode(source)) {
                return false;
            }

            transcode_valid_to_string(source, dest);
            return true;
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
