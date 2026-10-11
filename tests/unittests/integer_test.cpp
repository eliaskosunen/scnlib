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

#include "test_common.h"

#include <scn/scan.h>

#include <array>
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
    ASSERT_THAT(result, Succeeded());
    EXPECT_NE(result->begin(), input.end());
    EXPECT_EQ(std::get<0>(result->values()), 1452555457);
}

#if !SCN_DISABLE_LOCALE
TEST(IntegerTest, WonkyInputWithThsep)
{
    std::string_view input = "-0x,)27614,)24t14741";
    auto result = scn::scan<int>(input, "{:L}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 2);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, WonkyInputWithThsep2)
{
    std::string_view input = "-0b,28";
    auto result = scn::scan<int>(input, "{:L}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 2);
    EXPECT_EQ(result->value(), 0);
}

namespace {
struct comma_grouping_numpunct : std::numpunct<char> {
    char do_thousands_sep() const override
    {
        return ',';
    }
    std::string do_grouping() const override
    {
        return "\3";
    }
};
}  // namespace

// A thousands separator not followed by a digit is not part of the number
TEST(IntegerTest, TrailingThsepIsNotConsumed)
{
    const std::locale loc{std::locale::classic(), new comma_grouping_numpunct};

    auto result = scn::scan<int, int>(loc, "1, 2", "{:L}, {}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->values(), testing::FieldsAre(1, 2));

    std::string_view input = "12,345, x";
    auto trailing = scn::scan<int>(loc, input, "{:L}");
    ASSERT_THAT(trailing, Succeeded());
    EXPECT_EQ(trailing->value(), 12345);
    EXPECT_EQ(trailing->begin(), input.begin() + 6);

    auto repeated = scn::scan<int>(loc, "1,,", "{:L}");
    ASSERT_THAT(repeated, Succeeded());
    EXPECT_EQ(repeated->value(), 1);
    EXPECT_STREQ(repeated->begin(), ",,");
}
#endif

TEST(IntegerTest, BinaryFollowedByDec_Default)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, BinaryFollowedByDec_Decimal)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, BinaryFollowedByDec_Generic)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 1);
}
TEST(IntegerTest, BinaryFollowedByDec_Binary)
{
    std::string_view input = "0b12";
    auto result = scn::scan<int>(input, "{:b}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 1);
}

TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Default)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 12);
}
TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Decimal)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 12);
}
TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Generic)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 12);
}
TEST(IntegerTest, BinaryNoPrefixFollowedByDec_Binary)
{
    std::string_view input = "12";
    auto result = scn::scan<int>(input, "{:b}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 1);
}

TEST(IntegerTest, OctalFollowedByDec_Default)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalFollowedByDec_Decimal)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalFollowedByDec_Generic)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}
TEST(IntegerTest, OctalFollowedByDec_Octal)
{
    std::string_view input = "078";
    auto result = scn::scan<int>(input, "{:o}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}

TEST(IntegerTest, OctalNoPrefixFollowedByDec_Default)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalNoPrefixFollowedByDec_Decimal)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalNoPrefixFollowedByDec_Generic)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 78);
}
TEST(IntegerTest, OctalNoPrefixFollowedByDec_Octal)
{
    std::string_view input = "78";
    auto result = scn::scan<int>(input, "{:o}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}

TEST(IntegerTest, OctalLongPrefixFollowedByDec_Default)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, OctalLongPrefixFollowedByDec_Decimal)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, OctalLongPrefixFollowedByDec_Generic)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}
TEST(IntegerTest, OctalLongPrefixFollowedByDec_Octal)
{
    std::string_view input = "0o78";
    auto result = scn::scan<int>(input, "{:o}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 7);
}

TEST(IntegerTest, HexFollowedByNonDigit_Default)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, HexFollowedByNonDigit_Decimal)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.begin() + 1);
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, HexFollowedByNonDigit_Generic)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 0xf);
}
TEST(IntegerTest, HexFollowedByNonDigit_Hex)
{
    std::string_view input = "0xfg";
    auto result = scn::scan<int>(input, "{:x}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 0xf);
}

#if SCN_HAS_INT128
TEST(IntegerTest, Int128_Zero)
{
    std::string_view input = "0";
    auto result = scn::scan<scn::int128>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 0);
}
TEST(IntegerTest, Int128_Large)
{
    std::string_view input = "99999999999999999999999999";
    ASSERT_LT(input.size(), std::numeric_limits<scn::int128>::digits10);
    ASSERT_GT(input.size(), std::numeric_limits<std::int64_t>::digits10);
    auto result = scn::scan<scn::int128>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_NE(result->value(), 0);
    EXPECT_NE(result->value(), std::numeric_limits<std::int64_t>::max());
}
TEST(IntegerTest, UInt128)
{
    std::string_view input = "123456789";
    auto result = scn::scan<scn::uint128>(input, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end());
    EXPECT_EQ(result->value(), 123456789);
}
#endif

TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Default)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{}");
    ASSERT_THAT(result, Failed());
}
TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Decimal)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{:d}");
    ASSERT_THAT(result, Failed());
}
TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Generic)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{:i}");
    ASSERT_THAT(result, Failed());
}
TEST(IntegerTest, HexNoPrefixFollowedByNonDigit_Hex)
{
    std::string_view input = "fg";
    auto result = scn::scan<int>(input, "{:x}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value(), 0xf);
}

TEST(IntegerTest, OverflowPositive)
{
    auto result = scn::scan<std::int32_t>("99999999999999999999", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::value_positive_overflow));
}

TEST(IntegerTest, OverflowNegative)
{
    auto result = scn::scan<std::int32_t>("-99999999999999999999", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::value_negative_overflow));
}

TEST(IntegerTest, UnsignedNegative)
{
    auto result = scn::scan<unsigned>("-42", "{}");
    ASSERT_THAT(result, Failed());
}

TEST(IntegerTest, BinaryFormat)
{
    auto [result, val] = do_test<int>("0b1010", "{:b}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 10);
}

TEST(IntegerTest, OctalFormat)
{
    auto [result, val] = do_test<int>("0o77", "{:o}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 63);
}

TEST(IntegerTest, HexadecimalFormat)
{
    auto [result, val] = do_test<int>("0xFF", "{:x}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 255);
}

TEST(IntegerTest, AutoDetectHex)
{
    auto [result, val] = do_test<int>("0x10", "{:i}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 16);
}

TEST(IntegerTest, AutoDetectBinary)
{
    auto [result, val] = do_test<int>("0b10", "{:i}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 2);
}

TEST(IntegerTest, AutoDetectOctal)
{
    auto [result, val] = do_test<int>("010", "{:i}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 8);
}

TEST(IntegerTest, ShortInt)
{
    auto [result, val] = do_test<std::int16_t>("12345", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 12345);
}

TEST(IntegerTest, LongLongInt)
{
    auto [result, val] = do_test<std::int64_t>("9876543210", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 9876543210LL);
}

TEST(IntegerTest, Int8Max)
{
    auto [result, val] = do_test<std::int8_t>("127", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 127);
}

TEST(IntegerTest, Int8Min)
{
    auto [result, val] = do_test<std::int8_t>("-128", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, -128);
}

TEST(IntegerTest, UInt64Max)
{
    auto [result, val] = do_test<std::uint64_t>("18446744073709551615", "{}");
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 18446744073709551615ULL);
}

TEST(IntegerTest, Fuzz_RepeatedString)
{
    std::string_view input = "0\n0";

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0);
    ASSERT_NE(result->begin(), input.end());
    EXPECT_EQ(*result->begin(), '\n');
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0);
    EXPECT_THAT(result->range(), IsEmptyRange());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_THAT(result, Failed());
}
TEST(IntegerTest, Fuzz_RepeatedDeque)
{
    std::deque<char> input{'0', '\n', '0'};

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0);
    ASSERT_NE(result->begin(), input.end());
    EXPECT_EQ(*result->begin(), '\n');
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0);
    EXPECT_THAT(result->range(), IsEmptyRange());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_THAT(result, Failed());
}

TEST(IntegerTest, Fuzz_RepeatedString2)
{
    std::string_view input = "\n0";

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0);
    EXPECT_EQ(result->begin(), input.end());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_THAT(result, Failed());
}
TEST(IntegerTest, Fuzz_RepeatedDeque2)
{
    std::deque<char> input{'\n', '0'};

    auto it = input.begin();
    auto result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0);
    EXPECT_EQ(result->begin(), input.end());
    it = result->begin();

    result =
        scn::scan<signed char>(scn::ranges::subrange{it, input.end()}, "{:i}");
    EXPECT_THAT(result, Failed());
}

