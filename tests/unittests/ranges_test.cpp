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

#include <map>
#include <set>
#include <vector>

#include <scn/ranges.h>
#include <scn/scan.h>

TEST(RangesTest, VectorSequence)
{
    static_assert(scn::range_format_kind<std::vector<int>, char>::value ==
                  scn::range_format::sequence);

    auto result = scn::scan<std::vector<int>>("[123, 456]", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(123, 456));
}

TEST(RangesTest, Set)
{
    static_assert(scn::range_format_kind<std::set<int>, char>::value ==
                  scn::range_format::set);

    auto result = scn::scan<std::set<int>>("{123, 456}", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(123, 456));
}

TEST(RangesTest, Map)
{
    static_assert(scn::range_format_kind<std::map<int, int>, char>::value ==
                  scn::range_format::map);

    auto result = scn::scan<std::map<int, int>>("{12: 34, 56: 78}", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(),
                testing::ElementsAre(std::pair{12, 34}, std::pair{56, 78}));
}

TEST(RangesTest, VectorEmpty)
{
    auto result = scn::scan<std::vector<int>>("[]", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value().empty());
}

TEST(RangesTest, VectorSingleElement)
{
    auto result = scn::scan<std::vector<int>>("[42]", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(42));
}

TEST(RangesTest, SetEmpty)
{
    auto result = scn::scan<std::set<int>>("{}", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value().empty());
}

TEST(RangesTest, SetSingleElement)
{
    auto result = scn::scan<std::set<int>>("{999}", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(999));
}

TEST(RangesTest, MapEmpty)
{
    auto result = scn::scan<std::map<int, int>>("{}", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value().empty());
}

TEST(RangesTest, MapSingleElement)
{
    auto result = scn::scan<std::map<int, int>>("{10: 20}", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(std::pair{10, 20}));
}

TEST(RangesTest, VectorMalformedMissingCloseBracket)
{
    auto result = scn::scan<std::vector<int>>("[123, 456", "{}");
    ASSERT_FALSE(result);
}

TEST(RangesTest, SetMalformedMissingCloseBrace)
{
    auto result = scn::scan<std::set<int>>("{123, 456", "{}");
    ASSERT_FALSE(result);
}

TEST(RangesTest, MapMalformedMissingColon)
{
    auto result = scn::scan<std::map<int, int>>("{12 34}", "{}");
    ASSERT_FALSE(result);
}

TEST(RangesTest, VectorManyElements)
{
    auto result = scn::scan<std::vector<int>>("[1, 2, 3, 4, 5]", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(RangesTest, VectorNegativeNumbers)
{
    auto result = scn::scan<std::vector<int>>("[-1, -2, -3]", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(-1, -2, -3));
}

TEST(RangesTest, VectorDoubles)
{
    auto result = scn::scan<std::vector<double>>("[1.5, 2.5, 3.5]", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().size(), 3);
    EXPECT_NEAR(result->value()[0], 1.5, 0.001);
    EXPECT_NEAR(result->value()[1], 2.5, 0.001);
    EXPECT_NEAR(result->value()[2], 3.5, 0.001);
}

TEST(RangesTest, VectorWithExtraWhitespace)
{
    auto result = scn::scan<std::vector<int>>("[  1  ,  2  ,  3  ]", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(1, 2, 3));
}

TEST(RangesTest, SetDuplicates)
{
    auto result = scn::scan<std::set<int>>("{1, 2, 2, 3}", "{}");
    ASSERT_TRUE(result);
    EXPECT_THAT(result->value(), testing::ElementsAre(1, 2, 3));
}

TEST(RangesTest, MapMultipleEntries)
{
    auto result = scn::scan<std::map<int, int>>("{1: 10, 2: 20, 3: 30}", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().size(), 3);
    EXPECT_EQ(result->value()[1], 10);
    EXPECT_EQ(result->value()[2], 20);
    EXPECT_EQ(result->value()[3], 30);
}

TEST(RangesTest, VectorMalformedNoOpenBracket)
{
    auto result = scn::scan<std::vector<int>>("123, 456]", "{}");
    ASSERT_FALSE(result);
}

TEST(RangesTest, SetMalformedNoOpenBrace)
{
    auto result = scn::scan<std::set<int>>("123, 456}", "{}");
    ASSERT_FALSE(result);
}

TEST(RangesTest, VectorInvalidElementType)
{
    auto result = scn::scan<std::vector<int>>("[abc, def]", "{}");
    ASSERT_FALSE(result);
}
