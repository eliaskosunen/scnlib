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

#include <scn/detail/context.h>

TEST(ContextTest, MemberTypes)
{
    using scan_context = scn::basic_scan_context<std::string_view, char>;

    static_assert(std::is_same_v<scan_context::char_type, char>);
    static_assert(
        std::is_same_v<scan_context::range_type, std::string_view>);
    static_assert(std::is_same_v<scan_context::iterator,
                                 std::string_view::iterator>);
    static_assert(std::is_same_v<scan_context::sentinel,
                                 std::string_view::iterator>);
    static_assert(std::is_same_v<scan_context::parse_context_type,
                                 scn::basic_scan_parse_context<char>>);
    static_assert(std::is_same_v<scan_context::arg_type,
                                 scn::basic_scan_arg<scan_context>>);
    static_assert(std::is_same_v<scan_context::scanner_type<int>,
                                 scn::scanner<int, char>>);
}

TEST(ContextTest, Range) {
    SUCCEED();
}