namespace {
template <typename T, typename = void>
inline constexpr bool can_call_scan_int = false;
template <typename T>
inline constexpr bool can_call_scan_int<
    T,
    std::void_t<decltype(scn::scan_int<T>(std::string_view{}))>> = true;

template <typename T, typename = void>
inline constexpr bool can_call_scan_int_exhaustive_valid = false;
template <typename T>
inline constexpr bool can_call_scan_int_exhaustive_valid<
    T,
    std::void_t<decltype(scn::scan_int_exhaustive_valid<T>(
        std::string_view{}))>> = true;
}  // namespace

// Only the types instantiated in the compiled library can be used
static_assert(can_call_scan_int<int>);
static_assert(can_call_scan_int<unsigned long long>);
static_assert(can_call_scan_int<signed char>);
static_assert(!can_call_scan_int<bool>);
static_assert(!can_call_scan_int<char>);
static_assert(!can_call_scan_int<wchar_t>);
static_assert(!can_call_scan_int<char16_t>);
static_assert(!can_call_scan_int<char32_t>);
#if SCN_HAS_CHAR8
static_assert(!can_call_scan_int<char8_t>);
#endif
static_assert(can_call_scan_int_exhaustive_valid<int>);
static_assert(can_call_scan_int_exhaustive_valid<unsigned char>);
static_assert(!can_call_scan_int_exhaustive_valid<bool>);
static_assert(!can_call_scan_int_exhaustive_valid<char16_t>);
#if SCN_HAS_INT128
static_assert(can_call_scan_int<scn::int128>);
static_assert(can_call_scan_int<scn::uint128>);
// No instantiation of scan_int_exhaustive_valid_impl for 128-bit integers
static_assert(!can_call_scan_int_exhaustive_valid<scn::int128>);
static_assert(!can_call_scan_int_exhaustive_valid<scn::uint128>);
#endif

TEST(ScanIntTest, Simple)
{
    std::string_view input = "42";
    auto result = scn::scan_int<int>(input);
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, Bases)
{
    auto hex = scn::scan_int<int>("ff", 16);
    ASSERT_THAT(hex, Succeeded());
    EXPECT_EQ(hex->value(), 255);

    auto base36 = scn::scan_int<int>("z", 36);
    ASSERT_THAT(base36, Succeeded());
    EXPECT_EQ(base36->value(), 35);

    auto detected = scn::scan_int<int>("0x10", 0);
    ASSERT_THAT(detected, Succeeded());
    EXPECT_EQ(detected->value(), 16);

    constexpr std::array<int, 4> invalid_bases{-1, 1, 37, 100};
    for (int base : invalid_bases) {
        auto result = scn::scan_int<int>("1", base);
        ASSERT_FALSE(result) << base;
        EXPECT_EQ(result.error().code(),
                  scn::scan_error::invalid_format_string);
    }
}

TEST(ScanIntTest, Negative)
{
    std::string_view input = "-42";
    auto result = scn::scan_int<int>(input);
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    EXPECT_EQ(result->value(), -42);
}
TEST(ScanIntTest, Positive)
{
    std::string_view input = "+42";
    auto result = scn::scan_int<int>(input);
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, LeadingWhitespace)
{
    std::string_view input = "   42";
    auto result = scn::scan_int<int>(input);
    ASSERT_THAT(result, Succeeded());
    EXPECT_THAT(result->range(), IsEmptyRange());
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, TrailingWhitespace)
{
    std::string_view input = "42   ";
    auto result = scn::scan_int<int>(input);
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(std::string_view(result->range().data(), result->range().size()),
              "   ");
    EXPECT_EQ(result->value(), 42);
}
TEST(ScanIntTest, RangeError)
{
    std::string_view input = "999999999999999999999999999999999999";
    auto result = scn::scan_int<int>(input);
    ASSERT_THAT(result, FailedWith(scn::scan_error::value_positive_overflow));
}
TEST(ScanIntTest, Empty)
{
    auto result = scn::scan_int<int>("");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

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
    EXPECT_EQ(scn::scan_int_exhaustive_valid<long long>("1234567890123456"),
              1234567890123456);
    EXPECT_EQ(scn::scan_int_exhaustive_valid<long long>("-123456789"),
              -123456789);
}

TEST(IntegerArbitraryBaseTest, Simple)
{
    auto hex = scn::scan<int>("ff", "{:r16}");
    ASSERT_THAT(hex, Succeeded());
    EXPECT_EQ(hex->value(), 255);

    auto base36 = scn::scan<int>("Zz", "{:R36}");
    ASSERT_THAT(base36, Succeeded());
    EXPECT_EQ(base36->value(), 35 * 36 + 35);

    auto base3 = scn::scan<unsigned>("-12", "{:r3}");
    ASSERT_THAT(base3, Failed());

    auto negative = scn::scan<int>("-12", "{:r3}");
    ASSERT_THAT(negative, Succeeded());
    EXPECT_EQ(negative->value(), -5);
}

