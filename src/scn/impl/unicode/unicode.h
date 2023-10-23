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
#include <scn/impl/algorithms/common.h>
#include <scn/util/expected.h>
#include <scn/util/meta.h>
#include <scn/util/span.h>
#include <scn/util/string_view.h>

#include <cstdint>

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wold-style-cast")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wdocumentation")
SCN_CLANG_IGNORE("-Wdocumentation-unknown-command")
SCN_CLANG_IGNORE("-Wnewline-eof")
SCN_CLANG_IGNORE("-Wextra-semi")
SCN_CLANG_IGNORE("-Wold-style-cast")

SCN_MSVC_PUSH
SCN_MSVC_IGNORE(4146)  // unary minus applied to unsigned

#include <simdutf.h>

SCN_MSVC_POP

SCN_CLANG_POP

SCN_GCC_POP

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        enum class encoding { utf8 = 1, utf16 = 2, utf32 = 4 };

        template <typename CharT>
        constexpr encoding get_encoding()
        {
            static_assert(sizeof(CharT) == 1 || sizeof(CharT) == 2 ||
                          sizeof(CharT) == 4);
            constexpr std::array<encoding, 5> options = {
                encoding::utf8,   // 0 (error)
                encoding::utf8,   // 1
                encoding::utf16,  // 2
                encoding::utf16,  // 3 (error)
                encoding::utf32,  // 4
            };
            return options[sizeof(CharT)];
        }

        constexpr size_t max_code_point_length_in_encoding(encoding enc)
        {
            if (enc == encoding::utf8) {
                return 4;
            }
            else if (enc == encoding::utf16) {
                return 2;
            }
            else {
                return 1;
            }
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
                return simdutf::validate_utf16(
                    reinterpret_cast<const char16_t*>(input.data()),
                    input.size());
            }
            else if constexpr (enc == encoding::utf32) {
                return simdutf::validate_utf32(
                    reinterpret_cast<const char32_t*>(input.data()),
                    input.size());
            }
        }

        template <typename CharT>
        std::size_t code_point_length_by_starting_code_unit(CharT ch)
        {
            return detail::utf_code_point_length_by_starting_code_unit(ch);
        }

        template <typename CharT>
        char32_t decode_code_point_exhaustive(
            std::basic_string_view<CharT> input)
        {
            SCN_EXPECT(!input.empty());

            constexpr auto enc = get_encoding<CharT>();
            char32_t output{};
            size_t ret{};
            if constexpr (enc == encoding::utf8) {
                ret = simdutf::convert_utf8_to_utf32(
                    reinterpret_cast<const char*>(input.data()), input.size(),
                    &output);
            }
            else if constexpr (enc == encoding::utf16) {
                ret = simdutf::convert_utf16_to_utf32(
                    reinterpret_cast<const char16_t*>(input.data()),
                    input.size(), &output);
            }
            else if constexpr (enc == encoding::utf32) {
                output = static_cast<char32_t>(input[0]);
                ret = output < detail::invalid_code_point;
            }

            if (SCN_UNLIKELY(ret != 1)) {
                return detail::invalid_code_point;
            }
            return static_cast<char32_t>(output);
        }

        template <typename CharT>
        char32_t decode_code_point_exhaustive_valid(
            std::basic_string_view<CharT> input)
        {
            SCN_EXPECT(!input.empty());

            {
                const auto len =
                    code_point_length_by_starting_code_unit(input[0]);
                SCN_EXPECT(len == input.size());
            }

            SCN_EXPECT(validate_unicode(input));

            constexpr auto enc = get_encoding<CharT>();
            char32_t output;
            if constexpr (enc == encoding::utf8) {
                const auto ret = simdutf::convert_valid_utf8_to_utf32(
                    reinterpret_cast<const char*>(input.data()), input.size(),
                    &output);
                SCN_EXPECT(ret == 1);
            }
            else if constexpr (enc == encoding::utf16) {
                const auto ret = simdutf::convert_valid_utf16_to_utf32(
                    reinterpret_cast<const char16_t*>(input.data()),
                    input.size(), &output);
                SCN_EXPECT(ret == 1);
            }
            else if constexpr (enc == encoding::utf32) {
                return static_cast<char32_t>(input[0]);
            }

            return static_cast<char32_t>(output);
        }

        inline scan_expected<wchar_t> encode_code_point_as_wide_character(
            char32_t cp,
            bool error_on_overflow)
        {
            constexpr auto enc = get_encoding<wchar_t>();
            if constexpr (enc == encoding::utf32) {
                return static_cast<wchar_t>(cp);
            }
            else if constexpr (enc == encoding::utf16) {
                char16_t buf[2]{};
                auto result = simdutf::convert_valid_utf32_to_utf16(
                    reinterpret_cast<const char32_t*>(&cp), 1, buf);
                if (result != 1 && error_on_overflow) {
                    return unexpected_scan_error(scan_error::value_out_of_range,
                                                 "Non-BOM code point can't be "
                                                 "narrowed to a single 2-byte "
                                                 "wchar_t code unit");
                }
                return static_cast<wchar_t>(buf[0]);
            }
        }

        template <typename CharT>
        auto get_next_code_point_valid(std::basic_string_view<CharT> input)
            -> iterator_value_result<
                ranges::iterator_t<std::basic_string_view<CharT>>,
                char32_t>
        {
            SCN_EXPECT(!input.empty());
            SCN_EXPECT(validate_unicode(input));

            const auto len = code_point_length_by_starting_code_unit(input[0]);
            SCN_EXPECT(len != 0);

            constexpr auto enc = get_encoding<CharT>();
            char32_t output{};
            if constexpr (enc == encoding::utf8) {
                const auto ret = simdutf::convert_valid_utf8_to_utf32(
                    reinterpret_cast<const char*>(input.data()), len, &output);
                SCN_EXPECT(ret == 1);
            }
            else if constexpr (enc == encoding::utf16) {
                const auto ret = simdutf::convert_valid_utf16_to_utf32(
                    reinterpret_cast<const char16_t*>(input.data()), len,
                    &output);
                SCN_EXPECT(ret == 1);
            }
            else if constexpr (enc == encoding::utf32) {
                output = static_cast<char32_t>(input[0]);
            }

            return iterator_value_result<
                ranges::iterator_t<std::basic_string_view<CharT>>, char32_t>{
                input.begin() + len, static_cast<char32_t>(output)};
        }

        template <typename CharT>
        auto get_start_of_next_code_point(std::basic_string_view<CharT> input)
            -> ranges::iterator_t<std::basic_string_view<CharT>>
        {
            auto it = input.begin();
            for (; it != input.end(); ++it) {
                if (code_point_length_by_starting_code_unit(*it) != 0) {
                    break;
                }
            }

            return it;
        }

        template <typename CharT>
        auto get_next_code_point(std::basic_string_view<CharT> input)
            -> iterator_value_result<
                ranges::iterator_t<std::basic_string_view<CharT>>,
                char32_t>
        {
            SCN_EXPECT(!input.empty());

            const auto len = code_point_length_by_starting_code_unit(input[0]);
            if (SCN_UNLIKELY(len == 0)) {
                return {get_start_of_next_code_point(input),
                        detail::invalid_code_point};
            }

            constexpr auto enc = get_encoding<CharT>();
            std::size_t result{1};
            char32_t output{};
            if constexpr (enc == encoding::utf8) {
                result = simdutf::convert_utf8_to_utf32(
                    reinterpret_cast<const char*>(input.data()), len, &output);
            }
            else if constexpr (enc == encoding::utf16) {
                result = simdutf::convert_utf16_to_utf32(
                    reinterpret_cast<const char16_t*>(input.data()), len,
                    &output);
            }
            else if constexpr (enc == encoding::utf32) {
                SCN_EXPECT(len == 1);
                output = static_cast<char32_t>(input[0]);
            }

            if (SCN_UNLIKELY(result != 1)) {
                return {get_start_of_next_code_point(input.substr(1)),
                        detail::invalid_code_point};
            }

            return {input.begin() + len, output};
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
                return simdutf::utf32_length_from_utf16(
                    reinterpret_cast<const char16_t*>(input.data()),
                    input.size());
            }
            else if constexpr (enc == encoding::utf32) {
                return input.size();
            }
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

            if constexpr (src_enc == dest_enc) {
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
                    return simdutf::utf8_length_from_utf16(
                        reinterpret_cast<const char16_t*>(input.data()),
                        input.size());
                }
                else {
                    return simdutf::utf32_length_from_utf16(
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
        }

        template <typename CharT>
        span<char32_t>::iterator get_valid_code_points(
            std::basic_string_view<CharT> input,
            span<char32_t> output)
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
                offset = simdutf::convert_valid_utf16_to_utf32(
                    reinterpret_cast<const char16_t*>(input.data()),
                    input.size(), reinterpret_cast<char32_t*>(output.data()));
            }
            else if constexpr (enc == encoding::utf32) {
                SCN_EXPECT(output.size() >= input.size());
                std::memcpy(output.data(), input.size(),
                            input.size() * sizeof(CharT));
                offset = input.size();
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
                std::memcpy(output.data(), input.data(),
                            input.size() * sizeof(SourceCharT));
                return input.size();
            }

            if constexpr (src_enc == encoding::utf8) {
                if constexpr (dest_enc == encoding::utf16) {
                    return simdutf::convert_valid_utf8_to_utf16(
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
                    return simdutf::convert_valid_utf16_to_utf8(
                        reinterpret_cast<const char16_t*>(input.data()),
                        input.size(), output.data());
                }
                else {
                    return simdutf::convert_valid_utf16_to_utf32(
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
                    return simdutf::convert_valid_utf32_to_utf16(
                        reinterpret_cast<const char32_t*>(input.data()),
                        input.size(),
                        reinterpret_cast<char16_t*>(output.data()));
                }
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
        void transcode_to_string(std::basic_string_view<SourceCharT> source,
                                 std::basic_string<DestCharT>& dest)
        {
            static_assert(!std::is_same_v<SourceCharT, DestCharT>);

            auto do_transcode = [&](std::basic_string_view<SourceCharT> src,
                                    span<DestCharT> dst) {
                if constexpr (sizeof(SourceCharT) == 1 &&
                              sizeof(DestCharT) == 2) {
                    return simdutf::convert_utf8_to_utf16_with_errors(
                        src.data(), src.size(),
                        reinterpret_cast<char32_t*>(dst.data()));
                }
                else if constexpr (sizeof(SourceCharT) == 1 &&
                                   sizeof(DestCharT) == 4) {
                    return simdutf::convert_utf8_to_utf32_with_errors(
                        src.data(), src.size(),
                        reinterpret_cast<char32_t*>(dst.data()));
                }
                else if constexpr (sizeof(SourceCharT) == 2 &&
                                   sizeof(DestCharT) == 1) {
                    return simdutf::convert_utf16_to_utf8_with_errors(
                        reinterpret_cast<const char16_t*>(src.data()),
                        src.size(), dst.data());
                }
                else if constexpr (sizeof(SourceCharT) == 2 &&
                                   sizeof(DestCharT) == 4) {
                    return simdutf::convert_utf16_to_utf32_with_errors(
                        reinterpret_cast<const char16_t*>(src.data()),
                        src.size(), reinterpret_cast<char32_t*>(dst.data()));
                }
                else if constexpr (sizeof(SourceCharT) == 4 &&
                                   sizeof(DestCharT) == 1) {
                    return simdutf::convert_utf32_to_utf8_with_errors(
                        reinterpret_cast<const char32_t*>(src.data()),
                        src.size(), dst.data());
                }
                else if constexpr (sizeof(SourceCharT) == 4 &&
                                   sizeof(DestCharT) == 2) {
                    return simdutf::convert_utf32_to_utf16_with_errors(
                        reinterpret_cast<const char32_t*>(src.data()),
                        src.size(), reinterpret_cast<char16_t*>(dst.data()));
                }
            };

            auto it = source.begin();
            std::array<DestCharT,
                       32 * std::max(sizeof(DestCharT) / sizeof(SourceCharT),
                                     size_t{1})>
                tmp{};
            while (it != source.end()) {
                auto sv = detail::make_string_view_from_iterators<SourceCharT>(
                              it, source.end())
                              .substr(0, 32);

                auto tmp_view = span<DestCharT>{tmp.data(), tmp.size()};
                auto res = do_transcode(sv, tmp_view);
                if (SCN_UNLIKELY(res.error != simdutf::SUCCESS)) {
                    auto valid_sv = sv.substr(0, res.count);
                    auto n =
                        count_valid_transcoded_code_units<DestCharT>(valid_sv);
                    // Replacement character U+fffd
                    if constexpr (sizeof(DestCharT) == 1) {
                        tmp_view[n + 0] = static_cast<DestCharT>(0xef);
                        tmp_view[n + 1] = static_cast<DestCharT>(0xbf);
                        tmp_view[n + 2] = static_cast<DestCharT>(0xbd);
                        tmp_view = tmp_view.first(n + 3);
                    }
                    else {
                        tmp_view[n + 0] = static_cast<DestCharT>(0xfffd);
                        tmp_view = tmp_view.first(n + 1);
                    }
                }
                else {
                    tmp_view = tmp_view.first(res.count);
                }

                dest.append(tmp_view.data(), tmp_view.size());
                it = sv.end();
            }
        }

        template <typename CharT, typename Cb>
        void for_each_code_point(std::basic_string_view<CharT> input, Cb&& cb)
        {
            auto it = input.begin();
            while (it != input.end()) {
                auto res = get_next_code_point(
                    detail::make_string_view_from_iterators<CharT>(
                        it, input.end()));
                cb(res.value);

                it += ranges::distance(detail::to_address(it),
                                       detail::to_address(res.iterator));
            }
        }

        template <typename CharT, typename Cb>
        void for_each_code_point_valid(std::basic_string_view<CharT> input,
                                       Cb&& cb)
        {
            auto it = input.begin();
            while (it != input.end()) {
                auto res = get_next_code_point_valid(
                    detail::make_string_view_from_iterators<CharT>(
                        it, input.end()));
                cb(res.value);

                it += ranges::distance(detail::to_address(it),
                                       detail::to_address(res.iterator));
            }
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
