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

TEST(AlignAndFillTest, DefaultWithInt)
{
    auto r = scn::scan<int>("   42", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, DefaultWithRightAlignedChar)
{
    auto r = scn::scan<char>("   x", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), ' ');
    EXPECT_STREQ(r->begin(), "  x");
}

TEST(AlignAndFillTest, DefaultWithLeftAlignedChar)
{
    auto r = scn::scan<char>("x   ", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "   ");
}

TEST(AlignAndFillTest, CustomWidthDefaultAlignInt)
{
    auto r = scn::scan<int>("    42", "{:6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CustomWidthDefaultAlignChar)
{
    auto r = scn::scan<char>("x     ", "{:6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, CustomWidthDefaultAlignCharWithMoreInput)
{
    auto r = scn::scan<char>("x       ", "{:6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CustomPrecDefaultAlignInt)
{
    auto r = scn::scan<int>("    42", "{:.6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CustomPrecDefaultAlignChar)
{
    auto r = scn::scan<char>("x     ", "{:.6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CustomWidthCustomPrecDefaultAlignInt)
{
    auto r = scn::scan<int>(" 42 ", "{:2.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), " ");
}

TEST(AlignAndFillTest, NoWidth_NoPrec_RightAlign_CorrectFill)
{
    auto r = scn::scan<int>("***42", "{:*>}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_EqualPrec_RightAlign_CorrectFill)
{
    auto r = scn::scan<int>("***42", "{:*>.5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_LesserPrec_RightAlign_CorrectFill)
{
    auto r = scn::scan<int>("***42", "{:*>.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 4);
    EXPECT_STREQ(r->begin(), "2");
}

TEST(AlignAndFillTest, NoWidth_NoPrec_RightAlign_NoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*>}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_LargerPrec_RightAlign_NoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*>.5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, LargerWidth_LargerPrec_RightAlign_NoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*>5.5}");
    ASSERT_FALSE(r);
}

TEST(AlignAndFillTest, NoWidth_NoPrec_LeftAlign_CorrectFill)
{
    auto r = scn::scan<int>("42***", "{:*<}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_EqualPrec_LeftAlign_CorrectFill)
{
    auto r = scn::scan<int>("42***", "{:*<.5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_LesserPrec_LeftAlign_CorrectFill)
{
    auto r = scn::scan<int>("42***", "{:*<.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "*");
}

TEST(AlignAndFillTest, NoWidth_NoPrec_LeftAlign_NoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*<}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_LargerPrec_LeftAlign_NoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*<.5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, LargerWidth_LargerPrec_LeftAlign_NoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*<5.5}");
    ASSERT_FALSE(r);
}

TEST(AlignAndFillTest, NoWidth_NoPrec_CenterAlign_NoAlignInInput)
{
    auto r = scn::scan<int>("42", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_NoPrec_CenterAlign_CorrectFill_EqualBothSides)
{
    auto r = scn::scan<int>("*42*", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_NoPrec_CenterAlign_CorrectFill_MoreAfter)
{
    auto r = scn::scan<int>("*42**", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_NoPrec_CenterAlign_CorrectFill_MoreBefore)
{
    auto r = scn::scan<int>("**42*", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_EqualPrec_CenterAlign_CorrectFill)
{
    auto r = scn::scan<int>("**42**", "{:*^.6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, NoWidth_LesserPrec_CenterAlign_CorrectFill)
{
    auto r = scn::scan<int>("**42**", "{:*^.5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "*");
}

TEST(AlignAndFillTest, NoWidth_EvenLesserPrec_CenterAlign_CorrectFill)
{
    auto r = scn::scan<int>("**42**", "{:*^.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "**");
}

TEST(AlignAndFillTest, NoWidth_EvenMoreLesserPrec_CenterAlign_CorrectFill)
{
    auto r = scn::scan<int>("**42**", "{:*^.3}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 4);
    EXPECT_STREQ(r->begin(), "2**");
}

TEST(AlignAndFillTest, P1729_Ex3r0)
{
    auto r = scn::scan<int>("    42", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r1)
{
    auto r = scn::scan<char>("    x", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), ' ');
    EXPECT_STREQ(r->begin(), "   x");
}
TEST(AlignAndFillTest, P1729_Ex3r2)
{
    auto r = scn::scan<char>("x    ", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "    ");
}

TEST(AlignAndFillTest, P1729_Ex3r3)
{
    auto r = scn::scan<int>("    42", "{:6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r4)
{
    auto r = scn::scan<char>("x     ", "{:6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, P1729_Ex3r5)
{
    auto r = scn::scan<int>("***42", "{:*>}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r6)
{
    auto r = scn::scan<int>("***42", "{:*>5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r7)
{
    auto r = scn::scan<int>("***42", "{:*>4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r8)
{
    auto r = scn::scan<int>("***42", "{:*>.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 4);
    EXPECT_STREQ(r->begin(), "2");
}
TEST(AlignAndFillTest, P1729_Ex3r9)
{
    auto r = scn::scan<int>("***42", "{:*>4.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 4);
    EXPECT_STREQ(r->begin(), "2");
}

TEST(AlignAndFillTest, P1729_Ex3r10)
{
    auto r = scn::scan<int>("42", "{:*>}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r11)
{
    auto r = scn::scan<int>("42", "{:*>5}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::length_too_short);
}
TEST(AlignAndFillTest, P1729_Ex3r12)
{
    auto r = scn::scan<int>("42", "{:*>.5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r13)
{
    auto r = scn::scan<int>("42", "{:*>5.5}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::length_too_short);
}

TEST(AlignAndFillTest, P1729_Ex3r14)
{
    auto r = scn::scan<int>("42***", "{:*<}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r15)
{
    auto r = scn::scan<int>("42***", "{:*<5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r16)
{
    auto r = scn::scan<int>("42***", "{:*<4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r17)
{
    auto r = scn::scan<int>("42***", "{:*<.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "*");
}
TEST(AlignAndFillTest, P1729_Ex3r18)
{
    auto r = scn::scan<int>("42***", "{:*<4.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "*");
}

TEST(AlignAndFillTest, P1729_Ex3r19)
{
    auto r = scn::scan<int>("42", "{:*<}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r20)
{
    auto r = scn::scan<int>("42", "{:*<5}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::length_too_short);
}
TEST(AlignAndFillTest, P1729_Ex3r21)
{
    auto r = scn::scan<int>("42", "{:*<.5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r22)
{
    auto r = scn::scan<int>("42", "{:*<5.5}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::length_too_short);
}

TEST(AlignAndFillTest, P1729_Ex3r23)
{
    auto r = scn::scan<int>("42", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r24)
{
    auto r = scn::scan<int>("*42*", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r25)
{
    auto r = scn::scan<int>("*42**", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r26)
{
    auto r = scn::scan<int>("**42*", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, P1729_Ex3r27)
{
    auto r = scn::scan<int>("**42**", "{:*^6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r28)
{
    auto r = scn::scan<int>("*42**", "{:*^5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r29)
{
    auto r = scn::scan<int>("**42*", "{:*^5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r30)
{
    auto r = scn::scan<int>("**42*", "{:*^6}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::length_too_short);
}
TEST(AlignAndFillTest, P1729_Ex3r31)
{
    auto r = scn::scan<int>("**42*", "{:*^.6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r32)
{
    auto r = scn::scan<int>("**42*", "{:*^6.6}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::length_too_short);
}

TEST(AlignAndFillTest, P1729_Ex3r33)
{
    auto r = scn::scan<int>("#*42*", "{:*^}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::invalid_scanned_value);
}
TEST(AlignAndFillTest, P1729_Ex3r34)
{
    auto r = scn::scan<int>("#*42*", "#{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r35)
{
    auto r = scn::scan<int>("#*42*", "#{:#^}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::invalid_scanned_value);
}

TEST(AlignAndFillTest, P1729_Ex3r36)
{
    auto r = scn::scan<int>("***42*", "{:*^3}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, P1729_Ex3r37)
{
    auto r = scn::scan<int>("***42*", "{:*^.3}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::invalid_fill);
}

TEST(AlignAndFillTest, PythonParse1)
{
    auto r = scn::scan<std::string>("with     a herring", "with {:>} herring");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "a");
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, PythonParse1_All)
{
    auto r = scn::scan<std::string, std::string, std::string>(
        "with     a herring", "{}{:>}{}");
    ASSERT_TRUE(r);
    const auto& [v1, v2, v3] = r->values();
    EXPECT_EQ(v1, "with");
    EXPECT_EQ(v2, "a");
    EXPECT_EQ(v3, "herring");
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, PythonParse2)
{
    auto r =
        scn::scan<std::string>("spam     lovely     spam", "spam {:^} spam");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "lovely");
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, PythonParse2_All)
{
    auto r = scn::scan<std::string, std::string, std::string>(
        "spam     lovely     spam", "{}{:^}{}");
    ASSERT_TRUE(r);
    const auto& [v1, v2, v3] = r->values();
    EXPECT_EQ(v1, "spam");
    EXPECT_EQ(v2, "lovely");
    EXPECT_EQ(v3, "spam");
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, PythonParse3)
{
    auto r = scn::scan<std::string, std::string>("look", "{:.2}{:.2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(std::get<0>(r->values()), "lo");
    EXPECT_EQ(std::get<1>(r->values()), "ok");
    EXPECT_STREQ(r->begin(), "");
}
TEST(AlignAndFillTest, PythonParse4)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:4}");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code(), scn::scan_error::length_too_short);
}
TEST(AlignAndFillTest, PythonParse5)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(std::get<0>(r->values()), "look");
    EXPECT_EQ(std::get<1>(r->values()), "at");
    EXPECT_STREQ(r->begin(), " that");
}
TEST(AlignAndFillTest, PythonParse6)
{
    auto r = scn::scan<std::string, std::string>("look at that", "{:4}{:.4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(std::get<0>(r->values()), "look");
    EXPECT_EQ(std::get<1>(r->values()), "at");
    EXPECT_STREQ(r->begin(), " that");
}
TEST(AlignAndFillTest, PythonParse7)
{
    auto r = scn::scan<int, int>("0440", "{:.2}{:.2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(std::get<0>(r->values()), 4);
    EXPECT_EQ(std::get<1>(r->values()), 40);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, LeftAlignedWithSpaces)
{
    auto r = scn::scan<int>("42  ", "{: <.3}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), " ");
}

TEST(AlignAndFillTest, DoubleWideFillChar)
{
    auto r = scn::scan<int>("🤡🤡1🤡🤡🤡", "{:🤡^6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 1);
    EXPECT_EQ(*r->begin(), '\0');
}

TEST(CustomPrecisionTest, Ascii)
{
    auto r = scn::scan<std::string>("abc", "{:.2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "ab");
    EXPECT_STREQ(r->begin(), "c");
}

TEST(CustomPrecisionTest, SingleWidthText)
{
    auto r = scn::scan<std::string>("åäö", "{:.2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "åä");
    EXPECT_STREQ(r->begin(), "ö");
}

TEST(CustomPrecisionTest, DoubleWidthEmoji)
{
    auto r = scn::scan<std::string>("😂a", "{:.2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "😂");
    EXPECT_STREQ(r->begin(), "a");
}

TEST(CustomPrecisionTest, Fuzz1)
{
    auto r = scn::scan<std::string>("a😂", "{:^.2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "a");
    EXPECT_STREQ(r->begin(), "😂");
}
