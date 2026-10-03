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

// Every public header, to test that they can be included together.
// Their order is not significant.
#include <scn/chrono.h>
#include <scn/fwd.h>
#include <scn/istream.h>
#include <scn/macros.h>
#include <scn/ranges.h>
#include <scn/regex.h>
#include <scn/scan.h>
#include <scn/xchar.h>

#include <vector>

TEST(AllHeadersTest, Range)
{
    auto result = scn::scan<std::vector<int>>("[123, 456]", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().size(), 2u);
}

#if !SCN_DISABLE_REGEX
static_assert(scn::range_format_kind<scn::regex_matches, char>::value ==
              scn::range_format::disabled);
static_assert(scn::range_format_kind<scn::wregex_matches, wchar_t>::value ==
              scn::range_format::disabled);

TEST(AllHeadersTest, RegexMatchesWithRanges)
{
    auto result = scn::scan<scn::regex_matches>("abc", "{:/([a-z]+)/}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().size(), 2u);
}
#endif
