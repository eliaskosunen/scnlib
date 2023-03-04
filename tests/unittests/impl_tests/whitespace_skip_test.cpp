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

#include <scn/impl/reader/common.h>

namespace {
    std::ptrdiff_t PositionOfFirstNonSpace(std::string_view src)
    {
        auto it = scn::impl::skip_classic_whitespace(src, true);
        return scn::impl::range_nocopy_data(*it) - src.data();
    }
}  // namespace

TEST(WhitespaceSkipTest, AllSpace)
{
    EXPECT_EQ(PositionOfFirstNonSpace("    "), 4);
    EXPECT_EQ(PositionOfFirstNonSpace(" \n\t "), 4);

    EXPECT_EQ(PositionOfFirstNonSpace("        "), 8);
    EXPECT_EQ(PositionOfFirstNonSpace("  \n\t\r\v  "), 8);

    EXPECT_EQ(PositionOfFirstNonSpace("            "), 12);
    EXPECT_EQ(PositionOfFirstNonSpace("    \n\t\r\v    "), 12);
}

TEST(WhitespaceSkipTest, NoSpace)
{
    EXPECT_EQ(PositionOfFirstNonSpace("123 "), 0);
    EXPECT_EQ(PositionOfFirstNonSpace("123     "), 0);
    EXPECT_EQ(PositionOfFirstNonSpace("123          "), 0);
}

TEST(WhitespaceSkipTest, NonSpaceAtEnd)
{
    EXPECT_EQ(PositionOfFirstNonSpace("    a"), 4);
    EXPECT_EQ(PositionOfFirstNonSpace(" \n  a"), 4);

    EXPECT_EQ(PositionOfFirstNonSpace("        a"), 8);
    EXPECT_EQ(PositionOfFirstNonSpace(" \n      a"), 8);

    EXPECT_EQ(PositionOfFirstNonSpace("            a"), 12);
    EXPECT_EQ(PositionOfFirstNonSpace(" \n          a"), 12);
}

TEST(WhitespaceSkipTest, SpecialValues)
{
    EXPECT_EQ(PositionOfFirstNonSpace("\x80\x80\x80\x80"), 0);
    EXPECT_EQ(PositionOfFirstNonSpace("\x80\x80\x80\x80\x80\x80\x80\x80"), 0);
    EXPECT_EQ(PositionOfFirstNonSpace(
                  "\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"),
              0);

    EXPECT_EQ(PositionOfFirstNonSpace("\xff\xff\xff\xff"), 0);
    EXPECT_EQ(PositionOfFirstNonSpace("\xff\xff\xff\xff\xff\xff\xff\xff"), 0);
    EXPECT_EQ(PositionOfFirstNonSpace(
                  "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"),
              0);
}
