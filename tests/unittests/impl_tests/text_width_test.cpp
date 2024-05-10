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

#include "../wrapped_gtest.h"

#include <scn/impl.h>

using namespace std::string_view_literals;

TEST(CalculateTextWidthTest, SimpleCodePoint)
{
    EXPECT_EQ(scn::impl::calculate_valid_text_width("a"sv), 1);
}
TEST(CalculateTextWidthTest, MultipleSimpleCodePoints)
{
    EXPECT_EQ(scn::impl::calculate_valid_text_width("abc"sv), 3);
}
TEST(CalculateTextWidthTest, SingleWidthCodePoint)
{
    EXPECT_EQ(scn::impl::calculate_valid_text_width("ä"sv), 1);
}
TEST(CalculateTextWidthTest, EmojiWidth)
{
    EXPECT_EQ(scn::impl::calculate_valid_text_width("😀"sv), 2);
}

TEST(TakeWidthViewTest, TakeAllSimpleCodePoints)
{
    auto v = scn::impl::take_width("abc"sv, 3);
    EXPECT_THAT(v, testing::ElementsAre('a', 'b', 'c'));
}
TEST(TakeWidthViewTest, TakeSomeSimpleCodePoints)
{
    auto v = scn::impl::take_width("abc"sv, 2);
    EXPECT_THAT(v, testing::ElementsAre('a', 'b'));
}
TEST(TakeWidthViewTest, TakeSomeComplexCodePoints)
{
    auto v = scn::impl::take_width("åäö"sv, 1);
    EXPECT_THAT(v, testing::ElementsAre(0xc3, 0xa5));
}
TEST(TakeWidthViewTest, TakeSomeComplexCodePoints2)
{
    auto v = scn::impl::take_width("åäö"sv, 2);
    EXPECT_THAT(v, testing::ElementsAre(0xc3, 0xa5, 0xc3, 0xa4));
}
TEST(TakeWidthViewTest, TakeMoreThanSource)
{
    auto v = scn::impl::take_width("abc"sv, 4);
    EXPECT_THAT(v, testing::ElementsAre('a', 'b', 'c'));
}
TEST(TakeWidthViewTest, FindCodeUnitNotInRange)
{
    auto v = scn::impl::take_width("åäö"sv, 2);
    for (auto it = v.begin(); it != v.end(); ++it) {
        EXPECT_NE(*it, ' ');
    }
}
TEST(TakeWidthViewTest, BidirectionalSimpleCodePoints)
{
    auto v = scn::impl::take_width("abc"sv, 2);

    EXPECT_EQ(scn::ranges::distance(v.begin(), v.end()), 2);

    auto it = v.begin();
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 'a');

    ++it;
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 'b');
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 1);

    ++it;
    EXPECT_EQ(it, v.end());

    --it;
    EXPECT_NE(it, v.end());
    EXPECT_NE(it, v.begin());
    EXPECT_EQ(*it, 'b');
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 1);
    EXPECT_EQ(scn::ranges::distance(v.begin(), it), 1);

    --it;
    EXPECT_NE(it, v.end());
    EXPECT_EQ(it, v.begin());
    EXPECT_EQ(*it, 'a');
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 2);

    scn::ranges::advance(it, 2);
    EXPECT_EQ(it, v.end());
}
TEST(TakeWidthViewTest, BidirectionalComplexCodePoints)
{
    auto v = scn::impl::take_width("aä"sv, 2);

    EXPECT_EQ(scn::ranges::distance(v.begin(), v.end()), 3);

    auto it = v.begin();
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, 'a');

    ++it;
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, static_cast<char>(0xc3));
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 2);
    EXPECT_EQ(scn::ranges::distance(v.begin(), it), 1);

    ++it;
    EXPECT_NE(it, v.end());
    EXPECT_EQ(*it, static_cast<char>(0xa4));
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 1);
    EXPECT_EQ(scn::ranges::distance(v.begin(), it), 2);

    ++it;
    EXPECT_EQ(it, v.end());
    EXPECT_EQ(scn::ranges::distance(v.begin(), it), 3);

    --it;
    EXPECT_NE(it, v.end());
    EXPECT_NE(it, v.begin());
    EXPECT_EQ(*it, static_cast<char>(0xa4));
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 1);
    EXPECT_EQ(scn::ranges::distance(v.begin(), it), 2);

    --it;
    EXPECT_NE(it, v.end());
    EXPECT_NE(it, v.begin());
    EXPECT_EQ(*it, static_cast<char>(0xc3));
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 2);
    EXPECT_EQ(scn::ranges::distance(v.begin(), it), 1);

    --it;
    EXPECT_NE(it, v.end());
    EXPECT_EQ(it, v.begin());
    EXPECT_EQ(*it, 'a');
    EXPECT_EQ(scn::ranges::distance(it, v.end()), 3);

    scn::ranges::advance(it, 3);
    EXPECT_EQ(it, v.end());
}
