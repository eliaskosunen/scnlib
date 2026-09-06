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

TEST(FloatTest, FloatWithSuffix)
{
    auto result = scn::scan<double>("scn::scan for string_view: 0.0075ms",
                                    "scn::scan for string_view: {}ms");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 0.0075);
    EXPECT_TRUE(result->range().empty());
}

TEST(FloatTest, FloatWithDoubleSign)
{
    auto result = scn::scan<double>("--4", "{}");
    ASSERT_FALSE(result);
}

#if SCN_HAS_STD_F16
TEST(FloatTest, Float16)
{
    auto result = scn::scan<std::float16_t>("3.14", "{}");
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_F32
TEST(FloatTest, Float32)
{
    auto result = scn::scan<std::float32_t>("3.14", "{}");
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_F64
TEST(FloatTest, Float64)
{
    auto result = scn::scan<std::float64_t>("3.14", "{}");
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_F128
TEST(FloatTest, Float128)
{
    auto result = scn::scan<std::float128_t>("3.14", "{}");
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_BF16
TEST(FloatTest, BFloat16)
{
    auto result = scn::scan<std::bfloat16_t>("3.14", "{}");
    ASSERT_TRUE(result);
}
#endif

TEST(FloatTest, ScientificNotation)
{
    auto result = scn::scan<double>("1.23e4", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 12300.0);

    result = scn::scan<double>("1.5E-3", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 0.0015);

    result = scn::scan<double>("-2.5e+2", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), -250.0);
}

TEST(FloatTest, HexFloat)
{
    auto result = scn::scan<double>("0x1.8p3", "{:a}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 12.0);

    result = scn::scan<double>("0X1.FFFp10", "{:a}");
    ASSERT_TRUE(result);
}

TEST(FloatTest, Infinity)
{
    auto result = scn::scan<double>("inf", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(std::isinf(result->value()));
    EXPECT_GT(result->value(), 0);

    result = scn::scan<double>("-infinity", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(std::isinf(result->value()));
    EXPECT_LT(result->value(), 0);

    result = scn::scan<double>("+INF", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(std::isinf(result->value()));
}

TEST(FloatTest, NaN)
{
    auto result = scn::scan<double>("nan", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(std::isnan(result->value()));

    result = scn::scan<double>("NaN", "{}");
    ASSERT_TRUE(result);
    EXPECT_TRUE(std::isnan(result->value()));
}

TEST(FloatTest, PositiveZero)
{
    auto result = scn::scan<double>("0.0", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0.0);
    EXPECT_FALSE(std::signbit(result->value()));
}

TEST(FloatTest, NegativeZero)
{
    auto result = scn::scan<double>("-0.0", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value(), 0.0);
    EXPECT_TRUE(std::signbit(result->value()));
}

TEST(FloatTest, VeryLargeNumber)
{
    auto result = scn::scan<double>("1.7976931348623157e+308", "{}");
    ASSERT_TRUE(result);
    EXPECT_GT(result->value(), 1.7e308);
}

TEST(FloatTest, VerySmallNumber)
{
    auto result = scn::scan<double>("2.2250738585072014e-308", "{}");
    ASSERT_TRUE(result);
    EXPECT_LT(result->value(), 3e-308);
    EXPECT_GT(result->value(), 2e-308);
}

TEST(FloatTest, SubnormalNumber)
{
    auto result = scn::scan<double>("1e-320", "{}");
    ASSERT_TRUE(result);
    EXPECT_GT(result->value(), 0.0);
    EXPECT_LT(result->value(), 1e-300);
}

TEST(FloatTest, LeadingZeros)
{
    auto result = scn::scan<double>("00042.5", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 42.5);
}

TEST(FloatTest, TrailingDecimalPoint)
{
    auto result = scn::scan<double>("42.", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 42.0);
}

TEST(FloatTest, LeadingDecimalPoint)
{
    auto result = scn::scan<double>(".5", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 0.5);
}

TEST(FloatTest, MultipleFloats)
{
    auto result = scn::scan<double, double, double>("1.1 2.2 3.3", "{} {} {}");
    ASSERT_TRUE(result);
    auto [a, b, c] = result->values();
    EXPECT_DOUBLE_EQ(a, 1.1);
    EXPECT_DOUBLE_EQ(b, 2.2);
    EXPECT_DOUBLE_EQ(c, 3.3);
}

TEST(FloatTest, FloatThenInt)
{
    auto result = scn::scan<double, int>("3.14 42", "{} {}");
    ASSERT_TRUE(result);
    auto [f, i] = result->values();
    EXPECT_DOUBLE_EQ(f, 3.14);
    EXPECT_EQ(i, 42);
}

TEST(FloatTest, InvalidFloat)
{
    auto result = scn::scan<double>("not_a_number", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_scanned_value);
}

TEST(FloatTest, EmptyInput)
{
    auto result = scn::scan<double>("", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::end_of_input);
}

TEST(FloatTest, FloatOverflow)
{
    auto result = scn::scan<float>("1e500", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::value_positive_overflow);
}

TEST(FloatTest, SignedFloat)
{
    auto result = scn::scan<double>("+123.456", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), 123.456);

    result = scn::scan<double>("-789.012", "{}");
    ASSERT_TRUE(result);
    EXPECT_DOUBLE_EQ(result->value(), -789.012);
}
