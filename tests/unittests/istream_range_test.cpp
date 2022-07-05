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
#include <sstream>

static_assert(scn::ranges::bidirectional_range<scn::istreambuf_view>);
static_assert(scn::ranges::bidirectional_range<scn::istreambuf_subrange>);

static_assert(scn::ranges::view<scn::istreambuf_view>);
static_assert(scn::ranges::view<scn::istreambuf_subrange>);

TEST(IstreamRangeTest, StringStreamForward)
{
    auto ss = std::istringstream{"abc"};
    auto range = scn::istreambuf_view{ss};
    auto view = scn::istreambuf_subrange{range};

    std::string dest;
    for (auto ch : view) {
        dest.push_back(ch);
    }
    EXPECT_STREQ(dest.c_str(), "abc");

    EXPECT_NE(scn::ranges::begin(view), scn::ranges::end(view));
}

TEST(IstreamRangeTest, StringStreamBidir)
{
    auto ss = std::istringstream{"abc"};
    auto ssr = scn::istreambuf_view{ss};
    auto range = scn::istreambuf_subrange{ssr};

    auto it = range.begin();
    EXPECT_EQ(*it, 'a');

    ++it;
    EXPECT_EQ(*it, 'b');

    --it;
    EXPECT_EQ(*it, 'a');

    ++it;
    EXPECT_EQ(*it, 'b');

    ++it;
    EXPECT_EQ(*it, 'c');

    ++it;
    EXPECT_EQ(it, range.end());

    EXPECT_EQ(*range.begin(), 'a');
}

TEST(IstreamRangeTest, CopyingSubrange)
{
    auto ss = std::istringstream{"abc"};
    auto ssr = scn::istreambuf_view{ss};
    auto view = scn::istreambuf_subrange{ssr};

    auto it = view.begin();
    auto other = view;

    EXPECT_EQ(*it, 'a');
    ++it;

    EXPECT_NE(it, view.end());
    EXPECT_EQ(*other.begin(), 'a');
    EXPECT_NE(other.begin(), other.end());
}

TEST(IstreamRangeTest, ReadFromStreamAfterScanAndSync)
{
    auto ss = std::istringstream{"123 456"};
    auto view = scn::istreambuf_view{ss};

    {
        auto [result, i] = scn::scan<int>(view, "{}");
        EXPECT_TRUE(result);
        EXPECT_EQ(i, 123);
        view.sync(result.range().begin());
    }

    {
        int j{};
        EXPECT_TRUE(ss >> j);
        EXPECT_EQ(j, 456);
    }
}

TEST(IstreamRangeTest, ReadWithScanAfterReadFromStraem)
{
    auto ss = std::istringstream{"123 456"};
    auto view = scn::istreambuf_view{ss};

    {
        int j{};
        EXPECT_TRUE(ss >> j);
        EXPECT_EQ(j, 123);
    }

    {
        auto [result, i] = scn::scan<int>(view, "{}");
        EXPECT_TRUE(result);
        EXPECT_EQ(i, 456);
        view.sync(result.range().begin());
    }
}

TEST(IstreamRangeTest, ReadFromStreamAfterFailureWithScan)
{
    auto ss = std::istringstream{"foo 456"};
    auto view = scn::istreambuf_view{ss};

    {
        auto [result, i] = scn::scan<int>(view, "{}");
        EXPECT_FALSE(result);
        view.sync(result.range().begin());
    }

    {
        std::string s{};
        EXPECT_TRUE(ss >> s);
        EXPECT_EQ(s, "foo");

        int j{};
        EXPECT_TRUE(ss >> j);
        EXPECT_EQ(j, 456);
    }
}

TEST(IstreamRangeTest, ReadWithScanAfterFailed)
{
    auto ss = std::istringstream{"foo 456"};
    auto view = scn::istreambuf_view{ss};

    {
        int j{};
        EXPECT_FALSE(ss >> j);
    }

    {
        auto [result, str] = scn::scan<std::string>(view, "{}");
        EXPECT_TRUE(result);
        EXPECT_EQ(str, "foo");
        view.sync(result.range().begin());

        auto [result2, i] = scn::scan<int>(result.range(), "{}");
        EXPECT_TRUE(result2);
        EXPECT_EQ(i, 456);
        view.sync(result2.range().begin());
    }
}
