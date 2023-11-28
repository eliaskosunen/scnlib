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

#include <scn/scan.h>

TEST(CharacterSet, Simple)
{
    auto loc = std::locale{"fi_FI.UTF-8"};
    auto r = scn::scan<std::string>(loc, "abc123", "{:L[a-z]}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "abc");
    EXPECT_STREQ(r->range().data(), "123");
}

TEST(CharacterSet, Alpha)
{
    auto loc = std::locale{"fi_FI.UTF-8"};
    auto r = scn::scan<std::string>(loc, "abc123", "{:L[:alpha:]}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "abc");
    EXPECT_STREQ(r->range().data(), "123");
}

TEST(CharacterSet, Word)
{
    auto loc = std::locale{"fi_FI.UTF-8"};
    auto r = scn::scan<std::string>(loc, "abc_123 ", "{:L[\\w]}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "abc_123");
    EXPECT_STREQ(r->range().data(), " ");
}

TEST(CharacterSet, InvertedWord)
{
    auto loc = std::locale{"fi_FI.UTF-8"};
    auto r = scn::scan<std::string>(loc, " abc_123", "{:L[\\W]}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), " ");
    EXPECT_STREQ(r->range().data(), "abc_123");
}
TEST(CharacterSet, InvertedWord2)
{
    auto loc = std::locale{"fi_FI.UTF-8"};
    auto r = scn::scan<std::string>(loc, " abc_123", "{:L[^\\w]}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), " ");
    EXPECT_STREQ(r->range().data(), "abc_123");
}
TEST(CharacterSet, DoubleInvertedWord)
{
    auto loc = std::locale{"fi_FI.UTF-8"};
    auto r = scn::scan<std::string>(loc, "abc_123 ", "{:L[^\\W]}");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->value(), "abc_123");
    EXPECT_STREQ(r->range().data(), " ");
}
