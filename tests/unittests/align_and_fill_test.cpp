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

TEST(AlignAndFillTest, DefaultWithInt)
{
    auto r = scn::scan<int>("   42", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, DefaultWithChar1)
{
    auto r = scn::scan<char>("   x", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), ' ');
    EXPECT_STREQ(r->begin(), "  x");
}

TEST(AlignAndFillTest, DefaultWithChar2)
{
    auto r = scn::scan<char>("x   ", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "   ");
}

TEST(AlignAndFillTest, CustomWidthInt)
{
    auto r = scn::scan<int>("     42", "{:6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CustomWidthChar)
{
    auto r = scn::scan<char>("x     ", "{:6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 'x');
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, RightAlignWithCustomFillAndNoWidth)
{
    auto r = scn::scan<int>("***42", "{:*>}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, RightAlignWithCustomFillAndFullWidth)
{
    auto r = scn::scan<int>("***42", "{:*>5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, RightAlignWithCustomFillAndLesserWidth)
{
    auto r = scn::scan<int>("***42", "{:*>4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 4);
    EXPECT_STREQ(r->begin(), "2");
}

TEST(AlignAndFillTest, RightAlignWithCustomFillAndNoWidthButNoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*>}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, RightAlignWithCustomFillAndGreaterWidth)
{
    auto r = scn::scan<int>("42", "{:*>5}");
    ASSERT_FALSE(r);
}

TEST(AlignAndFillTest, LeftAlignWithCustomFillAndNoWidth)
{
    auto r = scn::scan<int>("42***", "{:*<}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, LeftAlignWithCustomFillAndFullWidth)
{
    auto r = scn::scan<int>("42***", "{:*<5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, LeftAlignWithCustomFillAndLesserWidth)
{
    auto r = scn::scan<int>("42***", "{:*<4}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 4);
    EXPECT_STREQ(r->begin(), "2");
}

TEST(AlignAndFillTest, LeftAlignWithCustomFillAndNoWidthButNoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*<}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, LeftAlignWithCustomFillAndGreaterWidth)
{
    auto r = scn::scan<int>("42", "{:*<5}");
    ASSERT_FALSE(r);
}

TEST(AlignAndFillTest, CenterAlignWithNoFillInInput)
{
    auto r = scn::scan<int>("42", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CenterAlignWithCorrectFillInInput)
{
    auto r = scn::scan<int>("*42*", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CenterAlignWithMoreFillAtEndOfInput)
{
    auto r = scn::scan<int>("*42**", "{:*^}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "*");
}

TEST(AlignAndFillTest, CenterAlignWithLessFillAtEndOfInput)
{
    auto r = scn::scan<int>("**42*", "{:*^}");
    ASSERT_FALSE(r);
}

TEST(AlignAndFillTest, CenterAlignWithCustomEvenWidth)
{
    auto r = scn::scan<int>("**42**", "{:*^6}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(AlignAndFillTest, CenterAlignWithCustomOddWidth)
{
    auto r = scn::scan<int>("*42**", "{:*^5}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), 42);
    EXPECT_STREQ(r->begin(), "");
}

TEST(CustomWidthTest, Ascii)
{
    auto r = scn::scan<std::string>("abc", "{:2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "ab");
    EXPECT_STREQ(r->begin(), "c");
}

TEST(CustomWidthTest, SingleWidthText)
{
    auto r = scn::scan<std::string>("åäö", "{:2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "åä");
    EXPECT_STREQ(r->begin(), "ö");
}

TEST(CustomWidthTest, DoubleWidthEmoji)
{
    auto r = scn::scan<std::string>("😂a", "{:2}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "😂");
    EXPECT_STREQ(r->begin(), "a");
}
