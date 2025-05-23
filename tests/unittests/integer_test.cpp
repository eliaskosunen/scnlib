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

#include <deque>

namespace {
template <typename... Args>
std::tuple<testing::AssertionResult, Args...> do_test(
    std::string_view src,
    scn::scan_format_string<std::string_view, Args...> fmt)
{
    auto result = scn::scan<Args...>(src, fmt);
    if (!result) {
        return {testing::AssertionFailure()
                    << "scan failed with " << result.error().code() << ": "
                    << result.error().msg(),
                Args{}...};
    }
    if (result->begin() != src.end()) {
        return {testing::AssertionFailure() << "result iterator not at end",
                Args{}...};
    }
    return std::tuple_cat(std::tuple{testing::AssertionSuccess()},
                          result->values());
}
}  // namespace

TEST(IntegerTest, Simple)
{
    auto [result, val] = do_test<int>("42", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}

TEST(IntegerTest, SkipPrecedingWhitespaceByDefault)
{
    auto [result, val] = do_test<int>(" \n42", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, SkipPrecedingWhitespaceByForce)
{
    auto [result, val] = do_test<int>(" \n42", " {}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, SkipWhitespaceBetweenValuesByDefault)
{
    auto [result, a, b] = do_test<int, int>("123 456", "{}{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}
TEST(IntegerTest, SkipWhitespaceBetweenValuesByForce)
{
    auto [result, a, b] = do_test<int, int>("123 456", "{} {}");
    EXPECT_TRUE(result);
    EXPECT_EQ(a, 123);
    EXPECT_EQ(b, 456);
}

TEST(IntegerTest, UnsignedWithDefaultFormat)
{
    auto [result, val] = do_test<unsigned>("42", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, UnsignedWithDecimalFormat)
{
    auto [result, val] = do_test<unsigned>("42", "{:i}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}
TEST(IntegerTest, UnsignedWithUnsignedFormat)
{
    auto [result, val] = do_test<unsigned>("42", "{:u}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 42);
}

TEST(IntegerTest, LeadingZeroesInDecimal)
{
    auto [result, val] = do_test<short>("0000000000000000100", "{:d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 100);
}
TEST(IntegerTest, LeadingZeroesInHexadecimalWithoutPrefix)
{
    auto [result, val] = do_test<short>("0000000000000000100", "{:x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0x100);
}
TEST(IntegerTest, LeadingZeroesInHexadecimalWithPrefix)
{
    auto [result, val] = do_test<short>("0x0000000000000000100", "{:x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0x100);
}

TEST(IntegerTest, Pointer)
{
    char source_buf[64]{};
    int value{42};
    std::snprintf(source_buf, 64, "%p", static_cast<const void*>(&value));
    std::string_view source{source_buf};

    auto [result, val] = do_test<void*>(source, "{}");
    ASSERT_TRUE(result);
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
    auto result = scn::scan<int>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_NE(result->begin(), input.end());
    EXPECT_EQ(std::get<0>(result->values()), 1452555457);
}

#if !SCN_DISABLE_LOCALE
TEST(IntegerTest, WonkyInputWithThsep)
{
    std::string_view input = "-0x,)27614,)24t14741";
    auto result = scn::scan<int>(input, "{:L}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 2);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, WonkyInputWithThsep2)
{
    std::string_view input = "-0b,28";
    auto result = scn::scan<int>(input, "{:L}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 2);
    EXPECT_EQ(result->value(), 0);
}
#endif

TEST(IntegerTest, BinaryFollowedByDec_Default)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, BinaryFollowedByDec_Decimal)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, BinaryFollowedByDec_Generic)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 1);
}
TEST(IntegerTest, BinaryFollowedByDec_Binary)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{:b}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 1);
}

TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Default)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 12);
}
TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Decimal)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 12);
}
TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Generic)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 12);
}
TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Binary)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{:b}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 1);
}

TEST(IntegerTest, OctalFollowedByDec_Default)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalFollowedByDec_Decimal)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalFollowedByDec_Generic)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}
TEST(IntegerTest, OctalFollowedByDec_Octal)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{:o}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}

TEST(IntegerTest, OctalNoPrefixFollowedByDec_Default)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalNoPrefixFollowedByDec_Decimal)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalNoPrefixFollowedByDec_Generic)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalNoPrefixFollowedByDec_Octal)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{:o}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}

