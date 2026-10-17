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

#include "float_classification.h"
#include "test_common.h"

#include <scn/scan.h>
#include <scn/xchar.h>

#include <deque>

TEST(FloatTest, FloatWithSuffix)
{
    auto result = scn::scan<double>("scn::scan for string_view: 0.0075ms",
                                    "scn::scan for string_view: {}ms");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 0.0075);
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(FloatTest, FloatWithDoubleSign)
{
    auto result = scn::scan<double>("--4", "{}");
    ASSERT_THAT(result, Failed());
}

#if SCN_HAS_STD_F16
TEST(FloatTest, Float16)
{
    auto result = scn::scan<std::float16_t>("3.14", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 3.14f16);
    EXPECT_THAT(result->range(), IsEmptyRange());
}
#endif
#if SCN_HAS_STD_F32
TEST(FloatTest, Float32)
{
    auto result = scn::scan<std::float32_t>("3.14", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 3.14f32);
    EXPECT_THAT(result->range(), IsEmptyRange());
}
#endif
#if SCN_HAS_STD_F64
TEST(FloatTest, Float64)
{
    auto result = scn::scan<std::float64_t>("3.14", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 3.14f64);
    EXPECT_THAT(result->range(), IsEmptyRange());
}
#endif
#if SCN_HAS_STD_F128
TEST(FloatTest, Float128)
{
    auto result = scn::scan<std::float128_t>("3.14", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 3.14f128);
    EXPECT_THAT(result->range(), IsEmptyRange());
}
#endif
#if SCN_HAS_STD_BF16
TEST(FloatTest, BFloat16)
{
    auto result = scn::scan<std::bfloat16_t>("3.14", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 3.14bf16);
    EXPECT_THAT(result->range(), IsEmptyRange());
}
#endif

TEST(FloatTest, ScientificNotation)
{
    auto result = scn::scan<double>("1.23e4", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 12300.0);

    result = scn::scan<double>("1.5E-3", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 0.0015);

    result = scn::scan<double>("-2.5e+2", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), -250.0);
}

