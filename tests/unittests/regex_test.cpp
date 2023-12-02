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

#include "wrapped_gtest.h"

#include <scn/detail/regex.h>
#include <scn/detail/scan.h>
#include <scn/detail/xchar.h>

using namespace std::string_view_literals;

TEST(RegexTest, InvalidRegexString)
{
    auto r = scn::scan<std::string>("foobar123", "{:/[a/}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::invalid_format_string);
}

TEST(RegexTest, InvalidRegexStringView)
{
    auto r = scn::scan<std::string_view>("foobar123", "{:/[a/}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::invalid_format_string);
}

TEST(RegexTest, InvalidRegexMatches)
{
    auto r = scn::scan<scn::regex_matches>("foobar123", "{:/[a/}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::invalid_format_string);
}

TEST(RegexTest, String)
{
    auto r = scn::scan<std::string>("foobar123", "{:/([a-zA-Z]+)/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, StringView)
{
    auto r = scn::scan<std::string_view>("foobar123", "{:/([a-zA-Z]+)/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, Matches)
{
    auto r =
        scn::scan<scn::regex_matches>("foobar123", "{:/([a-zA-Z]+)([0-9]+)/}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());
    EXPECT_THAT(r->value(), testing::ElementsAre(
                                testing::Optional(testing::Property(
                                    &scn::regex_match::get, "foobar123"sv)),
                                testing::Optional(testing::Property(
                                    &scn::regex_match::get, "foobar"sv)),
                                testing::Optional(testing::Property(
                                    &scn::regex_match::get, "123"sv))));
}

#if SCN_REGEX_SUPPORTS_NAMED_CAPTURES
TEST(RegexTest, NamedString)
{
    auto r = scn::scan<std::string>("foobar123",
                                    "{:/(?<prefix>[a-zA-Z]+)([0-9]+)/}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());
    EXPECT_EQ(r->value(), "foobar123");
}

TEST(RegexTest, NamedMatches)
{
    auto r = scn::scan<scn::regex_matches>("foobar123",
                                           "{:/(?<prefix>[a-zA-Z]+)([0-9]+)/}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());

    ASSERT_TRUE(r->value()[0]);
    EXPECT_EQ(r->value()[0]->get(), "foobar123");
    EXPECT_FALSE(r->value()[0]->name());

    ASSERT_TRUE(r->value()[1]);
    EXPECT_EQ(r->value()[1]->get(), "foobar");
    ASSERT_TRUE(r->value()[1]->name());
    EXPECT_EQ(*r->value()[1]->name(), "prefix");

    ASSERT_TRUE(r->value()[2]);
    EXPECT_EQ(r->value()[2]->get(), "123");
    EXPECT_FALSE(r->value()[2]->name());
}
#endif

#if SCN_REGEX_SUPPORTS_WIDE_STRINGS
TEST(RegexTest, WideStringView)
{
    auto r = scn::scan<std::wstring_view>(L"foobar123", L"{:/[a-zA-Z]+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), L"foobar");
}

TEST(RegexTest, WideString)
{
    auto r = scn::scan<std::wstring>(L"foobar123", L"{:/[a-zA-Z]+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), L"foobar");
}

TEST(RegexTest, WideMatches)
{
    auto r = scn::scan<scn::wregex_matches>(L"foobar123",
                                            L"{:/([a-zA-Z]+)([0-9]+)/}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());
    EXPECT_THAT(r->value(), testing::ElementsAre(
                                testing::Optional(testing::Property(
                                    &scn::wregex_match::get, L"foobar123"sv)),
                                testing::Optional(testing::Property(
                                    &scn::wregex_match::get, L"foobar"sv)),
                                testing::Optional(testing::Property(
                                    &scn::wregex_match::get, L"123"sv))));
}
#endif

TEST(RegexTest, TranscodeStringNarrowToWide)
{
    auto r = scn::scan<std::wstring>("foobar123", "{:/[a-zA-Z]+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), L"foobar");
}

#if SCN_REGEX_SUPPORTS_WIDE_STRINGS
TEST(RegexTest, TranscodeStringWideToNarrow)
{
    auto r = scn::scan<std::string>(L"foobar123", L"{:/[a-zA-Z]+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "foobar");
}
#endif
