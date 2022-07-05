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

#include <scn/scan.h>

#include <gtest/gtest.h>

TEST(StringViewTest, DefaultNarrowStringViewFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string_view>("abc def", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, "abc");
}
TEST(StringViewTest, DefaultWideStringViewFromWideSource)
{
    auto [result, str] = scn::scan<std::wstring_view>(L"abc def", L"{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringViewTest, StringPresentationNarrowStringViewFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string_view>("abc def", "{:s}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, "abc");
}
TEST(StringViewTest, StringPresentationWideStringViewFromWideSource)
{
    auto [result, str] = scn::scan<std::wstring_view>(L"abc def", L"{:s}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringViewTest, CharacterPresentationWithNoWidthCausesError)
{
    auto [result, _] =
        scn::scan<std::string_view>("abc def", scn::runtime("{:c}"));
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(StringViewTest, CharacterPresentationNarrowStringViewFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string_view>("abc def", "{:4c}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), "def");
    EXPECT_EQ(str, "abc ");
}
TEST(StringViewTest, CharacterPresentationWideStringViewFromWideSource)
{
    auto [result, str] = scn::scan<std::wstring_view>(L"abc def", L"{:4c}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L"def");
    EXPECT_EQ(str, L"abc ");
}

TEST(StringViewTest, CharacterSetPresentationNarrowStringViewFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string_view>("abc def", "{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, "abc");
}
TEST(StringViewTest, CharacterSetPresentationWideStringViewFromWideSource)
{
    auto [result, str] =
        scn::scan<std::wstring_view>(L"abc def", L"{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringViewTest, InvalidUtf8)
{
    auto source = std::string_view{"\x82\xf5"};
    auto [result, _] = scn::scan<std::string_view>(source, "{:64U}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), scn::scan_error::invalid_encoding);
}

TEST(StringViewTest, WonkyInput)
{
    auto source = std::string_view{"o \U0000000f\n\n\xc3"};
    auto [result, str] = scn::scan<std::string_view>(source, "{:64U}");
    std::tie(result, str) =
        scn::scan<std::string_view>(result.range(), "{:64U}");
    std::tie(result, str) =
        scn::scan<std::string_view>(result.range(), "{:64U}");
    std::tie(result, str) =
        scn::scan<std::string_view>(result.range(), "{:64U}");
    std::tie(result, str) =
        scn::scan<std::string_view>(result.range(), "{:64U}");
}
TEST(StringViewTest, WonkyInput2)
{
    const char source[] = {'o', ' ', '\x0f', '\n', '\n', '\xc3'};
    auto range = scn::scan_map_input_range(source);

    auto [result, str] = scn::scan<std::string_view>(range, "{:64U}");
    EXPECT_TRUE(result);
    range = std::move(result.range());

    auto [newresult, newstr] = scn::scan<std::string_view>(range, "{:64U}");
    EXPECT_FALSE(newresult);
}
