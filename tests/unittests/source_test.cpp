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

using ::testing::Test;

TEST(SourceTest, Simple)
{
    auto [r, i] = scn::scan<int>("123", "{}");
    EXPECT_TRUE(r);
    EXPECT_TRUE(r.range().empty());
    EXPECT_EQ(i, 123);
}
TEST(SourceTest, TwoArgs)
{
    auto [r, i, d] = scn::scan<int, double>("123 3.14", "{} {}");
    EXPECT_TRUE(r);
    EXPECT_TRUE(r.range().empty());
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

using std::get;

TEST(SourceTest, SourceIsStringLiteral)
{
    auto result = scn::scan<int, double>("123 3.14", "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_result_tuple<std::string_view, int, double>>);
    EXPECT_TRUE(result);
    EXPECT_TRUE(get<0>(result));
    EXPECT_TRUE(get<0>(result).range().empty());
    EXPECT_EQ(get<1>(result), 123);
    EXPECT_DOUBLE_EQ(get<2>(result), 3.14);
}

TEST(SourceTest, SourceIsStringView)
{
    auto result = scn::scan<int, double>(std::string_view{"123 3.14"}, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_result_tuple<std::string_view, int, double>>);
    EXPECT_TRUE(result);
    EXPECT_TRUE(get<0>(result));
    EXPECT_TRUE(get<0>(result).range().empty());
    EXPECT_EQ(get<1>(result), 123);
    EXPECT_DOUBLE_EQ(get<2>(result), 3.14);
}

TEST(SourceTest, SourceIsStringLvalue)
{
    auto source = std::string{"123 3.14"};
    auto result = scn::scan<int, double>(source, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_result_tuple<std::string_view, int, double>>);
    EXPECT_TRUE(get<0>(result));
    EXPECT_TRUE(get<0>(result).range().empty());
    EXPECT_EQ(get<1>(result), 123);
    EXPECT_DOUBLE_EQ(get<2>(result), 3.14);
}

TEST(SourceTest, SourceIsStringRvalue)
{
    auto result = scn::scan<int, double>(std::string{"123 3.14"}, "{} {}");
    static_assert(std::is_same_v<
                  decltype(result),
                  scn::scan_result_tuple<scn::ranges::dangling, int, double>>);
}

template <typename>
struct debug;

TEST(SourceTest, SourceIsIstreamViewLvalue)
{
    auto ss = std::istringstream{"123 3.14"};
    auto ssr = scn::istreambuf_view{ss};
    auto range = scn::istreambuf_subrange{ssr};

    auto result = scn::scan<int, double>(range, "{} {}");
    static_assert(
        std::is_same_v<
            decltype(result),
            scn::scan_result_tuple<scn::istreambuf_subrange, int, double>>);
}

TEST(SourceTest, SourceIsIstreamViewRvalue)
{
    auto ss = std::istringstream{"123 3.14"};
    auto ssr = scn::istreambuf_view{ss};

    auto result =
        scn::scan<int, double>(scn::istreambuf_subrange{ssr}, "{} {}");
    static_assert(
        std::is_same_v<
            decltype(result),
            scn::scan_result_tuple<scn::istreambuf_subrange, int, double>>);
}

TEST(SourceTest, SourceIsIstreamRangeLvalue)
{
    auto ss = std::istringstream{"123 3.14"};
    auto ssr = scn::istreambuf_view{ss};

    auto result = scn::scan<int, double>(ssr, "{} {}");
    static_assert(
        std::is_same_v<
            decltype(result),
            scn::scan_result_tuple<scn::istreambuf_subrange, int, double>>);
}

TEST(SourceTest, SourceIsIstreamRangeRvalue)
{
    auto ss = std::istringstream{"123 3.14"};

    auto result = scn::scan<int, double>(scn::istreambuf_view{ss}, "{} {}");
    static_assert(std::is_same_v<
                  decltype(result),
                  scn::scan_result_tuple<scn::ranges::dangling, int, double>>);
}
