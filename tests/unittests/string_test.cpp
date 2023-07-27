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

TEST(StringTest, DefaultNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, DefaultWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, DefaultNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, DefaultWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, StringPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:s}");
    EXPECT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, StringPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:s}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, StringPresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{:s}");
    EXPECT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, StringPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{:s}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, CharacterPresentationWithNoWidthCausesError)
{
    auto result = scn::scan<std::string>("abc def", scn::runtime("{:c}"));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(StringTest, CharacterPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), "def");
    EXPECT_EQ(result->value(), "abc ");
}
TEST(StringTest, CharacterPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L"def");
    EXPECT_EQ(result->value(), L"abc ");
}

TEST(StringTest, CharacterPresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{:4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L"def");
    EXPECT_EQ(result->value(), "abc ");
}
TEST(StringTest, CharacterPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{:4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), "def");
    EXPECT_EQ(result->value(), L"abc ");
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:[:alpha:]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:[:alpha:]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{:[:alpha:]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{:[:alpha:]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, WonkyInput)
{
    auto result = scn::scan<std::string>("o \x0f\n\n\xc3", "{:64c}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_encoding);
}
