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

#include "../wrapped_gtest.h"

#include <scn/impl/algorithms/find_whitespace.h>

using namespace std::string_view_literals;

TEST(FindClassicSpaceNarrowFastTest, ShortInput)
{
    auto src = "foo bar"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.begin() + 3);
}
TEST(FindClassicSpaceNarrowFastTest, ShortInputWithNoSpaces)
{
    auto src = "foobar"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.end());
}
TEST(FindClassicSpaceNarrowFastTest, LongerInput)
{
    auto src = "foobarbazhelloworld123 foo"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.end() - 4);
}
TEST(FindClassicSpaceNarrowFastTest, MultipleSpaces)
{
    auto src = "foo bar baz"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.begin() + 3);
}
