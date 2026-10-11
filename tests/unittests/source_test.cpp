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

#include "test_common.h"

#include <scn/scan.h>

#include <deque>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#if SCN_STD >= SCN_STD_20
#include <ranges>
#endif

using ::testing::Test;

static_assert(scn::detail::is_scannable_source<std::string>);
static_assert(scn::detail::is_scannable_source<std::string_view>);
static_assert(scn::detail::is_scannable_source<scn::scan_file>);
static_assert(!scn::detail::is_scannable_source<int>);
static_assert(!scn::detail::is_scannable_source<std::vector<int>>);

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
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());
    EXPECT_EQ(std::get<0>(r->values()), 123);
}
TEST(SourceTest, TwoArgs)
{
    auto r = scn::scan<int, double>("123 3.14", "{} {}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());
    auto [i, d] = r->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsStringLiteral)
{
    auto result = scn::scan<int, double>("123 3.14", "{} {}");
    static_assert(std::is_same_v<decltype(result),
                                 scan_result_helper<const char*, int, double>>);
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsCharArrayWithoutNullTerminator)
{
    const char source[3] = {'1', '2', '3'};
    auto result = scn::scan<int>(source, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);
    EXPECT_EQ(result->begin(), source + 3);
    EXPECT_EQ(result->end(), source + 3);
}

TEST(SourceTest, ResultRvalueAccessors)
{
    const auto result = scn::scan<int>(std::string_view{"123 4"}, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*std::move(*result).begin(), ' ');
    EXPECT_EQ(std::move(*result).range().size(), 2u);
}

TEST(SourceTest, SourceIsStringView)
{
    auto result = scn::scan<int, double>(std::string_view{"123 3.14"}, "{} {}");
    static_assert(std::is_same_v<
                  decltype(result),
                  scan_result_helper<std::string_view::iterator, int, double>>);
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
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
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
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
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(SourceTest, SourceIsSizedBidirectionalRange)
{
    // Longer than the 16-character chunk of the sized range buffer
    const auto str = std::string_view{"123 3.14 abcdefghijklmnopqrstuvwxyz"};
    auto source = std::list<char>(str.begin(), str.end());
    auto result = scn::scan<int, double, std::string>(source, "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    auto [i, d, s] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
    EXPECT_EQ(s, "abcdefghijklmnopqrstuvwxyz");
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
    ASSERT_THAT(result, Succeeded());
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
    ASSERT_THAT(result2, Succeeded());
    EXPECT_DOUBLE_EQ(result2->value(), 3.14);
}

TEST(SourceTest, SourceIsInputRangeResultBegin)
{
    auto source = std::string_view{"123 456"};
    auto input = scn::ranges::views::to_input(source);
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    auto it = result->begin();
    EXPECT_EQ(*it, ' ');
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
    ASSERT_THAT(result, Succeeded());
    auto [i, d] = result->values();
    EXPECT_EQ(i, 123);
    EXPECT_DOUBLE_EQ(d, 3.14);
}

// Clang before 16 can't compile the views of libstdc++'s <ranges>,
// even though __cpp_lib_ranges is defined
#if SCN_STD >= SCN_STD_20 && defined(__cpp_lib_ranges) && \
    __cpp_lib_ranges >= 201911L &&                        \
    !(SCN_STDLIB_GLIBCXX && SCN_CLANG && SCN_CLANG < SCN_COMPILER(16, 0, 0))
// These views have an `iterator_concept`, and either no `iterator_category`,
// or a weaker one
TEST(SourceTest, SourceIsIstreamView)
{
    auto stream = std::istringstream{"123 456"};
    stream >> std::noskipws;
    // Not `std::views::istream`, which is missing before libstdc++ 12
    auto view = std::ranges::istream_view<char>(stream);
    auto result = scn::scan<int>(view, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 123);

    auto result2 = scn::scan<int>(result->range(), "{}");
    ASSERT_THAT(result2, Succeeded());
    EXPECT_EQ(result2->value(), 456);
}

TEST(SourceTest, SourceIsTransformView)
{
    auto source = std::string_view{"789 x"};
    auto view =
        source | std::views::transform([](char c) noexcept { return c; });
    static_assert(scn::ranges::random_access_range<decltype(view)>);
    auto result = scn::scan<int>(view, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 789);
    EXPECT_EQ(*result->begin(), ' ');
}

// Forward ranges that can only be iterated over when non-const
TEST(SourceTest, SourceIsMutableTransformView)
{
    auto source = std::string_view{"789 x 12"};
    // `mutable` makes `operator()` non-const
    auto mutable_transform =
        source |
        std::views::transform([](char c) mutable noexcept { return c; });
    auto result = scn::scan<int>(mutable_transform, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 789);
    EXPECT_EQ(*result->begin(), ' ');
}

TEST(SourceTest, SourceIsFilterView)
{
    auto source = std::string_view{"789 x 12"};
    auto filter =
        source | std::views::filter([](char c) noexcept { return c != 'x'; });
    auto filter_result = scn::scan<int, int>(filter, "{} {}");
    ASSERT_THAT(filter_result, Succeeded());
    EXPECT_THAT(filter_result->values(), testing::FieldsAre(789, 12));
    EXPECT_TRUE(filter_result->range().begin() == filter_result->range().end());
}
#endif
