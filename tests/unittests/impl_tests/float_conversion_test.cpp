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

#include "float_conversion_test.h"

struct HighPrecisionDecimalTest : testing::Test {
    scn::impl::float_rounding_guard rounding_guard{};
};

using testing::FieldsAre;

TEST_F(HighPrecisionDecimalTest, RightShiftBy3)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {2, 9, 9, 7, 9, 2, 4, 5, 8};
    hpd.num_digits = 9;
    hpd.decimal_point = 0;
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.right_shift(3);
    EXPECT_EQ(hpd.num_digits, 10);
    EXPECT_EQ(hpd.decimal_point, -1);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(3, 7, 4, 7, 4, 0, 5, 7, 2, 5));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.left_shift(3);
    EXPECT_EQ(hpd.num_digits, 9);
    EXPECT_EQ(hpd.decimal_point, 0);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(2, 9, 9, 7, 9, 2, 4, 5, 8));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));
}

TEST_F(HighPrecisionDecimalTest, RightShiftBy29)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {2, 9, 9, 7, 9, 2, 4, 5, 8};
    hpd.num_digits = 9;
    hpd.decimal_point = 0;
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.right_shift(29);
    EXPECT_EQ(hpd.num_digits, 28);
    EXPECT_EQ(hpd.decimal_point, -9);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(5, 5, 8, 4, 0, 6, 9, 6, 7, 6, 6, 9, 7, 2,
                                     5, 4, 1, 8, 0, 9, 0, 8, 2, 0, 3, 1, 2, 5));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.left_shift(29);
    EXPECT_EQ(hpd.num_digits, 9);
    EXPECT_EQ(hpd.decimal_point, 0);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(2, 9, 9, 7, 9, 2, 4, 5, 8));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));
}

TEST_F(HighPrecisionDecimalTest, LeftShiftBy1)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {1, 2, 3};
    hpd.num_digits = 3;
    hpd.decimal_point = 1;
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(1, false));

    hpd.left_shift(1);
    EXPECT_EQ(hpd.num_digits, 3);
    EXPECT_EQ(hpd.decimal_point, 1);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(2, 4, 6));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(2, false));

    hpd.right_shift(1);
    EXPECT_EQ(hpd.num_digits, 3);
    EXPECT_EQ(hpd.decimal_point, 1);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(1, 2, 3));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(1, false));
}

TEST_F(HighPrecisionDecimalTest, LeftShiftBy3)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {3, 7, 4, 7, 4, 0, 5, 7, 2, 5};
    hpd.num_digits = 10;
    hpd.decimal_point = 10;
    EXPECT_THAT(hpd.rounded_significand<double>(),
                FieldsAre(3747405725ull, false));

    hpd.left_shift(3);
    EXPECT_EQ(hpd.num_digits, 9);
    EXPECT_EQ(hpd.decimal_point, 11);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(2, 9, 9, 7, 9, 2, 4, 5, 8));
    EXPECT_THAT(hpd.rounded_significand<double>(),
                FieldsAre(29979245800ull, false));

    hpd.right_shift(3);
    EXPECT_EQ(hpd.num_digits, 10);
    EXPECT_EQ(hpd.decimal_point, 10);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(3, 7, 4, 7, 4, 0, 5, 7, 2, 5));
    EXPECT_THAT(hpd.rounded_significand<double>(),
                FieldsAre(3747405725ull, false));
}

TEST_F(HighPrecisionDecimalTest, Pi_F32)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {3, 1, 4};
    hpd.num_digits = 3;
    hpd.decimal_point = 1;
    EXPECT_THAT(hpd.rounded_significand<float>(), FieldsAre(3, false));

    hpd.right_shift(1);
    EXPECT_EQ(hpd.num_digits, 3);
    EXPECT_EQ(hpd.decimal_point, 1);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(1, 5, 7));
    EXPECT_THAT(hpd.rounded_significand<float>(), FieldsAre(2, true));

    hpd.left_shift(23);
    EXPECT_EQ(hpd.num_digits, 10);
    EXPECT_EQ(hpd.decimal_point, 8);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(1, 3, 1, 7, 0, 1, 1, 4, 5, 6));
    EXPECT_THAT(hpd.rounded_significand<float>(), FieldsAre(13170115u, true));
}

