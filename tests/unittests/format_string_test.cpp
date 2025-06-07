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

#include <scn/scan.h>
#include <scn/xchar.h>

TEST(FormatStringTest, ConstructFromLiteral)
{
    scn::scan_format_string<std::string_view, int> str{"{}"};
    EXPECT_EQ(str, std::string_view{"{}"});
}

TEST(FormatStringTest, CompileTimeCheckLiteral)
{
    scn::scan_format_string<std::string_view, int> str{SCN_STRING("{}")};
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
    auto result = scn::scan<int>("42", scn::runtime_format("{}"));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), 42);
}
TEST(FormatStringTest, InvalidStringForceRuntime)
{
    auto result = scn::scan<int>("42", scn::runtime_format("{"));
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

TEST(FormatStringTest, EscapedBraces)
{
    auto result = scn::scan<int>("{}123", scn::runtime_format("{{}}{}"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 123);
}

TEST(FormatStringTest, TooManyArgsInFormatStringRuntime)
{
    auto result = scn::scan<int>("42", scn::runtime_format("{} {}"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, TooManyArgsInArgListCompileTime)
{
    auto result = scn::scan<int, int>("42", scn::runtime_format("{}"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, HasId)
{
    auto result = scn::scan<int>("42", scn::runtime_format("{0}"));
    EXPECT_TRUE(result);
}

TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOnlyOpenBrace)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndLineBreak)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{\n"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColon)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{:"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColonAndLineBreak)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{:\n"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, EmptyCharacterSet)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{:[]}"));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, InvertedCharacterSet)
{
    auto result =
        scn::scan<std::string>("abc 123\n", scn::runtime_format("{:[^\n]}"));
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), "abc 123");
}

TEST(FormatStringTest, NonTerminatedCharacterSet)
{
    auto result = scn::scan<std::string>("abc", scn::runtime_format("{:["));
    EXPECT_FALSE(result);
}
TEST(FormatStringTest, NonTerminatedCharacterSetWithStringView)
{
    auto result =
        scn::scan<std::string_view>("abc", scn::runtime_format("{:["));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, RangeSet)
{
    auto result = scn::scan<std::string>("abcd", "{:[a-c]}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "abc");
}
TEST(FormatStringTest, RangeSetRuntime)
{
    auto result =
        scn::scan<std::string>("abcd", scn::runtime_format("{:[a-c]}"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "abc");
}
TEST(FormatStringTest, InvalidRangeSet)
{
    auto result =
        scn::scan<std::string>("abcd", scn::runtime_format("{:[c-a]}"));
    ASSERT_FALSE(result);
}

TEST(FormatStringTest, ExtraArgInFormatString)
{
    auto result =
        scn::scan<std::string>("abc def", scn::runtime_format("{} {}"));
    EXPECT_FALSE(result);
}

TEST(FormatStringTest, SpaceSkipsAnyWhitespace)
{
    auto result = scn::scan<char, char>("a \n\tb", "{} {}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
}
TEST(FormatStringTest, AnyWhitespaceSkipsAnyWhitespace)
{
    auto result = scn::scan<char, char>("a \n\tb", "{}\n{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
}
TEST(FormatStringTest, AnyComboOfWhitespaceSkipsAnyWhitespace)
{
    auto result = scn::scan<char, char>("a \n\tb", "{}\n {}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
}

TEST(FormatStringTest, LiteralsAndWhitespace)
{
    auto result =
        scn::scan<std::string>("a b c", scn::runtime_format("a {} c"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "b");
}

TEST(FormatStringTest, LongFormatString1)
{
    auto result = scn::scan<std::string>(
        "abcdefghijklmnopqrstuvwxyz 1 234567890",
        scn::runtime_format("abcdefghijklmnopqrstuvwxyz {} 23456789"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "1");
}
TEST(FormatStringTest, LongFormatString2)
{
    auto result = scn::scan<std::string>(
        "123456789 0 abcdefghijklmnopqrstuvwxyz",
        scn::runtime_format("123456789 {} abcdefghijklmnopqrstuvwxyz"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "0");
}
TEST(FormatStringTest, LongFormatString3)
{
    auto result = scn::scan<char>(
        "abcdefghijklmnopqrstuvwxyz {}1{} 234567890",
        scn::runtime_format("abcdefghijklmnopqrstuvwxyz {{}}{}{{}} 23456789"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), '1');
}
TEST(FormatStringTest, LongFormatString4)
{
    auto result = scn::scan<char>(
        "123456789 {}0{} abcdefghijklmnopqrstuvwxyz",
        scn::runtime_format("123456789 {{}}{}{{}} abcdefghijklmnopqrstuvwxyz"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), '0');
}

TEST(FormatStringTest, MatchLiteralInvalidEncoding)
{
    auto result =
        scn::scan<>("\xc3\na\xa4", scn::runtime_format("\xc3\na\xa4"));
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(FormatStringTest, FuzzingError)
{
    using namespace std::string_view_literals;
    auto s =
        ":+{}{\000\000\000}\000\000\000\000\000\000\000\000\000\000\000\000}}U\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247{{\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\377\377\377\377\377\377\377\377\377\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000[}\000"sv;
    auto result = scn::scan<std::string>(s, scn::runtime_format(s));
    ASSERT_FALSE(result);
}

TEST(FormatStringTest, FuzzingError2)
{
    char narrow_s[] = {'{', 0, 0, 0,   ':', 0, 0, 0,  '[',
                       0,   0, 0, '^', 0,   0, 0, ']'};
    std::wstring ws;
    ws.resize(sizeof(narrow_s) / sizeof(wchar_t));
    std::memcpy(ws.data(), narrow_s, sizeof(narrow_s));
    auto result = scn::scan<wchar_t>(ws, scn::runtime_format(ws));
    ASSERT_FALSE(result);
}

TEST(FormatStringTest, FuzzingError3)
{
    auto result = scn::scan<std::string>("{:G}", scn::runtime_format("{:G}"));
    ASSERT_FALSE(result);
}
