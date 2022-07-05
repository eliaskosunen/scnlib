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

#include <scn/detail/input_map.h>

#include <deque>

using ::testing::Test;

TEST(InputMapTest, StringView)
{
    std::string_view input{"FooBar"};
    auto result = scn::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, Span)
{
    auto str = "FooBar";
    scn::span<const char> input{str, std::strlen(str)};
    auto result = scn::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, String)
{
    std::string input{"FooBar"};
    auto result = scn::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, StringLiteral)
{
    auto result = scn::scan_map_input_range("FooBar");
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, IstreambufView)
{
    std::istringstream iss{"abc"};
    auto view = scn::istreambuf_view{iss};
    auto result = scn::scan_map_input_range(view);
    static_assert(std::is_same_v<decltype(result), scn::istreambuf_subrange>);
}

TEST(InputMapTest, IstreambufSubrange)
{
    std::istringstream iss{"abc"};
    auto view = scn::istreambuf_view{iss};
    auto result = scn::scan_map_input_range(scn::istreambuf_subrange{view});
    static_assert(std::is_same_v<decltype(result), scn::istreambuf_subrange>);
}

TEST(InputMapTest, Vector)
{
    std::vector<char> input{'a', 'b', 'c'};
    auto result = scn::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, Deque)
{
    std::deque<char> input{'a', 'b', 'c'};
    auto result = scn::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), scn::erased_range>);
}

TEST(InputMapTest, ErasedRange)
{
    std::deque<char> input{'a', 'b', 'c'};
    auto range = scn::erase_range(input);
    auto result = scn::scan_map_input_range(range);
    static_assert(std::is_same_v<decltype(result), scn::erased_subrange>);
}