TEST(IntegerArbitraryBaseTest, NoBasePrefix)
{
    auto result = scn::scan<int>("0x1f", "{:r16}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0);
    EXPECT_STREQ(result->begin(), "x1f");
}

TEST(IntegerArbitraryBaseTest, InvalidBase)
{
    constexpr std::array<const char*, 8> formats{"{:r}",    "{:r1}", "{:r37}",
                                                 "{:r100}", "{:rx}", "{:r010}",
                                                 "{:r02}",  "{:r0}"};
    for (auto fmt : formats) {
        auto result = scn::scan<int>("1", scn::runtime_format(fmt));
        ASSERT_THAT(result, Failed()) << fmt;
        EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string)
            << fmt;
    }

    auto localized = scn::scan<int>("1", scn::runtime_format("{:Lr16}"));
    ASSERT_THAT(localized, Failed());
}

namespace {
char digit_to_char(unsigned digit)
{
    return "0123456789abcdefghijklmnopqrstuvwxyz"[digit];
}
unsigned char_to_digit(char ch)
{
    return ch <= '9' ? static_cast<unsigned>(ch - '0')
                     : static_cast<unsigned>(ch - 'a' + 10);
}

std::string to_base(uint64_t value, int base)
{
    std::string result;
    do {
        result.insert(result.begin(),
                      digit_to_char(static_cast<unsigned>(
                          value % static_cast<uint64_t>(base))));
        value /= static_cast<uint64_t>(base);
    } while (value != 0);
    return result;
}

// Digit-string addition, for values that don't fit in an uint64_t
std::string add_in_base(std::string_view lhs, std::string_view rhs, int base)
{
    std::string result;
    unsigned carry = 0;
    for (size_t i = 0; i < std::max(lhs.size(), rhs.size()) || carry != 0;
         ++i) {
        unsigned sum = carry;
        if (i < lhs.size()) {
            sum += char_to_digit(lhs[lhs.size() - 1 - i]);
        }
        if (i < rhs.size()) {
            sum += char_to_digit(rhs[rhs.size() - 1 - i]);
        }
        result.insert(result.begin(),
                      digit_to_char(sum % static_cast<unsigned>(base)));
        carry = sum / static_cast<unsigned>(base);
    }
    return result;
}

struct base_format {
    int base;
    std::string_view prefix;
    std::string format;

    // With `{:i}`, a leading zero would make the number octal
    bool allows_leading_zeroes() const
    {
        return !(format == "{:i}" && prefix.empty());
    }
};

std::vector<base_format> get_formats_for_all_bases()
{
    std::vector<base_format> formats;
    for (int base = 2; base <= 36; ++base) {
        formats.push_back({base, "", "{:r" + std::to_string(base) + "}"});
    }
    formats.push_back({2, "", "{:b}"});
    formats.push_back({2, "0b", "{:b}"});
    formats.push_back({2, "0b", "{:i}"});
    formats.push_back({8, "", "{:o}"});
    formats.push_back({8, "0o", "{:o}"});
    formats.push_back({8, "0", "{:i}"});
    formats.push_back({10, "", "{}"});
    formats.push_back({10, "", "{:d}"});
    formats.push_back({10, "", "{:i}"});
    formats.push_back({16, "", "{:x}"});
    formats.push_back({16, "0x", "{:x}"});
    formats.push_back({16, "0x", "{:i}"});
    return formats;
}

// Values around the limits of 64-bit integers, as digit strings in `base`
struct limit_digits {
    explicit limit_digits(int base)
        : u64_max(to_base(std::numeric_limits<uint64_t>::max(), base)),
          u64_max_plus_one(add_in_base(u64_max, "1", base)),
          i64_max(to_base(std::numeric_limits<int64_t>::max(), base)),
          i64_max_plus_one(add_in_base(i64_max, "1", base)),
          i64_max_plus_two(add_in_base(i64_max, "2", base)),
          // 2^64 + base^(max_digits-1): has max_digits digits in most bases,
          // and wraps around to base^(max_digits-1) in an uint64_t
          wrapping(add_in_base(u64_max_plus_one,
                               "1" + std::string(u64_max.size() - 1, '0'),
                               base))
    {
    }

    bool has_wrapping() const
    {
        return wrapping.size() == u64_max.size();
    }

