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
#include <scn/xchar.h>

TEST(CharTest, CharFromNarrow)
{
    auto result = scn::scan<char>("abc", "{}");
    ASSERT_TRUE(result);
    auto [ch] = result->values();
    EXPECT_EQ(ch, 'a');
}
TEST(CharTest, CharFromWide)
{
    static_assert(!scn::detail::is_scannable<char, wchar_t>::value);
}

TEST(CharTest, WcharFromNarrow)
{
    auto result = scn::scan<wchar_t>("abc", "{}");
    ASSERT_TRUE(result);
    auto [ch] = result->values();
    EXPECT_EQ(ch, L'a');
}
TEST(CharTest, WcharFromWide)
{
    auto result = scn::scan<wchar_t>(L"abc", L"{}");
    EXPECT_TRUE(result);
    auto [ch] = result->values();
    EXPECT_EQ(ch, L'a');
}
