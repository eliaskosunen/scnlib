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

#include "test_common.h"

#include <scn/scan.h>
#include <scn/xchar.h>

#include <array>
#include <deque>

namespace {
template <typename CharT>
std::basic_string<CharT> make_single_field_format(std::string_view arg_id,
                                                  std::string_view specs)
{
    std::basic_string<CharT> result{CharT{'{'}};
    for (char ch : arg_id) {
        result.push_back(static_cast<CharT>(ch));
    }
    result.push_back(CharT{':'});
    for (char ch : specs) {
        result.push_back(static_cast<CharT>(ch));
    }
    result.push_back(CharT{'}'});
    return result;
}

// "{:...}" takes the single-field fast path when its specs are valid, and
// "{0:...}" never does, so the results must be the same
template <typename T, typename CharT>
void expect_same_as_indexed_field(std::basic_string_view<CharT> source,
                                  std::initializer_list<std::string_view> specs)
{
    for (auto spec : specs) {
        const auto fast_format = make_single_field_format<CharT>("", spec);
        const auto general_format = make_single_field_format<CharT>("0", spec);
        auto fast = scn::scan<T>(
            source,
            scn::runtime_format(std::basic_string_view<CharT>(fast_format)));
        auto general = scn::scan<T>(
            source,
            scn::runtime_format(std::basic_string_view<CharT>(general_format)));
        ASSERT_EQ(fast.has_value(), general.has_value()) << spec;
        if (!fast) {
            EXPECT_EQ(fast.error().code(), general.error().code()) << spec;
            EXPECT_STREQ(fast.error().msg(), general.error().msg()) << spec;
            continue;
        }
        EXPECT_EQ(fast->value(), general->value()) << spec;
        EXPECT_EQ(fast->begin(), general->begin()) << spec;
    }
}
}  // namespace

namespace {
// Without width and precision, the reader skips the width handling.
// A precision larger than the input goes through it, but must give the same
// results.
template <typename T>
void expect_same_as_with_large_precision(
    std::string_view source,
    std::initializer_list<std::string_view> types)
{
    for (auto type : types) {
        const auto format = "{:" + std::string{type} + "}";
        const auto precision_format = "{:.1000" + std::string{type} + "}";
        auto plain = scn::scan<T>(source, scn::runtime_format(format));
        auto with_precision =
            scn::scan<T>(source, scn::runtime_format(precision_format));
        ASSERT_EQ(plain.has_value(), with_precision.has_value())
            << format << " " << source;
        if (!plain) {
            // Not the message: with a precision, the reader gets a
            // non-contiguous range, and some readers' messages differ there
            EXPECT_EQ(plain.error().code(), with_precision.error().code())
                << format << " " << source;
            continue;
        }
        EXPECT_EQ(plain->value(), with_precision->value())
            << format << " " << source;
        EXPECT_EQ(plain->begin(), with_precision->begin())
            << format << " " << source;
    }
}
}  // namespace

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
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::get<0>(result->values()), 42);
}

TEST(FormatStringTest, ValidStringRuntimeCheck)
{
    auto result = scn::scan<int>("42", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::get<0>(result->values()), 42);
}
// Fails to compile, as it should
#if !SCN_HAS_CONSTEVAL
TEST(FormatStringTest, InvalidStringRuntimeCheck)
{
    auto result = scn::scan<int>("42", "{");
    EXPECT_THAT(result, Failed());
}
#endif

