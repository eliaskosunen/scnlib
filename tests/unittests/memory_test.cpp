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

#include <scn/util/memory.h>

TEST(ToAddressTest, Pointer)
{
    auto i = 42;
    auto p = scn::detail::to_address(&i);
    static_assert(std::is_same_v<decltype(p), int*>);
    EXPECT_EQ(p, &i);
    EXPECT_EQ(*p, 42);
}

TEST(ToAddressTest, UniquePtr)
{
    auto u = std::make_unique<int>(42);
    auto p = scn::detail::to_address(u);
    static_assert(std::is_same_v<decltype(p), int*>);
    EXPECT_EQ(p, u.get());
    EXPECT_EQ(*p, 42);
}

TEST(ToAddressTest, StringViewIterator)
{
    auto sv = std::string_view{"42"};
    auto p = scn::detail::to_address(sv.begin());
    static_assert(std::is_same_v<decltype(p), const char*>);
    EXPECT_EQ(p, sv.data());
    EXPECT_EQ(*p, '4');
}
