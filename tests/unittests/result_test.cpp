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

#include <deque>
#include <forward_list>
#include <vector>

using ::testing::Test;

using scan_context = scn::basic_scan_context<std::string_view, char>;
using scan_args = scn::scan_args_for<std::string_view, char>;

namespace {
    scn::vscan_result<std::string_view> mock_vscan(std::string_view input,
                                                   scan_args)
    {
        return {input, {}};
    }

    scn::vscan_result<scn::istreambuf_subrange> mock_vscan(
        scn::istreambuf_subrange input,
        scan_args)
    {
        return {input, {}};
    }
}  // namespace

TEST(ResultTestMocked, StringView)
{
    auto source = std::string_view{"FooBar"};
    auto input = scn::scan_map_input_range(source);
    auto args = scn::make_scan_args<scan_context, int, double>();
    auto leftovers = mock_vscan(input, args);
    auto result =
        scn::make_scan_result(source, SCN_MOVE(leftovers), SCN_MOVE(args));

    static_assert(
        std::is_same_v<decltype(result),
                       scn::scan_result_tuple<std::string_view, int, double>>);

    {
        auto [r, i, d] = std::move(result);
        EXPECT_EQ(r.range(), "FooBar");
        EXPECT_EQ(i, 0);
        EXPECT_DOUBLE_EQ(i, 0.0);
    }
}

TEST(ResultTestMocked, IstreamRange)
{
    auto ss = std::istringstream{"FooBar"};
    auto source = scn::istreambuf_view{ss};
    auto input = scn::scan_map_input_range(source);
    auto args = scn::make_scan_args<scan_context, int, double>();
    auto leftovers = mock_vscan(input, args);
    auto result =
        scn::make_scan_result(source, SCN_MOVE(leftovers), SCN_MOVE(args));

    static_assert(
        std::is_same_v<
            decltype(result),
            scn::scan_result_tuple<scn::istreambuf_subrange, int, double>>);
}

TEST(ResultTestReal, StringLvalue)
{
    auto source = std::string{"foobar"};
    auto [result] = scn::scan(source, "");

    static_assert(
        std::is_same_v<decltype(result), scn::scan_result<std::string_view>>);
    EXPECT_EQ(result.range(), "foobar");
}
TEST(ResultTestReal, StringRvalue)
{
    auto [result] = scn::scan(std::string{"foobar"}, "");

    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_result<scn::ranges::dangling>>);
}
TEST(ResultTestReal, StringView)
{
    auto [result] = scn::scan(std::string_view{"foobar"}, "");

    static_assert(
        std::is_same_v<decltype(result), scn::scan_result<std::string_view>>);
    EXPECT_EQ(result.range(), "foobar");
}

TEST(ResultTestReal, VectorLvalue)
{
    auto source = std::vector<char>{'a', 'b', 'c'};
    auto [result] = scn::scan(source, "");

    static_assert(
        std::is_same_v<decltype(result), scn::scan_result<std::string_view>>);
    EXPECT_EQ(result.range(), "abc");
}
TEST(ResultTestReal, VectorRvalue)
{
    auto [result] = scn::scan(std::vector<char>{'a', 'b', 'c'}, "");

    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_result<scn::ranges::dangling>>);
}

TEST(ResultTestReal, DequeLvalue)
{
    auto source = std::deque<char>{'a', 'b', 'c'};
    auto [result] = scn::scan(source, "");

    using iterator = scn::ranges::iterator_t<const std::deque<char>&>;
    using sentinel = scn::ranges::sentinel_t<const std::deque<char>&>;
    static_assert(std::is_same_v<
                  decltype(result),
                  scn::scan_result<scn::ranges::subrange<iterator, sentinel>>>);
    EXPECT_EQ(result.range().size(), 3);
}
TEST(ResultTestReal, DequeRvalue)
{
    auto [result] = scn::scan(std::deque<char>{'a', 'b', 'c'}, "");

    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_result<scn::ranges::dangling>>);
}

TEST(ResultTestReal, ForwardListLvalue)
{
    auto source = std::forward_list<char>{'a', 'b', 'c'};
    auto [result] = scn::scan(source, "");

    using iterator = scn::ranges::iterator_t<const std::forward_list<char>&>;
    using sentinel = scn::ranges::sentinel_t<const std::forward_list<char>&>;
    static_assert(std::is_same_v<
                  decltype(result),
                  scn::scan_result<scn::ranges::subrange<iterator, sentinel>>>);
    EXPECT_EQ(
        scn::ranges::distance(result.range().begin(), result.range().end()), 3);
}
TEST(ResultTestReal, ForwardListRvalue)
{
    auto [result] = scn::scan(std::forward_list<char>{'a', 'b', 'c'}, "");

    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_result<scn::ranges::dangling>>);
}

TEST(ResultTestReal, ErasedRangeLvalue)
{
    auto source = scn::erased_range{std::string_view{"foobar"}};
    auto [result] = scn::scan(source, "");

    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_result<scn::erased_subrange>>);
    EXPECT_EQ(
        scn::ranges::distance(result.range().begin(), result.range().end()), 6);
}
TEST(ResultTestReal, ErasedRangeRvalue)
{
    auto [result] = scn::scan(scn::erased_range{"foobar"}, "");

    static_assert(std::is_same_v<decltype(result),
                                 scn::scan_result<scn::ranges::dangling>>);
}
