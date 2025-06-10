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

using ::testing::Test;

template <bool, typename>
struct scan_result_helper_impl;

template <typename It>
struct scan_result_helper_impl<false, It> {
    using type = scn::ranges::subrange<It>;
};
template <typename It>
struct scan_result_helper_impl<true, It> {
    using type = scn::ranges::dangling;
};

template <typename It, typename... Args>
using scan_result_helper = scn::scan_expected<scn::scan_result<
    typename scan_result_helper_impl<std::is_same_v<It, scn::ranges::dangling>,
                                     It>::type,
    Args...>>;

TEST(SourceTest, Simple)
{
    auto r = scn::scan<int>("123", "{}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());
    EXPECT_EQ(std::get<0>(r->values()), 123);
}
TEST(SourceTest, TwoArgs)
{
    auto r = scn::scan<int, double>("123 3.14", "{} {}");
    ASSERT_TRUE(r);
    EXPECT_TRUE(r->range().empty());
    auto [i, d] = r->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringLiteral)
{
    auto result = scn::scan<int, double>("123 3.14", "{} {}");
    static_assert(std::is_same_v<decltype(result),
                                 scan_result_helper<const char*, int, double>>);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringView)
{
    auto result = scn::scan<int, double>(std::string_view{"123 3.14"}, "{} {}");
    static_assert(std::is_same_v<
                  decltype(result),
                  scan_result_helper<std::string_view::iterator, int, double>>);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringLvalue)
{
    auto source = std::string{"123 3.14"};
    auto result = scn::scan<int, double>(source, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scan_result_helper<std::string::iterator, int, double>>);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringRvalue)
{
    auto result = scn::scan<int, double>(std::string{"123 3.14"}, "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scan_result_helper<scn::ranges::dangling, int, double>>);
}

TEST(SourceTest, SourceIsRandomAccessRange)
{
    auto source = std::deque<char>{'1', '2', '3', ' ', '3', '.', '1', '4'};
    auto result = scn::scan<int, double>(source, "{} {}");
    static_assert(std::is_same_v<
                  decltype(result),
                  scan_result_helper<std::deque<char>::iterator, int, double>>);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsInputRange)
{
    auto source = std::deque<char>{'1', '2', '3', ' ', '3', '.', '1', '4'};
    auto input = scn::ranges::views::to_input(source);
    static_assert(std::is_same_v<decltype(input),
                                 scn::ranges::to_input_view<
                                     scn::ranges::ref_view<std::deque<char>>>>);

    auto result = scn::scan<int>(input, "{}");
    using expected_result = scn::scan_expected<scn::scan_result<
        scn::ranges::pair_concat_view<
            scn::ranges::owning_view<std::string>,
            scn::detail::borrowed_tail_subrange_t<decltype(input)&>>,
        int>>;
    static_assert(std::is_same_v<decltype(result), expected_result>);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 123);

    auto result2 = scn::scan<double>(result->range(), "{}");
    using expected_result2 = scn::scan_expected<scn::scan_result<
        scn::ranges::pair_concat_view<
            scn::ranges::owning_view<std::string>,
            scn::detail::borrowed_tail_subrange_t<decltype(input)&>>,
        double>>;
    static_assert(std::is_same_v<decltype(result2), expected_result2>);
    static_assert(std::is_same_v<expected_result::value_type::source_type,
                                 expected_result2::value_type::source_type>);
    ASSERT_TRUE(result2);
    EXPECT_DOUBLE_EQ(result2->value(), 3.14);
}

TEST(SourceTest, SourceIsInputRangeRvalue)
{
    auto source = std::string{"123 3.14"};
    auto input = scn::ranges::views::to_input(std::move(source));
    static_assert(!scn::ranges::borrowed_range<decltype(input)>);
    auto result = scn::scan<int, double>(std::move(input), "{} {}");
    static_assert(
        std::is_same_v<decltype(result),
                       scan_result_helper<scn::ranges::dangling, int, double>>);
    ASSERT_TRUE(result);
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}
