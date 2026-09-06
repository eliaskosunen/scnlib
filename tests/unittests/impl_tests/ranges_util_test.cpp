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

// Test ranges::distance
TEST(RangesUtilTest, DistanceEmpty)
{
    std::string_view sv = "";
    auto dist = scn::ranges::distance(sv.begin(), sv.end());
    EXPECT_EQ(dist, 0);
}

TEST(RangesUtilTest, DistanceNonEmpty)
{
    std::string_view sv = "hello";
    auto dist = scn::ranges::distance(sv.begin(), sv.end());
    EXPECT_EQ(dist, 5);
}

// Test ranges::advance
TEST(RangesUtilTest, AdvanceByZero)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    scn::ranges::advance(it, 0);
    EXPECT_EQ(it, sv.begin());
}

TEST(RangesUtilTest, AdvanceByPositive)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    scn::ranges::advance(it, 2);
    EXPECT_EQ(*it, 's');
}

TEST(RangesUtilTest, AdvanceToEnd)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    scn::ranges::advance(it, sv.end());
    EXPECT_EQ(it, sv.end());
}

TEST(RangesUtilTest, AdvanceBounded)
{
    std::string_view sv = "test";
    auto it = sv.begin();
    auto result = scn::ranges::advance(it, 10, sv.end());
    EXPECT_EQ(it, sv.end());
    EXPECT_EQ(result, 6);  // Moved 4, wanted 10
}

// Test ranges::next
TEST(RangesUtilTest, NextDefault)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin());
    EXPECT_EQ(*it, 'e');
}

TEST(RangesUtilTest, NextByN)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin(), 2);
    EXPECT_EQ(*it, 's');
}

TEST(RangesUtilTest, NextToBound)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin(), sv.end());
    EXPECT_EQ(it, sv.end());
}

TEST(RangesUtilTest, NextBounded)
{
    std::string_view sv = "test";
    auto it = scn::ranges::next(sv.begin(), 10, sv.end());
    EXPECT_EQ(it, sv.end());
}
