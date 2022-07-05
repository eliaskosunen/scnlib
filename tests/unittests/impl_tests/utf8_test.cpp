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

#include <gtest/gtest.h>

#include <scn/impl/unicode/utf8.h>

namespace {
    constexpr auto zero = scn::make_code_point(0);
    constexpr auto latin_small_letter_a = scn::make_code_point(0x61);  // a
    constexpr auto latin_small_letter_a_with_diaeresis =
        scn::make_code_point(0xe4);                           // ä, 2 bytes
    constexpr auto euro_sign = scn::make_code_point(0x20ac);  // €, 3 bytes
    constexpr auto slightly_smiling_face =
        scn::make_code_point(0x1f642);  // 🙂, 4 bytes

    std::string code_point_to_string(scn::code_point cp)
    {
        std::string buf(4, '\0');
        const auto ch = static_cast<char32_t>(cp);
        auto res = simdutf::convert_valid_utf32_to_utf8(&ch, 1, buf.data());
        SCN_ENSURE(res != 0);
        buf.resize(res);
        return buf;
    }
}  // namespace

TEST(Utf8Test, CodePointLengthAndDecode)
{
    {
        auto a = code_point_to_string(latin_small_letter_a);
        EXPECT_EQ(scn::impl::utf8::code_point_length(a[0]), 1);

        scn::code_point cp{};
        auto res = scn::impl::utf8::decode_code_point(a, cp);
        EXPECT_TRUE(res);
        EXPECT_EQ(res.value(), a.data() + a.size());
        EXPECT_EQ(cp, latin_small_letter_a);
    }

    {
        auto a_with_dots =
            code_point_to_string(latin_small_letter_a_with_diaeresis);
        EXPECT_EQ(scn::impl::utf8::code_point_length(a_with_dots[0]), 2);

        scn::code_point cp{};
        auto res = scn::impl::utf8::decode_code_point(a_with_dots, cp);
        EXPECT_TRUE(res);
        EXPECT_EQ(res.value(), a_with_dots.data() + a_with_dots.size());
        EXPECT_EQ(cp, latin_small_letter_a_with_diaeresis);
    }

    {
        auto euro = code_point_to_string(euro_sign);
        EXPECT_EQ(scn::impl::utf8::code_point_length(euro[0]), 3);

        scn::code_point cp{};
        auto res = scn::impl::utf8::decode_code_point(euro, cp);
        EXPECT_TRUE(res);
        EXPECT_EQ(res.value(), euro.data() + euro.size());
        EXPECT_EQ(cp, euro_sign);
    }

    {
        auto emoji = code_point_to_string(slightly_smiling_face);
        EXPECT_EQ(scn::impl::utf8::code_point_length(emoji[0]), 4);

        scn::code_point cp{};
        auto res = scn::impl::utf8::decode_code_point(emoji, cp);
        EXPECT_TRUE(res);
        EXPECT_EQ(res.value(), emoji.data() + emoji.size());
        EXPECT_EQ(cp, slightly_smiling_face);
    }
}

TEST(Utf8Test, CountAndDecodeCodePoints)
{
    const std::string_view input{"aä€🙂"};
    EXPECT_EQ(input.size(), 10);

    const auto len = scn::impl::utf8::count_and_validate_code_points(input);
    EXPECT_TRUE(len);
    EXPECT_EQ(len.value(), 4);

    std::vector<scn::code_point> cps(len.value(), zero);
    const auto it =
        scn::impl::utf8::decode_valid_code_points(input, scn::make_span(cps));
    EXPECT_EQ(it, cps.data() + cps.size());
    EXPECT_EQ(cps[0], latin_small_letter_a);
    EXPECT_EQ(cps[1], latin_small_letter_a_with_diaeresis);
    EXPECT_EQ(cps[2], euro_sign);
    EXPECT_EQ(cps[3], slightly_smiling_face);
}
