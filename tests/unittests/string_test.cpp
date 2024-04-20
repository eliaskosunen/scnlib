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
    auto result = scn::scan<std::string>("abc def", scn::runtime_format("{:c}"));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(StringTest, CharacterPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:.4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), "def");
    EXPECT_EQ(result->value(), "abc ");
}
TEST(StringTest, CharacterPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:.4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L"def");
    EXPECT_EQ(result->value(), L"abc ");
}

TEST(StringTest, CharacterPresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{:.4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L"def");
    EXPECT_EQ(result->value(), "abc ");
}
TEST(StringTest, CharacterPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{:.4c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), "def");
    EXPECT_EQ(result->value(), L"abc ");
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:[a-z]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:[a-z]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{:[a-z]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{:[a-z]}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, WonkyInput)
{
    const char source[] = {'o', ' ', '\x0f', '\n', '\n', '\xc3'};
    auto input = std::string_view{source, sizeof(source)};

    auto result = scn::scan<std::string>(input, "{:.64c}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
#if 0
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    EXPECT_EQ(result->value(), input);
#endif
}

TEST(StringTest, WonkyInputAndFormatWithTranscoding)
{
    const char source[] = {'a', ']', 'c', '{', '}', '\xdf', ':', '\xb1'};
    auto input = std::string_view{source, sizeof(source)};

    auto result = scn::scan<std::wstring>(input, scn::runtime_format(input));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
}

TEST(StringTest, WonkyInput2)
{
    const auto input =
        std::string_view{"\303 \245å\377åä\3035\377ååíääccccc\307c\244c"};

    auto result = scn::scan<std::string_view>(input, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
#if 0
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "\303");

    result = scn::scan<std::string_view>(result->range(), "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), input.substr(2));
#endif
}

TEST(StringTest, WonkyInput3)
{
    const char source[] = {
        '\216', '\030', 0,      0,      0,      0,      0,      0,      0,
        '\216', '\'',   'a',    '\216', '\216', '\216', '\216', '\216', '\216',
        '\216', '\216', '\216', '\216', '\216', '\360', '\237', '\237'};
    auto input = std::string_view{source, sizeof(source)};

    auto result = scn::scan<std::string>(input, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
#if 0
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
#endif
}

TEST(StringTest, RecoveryFromInvalidEncoding)
{
    const auto source = std::string_view{"a\xc3 "};
    auto result = scn::scan<std::string>(source, "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
#if 0
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "a\xc3");
    EXPECT_EQ(result->begin(), source.end() - 1);
#endif
}
