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

#include <scn/detail/erased_range.h>
#include <scn/detail/istream_range.h>

#include <deque>

class ErasedRangeTest
    : public testing::TestWithParam<std::shared_ptr<scn::erased_range>> {
protected:
    static auto& range()
    {
        return *GetParam();
    }
};

static_assert(scn::ranges::range<scn::erased_range>);
static_assert(scn::ranges::input_range<scn::erased_range>);
static_assert(scn::ranges::forward_range<scn::erased_range>);

TEST_P(ErasedRangeTest, Test)
{
    EXPECT_EQ(range().begin(), scn::ranges::begin(range()));
    EXPECT_NE(range().begin(), scn::ranges::end(range()));

    std::string dest;
    for (auto ch : range()) {
        dest.push_back(ch);
    }
    EXPECT_EQ(dest, "abc");

    dest.clear();
    for (auto ch : range()) {
        dest.push_back(ch);
    }
    EXPECT_EQ(dest, "abc");

    // FIXME
#if 0
    dest.clear();
    for (const char& ch : scn::ranges::reverse(range())) {
        dest.push_back(ch);
    }
    EXPECT_EQ(dest, "cba");
#endif
}

INSTANTIATE_TEST_SUITE_P(
    String,
    ErasedRangeTest,
    testing::Values(std::make_shared<scn::erased_range>(std::string{"abc"})));
INSTANTIATE_TEST_SUITE_P(StringView,
                         ErasedRangeTest,
                         testing::Values(std::make_shared<scn::erased_range>(
                             std::string_view{"abc"})));
INSTANTIATE_TEST_SUITE_P(Vector,
                         ErasedRangeTest,
                         testing::Values(std::make_shared<scn::erased_range>(
                             std::vector<char>{'a', 'b', 'c'})));
INSTANTIATE_TEST_SUITE_P(Deque,
                         ErasedRangeTest,
                         testing::Values(std::make_shared<scn::erased_range>(
                             std::deque<char>{'a', 'b', 'c'})));

// FIXME
#if 0
namespace {
    std::istringstream ss{"abc"};
}  // namespace

// static_assert(scn::ranges::viewable_range<scn::istreambuf_view&>);
static_assert(scn::ranges::viewable_range<scn::istreambuf_view>);
static_assert(scn::ranges::viewable_range<scn::istreambuf_subrange&>);

INSTANTIATE_TEST_SUITE_P(StringStream,
                         ErasedRangeTest,
                         testing::Values(std::make_shared<scn::erased_range>(
                             scn::istreambuf_view{ss})));
#endif