TEST_F(HighPrecisionDecimalTest, PlanckConstant)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {6, 6, 2, 6, 0, 7, 0, 1, 5};
    hpd.num_digits = 9;
    hpd.decimal_point = -33;
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.left_shift(60);
    EXPECT_EQ(hpd.num_digits, 26);
    EXPECT_EQ(hpd.decimal_point, -15);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(7, 6, 3, 9, 3, 3, 8, 7, 6, 6, 9, 6, 8, 5,
                                     1, 6, 2, 3, 3, 2, 9, 1, 3, 6, 6, 4));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.left_shift(49);
    EXPECT_EQ(hpd.num_digits, 41);
    EXPECT_EQ(hpd.decimal_point, 0);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(4, 3, 0, 0, 5, 6, 5, 4, 0, 3, 0, 3, 4, 5,
                                     4, 9, 2, 6, 0, 6, 0, 0, 1, 5, 1, 2, 6, 1,
                                     4, 6, 6, 2, 3, 1, 3, 6, 0, 7, 1, 6, 8));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.left_shift(1);
    EXPECT_EQ(hpd.num_digits, 41);
    EXPECT_EQ(hpd.decimal_point, 0);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(8, 6, 0, 1, 1, 3, 0, 8, 0, 6, 0, 6, 9, 0,
                                     9, 8, 5, 2, 1, 2, 0, 0, 3, 0, 2, 5, 2, 2,
                                     9, 3, 2, 4, 6, 2, 7, 2, 1, 4, 3, 3, 6));
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(1, true));

    hpd.left_shift(53);
    EXPECT_EQ(hpd.num_digits, 57);
    EXPECT_EQ(hpd.decimal_point, 16);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(
                    7, 7, 4, 7, 2, 0, 9, 8, 9, 8, 6, 3, 5, 5, 3, 7, 1, 9, 9, 0,
                    8, 5, 8, 6, 2, 1, 5, 2, 0, 5, 5, 2, 8, 5, 0, 6, 8, 3, 9, 5,
                    2, 3, 0, 3, 1, 4, 7, 4, 1, 4, 9, 2, 6, 1, 3, 1, 2));
    EXPECT_THAT(hpd.rounded_significand<double>(),
                FieldsAre(7747209898635537ull, false));
}

TEST_F(HighPrecisionDecimalTest, PlanckConstant_ShiftAllAtOnce)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {6, 6, 2, 6, 0, 7, 0, 1, 5};
    hpd.num_digits = 9;
    hpd.decimal_point = -33;
    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(0, false));

    hpd.left_shift(110 + 53);
    EXPECT_EQ(hpd.num_digits, 57);
    EXPECT_EQ(hpd.decimal_point, 16);
    EXPECT_THAT(std::vector<std::uint32_t>(hpd.digits.begin(),
                                           hpd.digits.begin() + hpd.num_digits),
                testing::ElementsAre(
                    7, 7, 4, 7, 2, 0, 9, 8, 9, 8, 6, 3, 5, 5, 3, 7, 1, 9, 9, 0,
                    8, 5, 8, 6, 2, 1, 5, 2, 0, 5, 5, 2, 8, 5, 0, 6, 8, 3, 9, 5,
                    2, 3, 0, 3, 1, 4, 7, 4, 1, 4, 9, 2, 6, 1, 3, 1, 2));
    EXPECT_THAT(hpd.rounded_significand<double>(),
                FieldsAre(7747209898635537ull, false));
}

TEST_F(HighPrecisionDecimalTest, RoundedSignificand)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {7, 7, 4, 7, 2, 1, 9, 9};
    hpd.num_digits = 8;
    hpd.decimal_point = 5;

    EXPECT_THAT(hpd.rounded_significand<double>(), FieldsAre(77472u, false));
}

TEST(SaturatingExponentTest, Parse)
{
    using scn::impl::max_exponent_magnitude;
    using scn::impl::parse_saturating_exponent;

    EXPECT_EQ(parse_saturating_exponent("0"sv, true), 0);
    EXPECT_EQ(parse_saturating_exponent("123"sv, true), 123);
    EXPECT_EQ(parse_saturating_exponent("123"sv, false), -123);
    EXPECT_EQ(parse_saturating_exponent(L"123"sv, true), 123);

    // max_exponent_magnitude is 2^59
    EXPECT_EQ(parse_saturating_exponent("576460752303423487"sv, true),
              max_exponent_magnitude - 1);
    EXPECT_EQ(parse_saturating_exponent("576460752303423488"sv, true),
              max_exponent_magnitude);
    EXPECT_EQ(parse_saturating_exponent("99999999999999999999"sv, true),
              max_exponent_magnitude);
    EXPECT_EQ(parse_saturating_exponent("99999999999999999999"sv, false),
              -max_exponent_magnitude);
    EXPECT_EQ(parse_saturating_exponent(std::string_view{std::string(400, '9')},
                                        true),
              max_exponent_magnitude);
}