TEST(FormatStringTest, ValidStringForceRuntime)
{
    auto result = scn::scan<int>("42", scn::runtime_format("{}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::get<0>(result->values()), 42);
}
TEST(FormatStringTest, InvalidStringForceRuntime)
{
    auto result = scn::scan<int>("42", scn::runtime_format("{"));
    EXPECT_THAT(result, Failed());
}

#if !SCN_HAS_CONSTEVAL
TEST(FormatStringTest, TooManyArgsInFormatStringLiteral)
{
    auto result = scn::scan<int>("42", "{} {}");
    EXPECT_THAT(result, Failed());
}
TEST(FormatStringTest, TooManyArgsInArgListLiteral)
{
    auto result = scn::scan<int, int>("42", "{}");
    EXPECT_THAT(result, Failed());
}
#endif

TEST(FormatStringTest, EscapedBraces)
{
    auto result = scn::scan<int>("{}123", scn::runtime_format("{{}}{}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
}

TEST(FormatStringTest, TooManyArgsInFormatStringRuntime)
{
    auto result = scn::scan<int>("42 43", scn::runtime_format("{} {}"));
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}
TEST(FormatStringTest, TooManyArgsInArgListCompileTime)
{
    auto result = scn::scan<int, int>("42", scn::runtime_format("{}"));
    EXPECT_THAT(result, Failed());
}

TEST(FormatStringTest, HasId)
{
    auto result = scn::scan<int>("42", scn::runtime_format("{0}"));
    EXPECT_THAT(result, Succeeded());
}

TEST(FormatStringTest, AutomaticThenManualIndexingRuntime)
{
    auto result = scn::scan<int, int>("1 2", scn::runtime_format("{} {0}"));
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}
TEST(FormatStringTest, ManualThenAutomaticIndexingRuntime)
{
    auto result = scn::scan<int, int>("1 2", scn::runtime_format("{1} {}"));
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOnlyOpenBrace)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{"));
    EXPECT_THAT(result, Failed());
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndLineBreak)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{\n"));
    EXPECT_THAT(result, Failed());
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColon)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{:"));
    EXPECT_THAT(result, Failed());
}
TEST(FormatStringTest, UnexpectedEndOfSpecs_WithOpenBraceAndColonAndLineBreak)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{:\n"));
    EXPECT_THAT(result, Failed());
}

TEST(FormatStringTest, EmptyCharacterSet)
{
    auto result = scn::scan<std::string>("42", scn::runtime_format("{:[]}"));
    EXPECT_THAT(result, Failed());
}
TEST(FormatStringTest, InvertedCharacterSet)
{
    auto result =
        scn::scan<std::string>("abc 123\n", scn::runtime_format("{:[^\n]}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::get<0>(result->values()), "abc 123");
}

TEST(FormatStringTest, NonTerminatedCharacterSet)
{
    auto result = scn::scan<std::string>("abc", scn::runtime_format("{:["));
    EXPECT_THAT(result, Failed());
}
TEST(FormatStringTest, NonTerminatedCharacterSetWithStringView)
{
    auto result =
        scn::scan<std::string_view>("abc", scn::runtime_format("{:["));
    EXPECT_THAT(result, Failed());
}

TEST(FormatStringTest, RangeSet)
{
    auto result = scn::scan<std::string>("abcd", "{:[a-c]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "abc");
}
TEST(FormatStringTest, RangeSetRuntime)
{
    auto result =
        scn::scan<std::string>("abcd", scn::runtime_format("{:[a-c]}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "abc");
}
TEST(FormatStringTest, InvalidRangeSet)
{
    auto result =
        scn::scan<std::string>("abcd", scn::runtime_format("{:[c-a]}"));
    ASSERT_THAT(result, Failed());
}

TEST(FormatStringTest, FillAndAlignmentAreInvalid)
{
    for (const char* fmt :
         {"{:<}", "{:>5}", "{:^5}", "{:*^5}", "{:x<3}", "{:_>4c}", "{:<5s}",
          "{:\xc3\xa4>4}", "{:\xc3\x28>4}", "{:{^}", "{:\xe4\xb8"}) {
        EXPECT_THAT(scn::scan<std::string>("abcd", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
        EXPECT_THAT(scn::scan<int>("42", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}

TEST(FormatStringTest, ZeroPrecisionErrorMessage)
{
    auto result = scn::scan<std::string>("abcd", scn::runtime_format("{:.0}"));
    ASSERT_FALSE(result);
    EXPECT_STREQ(result.error().msg(), "Precision must be greater than zero");
}

TEST(FormatStringTest, CharacterSetClassErrorMessage)
{
    auto result =
        scn::scan<std::string>("abcd", scn::runtime_format("{:[[:alpha:]]}"));
    ASSERT_FALSE(result);
    EXPECT_STREQ(result.error().msg(),
                 "Character classes are not supported in [character set]; use "
                 "a regex or explicit ranges");
}

TEST(FormatStringTest, ZeroPrecision)
{
    constexpr std::array<const char*, 4> formats{"{:.0}", "{:.0c}", "{:.0f}",
                                                 "{:5.0}"};
    for (auto fmt : formats) {
        EXPECT_THAT(scn::scan<std::string>("abcd", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
    EXPECT_THAT(scn::scan<double>("1.5", scn::runtime_format("{:.0f}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(FormatStringTest, PrecisionOne)
{
    auto result = scn::scan<std::string>("abcd", scn::runtime_format("{:.1}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a");
    EXPECT_STREQ(result->begin(), "bcd");
}

TEST(FormatStringTest, CharacterSetClassesAndEscapesRejected)
{
    constexpr std::array<const char*, 7> formats{
        "{:[:alpha:]}", "{:[[:alpha:]]}", "{:[a[:digit:]]}", "{:[^[:digit:]]}",
        "{:[\\d]}",     "{:[\\w]}",       "{:[a-\\d]}"};
    for (auto fmt : formats) {
        EXPECT_THAT(scn::scan<std::string>("abc1", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
        EXPECT_THAT(
            scn::scan<std::string_view>("abc1", scn::runtime_format(fmt)),
            FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}

TEST(FormatStringTest, CharacterSetLiteralColon)
{
    auto result =
        scn::scan<std::string>("a:b:c", scn::runtime_format("{:[a:b]}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a:b:");
    EXPECT_STREQ(result->begin(), "c");
}

TEST(FormatStringTest, CharacterSetOnlyColon)
{
    auto result = scn::scan<std::string>("::a", "{:[:]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "::");
    EXPECT_STREQ(result->begin(), "a");
}

TEST(FormatStringTest, CharacterSetLiteralBackslash)
{
    // Backslash is a literal: [a\-z] is 'a' and the range '\' to 'z'
    auto result =
        scn::scan<std::string>("a\\q-", scn::runtime_format("{:[a\\-z]}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a\\q");
    EXPECT_STREQ(result->begin(), "-");
}

TEST(FormatStringTest, SingleFieldWithSpecsSameAsIndexedField)
{
    const std::initializer_list<std::string_view> int_specs = {
        "",   "d",  "x",  "b",     "o",   "i",    "u",    "r7",
        "L",  "Ld", "Lx", "*^5.3", "*>4", ">4d",  ".2",   ".2d",
        "<6", "c",  "Lb", ".0",    "q",   "5.2d", "[0-9]"};
    constexpr std::array<const char*, 7> int_sources{
        "  -123abc", "0x1F rest", "**12** x", "101", "", "+7", "\xc3\xa4"};
    for (auto source : int_sources) {
        expect_same_as_indexed_field<int>(std::string_view{source}, int_specs);
        expect_same_as_indexed_field<unsigned>(std::string_view{source},
                                               int_specs);
        expect_same_as_indexed_field<long long>(std::string_view{source},
                                                int_specs);
    }

    constexpr std::array<const char*, 5> double_sources{"3.25e2 x", "0x1.8p1",
                                                        "inf", "abc", " -0.5"};
    for (auto source : double_sources) {
        expect_same_as_indexed_field<double>(
            std::string_view{source},
            {"", "f", "e", "g", "a", "L", "*<8.3f", ".3", "x", "c"});
    }

    const std::initializer_list<std::string_view> string_specs = {
        "",     "s",           "c",        ".3", "*^7", "[a-c]",
        "[^ ]", "[[:alpha:]]", "/[a-z]+/", ".0", "d"};
    constexpr std::array<const char*, 4> string_sources{"abcd efg", "  xyz",
                                                        "*ab*", ""};
    for (auto source : string_sources) {
        expect_same_as_indexed_field<std::string>(std::string_view{source},
                                                  string_specs);
        expect_same_as_indexed_field<std::string_view>(std::string_view{source},
                                                       string_specs);
    }

    constexpr std::array<const char*, 3> char_sources{"abc", "  x", "7"};
    for (auto source : char_sources) {
        expect_same_as_indexed_field<char>(std::string_view{source},
                                           {"", "c", "d", "*>3", "s", "x"});
    }

    constexpr std::array<const char*, 4> bool_sources{"true", "1", "false x",
                                                      "yes"};
    for (auto source : bool_sources) {
        expect_same_as_indexed_field<bool>(std::string_view{source},
                                           {"", "s", "i", "L", "d", "c"});
    }

    constexpr std::array<const wchar_t*, 3> wide_sources{L" 42 x", L"ff",
                                                         L"[ab]"};
    for (auto source : wide_sources) {
        expect_same_as_indexed_field<int>(std::wstring_view{source},
                                          {"x", "d", "*^5.3", "c"});
        expect_same_as_indexed_field<std::wstring>(std::wstring_view{source},
                                                   {"s", "[a-z]", ".2", "d"});
    }
}

TEST(FormatStringTest, SpecsWithoutWidthSameAsWithLargePrecision)
{
    constexpr std::array<const char*, 8> sources{
        "  -123abc", "0x1F rest", "101", "", "   ", "+7", "z", "\xc3\xa4"};
    for (auto source : sources) {
        expect_same_as_with_large_precision<int>(
            source, {"d", "x", "b", "o", "i", "u", "r7", "L", "Ld"});
        expect_same_as_with_large_precision<unsigned>(source,
                                                      {"d", "x", "i", "u"});
        expect_same_as_with_large_precision<double>(source,
                                                    {"f", "e", "g", "a", "L"});
        expect_same_as_with_large_precision<bool>(source, {"i"});
    }
    constexpr std::array<const char*, 4> double_sources{"3.25e2 x", "0x1.8p1",
                                                        "inf", " -0.5"};
    for (auto source : double_sources) {
        expect_same_as_with_large_precision<double>(source,
                                                    {"f", "e", "g", "a", "L"});
    }
    constexpr std::array<const char*, 4> bool_sources{"true", "1", "false x",
                                                      "  0"};
    for (auto source : bool_sources) {
        expect_same_as_with_large_precision<bool>(source, {"i"});
    }
}

TEST(FormatStringTest, SingleFieldWithSpecsContiguous)
{
    auto result = scn::scan<int>("  ff rest", scn::runtime_format("{:x}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0xff);
    EXPECT_EQ(std::string(result->begin(), result->end()), " rest");
}

TEST(FormatStringTest, SingleFieldWithSpecsNonContiguous)
{
    auto source = std::deque<char>{'1', '2', '3', ' ', 'a'};
    auto result = scn::scan<int>(source, scn::runtime_format("{:.2d}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 12);
    ASSERT_NE(result->begin(), source.end());
    EXPECT_EQ(*result->begin(), '3');
}

TEST(FormatStringTest, SingleFieldWithSpecsInvalidInput)
{
    EXPECT_THAT(scn::scan<int>("abc", scn::runtime_format("{:d}")),
                FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_THAT(
        scn::scan<int>(std::deque<char>{'2'}, scn::runtime_format("{:b}")),
        FailedWith(scn::scan_error::invalid_scanned_value));
}

// Not a single field: handled by the general format string parser
TEST(FormatStringTest, SingleFieldWithSpecsEscapedBrace)
{
    auto result = scn::scan<int>("1}", scn::runtime_format("{:d}}}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 1);
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(FormatStringTest, SingleFieldWithSpecsUnmatchedBrace)
{
    auto result = scn::scan<int>("1}", scn::runtime_format("{:d}}"));
    ASSERT_FALSE(result);
    EXPECT_STREQ(result.error().msg(), "Unmatched '}' in format string");
}

TEST(FormatStringTest, SingleFieldWithSpecsTwoFields)
{
    EXPECT_THAT(scn::scan<int>("1 2", scn::runtime_format("{:d}{:d}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(FormatStringTest, SpaceSkipsAnyWhitespace)
{
    auto result = scn::scan<char, char>("a \n\tb", "{} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
}
TEST(FormatStringTest, AnyWhitespaceSkipsAnyWhitespace)
{
    auto result = scn::scan<char, char>("a \n\tb", "{}\n{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
}
TEST(FormatStringTest, AnyComboOfWhitespaceSkipsAnyWhitespace)
{
    auto result = scn::scan<char, char>("a \n\tb", "{}\n {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
}

TEST(FormatStringTest, LiteralsAndWhitespace)
{
    auto result =
        scn::scan<std::string>("a b c", scn::runtime_format("a {} c"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "b");
}

TEST(FormatStringTest, LongFormatString1)
{
    auto result = scn::scan<std::string>(
        "abcdefghijklmnopqrstuvwxyz 1 234567890",
        scn::runtime_format("abcdefghijklmnopqrstuvwxyz {} 23456789"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "1");
}
TEST(FormatStringTest, LongFormatString2)
{
    auto result = scn::scan<std::string>(
        "123456789 0 abcdefghijklmnopqrstuvwxyz",
        scn::runtime_format("123456789 {} abcdefghijklmnopqrstuvwxyz"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "0");
}
TEST(FormatStringTest, LongFormatString3)
{
    auto result = scn::scan<char>(
        "abcdefghijklmnopqrstuvwxyz {}1{} 234567890",
        scn::runtime_format("abcdefghijklmnopqrstuvwxyz {{}}{}{{}} 23456789"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), '1');
}
TEST(FormatStringTest, LongFormatString4)
{
    auto result = scn::scan<char>(
        "123456789 {}0{} abcdefghijklmnopqrstuvwxyz",
        scn::runtime_format("123456789 {{}}{}{{}} abcdefghijklmnopqrstuvwxyz"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), '0');
}

TEST(FormatStringTest, MatchLiteralInvalidEncoding)
{
    auto result =
        scn::scan<>("\xc3\na\xa4", scn::runtime_format("\xc3\na\xa4"));
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(FormatStringTest, FuzzingError)
{
    using namespace std::string_view_literals;
    auto s =
        ":+{}{\000\000\000}\000\000\000\000\000\000\000\000\000\000\000\000}}U\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247{{\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\247\377\377\377\377\377\377\377\377\377\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\346\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000[}\000"sv;
    auto result = scn::scan<std::string>(s, scn::runtime_format(s));
    ASSERT_THAT(result, Failed());
}

TEST(FormatStringTest, FuzzingError2)
{
    char narrow_s[] = {'{', 0, 0, 0,   ':', 0, 0, 0,  '[',
                       0,   0, 0, '^', 0,   0, 0, ']'};
    std::wstring ws;
    ws.resize(sizeof(narrow_s) / sizeof(wchar_t));
    std::memcpy(ws.data(), narrow_s, sizeof(narrow_s));
    auto result = scn::scan<wchar_t>(ws, scn::runtime_format(ws));
    ASSERT_THAT(result, Failed());
}

TEST(FormatStringTest, FuzzingError3)
{
    auto result = scn::scan<std::string>("{:G}", scn::runtime_format("{:G}"));
    ASSERT_THAT(result, Failed());
}

TEST(FormatStringTest, ZeroWidthIsError)
{
    for (auto fmt : {"{:0}", "{:0d}", "{:0.5}", "{:00}"}) {
        EXPECT_THAT(scn::scan<int>("42", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}

TEST(FormatStringTest, LeadingZeroInWidthOrPrecisionIsError)
{
    for (auto fmt :
         {"{:05}", "{:005}", "{:.05}", "{:5.05}", "{:05.5}", "{:.00}"}) {
        EXPECT_THAT(scn::scan<int>("42000", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}

TEST(FormatStringTest, PrecisionWithoutDigitsIsError)
{
    for (auto fmt : {"{:.}", "{:5.}", "{:.d}", "{:.L}", "{:5.L}"}) {
        EXPECT_THAT(scn::scan<int>("42", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}
