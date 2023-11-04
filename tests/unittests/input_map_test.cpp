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

TEST(InputMapTest, StringView)
{
    std::string_view input{"FooBar"};
    auto result = scn::detail::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, Span)
{
    auto str = "FooBar";
    scn::span<const char> input{str, std::strlen(str)};
    auto result = scn::detail::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, String)
{
    std::string input{"FooBar"};
    auto result = scn::detail::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, StringLiteral)
{
    auto result = scn::detail::scan_map_input_range("FooBar");
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, SubrangeOfString)
{
    std::string source{"abc"};
    auto subr = scn::ranges::subrange{source};
    auto result = scn::detail::scan_map_input_range(subr);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, SubrangeOfStringView)
{
    std::string_view source{"abc"};
    auto subr = scn::ranges::subrange{source};
    auto result = scn::detail::scan_map_input_range(subr);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

#if SCN_HAS_STD_SPAN
TEST(InputMapTest, StdSpan)
{
    std::string source{"abc"};
    std::span<const char> s{source.data(), source.size()};
    auto result = scn::detail::scan_map_input_range(s);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, StdSpanSubrange)
{
    std::string source{"abc"};
    std::span<const char> s{source.data(), source.size()};
    auto subr = scn::ranges::subrange{s};
    auto result = scn::detail::scan_map_input_range(s);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}
#endif

TEST(InputMapTest, IstreambufView)
{
    std::istringstream iss{"abc"};
    auto view = scn::istreambuf_view{iss};
    auto result = scn::detail::scan_map_input_range(view);
    static_assert(std::is_same_v<decltype(result), scn::istreambuf_subrange>);
}

TEST(InputMapTest, IstreambufSubrange)
{
    std::istringstream iss{"abc"};
    auto view = scn::istreambuf_view{iss};
    auto result =
        scn::detail::scan_map_input_range(scn::istreambuf_subrange{view});
    static_assert(std::is_same_v<decltype(result), scn::istreambuf_subrange>);
}

TEST(InputMapTest, SubrangeOfIstreambufView)
{
    std::istringstream iss{"abc"};
    auto view = scn::istreambuf_view{iss};
    auto subr = scn::ranges::subrange{view};
    auto result = scn::detail::scan_map_input_range(subr);
    static_assert(std::is_same_v<decltype(result), scn::istreambuf_subrange>);
}

TEST(InputMapTest, Vector)
{
    std::vector<char> input{'a', 'b', 'c'};
    auto result = scn::detail::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, SubrangeOfVector)
{
    std::vector<char> input{'a', 'b', 'c'};
    auto subr = scn::ranges::subrange{input};
    auto result = scn::detail::scan_map_input_range(subr);
    static_assert(std::is_same_v<decltype(result), std::string_view>);
}

TEST(InputMapTest, Deque)
{
    std::deque<char> input{'a', 'b', 'c'};
    auto result = scn::detail::scan_map_input_range(input);
    static_assert(std::is_same_v<decltype(result), scn::erased_range>);
}

TEST(InputMapTest, SubrangeOfDeque)
{
    std::deque<char> input{'a', 'b', 'c'};
    auto subr = scn::ranges::subrange{input};
    auto result = scn::detail::scan_map_input_range(subr);
    static_assert(std::is_same_v<decltype(result), scn::erased_range>);
}

TEST(InputMapTest, ErasedRange)
{
    std::deque<char> input{'a', 'b', 'c'};
    auto range = scn::erase_range(input);
    auto result = scn::detail::scan_map_input_range(range);
    static_assert(std::is_same_v<decltype(result), scn::erased_subrange>);
}

TEST(InputMapTest, ErasedSubrange)
{
    std::deque<char> input{'a', 'b', 'c'};
    auto range = scn::erase_range(input);
    auto subr = scn::erased_subrange{range};
    auto result = scn::detail::scan_map_input_range(subr);
    static_assert(std::is_same_v<decltype(result), scn::erased_subrange>);
}

TEST(InputMapTest, SubrangeOfErased)
{
    std::deque<char> input{'a', 'b', 'c'};
    auto range = scn::erase_range(input);
    auto subr = scn::ranges::subrange{range};
    auto result = scn::detail::scan_map_input_range(subr);
    static_assert(std::is_same_v<decltype(result), scn::erased_subrange>);
}
