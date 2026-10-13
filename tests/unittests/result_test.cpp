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
#include <forward_list>
#include <vector>

using ::testing::Test;

template <typename It, typename S = It>
using scan_result_for =
    scn::scan_expected<scn::scan_result<scn::ranges::subrange<It, S>>>;
using dangling_scan_result =
    scn::scan_expected<scn::scan_result<scn::ranges::dangling>>;

TEST(ResultTestReal, StringLvalue)
{
    auto source = std::string{"foobar"};
    auto result = scn::scan(source, "");

    static_assert(std::is_same_v<decltype(result),
                                 scan_result_for<std::string::iterator>>);
    EXPECT_STREQ(&*result->begin(), "foobar");
}
TEST(ResultTestReal, StringRvalue)
{
    auto result = scn::scan(std::string{"foobar"}, "");

    static_assert(std::is_same_v<decltype(result), dangling_scan_result>);
}
TEST(ResultTestReal, StringView)
{
    auto result = scn::scan(std::string_view{"foobar"}, "");

    static_assert(std::is_same_v<decltype(result),
                                 scan_result_for<std::string_view::iterator>>);
    EXPECT_STREQ(&*result->begin(), "foobar");
}

TEST(ResultTestReal, VectorLvalue)
{
    auto source = std::vector<char>{'a', 'b', 'c'};
    auto result = scn::scan(source, "");

    static_assert(std::is_same_v<decltype(result),
                                 scan_result_for<std::vector<char>::iterator>>);
}
TEST(ResultTestReal, VectorRvalue)
{
    auto result = scn::scan(std::vector<char>{'a', 'b', 'c'}, "");

    static_assert(std::is_same_v<decltype(result), dangling_scan_result>);
}

TEST(ResultTestReal, DequeLvalue)
{
    auto source = std::deque<char>{'a', 'b', 'c'};
    auto result = scn::scan(source, "");

    static_assert(std::is_same_v<decltype(result),
                                 scan_result_for<std::deque<char>::iterator>>);
}
TEST(ResultTestReal, DequeRvalue)
{
    auto result = scn::scan(std::deque<char>{'a', 'b', 'c'}, "");

    static_assert(std::is_same_v<decltype(result), dangling_scan_result>);
}

TEST(ResultTestReal, ForwardListLvalue)
{
    auto source = std::forward_list<char>{'a', 'b', 'c'};
    auto result = scn::scan(source, "");

    static_assert(
        std::is_same_v<decltype(result),
                       scan_result_for<std::forward_list<char>::iterator>>);
}
TEST(ResultTestReal, ForwardListRvalue)
{
    auto result = scn::scan(std::forward_list<char>{'a', 'b', 'c'}, "");

    static_assert(std::is_same_v<decltype(result), dangling_scan_result>);
}

TEST(ResultTest, Destructuring)
{
    auto result = scn::scan<int>("42", "{}");
    ASSERT_THAT(result, Succeeded());
    std::tuple<int> values{};
    values = result->values();
    EXPECT_EQ(std::get<0>(values), 42);
    EXPECT_THAT(result->range(), IsEmptyRange());
}
TEST(ResultTest, TuplePassthrough)
{
    std::tuple<int> values;
    auto result = scn::scan<int>("42", "{}", std::move(values));
    ASSERT_THAT(result, Succeeded());
    auto [value] = result->values();
    EXPECT_EQ(value, 42);
    EXPECT_THAT(result->range(), IsEmptyRange());
}
TEST(ResultTest, TuplePassthroughWithImplicitTypes)
{
    std::tuple<int> values;
    auto result = scn::scan("42", "{}", std::move(values));
    ASSERT_THAT(result, Succeeded());
    auto [value] = result->values();
    EXPECT_EQ(value, 42);
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(ResultTest, HasValueCheck)
{
    auto success = scn::scan<int>("42", "{}");
    ASSERT_THAT(success, Succeeded());
    EXPECT_TRUE(success.has_value());

    auto failure = scn::scan<int>("not a number", "{}");
    ASSERT_THAT(failure, FailedWith(scn::scan_error::invalid_scanned_value));
    EXPECT_FALSE(failure.has_value());
}