TEST(IntegerTest, OctalLongPrefixFollowedByDec_Default)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, OctalLongPrefixFollowedByDec_Decimal)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, OctalLongPrefixFollowedByDec_Generic)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}
TEST(IntegerTest, OctalLongPrefixFollowedByDec_Octal)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{:o}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}

TEST(IntegerTest, HexFollowedByNonDigit_Default)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, HexFollowedByNonDigit_Decimal)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, HexFollowedByNonDigit_Generic)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 0xf);
}
TEST(IntegerTest, HexFollowedByNonDigit_Hex)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{:x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 0xf);
}

#if SCN_HAS_INT128
TEST(IntegerTest, Int128_Zero)
{
    std::string_view input = "0";
    auto result = scn::scan<scn::int128>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, Int128_Large)
{
    std::string_view input = "99999999999999999999999999";
    ASSERT_LT(input.size(), std::numeric_limits<scn::int128>::digits10);
    ASSERT_GT(input.size(), std::numeric_limits<std::int64_t>::digits10);
    auto result = scn::scan<scn::int128>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_NE(result->value(), 0);
    EXPECT_NE(result->value(), std::numeric_limits<std::int64_t>::max());
}
TEST(IntegerTest, UInt128)
{
    std::string_view input = "123456789";
    auto result = scn::scan<scn::uint128>(input, "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 123456789);
}
#endif

TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Default)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_FALSE(result);
}
TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Decimal)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_FALSE(result);
}
TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Generic)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_FALSE(result);
}
TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Hex)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{:x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 0xf);
}

TEST(IntegerTest, Fuzz_RepeatedString)
{
    std::string_view input = "0\n0";

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0);
    ASSERT_NE(result->begin(), input.end());
    EXPECT_EQ(*result->begin(), '\n');
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0);
    EXPECT_TRUE(result->range().empty());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_FALSE(result);
}
TEST(IntegerTest, Fuzz_RepeatedDeque)
{
    std::deque<char> input{'0', '\n', '0'};

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0);
    ASSERT_NE(result->begin(), input.end());
    EXPECT_EQ(*result->begin(), '\n');
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0);
    EXPECT_TRUE(result->range().empty());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_FALSE(result);
}

TEST(IntegerTest, Fuzz_RepeatedString2)
{
    std::string_view input = "\n0";

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0);
    EXPECT_EQ(result->begin(), input.end());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_FALSE(result);
}
TEST(IntegerTest, Fuzz_RepeatedDeque2)
{
    std::deque<char> input{'\n', '0'};

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0);
    EXPECT_EQ(result->begin(), input.end());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_FALSE(result);
}

TEST(ScanIntTest, Simple)
{
    std::string_view input = "42";
    auto result = scn::scan_int<int>(input);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, Negative)
{
    std::string_view input = "-42";
    auto result = scn::scan_int<int>(input);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    EXPECT_EQ(result->value(), -42);
}
TEST(ScanIntTest, Positive)
{
    std::string_view input = "+42";
    auto result = scn::scan_int<int>(input);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, LeadingWhitespace)
{
    std::string_view input = "   42";
    auto result = scn::scan_int<int>(input);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->range().empty());
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, TrailingWhitespace)
{
    std::string_view input = "42   ";
    auto result = scn::scan_int<int>(input);
    ASSERT_TRUE(result);
    EXPECT_EQ(std::string_view(result->range().data(), result->range().size()),
              "   ");
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, RangeError)
{
    std::string_view input = "999999999999999999999999999999999999";
    auto result = scn::scan_int<int>(input);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::value_positive_overflow);
}
TEST(ScanIntTest, Empty)
{
    auto result = scn::scan_int<int>("");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
}

#if !SCN_IS_BIG_ENDIAN
TEST(ScanIntExhaustiveValidTest, Simple)
{
    EXPECT_EQ(scn::scan_int_exhaustive_valid<int>("42"), 42);
}
TEST(ScanIntExhaustiveValidTest, Negative)
{
    EXPECT_EQ(scn::scan_int_exhaustive_valid<int>("-42"), -42);
}
TEST(ScanIntExhaustiveValidTest, Large)
{
    EXPECT_EQ(scn::scan_int_exhaustive_valid<long long>("999999999999"),
              999999999999);
}
#endif
