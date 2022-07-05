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

#include <gmock/gmock.h>

#include <map>
#include <set>
#include <vector>

#include <scn/detail/scan.h>
#include <scn/detail/scanner_range.h>

template <typename>
struct debug;

TEST(RangesTest, VectorSequence)
{
    static_assert(scn::range_format_kind<std::vector<int>, char>::value ==
                  scn::range_format::sequence);

    auto [result, vec] = scn::scan<std::vector<int>>("[123, 456]", "{}");
    EXPECT_TRUE(result);
    EXPECT_THAT(vec, testing::ElementsAre(123, 456));
}

TEST(RangesTest, Set)
{
    static_assert(scn::range_format_kind<std::set<int>, char>::value ==
                  scn::range_format::set);

    auto [result, set] = scn::scan<std::set<int>>("{123, 456}", "{}");
    EXPECT_TRUE(result);
    EXPECT_THAT(set, testing::ElementsAre(123, 456));
}

TEST(RangesTest, Map)
{
    static_assert(scn::range_format_kind<std::map<int, int>, char>::value ==
                  scn::range_format::map);

    auto [result, map] =
        scn::scan<std::map<int, int>>("{12: 34, 56: 78}", "{}");
    EXPECT_TRUE(result);
    EXPECT_THAT(map,
                testing::ElementsAre(std::pair{12, 34}, std::pair{56, 78}));
}
