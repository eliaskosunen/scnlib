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

TEST(InputMapTest, Deque)
{
    auto str = std::deque<char>{'f', 'o', 'o', 'b', 'a', 'r'};
    auto buf = scn::detail::make_scan_buffer(str);
    static_assert(
        std::is_same_v<
            decltype(buf),
            scn::detail::basic_scan_sized_range_buffer<std::deque<char>>>);
    EXPECT_EQ(collect(buf.get()), "foobar");
}

TEST(InputMapTest, DequeSubrange)
{
    auto str = std::deque<char>{'f', 'o', 'o', 'b', 'a', 'r'};
    auto subr = scn::ranges::subrange{str.begin(), str.end()};
    auto buf = scn::detail::make_scan_buffer(subr);
    static_assert(
        std::is_same_v<decltype(buf),
                       scn::detail::basic_scan_sized_range_buffer<
                           scn::ranges::subrange<std::deque<char>::iterator>>>);
    EXPECT_EQ(collect(buf.get()), "foobar");
}

TEST(InputMapTest, File)
{
    scn::scan_file file{stdin};
    auto buf = scn::detail::make_scan_buffer(file);
    static_assert(std::is_same_v<decltype(buf), scn::detail::scan_file_buffer>);
}
