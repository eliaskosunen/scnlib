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

#include <deque>
#include <locale>
#include <string>
#include <string_view>

TEST(WidthPrecisionTest, DefaultWithInt)
{
    auto r = scn::scan<int>("   42", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(WidthPrecisionTest, DefaultWithRightAlignedChar)
{
    auto r = scn::scan<char>("   x", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), ' ');
    EXPECT_STREQ(r->begin(), "  x");
}

TEST(WidthPrecisionTest, DefaultWithLeftAlignedChar)
{
    auto r = scn::scan<char>("x   ", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "   ");
}

// Skipped whitespace doesn't count toward the width
TEST(WidthPrecisionTest, CustomWidthInt)
{
    auto r = scn::scan<int>("    42", "{:2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(WidthPrecisionTest, CustomPrecInt)
{
    auto r = scn::scan<int>("    42", "{:.6}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(WidthPrecisionTest, CustomWidthCustomPrecInt)
{
    auto r = scn::scan<int>(" 42 ", "{:2.4}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), " ");
}

TEST(WidthPrecisionTest, P1729_Ex3r0)
{
    auto r = scn::scan<int>("    42", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(WidthPrecisionTest, P1729_Ex3r1)
{
    auto r = scn::scan<char>("    x", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), ' ');
    EXPECT_STREQ(r->begin(), "   x");
}
TEST(WidthPrecisionTest, P1729_Ex3r2)
{
    auto r = scn::scan<char>("x    ", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "    ");
}

// P1729 reads 42 here, with the whitespace counted toward the width.
// Like `scanf`, scnlib doesn't count skipped whitespace.
TEST(WidthPrecisionTest, WidthWithLeadingWhitespaceIsTooShort)
{
    auto r = scn::scan<int>("    42", "{:6}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::length_too_short));
}
TEST(WidthPrecisionTest, PythonParse3)
{
    auto r = scn::scan<std::string, std::string>("look", "{:.2}{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(std::get<0>(r->values()), "lo");
    EXPECT_EQ(std::get<1>(r->values()), "ok");
    EXPECT_STREQ(r->begin(), "");
}
TEST(WidthPrecisionTest, PythonParse4)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:4}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::length_too_short));
}
TEST(WidthPrecisionTest, PythonParse5)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:.4}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(std::get<0>(r->values()), "look");
    EXPECT_EQ(std::get<1>(r->values()), "at");
    EXPECT_STREQ(r->begin(), " that");
}
TEST(WidthPrecisionTest, PythonParse6)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:.4}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(std::get<0>(r->values()), "look");
    EXPECT_EQ(std::get<1>(r->values()), "at");
    EXPECT_STREQ(r->begin(), " that");
}
TEST(WidthPrecisionTest, PythonParse7)
{
    auto r = scn::scan<int, int>("0440", "{:.2}{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(std::get<0>(r->values()), 4);
    EXPECT_EQ(std::get<1>(r->values()), 40);
    EXPECT_STREQ(r->begin(), "");
}

TEST(CustomPrecisionTest, Ascii)
{
    auto r = scn::scan<std::string>("abc", "{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "ab");
    EXPECT_STREQ(r->begin(), "c");
}

TEST(CustomPrecisionTest, SingleWidthText)
{
    auto r = scn::scan<std::string>("åäö", "{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "åä");
    EXPECT_STREQ(r->begin(), "ö");
}

TEST(CustomPrecisionTest, Emoji)
{
    auto r = scn::scan<std::string>("😂ab", "{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "😂a");
    EXPECT_STREQ(r->begin(), "b");
}

TEST(CustomPrecisionTest, Fuzz1)
{
    auto r = scn::scan<std::string>("a😂", "{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "a😂");
    EXPECT_STREQ(r->begin(), "");
}

TEST(CustomPrecisionTest, LeadingWhitespaceNotCounted)
{
    auto r = scn::scan<int>("   42", "{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

namespace {
// Checks that a contiguous and a non-contiguous source give the same result
template <typename T, typename... Locale>
void expect_value(std::string_view input,
                  std::string_view format,
                  const T& expected,
                  std::string_view expected_rest,
                  const Locale&... loc)
{
    auto contiguous = scn::scan<T>(loc..., input, scn::runtime_format(format));
    ASSERT_THAT(contiguous, Succeeded()) << format << " " << input;
    EXPECT_EQ(contiguous->value(), expected) << format << " " << input;
    EXPECT_EQ(std::string_view(scn::detail::to_address(contiguous->begin()),
                               contiguous->range().size()),
              expected_rest)
        << format << " " << input;

    auto source = std::deque<char>(input.begin(), input.end());
    auto noncontiguous =
        scn::scan<T>(loc..., source, scn::runtime_format(format));
    ASSERT_THAT(noncontiguous, Succeeded()) << format << " " << input;
    EXPECT_EQ(noncontiguous->value(), expected) << format << " " << input;
    EXPECT_EQ(std::string(noncontiguous->begin(), noncontiguous->end()),
              expected_rest)
        << format << " " << input;
}

template <typename T, typename... Locale>
void expect_error(std::string_view input,
                  std::string_view format,
                  scn::scan_error::code_t expected,
                  const Locale&... loc)
{
    auto contiguous = scn::scan<T>(loc..., input, scn::runtime_format(format));
    EXPECT_THAT(contiguous, FailedWith(expected)) << format << " " << input;

    auto source = std::deque<char>(input.begin(), input.end());
    auto noncontiguous =
        scn::scan<T>(loc..., source, scn::runtime_format(format));
    EXPECT_THAT(noncontiguous, FailedWith(expected)) << format << " " << input;
}
}  // namespace

TEST(WidthPrecisionTest, ExactWidthFields)
{
    auto r = scn::scan<int, int, int>("20261020", "{:4}{:2}{:2}");
    ASSERT_THAT(r, Succeeded());
    auto [year, month, day] = r->values();
    EXPECT_EQ(year, 2026);
    EXPECT_EQ(month, 10);
    EXPECT_EQ(day, 20);
    EXPECT_THAT(r->range(), IsEmptyRange());

    auto source = std::deque<char>{'2', '0', '2', '6', '1', '0', '2', '0'};
    auto from_deque = scn::scan<int, int, int>(source, "{:4}{:2}{:2}");
    ASSERT_THAT(from_deque, Succeeded());
    EXPECT_EQ(from_deque->values(), r->values());

    auto leading_zeros = scn::scan<int, int, int>("20260131", "{:4}{:2}{:2}");
    ASSERT_THAT(leading_zeros, Succeeded());
    EXPECT_EQ(leading_zeros->values(), std::make_tuple(2026, 1, 31));
}

TEST(WidthPrecisionTest, WidthStopsALongerValue)
{
    expect_value<int>("12345678", "{:5}", 12345, "678");
}

TEST(WidthPrecisionTest, ValueShorterThanWidth)
{
    expect_error<int>("12", "{:5}", scn::scan_error::length_too_short);
    expect_error<int>("12,", "{:5}", scn::scan_error::length_too_short);
}

TEST(WidthPrecisionTest, PrecisionStopsALongerValue)
{
    expect_value<int>("12345", "{:.4}", 1234, "5");
}

TEST(WidthPrecisionTest, LeadingWhitespaceNotCounted)
{
    expect_value<int>("   42", "{:2}", 42, "");

    auto r = scn::scan<int, int>("   42    7", "{:5}{:5}");
    EXPECT_THAT(r, FailedWith(scn::scan_error::length_too_short));
}

TEST(WidthPrecisionTest, StringCountsCodePoints)
{
    expect_value<std::string>("\xc3\xa4\xc3\xb6x", "{:2}", "\xc3\xa4\xc3\xb6",
                              "x");

    auto sv = scn::scan<std::string_view>("\xc3\xa4\xc3\xb6", "{:.1}");
    ASSERT_THAT(sv, Succeeded());
    EXPECT_EQ(sv->value(), "\xc3\xa4");
    EXPECT_STREQ(sv->begin(), "\xc3\xb6");
}

TEST(WidthPrecisionTest, WideStringCountsCodePoints)
{
    auto r = scn::scan<std::wstring>(L"\u00e4\u00f6x", L"{:2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), L"\u00e4\u00f6");
    EXPECT_EQ(std::wstring_view(scn::detail::to_address(r->begin()),
                                r->range().size()),
              L"x");
}

TEST(WidthPrecisionTest, WideStringCountsNonBmpCodePoints)
{
    auto r = scn::scan<std::wstring>(L"\U0001F602ab", L"{:2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), L"\U0001F602a");
    EXPECT_EQ(*r->begin(), L'b');
}

TEST(WidthPrecisionTest, NumbersCountCodeUnits)
{
    expect_value<int>("-0x1F", "{:4x}", -1, "F");
    expect_value<int>("+12", "{:3}", 12, "");
}

// A stray continuation byte counts as part of the code point before it,
// and an invalid lead byte counts as one code point.
// Either way, the string reader rejects the field.
TEST(WidthPrecisionTest, InvalidEncodingInBoundedString)
{
    expect_error<std::string>(
        "a\x80"
        "bc",
        "{:.2}", scn::scan_error::invalid_scanned_value);
    expect_error<std::string>(
        "\xf8"
        "bc",
        "{:.2}", scn::scan_error::invalid_scanned_value);
}

TEST(WidthPrecisionTest, CharacterSet)
{
    expect_value<std::string>("abcd", "{:3[a-z]}", "abc", "d");
    expect_error<std::string>("ab   ", "{:5[a-z]}",
                              scn::scan_error::length_too_short);
}

TEST(WidthPrecisionTest, EmptyInputWithBoundsIsEndOfInput)
{
    expect_error<std::string>("", "{:3[a-z]}", scn::scan_error::end_of_input);
    expect_error<std::string>("", "{:.3c}", scn::scan_error::end_of_input);
    expect_error<int>("   ", "{:3}", scn::scan_error::end_of_input);
}

#if !SCN_DISABLE_TYPE_DOUBLE
// Precision on floats counts characters, not digits after the point
TEST(WidthPrecisionTest, FloatPrecision)
{
    expect_value<double>("3.14", "{:.2f}", 3.0, "14");
}

TEST(WidthPrecisionTest, FloatExponentCutShort)
{
    expect_value<double>("1e5", "{:3e}", 100000.0, "");
    expect_error<double>("1e5", "{:2e}",
                         scn::scan_error::invalid_scanned_value);
}
#endif

#if !SCN_DISABLE_LOCALE
namespace {
struct comma_grouping_numpunct : std::numpunct<char> {
    char do_thousands_sep() const override
    {
        return ',';
    }
    std::string do_grouping() const override
    {
        return "\3";
    }
};
}  // namespace

// Thousands separators count toward the width and precision
TEST(WidthPrecisionTest, LocalizedInt)
{
    const std::locale loc{std::locale::classic(), new comma_grouping_numpunct};

    expect_value<int>("12,345", "{:5L}", 1234, "5", loc);
    expect_value<int>("1,234", "{:.3L}", 12, "34", loc);
}

#if !SCN_DISABLE_TYPE_DOUBLE
TEST(WidthPrecisionTest, LocalizedFloat)
{
    const std::locale loc{std::locale::classic(), new comma_grouping_numpunct};

    expect_value<double>("1,234.5", "{:5L}", 1234.0, ".5", loc);
    expect_value<double>("1,234.5", "{:.3L}", 12.0, "34.5", loc);
}
#endif
#endif
