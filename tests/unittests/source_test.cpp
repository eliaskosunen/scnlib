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
#include "scn/detail/result.h"

using ::testing::Test;

template <typename R, typename... Args>
using scan_result_tuple_helper = std::tuple<scn::scan_result<R>, Args...>;

TEST(SourceTest, Simple)
{
    auto r = scn::scan<int>("123", "{}");
    ASSERT_TRUE(r);
    EXPECT_EQ(*r->begin(), '\0');
    EXPECT_EQ(std::get<0>(r->values()), 123);
}
TEST(SourceTest, TwoArgs)
{
    auto r = scn::scan<int, double>("123 3.14", "{} {}");
    ASSERT_TRUE(r);
    EXPECT_EQ(*r->begin(), '\0');
    auto [i, d] = r->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringLiteral)
{
    auto result = scn::scan<int, double>("123 3.14", "{} {}");
    static_assert(
        std::is_same_v<
            decltype(result),
            scn::scan_expected<scn::scan_result<const char*, int, double>>>);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringView)
{
    auto result = scn::scan<int, double>(std::string_view{"123 3.14"}, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_expected<scn::scan_result<
                           std::string_view::iterator, int, double>>>);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringLvalue)
{
    auto source = std::string{"123 3.14"};
    auto result = scn::scan<int, double>(source, "{} {}");
    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_expected<scn::scan_result<
                                     std::string::iterator, int, double>>>);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringRvalue)
{
    auto result = scn::scan<int, double>(std::string{"123 3.14"}, "{} {}");
    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_expected<scn::scan_result<
                                     scn::ranges::dangling, int, double>>>);
}

TEST(SourceTest, SourceIsIstreamViewLvalue)
{
    auto ss = std::istringstream{"123 3.14"};
    auto ssr = scn::istreambuf_view{ss};
    auto range = scn::istreambuf_subrange{ssr};

    auto result = scn::scan<int, double>(range, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_expected<scn::scan_result<
                           scn::istreambuf_subrange::iterator, int, double>>>);
}

TEST(SourceTest, SourceIsIstreamViewRvalue)
{
    auto ss = std::istringstream{"123 3.14"};
    auto ssr = scn::istreambuf_view{ss};

    auto result =
        scn::scan<int, double>(scn::istreambuf_subrange{ssr}, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_expected<scn::scan_result<
                           scn::istreambuf_subrange::iterator, int, double>>>);
}

TEST(SourceTest, SourceIsIstreamRangeLvalue)
{
    auto ss = std::istringstream{"123 3.14"};
    auto ssr = scn::istreambuf_view{ss};

    auto result = scn::scan<int, double>(ssr, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_expected<scn::scan_result<
                           scn::istreambuf_view::iterator, int, double>>>);
}

TEST(SourceTest, SourceIsIstreamRangeRvalue)
{
    auto ss = std::istringstream{"123 3.14"};

    auto result = scn::scan<int, double>(scn::istreambuf_view{ss}, "{} {}");
    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_expected<scn::scan_result<
                                     scn::ranges::dangling, int, double>>>);
}
