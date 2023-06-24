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
    auto result = scn::scan<int>("42", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), 42);
}

TEST(ScanTest, MultipleValues)
{
    auto result = scn::scan<int, int>("123 456", "{} {}");
    ASSERT_TRUE(result);
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(ScanTest, StringValue)
{
    auto result = scn::scan<std::string>("abc def", "abc {}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(std::get<0>(result->values()).c_str(), "def");
}

TEST(ScanTest, LiteralSkip)
{
    auto result = scn::scan<int>("abc 123", "abc {}");
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), 123);
}

TEST(ScanTest, ResultUse)
{
    auto source = std::string_view{"123 456"};
    auto result = scn::scan<int>(source, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(result->values()), 123);

    auto result2 = scn::scan<int>(
        scn::ranges::subrange{result->begin(), source.end()}, "{}");
    ASSERT_TRUE(result2);
    EXPECT_EQ(std::get<0>(result2->values()), 456);
}

TEST(ScanTest, IntValue)
{
    auto result = scn::scan_value<int>("123");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 123);
}

TEST(ScanTest, Discard)
{
    auto result =
        scn::scan<int, scn::discard<int>, int>("123 456 789", "{} {} {}");
    ASSERT_TRUE(result);
    auto [a, _, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 789);
}

TEST(ScanTest, CodePoint)
{
    auto result = scn::scan<scn::code_point>("ä", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0xe4);
}

TEST(ScanTest, BoolNumeric)
{
    auto result = scn::scan<bool>("1", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value());
}
TEST(ScanTest, BoolText)
{
    auto result = scn::scan<bool>("true", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->value());
}

TEST(ScanTest, DefaultValueSuccess)
{
    auto result = scn::scan<int>("42", "{}", {123});
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanTest, DefaultValueFail)
{
    auto result = scn::scan<int>("foobar", "{}", {123});
    ASSERT_FALSE(result);
}
TEST(ScanTest, DefaultValueString)
{
    std::string initial_string;
    initial_string.reserve(256);
    const auto addr = initial_string.data();

    auto result =
        scn::scan<std::string>("foobar", "{}", {std::move(initial_string)});
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), "foobar");
    EXPECT_EQ(result->value().data(), addr);
    EXPECT_GE(result->value().capacity(), 256);
}
