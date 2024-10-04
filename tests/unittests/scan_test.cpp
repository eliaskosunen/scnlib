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

#include <deque>

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
    auto result = scn::scan<char32_t>("ä", "{}");
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

TEST(ScanTest, NumberedArguments)
{
    auto result = scn::scan<int, int>("123 456", "{0} {1}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(ScanTest, NumberedArgumentsSwapped)
{
    auto result = scn::scan<int, int>("123 456", "{1} {0}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 456);
    EXPECT_EQ(b, 123);
}

TEST(ScanTest, NumberedArgumentsRepeatedSingleArg)
{
    auto result = scn::scan<int>("123 456", scn::runtime_format("{0} {0}"));
    ASSERT_FALSE(result);
}

TEST(ScanTest, NumberedArgumentsRepeatedDoubleArg)
{
    auto result =
        scn::scan<int, int>("123 456", scn::runtime_format("{0} {0}"));
    ASSERT_FALSE(result);
}

TEST(ScanTest, NumberedArgumentsOutOfRange)
{
    auto result = scn::scan<int>("123 456", scn::runtime_format("{1}"));
    ASSERT_FALSE(result);
}

TEST(ScanTest, FuzzerFailStringInput)
{
    auto result = scn::scan<std::string>("]]\360\n", "{}");
    ASSERT_FALSE(result);
}
TEST(ScanTest, FuzzerFailDequeInput)
{
    using namespace std::string_view_literals;
    auto in = "]\360\n"sv;
    std::deque<char> rng{};
    std::copy(in.begin(), in.end(), std::back_inserter(rng));

    auto result = scn::scan<std::string>(rng, "{}");
    ASSERT_FALSE(result);
}

TEST(ScanTest, DeconstructedTimestamp)
{
    auto res = scn::scan<int, int, int, int, int, double>(
        "2024-03-23T09:20:33.576864", "{:4}-{:2}-{:2}T{:2}:{:2}:{}");
    ASSERT_TRUE(res);
    EXPECT_EQ(std::get<0>(res->values()), 2024);
    EXPECT_EQ(std::get<1>(res->values()), 3);
    EXPECT_EQ(std::get<2>(res->values()), 23);
    EXPECT_EQ(std::get<3>(res->values()), 9);
    EXPECT_EQ(std::get<4>(res->values()), 20);
    EXPECT_DOUBLE_EQ(std::get<5>(res->values()), 33.576864);
}
TEST(ScanTest, DeconstructedTimestamp2)
{
    auto res = scn::scan<int, int, int, int, int>("2024-03-23T09:20:33.576864",
                                                  "{:4}-{:2}-{:2}T{:2}:{:2}:");
    ASSERT_TRUE(res);
    EXPECT_EQ(std::get<0>(res->values()), 2024);
    EXPECT_EQ(std::get<1>(res->values()), 3);
    EXPECT_EQ(std::get<2>(res->values()), 23);
    EXPECT_EQ(std::get<3>(res->values()), 9);
    EXPECT_EQ(std::get<4>(res->values()), 20);
    EXPECT_STREQ(res->range().data(), "33.576864");
}

TEST(ScanTest, LotsOfArguments)
{
    auto res = scn::scan<int, int, int, int, int, int, int, double>(
        "1 2 3 4 5 6 7 8.9", "{} {} {} {} {} {} {} {}");
    ASSERT_TRUE(res);
    auto [a1, a2, a3, a4, a5, a6, a7, a8] = res->values();
    EXPECT_EQ(a1, 1);
    EXPECT_EQ(a2, 2);
    EXPECT_EQ(a3, 3);
    EXPECT_EQ(a4, 4);
    EXPECT_EQ(a5, 5);
    EXPECT_EQ(a6, 6);
    EXPECT_EQ(a7, 7);
    EXPECT_DOUBLE_EQ(a8, 8.9);
}
TEST(ScanTest, EvenMoreArguments)
{
    auto res = scn::scan<int, int, int, int, int, int, int, int, int, int, int,
                         int, int, int, int, int, int, int, int, int, int, int,
                         int, int, int, int, int, int, int, int, int, int, int>(
        "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 "
        "27 28 29 30 31 32 33",
        "{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} "
        "{} {} {} {} {} {} {} {} {} {}");
    ASSERT_TRUE(res);
}

TEST(ScanTest, DoubleNewline)
{
    auto res = scn::scan<int>("1\n\n", "{}\n\n");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->value(), 1);
    EXPECT_EQ(res->begin(), res->end());
}
TEST(ScanTest, DoubleNewline2)
{
    auto res = scn::scan<int, int>("1\n\n2", "{}\n\n{}");
    ASSERT_TRUE(res);
    auto [a, b] = res->values();
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
    EXPECT_EQ(res->begin(), res->end());
}

TEST(ScanTest, Pointer)
{
    auto res =
        scn::scan<void*, const void*>("0xdeadbeef 0XABBAABBA", "{} {:p}");
    ASSERT_TRUE(res);
    auto [a, b] = res->values();
    EXPECT_EQ(reinterpret_cast<uintptr_t>(a), 0xdeadbeef);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(b), 0xABBAABBA);
}
