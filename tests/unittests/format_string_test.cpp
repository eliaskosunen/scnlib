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
    auto [result, val] = scn::scan<int>("42", SCN_STRING("{}"));
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}
// Fails to compile, as it should
#if 0
TEST(FormatStringTest, InvalidStringCompileTimeCheck)
{
    auto [result, _] = scn::scan<int>("42", SCN_STRING("{"));
    EXPECT_FALSE(result);
}
#endif

TEST(FormatStringTest, ValidStringRuntimeCheck)
{
    auto [result, val] = scn::scan<int>("42", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}
// Fails to compile, as it should
#if !SCN_HAS_CONSTEVAL
TEST(FormatStringTest, InvalidStringRuntimeCheck)
{
    auto [result, _] = scn::scan<int>("42", "{");
    EXPECT_FALSE(result);
}
#endif

TEST(FormatStringTest, ValidStringForceRuntime)
{
    auto [result, val] = scn::scan<int>("42", scn::runtime("{}"));
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}
TEST(FormatStringTest, InvalidStringForceRuntime)
{
    auto [result, _] = scn::scan<int>("42", scn::runtime("{"));
    EXPECT_FALSE(result);
}

#if !SCN_HAS_CONSTEVAL
TEST(FormatStringTest, TooManyArgsInFormatStringLiteral)
{
    auto [result, _] = scn::scan<int>("42", "{} {}");
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, TooManyArgsInArgListLiteral)
{
    auto [result, i, j] = scn::scan<int, int>("42", "{}");
    EXPECT_FALSE(result);
    SCN_UNUSED(i);
    SCN_UNUSED(j);
}
#endif

TEST(FormatStringTest, TooManyArgsInFormatStringRuntime)
{
    auto [result, _] = scn::scan<int>("42", scn::runtime("{} {}"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, TooManyArgsInArgListCompileTime)
{
    auto [result, i, j] = scn::scan<int, int>("42", scn::runtime("{}"));
    EXPECT_FALSE(result);
    SCN_UNUSED(i);
    SCN_UNUSED(j);
}

TEST(FormatStringTest, HasId)
{
    auto [result, _] = scn::scan<int>("42", scn::runtime("{0}"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOnlyOpenBrace)
{
    auto [result, _] = scn::scan<std::string>("42", scn::runtime("{"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndLineBreak)
{
    auto [result, _] = scn::scan<std::string>("42", scn::runtime("{\n"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColon)
{
    auto [result, _] = scn::scan<std::string>("42", scn::runtime("{:"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColonAndLineBreak)
{
    auto [result, _] = scn::scan<std::string>("42", scn::runtime("{:\n"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, EmptyCharacterSet)
{
    auto [result, _] = scn::scan<std::string>("42", scn::runtime("{:[]}"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, AlphaCharacterSet)
{
    auto [result, word] = scn::scan<std::string>("abc", "{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(word, "abc");
}
TEST(FormatStringTest, AlphaCharacterSetWithStringView)
{
    static_assert(scn::ranges::contiguous_range<std::string_view>);
    static_assert(scn::ranges::contiguous_range<scn::ranges::subrange<const char*>>);
    //static_assert(scn::ranges::contiguous_range<scn::ranges::subrange<std::string_view::iterator>>);
    static_assert(std::is_same_v<decltype(scn::ranges::data(
                                     SCN_DECLVAL(std::string_view&))),
                                 const char*>);
    /*
    static_assert(std::is_same_v<decltype(
                                     SCN_DECLVAL(scn::ranges::subrange<std::string_view::iterator>&).data()),
                                 const char*>);
                                 */
    auto [result, word] = scn::scan<std::string_view>("abc", "{:[:alpha:]}");
    EXPECT_TRUE(result);
    EXPECT_EQ(word, "abc");
}
TEST(FormatStringTest, InvertedCharacterSet)
{
    auto [result, word] =
        scn::scan<std::string>("abc\n", scn::runtime("{:[^\n]}"));
    EXPECT_TRUE(result);
    EXPECT_EQ(word, "abc");
}

TEST(FormatStringTest, NonTerminatedCharacterSet)
{
    auto [result, _] = scn::scan<std::string>("abc", scn::runtime("{:["));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, NonTerminatedCharacterSetWithStringView)
{
    auto [result, _] = scn::scan<std::string_view>("abc", scn::runtime("{:["));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, ExtraArgInFormatString)
{
    auto [result, _] = scn::scan<std::string>("abc def", scn::runtime("{} {}"));
    EXPECT_FALSE(result);
}