    std::string u64_max, u64_max_plus_one, i64_max, i64_max_plus_one,
        i64_max_plus_two, wrapping;
};

template <typename T>
void check_scan_succeeds(const std::string& input,
                         const std::string& format,
                         T expected)
{
    auto result = scn::scan<T>(input, scn::runtime_format(format));
    ASSERT_THAT(result, Succeeded()) << input << ' ' << format;
    EXPECT_EQ(result->value(), expected) << input << ' ' << format;
    EXPECT_TRUE(result->range().empty()) << input << ' ' << format;
}

template <typename T>
void check_scan_fails(const std::string& input,
                      const std::string& format,
                      enum scn::scan_error::code code)
{
    EXPECT_THAT(scn::scan<T>(input, scn::runtime_format(format)),
                FailedWith(code))
        << input << ' ' << format;
}

template <typename T>
void test_signed_64bit_limits()
{
    static_assert(std::is_signed_v<T> && sizeof(T) == 8);
    constexpr auto max = std::numeric_limits<T>::max();
    constexpr auto min = std::numeric_limits<T>::min();
    constexpr auto positive_overflow = scn::scan_error::value_positive_overflow;
    constexpr auto negative_overflow = scn::scan_error::value_negative_overflow;

    for (const auto& base_fmt : get_formats_for_all_bases()) {
        const auto& format = base_fmt.format;
        const limit_digits digits{base_fmt.base};
        const auto positive = [&](const std::string& d) {
            return std::string{base_fmt.prefix} + d;
        };
        const auto negative = [&](const std::string& d) {
            return "-" + positive(d);
        };

        check_scan_succeeds<T>(positive(digits.i64_max), format, max);
        check_scan_succeeds<T>(negative(digits.i64_max_plus_one), format, min);
        if (base_fmt.allows_leading_zeroes()) {
            check_scan_succeeds<T>(positive("000" + digits.i64_max), format,
                                   max);
            check_scan_succeeds<T>(negative("000" + digits.i64_max_plus_one),
                                   format, min);
        }

        check_scan_fails<T>(positive(digits.i64_max_plus_one), format,
                            positive_overflow);
        check_scan_fails<T>(negative(digits.i64_max_plus_two), format,
                            negative_overflow);
        check_scan_fails<T>(positive(digits.u64_max), format,
                            positive_overflow);
        check_scan_fails<T>(negative(digits.u64_max), format,
                            negative_overflow);
        check_scan_fails<T>(positive(digits.u64_max_plus_one), format,
                            positive_overflow);
        check_scan_fails<T>(negative(digits.u64_max_plus_one), format,
                            negative_overflow);
        if (digits.has_wrapping()) {
            check_scan_fails<T>(positive(digits.wrapping), format,
                                positive_overflow);
            check_scan_fails<T>(negative(digits.wrapping), format,
                                negative_overflow);
        }
    }
}

template <typename T>
void test_unsigned_64bit_limits()
{
    static_assert(std::is_unsigned_v<T> && sizeof(T) == 8);
    constexpr auto max = std::numeric_limits<T>::max();
    constexpr auto positive_overflow = scn::scan_error::value_positive_overflow;

    for (const auto& base_fmt : get_formats_for_all_bases()) {
        const auto& format = base_fmt.format;
        const limit_digits digits{base_fmt.base};
        const auto positive = [&](const std::string& d) {
            return std::string{base_fmt.prefix} + d;
        };

        check_scan_succeeds<T>(positive(digits.u64_max), format, max);
        check_scan_succeeds<T>(positive("0"), format, T{0});
        if (base_fmt.allows_leading_zeroes()) {
            check_scan_succeeds<T>(positive("000" + digits.u64_max), format,
                                   max);
        }

        check_scan_fails<T>(positive(digits.u64_max_plus_one), format,
                            positive_overflow);
        if (digits.has_wrapping()) {
            check_scan_fails<T>(positive(digits.wrapping), format,
                                positive_overflow);
        }
    }
}
}  // namespace

TEST(IntegerOverflowTest, Signed64BitLimitsInAllBases)
{
    test_signed_64bit_limits<long long>();
    test_signed_64bit_limits<int64_t>();
}

TEST(IntegerOverflowTest, Unsigned64BitLimitsInAllBases)
{
    test_unsigned_64bit_limits<unsigned long long>();
    test_unsigned_64bit_limits<uint64_t>();
}