TEST(FloatTest, HexFloat)
{
    auto result = scn::scan<double>("0x1.8p3", "{:a}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 12.0);

    result = scn::scan<double>("0X1.FFFp10", "{:a}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 2047.75);
}

TEST(FloatTest, Infinity)
{
    auto result = scn::scan<double>("inf", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(classify_float(result->value()).is_inf());
    EXPECT_FALSE(classify_float(result->value()).signbit());

    result = scn::scan<double>("-infinity", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(classify_float(result->value()).is_inf());
    EXPECT_TRUE(classify_float(result->value()).signbit());

    result = scn::scan<double>("+INF", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(classify_float(result->value()).is_inf());
}

TEST(FloatTest, NaN)
{
    auto result = scn::scan<double>("nan", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(classify_float(result->value()).is_nan());

    result = scn::scan<double>("NaN", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(classify_float(result->value()).is_nan());
}

TEST(FloatTest, NaNWithPayloadFromNonContiguousSource)
{
    const std::string_view str = "-nan(0x1_2) ";
    std::deque<char> input(str.begin(), str.end());

    auto result = scn::scan<double>(
        scn::ranges::subrange{input.begin(), input.end()}, "{}");
    ASSERT_THAT(result, Succeeded());
    ASSERT_NE(result->begin(), input.end());
    EXPECT_EQ(*result->begin(), ' ');

    // No payloads for a type without a custom conversion
    constexpr bool has_payloads =
        !SCN_DISABLE_EKNAN && !SCN_FORCE_STRTOD_FALLBACK;
    EXPECT_TRUE(check_nan(
        result->value(), true,
        has_payloads ? std::optional<std::uint64_t>{0x12} : std::nullopt));
}

TEST(FloatTest, PositiveZero)
{
    auto result = scn::scan<double>("0.0", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 0.0);
    EXPECT_FALSE(classify_float(result->value()).signbit());
}

TEST(FloatTest, NegativeZero)
{
    auto result = scn::scan<double>("-0.0", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(classify_float(result->value()).is_zero());
    EXPECT_TRUE(classify_float(result->value()).signbit());
}

TEST(FloatTest, VeryLargeNumber)
{
    auto result = scn::scan<double>("1.7976931348623157e+308", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), std::numeric_limits<double>::max());
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(FloatTest, LongDoubleExponentOutOfIntRange)
{
    auto overflow = scn::scan<long double>("1e2147483647", "{}");
    ASSERT_THAT(overflow, FailedWith(scn::scan_error::value_positive_overflow));

    auto underflow = scn::scan<long double>("1e-2147483648", "{}");
    ASSERT_THAT(underflow,
                FailedWith(scn::scan_error::value_positive_underflow));
}

TEST(FloatTest, VerySmallNumber)
{
    auto result = scn::scan<double>("2.2250738585072014e-308", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_LT(result->value(), 3e-308);
    EXPECT_GT(result->value(), 2e-308);
}

TEST(FloatTest, SubnormalNumber)
{
    auto result = scn::scan<double>("1e-320", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(classify_float(result->value()).is_subnormal());
    EXPECT_FALSE(classify_float(result->value()).signbit());
}

TEST(FloatTest, LeadingZeros)
{
    auto result = scn::scan<double>("00042.5", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 42.5);
}

TEST(FloatTest, TrailingDecimalPoint)
{
    auto result = scn::scan<double>("42.", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 42.0);
}

TEST(FloatTest, LeadingDecimalPoint)
{
    auto result = scn::scan<double>(".5", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 0.5);
}

TEST(FloatTest, MultipleFloats)
{
    auto result = scn::scan<double, double, double>("1.1 2.2 3.3", "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b, c] = result->values();
    EXPECT_DOUBLE_EQ(a, 1.1);
    EXPECT_DOUBLE_EQ(b, 2.2);
    EXPECT_DOUBLE_EQ(c, 3.3);
}

TEST(FloatTest, FloatThenInt)
{
    auto result = scn::scan<double, int>("3.14 42", "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [f, i] = result->values();
    EXPECT_DOUBLE_EQ(f, 3.14);
    EXPECT_EQ(i, 42);
}

TEST(FloatTest, InvalidFloat)
{
    auto result = scn::scan<double>("not_a_number", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(FloatTest, EmptyInput)
{
    auto result = scn::scan<double>("", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

TEST(FloatTest, FloatOverflow)
{
    auto result = scn::scan<float>("1e500", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::value_positive_overflow));
}

TEST(FloatTest, SignedFloat)
{
    auto result = scn::scan<double>("+123.456", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), 123.456);

    result = scn::scan<double>("-789.012", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_DOUBLE_EQ(result->value(), -789.012);
}

TEST(FloatTest, ErrorMessagesDontNameBackend)
{
    const auto expect_neutral = [](const scn::scan_error& error) {
        const std::string_view msg = error.msg();
        EXPECT_EQ(msg.find("fast_float"), std::string_view::npos) << msg;
        EXPECT_EQ(msg.find("from_chars"), std::string_view::npos) << msg;
        EXPECT_EQ(msg.find("strtod"), std::string_view::npos) << msg;
    };

    auto too_large = scn::scan<double>("1e999999999999", "{}");
    ASSERT_THAT(too_large,
                FailedWith(scn::scan_error::value_positive_overflow));
    expect_neutral(too_large.error());

    auto too_small = scn::scan<double>("1e-999999999999", "{}");
    ASSERT_THAT(too_small,
                FailedWith(scn::scan_error::value_positive_underflow));
    expect_neutral(too_small.error());

    auto invalid = scn::scan<double>(".", "{}");
    ASSERT_THAT(invalid, Failed());
    expect_neutral(invalid.error());

    auto no_exponent = scn::scan<double>("1.5", scn::runtime_format("{:e}"));
    ASSERT_THAT(no_exponent, Failed());
    expect_neutral(no_exponent.error());
}

// Different backends parse these (fast_float, std::from_chars, the custom
// parser, or strtold), depending on the character and float type
TEST(FloatTest, ErrorMessagesAreTheSameForEveryBackend)
{
    const auto check = [](std::string_view input,
                          scn::scan_error::code_t expected_code) {
        const auto wide_input = std::wstring(input.begin(), input.end());
        const auto reference = scn::scan<double>(input, "{}");
        ASSERT_THAT(reference, FailedWith(expected_code)) << input;
        const std::string_view expected_msg = reference.error().msg();

        const auto expect_same = [&](const auto& result, const char* label) {
            ASSERT_THAT(result, FailedWith(expected_code))
                << input << " (" << label << ")";
            EXPECT_EQ(std::string_view{result.error().msg()}, expected_msg)
                << input << " (" << label << ")";
        };
        expect_same(scn::scan<float>(input, "{}"), "float");
        expect_same(scn::scan<long double>(input, "{}"), "long double");
        expect_same(scn::scan<double>(wide_input, L"{}"), "wchar_t double");
        expect_same(scn::scan<long double>(wide_input, L"{}"),
                    "wchar_t long double");
    };

    check("1e999999999", scn::scan_error::value_positive_overflow);
    check("0x1p999999999", scn::scan_error::value_positive_overflow);
    check("1e-999999999", scn::scan_error::value_positive_underflow);
    check("0x1p-999999999", scn::scan_error::value_positive_underflow);
}