// A long run of digits mustn't cancel out a saturated exponent.
// The digit offsets below stand for inputs like
// "0." + (2^30 - 5 zeroes) + "1e99999999999",
// which would be too large to test with.
TEST(SaturatingExponentTest, AdjustedBySignificandDigits)
{
    using scn::impl::adjusted_exponent;
    using scn::impl::max_exponent_magnitude;
    // Way beyond the exponent range of any floating-point type
    constexpr auto out_of_range = std::int64_t{1} << 30;

    const auto make_parts = [](std::string_view exponent, bool positive) {
        scn::impl::float_parts<char> parts{};
        parts.exponent = exponent;
        parts.exponent_positive = positive;
        return parts;
    };

    EXPECT_EQ(adjusted_exponent(make_parts("10", true), -3), 7);
    EXPECT_EQ(adjusted_exponent(make_parts("10", false), 3), -7);

    // Decimal: `digit_offset` is the number of digits before the radix point,
    // or minus the number of zeroes right after it
    const auto many_digits = (std::int64_t{1} << 30) - 5;
    EXPECT_EQ(adjusted_exponent(make_parts("99999999999", true), -many_digits),
              99999999999 - many_digits);
    EXPECT_EQ(adjusted_exponent(make_parts("99999999999", false), many_digits),
              -99999999999 + many_digits);
    EXPECT_GT(adjusted_exponent(make_parts("99999999999", true), -many_digits),
              out_of_range);
    EXPECT_EQ(adjusted_exponent(make_parts("99999999999999999999", true),
                                -many_digits),
              max_exponent_magnitude - many_digits);
    EXPECT_EQ(adjusted_exponent(make_parts("99999999999999999999", false),
                                many_digits),
              -max_exponent_magnitude + many_digits);

    // Hex: `digit_offset` is 4 times the (negated) number of digits after the
    // radix point
    const auto many_hex_digits = (std::int64_t{1} << 28) - 1;
    EXPECT_EQ(adjusted_exponent(make_parts("99999999999", true),
                                -4 * many_hex_digits),
              99999999999 - 4 * many_hex_digits);
    EXPECT_GT(adjusted_exponent(make_parts("99999999999", true),
                                -4 * many_hex_digits),
              out_of_range);
    EXPECT_EQ(adjusted_exponent(make_parts("99999999999999999999", false),
                                4 * many_hex_digits),
              -max_exponent_magnitude + 4 * many_hex_digits);
}

TEST_F(HighPrecisionDecimalTest, PopulateClampsDecimalPoint)
{
    using hpd_type = scn::impl::high_precision_decimal;
    constexpr auto max = hpd_type::max_decimal_point_magnitude;

    scn::impl::float_parts<char> parts{};
    parts.before_radix_point = "12";
    parts.after_radix_point = "5";
    parts.exponent = "3";
    {
        hpd_type hpd{};
        hpd_type::populate(hpd, parts);
        EXPECT_EQ(hpd.num_digits, 3);
        EXPECT_EQ(hpd.decimal_point, 5);
    }

    parts.exponent = "99999999999";
    {
        hpd_type hpd{};
        hpd_type::populate(hpd, parts);
        EXPECT_EQ(hpd.decimal_point, max);
    }

    parts.exponent_positive = false;
    {
        hpd_type hpd{};
        hpd_type::populate(hpd, parts);
        EXPECT_EQ(hpd.decimal_point, -max);
    }
}

struct SimpleDecimalConversionTest : testing::Test {
    scn::impl::float_rounding_guard rounding_guard{};
};

TEST_F(SimpleDecimalConversionTest, One)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {1, 0};
    hpd.num_digits = 2;
    hpd.decimal_point = 1;

    float f{};
    EXPECT_TRUE(scn::impl::simple_decimal_conversion(f, hpd));
    EXPECT_TRUE(check_floating_eq(f, 1.0f));
}

TEST_F(SimpleDecimalConversionTest, Pi)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {3, 1, 4};
    hpd.num_digits = 3;
    hpd.decimal_point = 1;

    float f{};
    EXPECT_TRUE(scn::impl::simple_decimal_conversion(f, hpd));
    EXPECT_TRUE(check_floating_eq(f, 3.14f));
}

TEST_F(SimpleDecimalConversionTest, PlanckConstant)
{
    auto hpd = scn::impl::high_precision_decimal{};
    hpd.digits = {6, 6, 2, 6, 0, 7, 0, 1, 5};
    hpd.num_digits = 9;
    hpd.decimal_point = -33;

    double d{};
    EXPECT_TRUE(scn::impl::simple_decimal_conversion(d, hpd));
    EXPECT_TRUE(check_floating_eq(d, 6.62607015e-34));
    EXPECT_TRUE(check_floating_eq(d, 0x1.b860bde023111p-111));
}

TEST(FloatConversionTest, NanPayloadSupport)
{
    using scn::impl::float_conversion::supports_nan_payload;
    constexpr bool expected = !SCN_DISABLE_EKNAN;

    EXPECT_EQ(supports_nan_payload<float>, expected);
    EXPECT_EQ(supports_nan_payload<double>, expected);
    EXPECT_EQ(supports_nan_payload<long double>, expected);
#if SCN_HAS_STD_F16
    EXPECT_EQ(supports_nan_payload<std::float16_t>, expected);
#endif
#if SCN_HAS_STD_F32
    EXPECT_EQ(supports_nan_payload<std::float32_t>, expected);
#endif
#if SCN_HAS_STD_F64
    EXPECT_EQ(supports_nan_payload<std::float64_t>, expected);
#endif
#if SCN_HAS_STD_F128
    EXPECT_EQ(supports_nan_payload<std::float128_t>, expected);
#endif
#if SCN_HAS_STD_BF16
    EXPECT_EQ(supports_nan_payload<std::bfloat16_t>, expected);
#endif
}
