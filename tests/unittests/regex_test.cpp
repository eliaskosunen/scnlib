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

#include <scn/regex.h>
#include <scn/scan.h>
#include <scn/xchar.h>

using namespace std::string_view_literals;

#if !SCN_DISABLE_REGEX

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

TEST(RegexTest, AlphaCharacterClass)
{
    auto r = scn::scan<std::string_view>("foobar123", "{:/[[:alpha:]]+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, AlphaCharacterClassWithNonAscii)
{
#if SCN_REGEX_BOOST_USE_ICU
    // [[:alpha:]] uses the ICU with Boost.Regex + ICU
    auto r = scn::scan<std::string_view>("fööbär123", "{:/[[:alpha:]]+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "fööbär");
#else
    // [[:alpha:]] is ASCII only with
    // std::regex and re2, and Boost.Regex without ICU
    auto r = scn::scan<std::string_view>("fööbär123", "{:/[[:alpha:]]+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "f");
#endif
}

#if SCN_REGEX_BACKEND != SCN_REGEX_BACKEND_STD
// std::regex doesnt support Unicode character classes
TEST(RegexTest, LetterUnicodeCharacterClass)
{
    // L = Letter
    auto r = scn::scan<std::string_view>("foobar123", "{:/\\pL+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, LetterUnicodeCharacterClassWithNonAscii)
{
#if SCN_REGEX_SUPPORTS_UTF8_CLASSIFICATION
    auto r = scn::scan<std::string_view>("fööbär123", "{:/\\pL+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "fööbär");
#else
    auto r = scn::scan<std::string_view>("fööbär123", "{:/\\pL+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "f");
#endif
}

TEST(RegexTest, EmojiWithSoUnicodeCharacterClass)
{
#if SCN_REGEX_SUPPORTS_UTF8_CLASSIFICATION
    // U+1F600 "GRINNING FACE" and U+1F601 "GRINNING FACE WITH SMILING EYES"
    auto r = scn::scan<std::string_view>("\xf0\x9f\x98\x80\xf0\x9f\x98\x81 abc",
                                         "{:/\\p{So}+/}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "\xf0\x9f\x98\x80\xf0\x9f\x98\x81");
#else
    auto r = scn::scan<std::string_view>("\xf0\x9f\x98\x80\xf0\x9f\x98\x81 abc",
                                         "{:/\\p{So}+/}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::invalid_format_string);
#endif
}
#endif

TEST(RegexTest, NoCaseFlagStringView)
{
    auto r = scn::scan<std::string_view>("FooBar123", "{:/[a-z]+/i}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "FooBar");
}

TEST(RegexTest, NoCaseFlagMatches)
{
    auto r = scn::scan<scn::regex_matches>("FooBar123", "{:/([a-z]+)/i}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_THAT(r->value(),
                testing::ElementsAre(testing::Optional(testing::Property(
                                         &scn::regex_match::get, "FooBar"sv)),
                                     testing::Optional(testing::Property(
                                         &scn::regex_match::get, "FooBar"sv))));
}

TEST(RegexTest, NoCaseAndNoCaptureFlagStringView)
{
    auto r = scn::scan<std::string_view>("FooBar123", "{:/[a-z]+/in}");
    ASSERT_TRUE(r);
    EXPECT_FALSE(r->range().empty());
    EXPECT_EQ(r->value(), "FooBar");
}

TEST(RegexTest, NoCaseAndNoCaptureFlagMatches)
{
    auto r =
        scn::scan<scn::regex_matches>("FooBar123", "{:/([a-z]+)([0-9]+)/in}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());
    EXPECT_THAT(r->value(),
                testing::ElementsAre(testing::Optional(
                    testing::Property(&scn::regex_match::get, "FooBar123"sv))));
}

TEST(RegexTest, EscapedSlashInPattern)
{
    auto r = scn::scan<std::string_view>("foo/bar", "{:/[a-z]+\\/[a-z]+/}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());
    EXPECT_THAT(r->value(), "foo/bar");
}

#endif  // !SCN_DISABLE_REGEX