TEST(IntegerOverflowTest, WrappingIsDetectedOnlyInBasesWithRoom)
{
    for (int base = 2; base <= 36; ++base) {
        const bool can_wrap = base != 2 && base != 4 && base != 16;
        EXPECT_EQ(limit_digits{base}.has_wrapping(), can_wrap) << base;
    }
}

TEST(IntegerOverflowTest, ScanIntLimitsInAllBases)
{
    for (int base = 2; base <= 36; ++base) {
        const limit_digits digits{base};

        auto result = scn::scan_int<unsigned long long>(digits.u64_max, base);
        ASSERT_THAT(result, Succeeded()) << base;
        EXPECT_EQ(result->value(), std::numeric_limits<uint64_t>::max());
        EXPECT_THAT(
            scn::scan_int<unsigned long long>(digits.u64_max_plus_one, base),
            FailedWith(scn::scan_error::value_positive_overflow))
            << base;

        auto min_result =
            scn::scan_int<long long>("-" + digits.i64_max_plus_one, base);
        ASSERT_THAT(min_result, Succeeded()) << base;
        EXPECT_EQ(min_result->value(), std::numeric_limits<long long>::min());
        EXPECT_THAT(scn::scan_int<long long>(digits.i64_max_plus_one, base),
                    FailedWith(scn::scan_error::value_positive_overflow))
            << base;
        EXPECT_THAT(
            scn::scan_int<long long>("-" + digits.i64_max_plus_two, base),
            FailedWith(scn::scan_error::value_negative_overflow))
            << base;

        if (digits.has_wrapping()) {
            EXPECT_THAT(
                scn::scan_int<unsigned long long>(digits.wrapping, base),
                FailedWith(scn::scan_error::value_positive_overflow))
                << base;
            EXPECT_THAT(scn::scan_int<long long>("-" + digits.wrapping, base),
                        FailedWith(scn::scan_error::value_negative_overflow))
                << base;
        }
    }
}

TEST(IntegerOverflowTest, WrappingRepros)
{
    using ull = unsigned long long;
    EXPECT_THAT(scn::scan<ull>("30000000000000000000", "{}"),
                FailedWith(scn::scan_error::value_positive_overflow));
    EXPECT_THAT(scn::scan<ull>("65340232221128654848", "{}"),
                FailedWith(scn::scan_error::value_positive_overflow));
    EXPECT_THAT(scn::scan_int<ull>("30000000000000000000"),
                FailedWith(scn::scan_error::value_positive_overflow));
    EXPECT_THAT(scn::scan<long long>("-3000000000000000000000", "{:o}"),
                FailedWith(scn::scan_error::value_negative_overflow));
    EXPECT_THAT(scn::scan<long long>("-03000000000000000000000", "{:i}"),
                FailedWith(scn::scan_error::value_negative_overflow));
    EXPECT_THAT(scn::scan<ull>(std::string(41, '2'), "{:r3}"),
                FailedWith(scn::scan_error::value_positive_overflow));
}

TEST(IntegerOverflowTest, NarrowerTypeLimits)
{
    constexpr std::array<int, 3> bases{3, 10, 36};
    for (int base : bases) {
        const auto format = "{:r" + std::to_string(base) + "}";

        check_scan_succeeds<uint32_t>(to_base(0xffffffff, base), format,
                                      0xffffffff);
        check_scan_fails<uint32_t>(to_base(0x100000000, base), format,
                                   scn::scan_error::value_positive_overflow);

        check_scan_succeeds<int32_t>("-" + to_base(0x80000000, base), format,
                                     std::numeric_limits<int32_t>::min());
        check_scan_fails<int32_t>("-" + to_base(0x80000001, base), format,
                                  scn::scan_error::value_negative_overflow);

        check_scan_succeeds<uint8_t>(to_base(255, base), format, 255);
        check_scan_fails<uint8_t>(to_base(256, base), format,
                                  scn::scan_error::value_positive_overflow);

        check_scan_succeeds<int16_t>("-" + to_base(32768, base), format,
                                     std::numeric_limits<int16_t>::min());
        check_scan_fails<int16_t>("-" + to_base(32769, base), format,
                                  scn::scan_error::value_negative_overflow);
    }
}

TEST(ScanIntExhaustiveValidTest, LeadingZeroes)
{
    EXPECT_EQ(scn::scan_int_exhaustive_valid<long long>(
                  "000000000000009223372036854775807"),
              std::numeric_limits<long long>::max());
    EXPECT_EQ(scn::scan_int_exhaustive_valid<unsigned long long>(
                  "0000000000000000000000000000001"),
              1u);
}
