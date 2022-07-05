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

TEST(ScanTest, SingleValue)
{
    auto [result, i] = scn::scan<int>("42", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(i, 42);
}

TEST(ScanTest, MultipleValues)
{
    auto [result, a, b] = scn::scan<int, int>("123 456", "{} {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(ScanTest, StringValue)
{
    auto [result, a] = scn::scan<std::string>("abc def", "abc {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_STREQ(a.c_str(), "def");
}

TEST(ScanTest, LiteralSkip)
{
    auto [result, a] = scn::scan<int>("abc 123", "abc {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(a, 123);
}

TEST(ScanTest, ResultUse)
{
    auto [result, a] = scn::scan<int>("123 456", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(a, 123);

    auto [result2, b] = scn::scan<int>(result.range(), "{}");
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result2.range().empty());
    EXPECT_EQ(b, 456);
}

TEST(ScanTest, IntValue)
{
    auto [result, i] = scn::scan_value<int>("123");
    EXPECT_TRUE(result);
    EXPECT_EQ(i, 123);
}

TEST(ScanTest, Discard)
{
    auto [result, a, _, b] =
        scn::scan<int, scn::discard<int>, int>("123 456 789", "{} {} {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 789);
}

TEST(ScanTest, CodePoint)
{
    auto [result, cp] = scn::scan<scn::code_point>("ä", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(cp, 0xe4);
}

TEST(ScanTest, BoolNumeric)
{
    auto [result, val] = scn::scan<bool>("1", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_TRUE(val);
}
TEST(ScanTest, BoolText)
{
    auto [result, val] = scn::scan<bool>("true", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_TRUE(val);
}

TEST(ScanTest, DefaultValueSuccess)
{
    auto [result, val] = scn::scan<int>("42", "{}", {123});
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val, 42);
}
TEST(ScanTest, DefaultValueFail)
{
    auto [result, val] = scn::scan<int>("foobar", "{}", {123});
    EXPECT_FALSE(result);
    EXPECT_EQ(val, 0);
}
TEST(ScanTest, DefaultValueString)
{
    std::string initial_string;
    initial_string.reserve(256);
    const auto addr = initial_string.data();

    auto [result, str] =
        scn::scan<std::string>("foobar", "{}", {std::move(initial_string)});
    EXPECT_TRUE(result);
    EXPECT_EQ(str, "foobar");
    EXPECT_EQ(str.data(), addr);
    EXPECT_GE(str.capacity(), 256);
}
