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

#include <scn/detail/input_map.h>
#include <scn/util/span.h>

#include <deque>

#if SCN_HAS_STD_SPAN
#include <span>
#endif

using ::testing::Test;

using namespace std::string_view_literals;

namespace {
    template <typename Range>
    std::string collect(Range r)
    {
        std::string str;
        for (auto it = scn::ranges::begin(r); it != scn::ranges::end(r); ++it) {
            str.push_back(*it);
        }
        return str;
    }
}  // namespace

TEST(InputMapTest, RefBuffer)
{
    auto first = scn::detail::make_string_scan_buffer("foobar"sv);
    auto second = scn::detail::make_scan_buffer(first.get());
    static_assert(std::is_same_v<decltype(second),
                                 scn::detail::basic_scan_ref_buffer<char>>);
    EXPECT_EQ(collect(second.get()), "foobar");
}

#if 0
TEST(InputMapTest, StringView)
{
    auto buf = scn::detail::make_scan_buffer("foobar"sv);
    static_assert(std::is_same_v<decltype(buf),
                                 scn::detail::basic_scan_string_buffer<char>>);
    EXPECT_EQ(collect(buf.get()), "foobar");
}

TEST(InputMapTest, StringLiteral)
{
    auto buf = scn::detail::make_scan_buffer("foobar");
    static_assert(std::is_same_v<decltype(buf),
                                 scn::detail::basic_scan_string_buffer<char>>);
    EXPECT_EQ(collect(buf.get()), "foobar");
}

TEST(InputMapTest, StdString)
{
    auto str = std::string{"foobar"};
    auto buf = scn::detail::make_scan_buffer(str);
    static_assert(std::is_same_v<decltype(buf),
                                 scn::detail::basic_scan_string_buffer<char>>);
    EXPECT_EQ(collect(buf.get()), "foobar");
}

TEST(InputMapTest, Span)
{
    auto str = "foobar"sv;
    auto buf = scn::detail::make_scan_buffer(
        scn::span<const char>{str.data(), str.size()});
    static_assert(std::is_same_v<decltype(buf),
                                 scn::detail::basic_scan_string_buffer<char>>);
    EXPECT_EQ(collect(buf.get()), "foobar");
}

TEST(InputMapTest, StringViewTake)
{
    auto buf =
        scn::detail::make_scan_buffer(scn::ranges::take_view("foobar"sv, 3));
    static_assert(std::is_same_v<decltype(buf),
                                 scn::detail::basic_scan_string_buffer<char>>);
    EXPECT_EQ(collect(buf.get()), "foo");
}
#endif

TEST(InputMapTest, ReversedStringView)
{
    auto buf =
        scn::detail::make_scan_buffer(scn::ranges::reverse_view("foobar"sv));
    static_assert(
        std::is_same_v<decltype(buf),
                       scn::detail::basic_scan_forward_buffer_impl<
                           scn::ranges::reverse_view<std::string_view>>>);
    EXPECT_EQ(collect(buf.get()), "raboof");
}

TEST(InputMapTest, Deque)
{
    auto str = std::deque<char>{'f', 'o', 'o', 'b', 'a', 'r'};
    auto buf = scn::detail::make_scan_buffer(str);
    static_assert(
        std::is_same_v<decltype(buf),
                       scn::detail::basic_scan_forward_buffer_impl<
                           scn::ranges::ref_view<const std::deque<char>>>>);
    EXPECT_EQ(collect(buf.get()), "foobar");
}

TEST(InputMapTest, File)
{
    auto buf = scn::detail::make_scan_buffer(stdin);
    static_assert(std::is_same_v<decltype(buf), scn::detail::scan_file_buffer>);
}
