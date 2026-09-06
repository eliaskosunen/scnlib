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

#include <scn/istream.h>

#include "wrapped_gtest.h"

#if !SCN_DISABLE_IOSTREAM

#include <sstream>

using testing::FieldsAre;

TEST(IstreamSourceTest, Stringstream)
{
    std::istringstream ss{"123 abc"};
    auto res = scn::scan<int, std::string>(ss, "{} {}");
    ASSERT_TRUE(res);
    EXPECT_THAT(res->values(), FieldsAre(123, "abc"));
}

TEST(IstreamSourceTest, StringstreamDouble)
{
    std::istringstream ss{"3.14 2.71"};
    auto res = scn::scan<double, double>(ss, "{} {}");
    ASSERT_TRUE(res);
    auto [a, b] = res->values();
    EXPECT_NEAR(a, 3.14, 0.001);
    EXPECT_NEAR(b, 2.71, 0.001);
}

TEST(IstreamSourceTest, StringstreamMultiline)
{
    std::istringstream ss{"123\n456\n789"};
    auto res1 = scn::scan<int>(ss, "{}");
    ASSERT_TRUE(res1);
    EXPECT_EQ(res1->value(), 123);

    auto res2 = scn::scan<int>(ss, "{}");
    ASSERT_TRUE(res2);
    EXPECT_EQ(res2->value(), 456);

    auto res3 = scn::scan<int>(ss, "{}");
    ASSERT_TRUE(res3);
    EXPECT_EQ(res3->value(), 789);
}

TEST(IstreamSourceTest, StringstreamEOF)
{
    std::istringstream ss{"42"};
    auto res1 = scn::scan<int>(ss, "{}");
    ASSERT_TRUE(res1);
    EXPECT_EQ(res1->value(), 42);

    auto res2 = scn::scan<int>(ss, "{}");
    ASSERT_FALSE(res2);
    EXPECT_EQ(res2.error().code(), scn::scan_error::end_of_input);
}

TEST(IstreamSourceTest, EmptyStringstream)
{
    std::istringstream ss{""};
    auto res = scn::scan<int>(ss, "{}");
    ASSERT_FALSE(res);
    EXPECT_EQ(res.error().code(), scn::scan_error::end_of_input);
}

TEST(IstreamSourceTest, StringstreamWithWhitespace)
{
    std::istringstream ss{"  42  "};
    auto res = scn::scan<int>(ss, "{}");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->value(), 42);
}

TEST(IstreamSourceTest, StringstreamChar)
{
    std::istringstream ss{"abc"};
    auto res = scn::scan<char, char, char>(ss, "{}{}{}");
    ASSERT_TRUE(res);
    auto [a, b, c] = res->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
    EXPECT_EQ(c, 'c');
}

TEST(IstreamSourceTest, StringstreamBool)
{
    std::istringstream ss{"true false"};
    auto res = scn::scan<bool, bool>(ss, "{} {}");
    ASSERT_TRUE(res);
    auto [a, b] = res->values();
    EXPECT_TRUE(a);
    EXPECT_FALSE(b);
}

TEST(IstreamSourceTest, StringstreamLongString)
{
    std::istringstream ss{"verylongstringwithoutspaces"};
    auto res = scn::scan<std::string>(ss, "{}");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->value(), "verylongstringwithoutspaces");
}

TEST(IstreamSourceTest, StringstreamNegativeDouble)
{
    std::istringstream ss{"-3.14"};
    auto res = scn::scan<double>(ss, "{}");
    ASSERT_TRUE(res);
    EXPECT_NEAR(res->value(), -3.14, 0.001);
}

TEST(IstreamSourceTest, StringstreamMixedTypes)
{
    std::istringstream ss{"42 hello 3.14 true"};
    auto res = scn::scan<int, std::string, double, bool>(ss, "{} {} {} {}");
    ASSERT_TRUE(res);
    auto [i, s, d, b] = res->values();
    EXPECT_EQ(i, 42);
    EXPECT_EQ(s, "hello");
    EXPECT_NEAR(d, 3.14, 0.001);
    EXPECT_TRUE(b);
}

#endif
