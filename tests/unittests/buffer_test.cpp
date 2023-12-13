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

#include <scn/detail/scan_buffer.h>

using namespace std::string_view_literals;

template <typename Range>
std::string collect(Range r)
{
    std::string str;
    for (auto it = scn::ranges::begin(r); it != scn::ranges::end(r); ++it) {
        str.push_back(*it);
    }
    return str;
}

TEST(ScanBufferTest, StringView)
{
    auto buf = scn::detail::make_string_scan_buffer("foobar"sv);
    static_assert(std::is_same_v<decltype(buf),
                                 scn::detail::basic_scan_string_buffer<char>>);

    EXPECT_TRUE(buf.is_contiguous());
    EXPECT_EQ(buf.chars_available(), 6);
    EXPECT_EQ(collect(buf.get()), "foobar");
    EXPECT_EQ(buf.get_contiguous(), "foobar");
}

TEST(ScanBufferTest, TakeStringView)
{
    auto range = scn::ranges::take_view("foobar"sv, 3);
    auto buf = scn::detail::make_forward_scan_buffer(range);
    static_assert(
        std::is_same_v<
            decltype(buf),
            scn::detail::basic_scan_forward_buffer_impl<decltype(range)>>);

    EXPECT_FALSE(buf.is_contiguous());
    EXPECT_EQ(buf.chars_available(), 0);

    auto view = buf.get();
    auto it = view.begin();
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, 'f');
    ++it;
    EXPECT_NE(it, view.end());
    EXPECT_EQ(*it, 'o');
    ++it;
    EXPECT_NE(it, view.end());

    EXPECT_EQ(collect(buf.get()), "foo");
    EXPECT_EQ(buf.chars_available(), 3);
    EXPECT_EQ(collect(buf.get()), "foo");
}

TEST(ScanBufferTest, ReverseStringView)
{
    auto range = scn::ranges::reverse_view("foobar"sv);
    auto buf = scn::detail::make_forward_scan_buffer(range);
    static_assert(
        std::is_same_v<
            decltype(buf),
            scn::detail::basic_scan_forward_buffer_impl<decltype(range)>>);

    EXPECT_FALSE(buf.is_contiguous());
    EXPECT_EQ(buf.chars_available(), 0);

    EXPECT_EQ(collect(buf.get()), "raboof");
    EXPECT_EQ(buf.chars_available(), 6);
}
