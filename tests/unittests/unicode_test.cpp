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

#include "test_common.h"

#include <scn/scan.h>

#include <array>

using namespace std::string_view_literals;

TEST(UnicodeTest, Utf8CpLength)
{
    EXPECT_EQ(scn::detail::utf8_code_point_length_by_starting_code_unit('a'),
              1);
    EXPECT_EQ(scn::detail::utf8_code_point_length_by_starting_code_unit(
                  uint8_t{0xc4}),
              2);
    EXPECT_EQ(scn::detail::utf8_code_point_length_by_starting_code_unit(
                  uint8_t{0xe4}),
              3);
    EXPECT_EQ(scn::detail::utf8_code_point_length_by_starting_code_unit(
                  uint8_t{0xf4}),
              4);
    EXPECT_EQ(scn::detail::utf8_code_point_length_by_starting_code_unit(
                  uint8_t{0xa4}),
              0);
}

TEST(UnicodeTest, Utf8Decode)
{
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("a"),
              static_cast<char32_t>('a'));
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive_valid("a"),
              static_cast<char32_t>('a'));

    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("ä"),
              char32_t{0xe4});
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive_valid("ä"),
              char32_t{0xe4});

    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("�"),
              char32_t{0xfffd});
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive_valid("�"),
              char32_t{0xfffd});

    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("😀"),
              char32_t{0x1f600});
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive_valid("😀"),
              char32_t{0x1f600});
}

TEST(UnicodeTest, Utf16CpLength)
{
    EXPECT_EQ(scn::detail::utf16_code_point_length_by_starting_code_unit(
                  uint16_t{'a'}),
              1);
    EXPECT_EQ(scn::detail::utf16_code_point_length_by_starting_code_unit(
                  uint16_t{0xe4}),
              1);
    EXPECT_EQ(scn::detail::utf16_code_point_length_by_starting_code_unit(
                  uint16_t{0xdaaa}),
              2);
    EXPECT_EQ(scn::detail::utf16_code_point_length_by_starting_code_unit(
                  uint16_t{0xdeee}),
              0);
}

TEST(UnicodeTest, Utf8DecodeInvalid)
{
    constexpr auto invalid = scn::detail::invalid_code_point;
    constexpr std::array<std::string_view, 9> inputs{
        "\xc0\xaf"sv,          // overlong '/'
        "\xc1\xbf"sv,          // overlong U+007F
        "\xe0\x80\xaf"sv,      // overlong '/'
        "\xe0\x9f\xbf"sv,      // overlong U+07FF
        "\xf0\x80\x80\xaf"sv,  // overlong '/'
        "\xf0\x8f\xbf\xbf"sv,  // overlong U+FFFF
        "\xed\xa0\x80"sv,      // U+D800
        "\xed\xbf\xbf"sv,      // U+DFFF
        "\xf4\x90\x80\x80"sv,  // U+110000
    };
    for (auto input : inputs) {
        EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive(input),
                  invalid)
            << testing::PrintToString(input);
    }
}

TEST(UnicodeTest, Utf8DecodeBoundaries)
{
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("\xc2\x80"sv),
              char32_t{0x80});
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("\xe0\xa0\x80"sv),
              char32_t{0x800});
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("\xed\x9f\xbf"sv),
              char32_t{0xd7ff});
    EXPECT_EQ(scn::detail::decode_utf8_code_point_exhaustive("\xee\x80\x80"sv),
              char32_t{0xe000});
    EXPECT_EQ(
        scn::detail::decode_utf8_code_point_exhaustive("\xf0\x90\x80\x80"sv),
        char32_t{0x10000});
    EXPECT_EQ(
        scn::detail::decode_utf8_code_point_exhaustive("\xf4\x8f\xbf\xbf"sv),
        char32_t{0x10ffff});
}

TEST(UnicodeTest, Utf32DecodeSurrogate)
{
    EXPECT_EQ(scn::detail::decode_code_point_exhaustive(U"\xd800"sv),
              scn::detail::invalid_code_point);
    EXPECT_EQ(scn::detail::decode_code_point_exhaustive(U"\xdfff"sv),
              scn::detail::invalid_code_point);
    EXPECT_EQ(scn::detail::decode_code_point_exhaustive(U"\xe000"sv),
              char32_t{0xe000});
}

TEST(UnicodeTest, Utf16DecodeUnpairedSurrogate)
{
    EXPECT_EQ(scn::detail::decode_code_point_exhaustive(u"\xd800"sv),
              scn::detail::invalid_code_point);
    EXPECT_EQ(scn::detail::decode_code_point_exhaustive(u"\xdfff"sv),
              scn::detail::invalid_code_point);
    EXPECT_EQ(scn::detail::decode_code_point_exhaustive(u"\xe000"sv),
              char32_t{0xe000});
}

TEST(UnicodeTest, ScanOverlongUtf8)
{
    auto result = scn::scan<std::string>("\xc0\xaf", "{}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(UnicodeTest, ScanUtf8Surrogate)
{
    auto result = scn::scan<std::string>("\xed\xa0\x80", "{}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}
