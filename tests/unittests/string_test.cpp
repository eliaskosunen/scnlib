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

TEST(StringTest, DefaultNarrowStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string>("abc def", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, "abc");
}
TEST(StringTest, DefaultWideStringFromWideSource)
{
    auto [result, str] = scn::scan<std::wstring>(L"abc def", L"{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringTest, DefaultNarrowStringFromWideSource)
{
    auto [result, str] = scn::scan<std::string>(L"abc def", L"{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, "abc");
}
TEST(StringTest, DefaultWideStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::wstring>("abc def", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringTest, StringPresentationNarrowStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string>("abc def", "{:s}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, "abc");
}
TEST(StringTest, StringPresentationWideStringFromWideSource)
{
    auto [result, str] = scn::scan<std::wstring>(L"abc def", L"{:s}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringTest, StringPresentationNarrowStringFromWideSource)
{
    auto [result, str] = scn::scan<std::string>(L"abc def", L"{:s}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, "abc");
}
TEST(StringTest, StringPresentationWideStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::wstring>("abc def", "{:s}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringTest, CharacterPresentationWithNoWidthCausesError)
{
    auto [result, _] = scn::scan<std::string>("abc def", scn::runtime("{:c}"));
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(StringTest, CharacterPresentationNarrowStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string>("abc def", "{:4c}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), "def");
    EXPECT_EQ(str, "abc ");
}
TEST(StringTest, CharacterPresentationWideStringFromWideSource)
{
    auto [result, str] = scn::scan<std::wstring>(L"abc def", L"{:4c}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L"def");
    EXPECT_EQ(str, L"abc ");
}

TEST(StringTest, CharacterPresentationNarrowStringFromWideSource)
{
    auto [result, str] = scn::scan<std::string>(L"abc def", L"{:4c}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L"def");
    EXPECT_EQ(str, "abc ");
}
TEST(StringTest, CharacterPresentationWideStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::wstring>("abc def", "{:4c}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), "def");
    EXPECT_EQ(str, L"abc ");
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::string>("abc def", "{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromWideSource)
{
    auto [result, str] = scn::scan<std::wstring>(L"abc def", L"{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromWideSource)
{
    auto [result, str] = scn::scan<std::string>(L"abc def", L"{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), L" def");
    EXPECT_EQ(str, "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromNarrowSource)
{
    auto [result, str] = scn::scan<std::wstring>("abc def", "{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(result.range(), " def");
    EXPECT_EQ(str, L"abc");
}

TEST(StringTest, WonkyInput)
{
    auto [result, str] = scn::scan<std::string>("o \x0f\n\n\xc3", "{:64U}");
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_encoding);
}
