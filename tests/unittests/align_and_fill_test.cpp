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

TEST(AlignAndFillTest, DefaultWithInt)
{
    auto r = scn::scan<int>("   42", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, DefaultWithRightAlignedChar)
{
    auto r = scn::scan<char>("   x", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), ' ');
    EXPECT_STREQ(r->begin(), "  x");
}

TEST(AlignAndFillTest, DefaultWithLeftAlignedChar)
{
    auto r = scn::scan<char>("x   ", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "   ");
}

TEST(AlignAndFillTest, CustomWidthDefaultAlignInt)
{
    auto r = scn::scan<int>("    42", "{:6}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CustomPrecDefaultAlignInt)
{
    auto r = scn::scan<int>("    42", "{:.6}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CustomWidthCustomPrecDefaultAlignInt)
{
    auto r = scn::scan<int>(" 42 ", "{:2.4}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), " ");
}

TEST(AlignAndFillTest, P1729_Ex3r0)
{
    auto r = scn::scan<int>("    42", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r1)
{
    auto r = scn::scan<char>("    x", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), ' ');
    EXPECT_STREQ(r->begin(), "   x");
}
TEST(AlignAndFillTest, P1729_Ex3r2)
{
    auto r = scn::scan<char>("x    ", "{}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "    ");
}

TEST(AlignAndFillTest, P1729_Ex3r3)
{
    auto r = scn::scan<int>("    42", "{:6}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, PythonParse3)
{
    auto r = scn::scan<std::string, std::string>("look", "{:.2}{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(std::get<0>(r->values()), "lo");
    EXPECT_EQ(std::get<1>(r->values()), "ok");
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, PythonParse4)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:4}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::length_too_short));
}
TEST(AlignAndFillTest, PythonParse5)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:.4}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(std::get<0>(r->values()), "look");
    EXPECT_EQ(std::get<1>(r->values()), "at");
    EXPECT_STREQ(r->begin(), " that");
}
TEST(AlignAndFillTest, PythonParse6)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:.4}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(std::get<0>(r->values()), "look");
    EXPECT_EQ(std::get<1>(r->values()), "at");
    EXPECT_STREQ(r->begin(), " that");
}
TEST(AlignAndFillTest, PythonParse7)
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

TEST(CustomPrecisionTest, DoubleWidthEmoji)
{
    auto r = scn::scan<std::string>("😂a", "{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "😂");
    EXPECT_STREQ(r->begin(), "a");
}

TEST(CustomPrecisionTest, Fuzz1)
{
    auto r = scn::scan<std::string>("a😂", "{:.2}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "a");
    EXPECT_STREQ(r->begin(), "😂");
}

// The precision is used up by whitespace skipped before the value
TEST(CustomPrecisionTest, LeadingWhitespaceExceedsPrecision)
{
    auto r = scn::scan<int>("   42", "{:.2}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::end_of_input));
}
