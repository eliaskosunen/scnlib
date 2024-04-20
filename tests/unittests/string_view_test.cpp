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
#include <scn/xchar.h>

#include "wrapped_gtest.h"

TEST(StringViewTest, DefaultNarrowStringViewFromNarrowSource)
{
    auto result = scn::scan<std::string_view>("abc def", "{}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringViewTest, DefaultWideStringViewFromWideSource)
{
    auto result = scn::scan<std::wstring_view>(L"abc def", L"{}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringViewTest, StringPresentationNarrowStringViewFromNarrowSource)
{
    auto result = scn::scan<std::string_view>("abc def", "{:s}");
    EXPECT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringViewTest, StringPresentationWideStringViewFromWideSource)
{
    auto result = scn::scan<std::wstring_view>(L"abc def", L"{:s}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringViewTest, CharacterPresentationWithNoWidthCausesError)
{
    auto result =
        scn::scan<std::string_view>("abc def", scn::runtime_format("{:c}"));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(StringViewTest, CharacterPresentationNarrowStringViewFromNarrowSource)
{
    auto result = scn::scan<std::string_view>("abc def", "{:.4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), "def");
    EXPECT_EQ(result->value(), "abc ");
}
TEST(StringViewTest, CharacterPresentationWideStringViewFromWideSource)
{
    auto result = scn::scan<std::wstring_view>(L"abc def", L"{:.4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L"def");
    EXPECT_EQ(result->value(), L"abc ");
}

TEST(StringViewTest, CharacterSetPresentationNarrowStringViewFromNarrowSource)
{
    auto result = scn::scan<std::string_view>("abc def", "{:[a-z]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringViewTest, CharacterSetPresentationWideStringViewFromWideSource)
{
    auto result = scn::scan<std::wstring_view>(L"abc def", L"{:[a-z]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringViewTest, InvalidUtf8)
{
    auto source = std::string_view{"\x82\xf5"};
    auto result = scn::scan<std::string_view>(source, "{:.64c}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
#if 0
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    EXPECT_EQ(result->value(), source);
#endif
}

TEST(StringViewTest, WonkyInput)
{
    auto source = std::string_view{"o \U0000000f\n\n\xc3"};
    auto it = source.begin();
    for (int i = 0; i < 5; ++i) {
        if (it == source.end()) {
            break;
        }
        auto result = scn::scan<std::string_view>(
            scn::ranges::subrange{it, source.end()}, "{:.64c}");
        if (result) {
            it = result->begin();
        }
    }
}
TEST(StringViewTest, WonkyInput2)
{
    const char source[] = {'o', ' ', '\x0f', '\n', '\n', '\xc3'};
    auto input = std::string_view{source, sizeof(source)};

    auto result = scn::scan<std::string_view>(input, "{:.64c}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
#if 0
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    EXPECT_EQ(result->value(), input);
#endif
}
