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

#include <scn/detail/scan.h>

TEST(FormatStringTest, ConstructFromLiteral)
{
    scn::format_string<int> str{"{}"};
    EXPECT_EQ(str, std::string_view{"{}"});
}

TEST(FormatStringTest, CompileTimeCheckLiteral)
{
    scn::format_string<int> str{SCN_STRING("{}")};
    EXPECT_EQ(str, std::string_view{"{}"});
}

TEST(FormatStringTest, ValidStringCompileTimeCheck)
{
    auto result = scn::scan<int>("42", SCN_STRING("{}"));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), 42);
}
// Fails to compile, as it should
#if 0
TEST(FormatStringTest, InvalidStringCompileTimeCheck)
{
    auto result = scn::scan<int>("42", SCN_STRING("{"));
    EXPECT_FALSE(result);
}
#endif

TEST(FormatStringTest, ValidStringRuntimeCheck)
{
    auto result = scn::scan<int>("42", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), 42);
}
// Fails to compile, as it should
#if !SCN_HAS_CONSTEVAL
TEST(FormatStringTest, InvalidStringRuntimeCheck)
{
    auto result = scn::scan<int>("42", "{");
    EXPECT_FALSE(result);
}
#endif

TEST(FormatStringTest, ValidStringForceRuntime)
{
    auto result = scn::scan<int>("42", scn::runtime("{}"));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), 42);
}
TEST(FormatStringTest, InvalidStringForceRuntime)
{
    auto result = scn::scan<int>("42", scn::runtime("{"));
    EXPECT_FALSE(result);
}

#if !SCN_HAS_CONSTEVAL
TEST(FormatStringTest, TooManyArgsInFormatStringLiteral)
{
    auto result = scn::scan<int>("42", "{} {}");
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, TooManyArgsInArgListLiteral)
{
    auto result = scn::scan<int, int>("42", "{}");
    EXPECT_FALSE(result);
}
#endif

TEST(FormatStringTest, TooManyArgsInFormatStringRuntime)
{
    auto result = scn::scan<int>("42", scn::runtime("{} {}"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, TooManyArgsInArgListCompileTime)
{
    auto result = scn::scan<int, int>("42", scn::runtime("{}"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, HasId)
{
    auto result = scn::scan<int>("42", scn::runtime("{0}"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOnlyOpenBrace)
{
    auto result = scn::scan<std::string>("42", scn::runtime("{"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndLineBreak)
{
    auto result = scn::scan<std::string>("42", scn::runtime("{\n"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColon)
{
    auto result = scn::scan<std::string>("42", scn::runtime("{:"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColonAndLineBreak)
{
    auto result = scn::scan<std::string>("42", scn::runtime("{:\n"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, EmptyCharacterSet)
{
    auto result = scn::scan<std::string>("42", scn::runtime("{:[]}"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, AlphaCharacterSet)
{
    auto result = scn::scan<std::string>("abc123", "{:[:alpha:]}");
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), "abc");
}
TEST(FormatStringTest, AlphaCharacterSetWithStringView)
{
    static_assert(scn::ranges::contiguous_range<std::string_view>);
    static_assert(
        scn::ranges::contiguous_range<scn::ranges::subrange<const char*>>);
    // static_assert(scn::ranges::contiguous_range<scn::ranges::subrange<std::string_view::iterator>>);
    static_assert(std::is_same_v<decltype(scn::ranges::data(
                                     SCN_DECLVAL(std::string_view&))),
                                 const char*>);
    /*
    static_assert(std::is_same_v<decltype(
                                     SCN_DECLVAL(scn::ranges::subrange<std::string_view::iterator>&).data()),
                                 const char*>);
                                 */
    auto result = scn::scan<std::string_view>("abc123", "{:[:alpha:]}");
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), "abc");
}
TEST(FormatStringTest, InvertedCharacterSet)
{
    auto result = scn::scan<std::string>("abc 123\n", scn::runtime("{:[^\n]}"));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), "abc 123");
}

TEST(FormatStringTest, NonTerminatedCharacterSet)
{
    auto result = scn::scan<std::string>("abc", scn::runtime("{:["));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, NonTerminatedCharacterSetWithStringView)
{
    auto result = scn::scan<std::string_view>("abc", scn::runtime("{:["));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, ExtraArgInFormatString)
{
    auto result = scn::scan<std::string>("abc def", scn::runtime("{} {}"));
    EXPECT_FALSE(result);
}
