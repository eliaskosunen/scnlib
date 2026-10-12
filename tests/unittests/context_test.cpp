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

#include <string_view>

TEST(ParseContextTest, AdvanceTo)
{
    const char storage[] = "abcdef";
    scn::scan_parse_context ctx{scn::source_tag<std::string_view>,
                                std::string_view{storage + 1, 3}};

    ctx.advance_to(ctx.begin() + 1);
    EXPECT_EQ(ctx.begin(), storage + 2);
    EXPECT_EQ(ctx.end(), storage + 4);

    ctx.advance_to(ctx.end());
    EXPECT_EQ(ctx.begin(), ctx.end());
    EXPECT_TRUE(ctx.get_error());
}

TEST(ParseContextTest, AdvanceToOutOfRange)
{
    const char storage[] = "abcdef";
    scn::scan_parse_context ctx{scn::source_tag<std::string_view>,
                                std::string_view{storage + 1, 3}};

    ctx.advance_to(storage + 5);
    EXPECT_EQ(ctx.begin(), storage + 1);
    EXPECT_FALSE(ctx.get_error());

    scn::scan_parse_context before{scn::source_tag<std::string_view>,
                                   std::string_view{storage + 1, 3}};
    before.advance_to(storage);
    EXPECT_EQ(before.begin(), storage + 1);
    EXPECT_FALSE(before.get_error());
}
