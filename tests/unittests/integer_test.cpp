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

#include <scn/detail/scan.h>

TEST(IntegerTest, Simple)
{
    auto [result, val] = scn::scan<int>("42", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val, 42);
}

TEST(IntegerTest, SkipPrecedingWhitespaceByDefault)
{
    auto [result, val] = scn::scan<int>(" \n42", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, SkipPrecedingWhitespaceByForce)
{
    auto [result, val] = scn::scan<int>(" \n42", " {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, SkipWhitespaceBetweenValuesByDefault)
{
    auto [result, a, b] = scn::scan<int, int>("123 456", "{}{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}
TEST(IntegerTest, SkipWhitespaceBetweenValuesByForce)
{
    auto [result, a, b] = scn::scan<int, int>("123 456", "{} {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(IntegerTest, UnsignedWithDefaultFormat)
{
    auto [result, val] = scn::scan<unsigned>("42", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, UnsignedWithDecimalFormat)
{
    auto [result, val] = scn::scan<unsigned>("42", "{:i}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, UnsignedWithUnsignedFormat)
{
    auto [result, val] = scn::scan<unsigned>("42", "{:u}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val, 42);
}

TEST(IntegerTest, Pointer)
{
    char source_buf[64]{};
    int value{42};
    std::snprintf(source_buf, 64, "%p", static_cast<const void*>(&value));
    std::string_view source{source_buf};

    auto [result, val] = scn::scan<void*>(source, "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val, &value);
    EXPECT_EQ(*static_cast<const int*>(val), value);
}

namespace {
    std::string_view get_long_input()
    {
        return "1452555457 -184798174 -1652546625 -2047874506 328160201 "
               "-1742626756 -2104166651 -361330339 -1064849314 -1140256799 "
               "-77457874 1035003058 -1608973386 -364576541 924414610 "
               "-565032411 2113656804 66526789 -520585648 1079228960 "
               "-1012538263 -791727985 -858355297 -852074158 969974399 "
               "1642644672 -1952912297 880585823 873842844 -473822935 "
               "-1816376564 -1005862253 -661864658 -1307402335 1630039865 "
               "840811653 -1586244752 1109303204 1328768838 1848854057 "
               "1406603349 -1204313777 -1703869320 -1019691744 2042313234 "
               "-810580417 -101344325 -1122229352 -104477533 -419004291 "
               "-1160309244 -1186534409 1427634555 -226701969 423863886 "
               "1406499283 -1729619223 -463219595 -1522636674 1694345924 "
               "1419806805 115071386 -445258046 -993164105 854616875 "
               "1000331309 -1311414169 1691697359 -193402913 -1427871577 "
               "1878558562 -1033215863 -325223198 -1299704348 -324671872 "
               "1752548020 -790926043 -1304924709 -851161885 29627141 "
               "-1291891913 -1965349957 677096279 -728279334 -1696288799 "
               "-1870884715 1350724467 -880882936 871236574 -767014908 "
               "-1997582959 -1568170814 -230983998 1512649082 2016579559 "
               "600570696 -1052567846 1967307875 -512726237 -1957472780 "
               "-1656353216 2108184007 1236084848 1610008127 1710656200 "
               "126598604 -148883527 -1161501624 -1090318495 -34680478 "
               "1316194429 -1705032293 1575287842 -1177882817 1065014342 "
               "416929349 -1917198405 852065756 -1412594178 -1605733035 "
               "-1956303950 610686248 713602964 1417685924 -718145659 "
               "1361788393 524810647 -756671677 496364848 2011161096 "
               "-864257237 -197094037 1330741570 -816189669 -235680849 "
               "-1523110578 1882201631 -2126884251 609616291 -1335875805 "
               "-854354418 -410917675 -236519164 -447207753 1202334876 "
               "803903497 -605856953 907537779 -365278899 2146027685 "
               "1760175337 -502436335 417469866 1214405189 554749409 "
               "1479834401 1538757135 538313906 72685284 -909183582 "
               "1439501153 ";
    }
}  // namespace

TEST(IntegerTest, LongInput)
{
    std::string_view input = get_long_input();
    auto [result, i] = scn::scan<int>(input, "{}");

    EXPECT_TRUE(result);
    EXPECT_FALSE(result.range().empty());
    EXPECT_EQ(i, 1452555457);
}
