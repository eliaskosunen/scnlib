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

#include "nan_repr.h"

#include <cmath>

template <typename T>
class NanReprTest : public testing::Test {};

using TypeList = testing::Types<float,
                                double,
                                long double
#if SCN_HAS_STD_F16
                                ,
                                std::float16_t
#endif
#if SCN_HAS_STD_F32
                                ,
                                std::float32_t
#endif
#if SCN_HAS_STD_F64
                                ,
                                std::float64_t
#endif
#if SCN_HAS_STD_F128
                                ,
                                std::float128_t
#endif
#if SCN_HAS_STD_BF16
                                ,
                                std::bfloat16_t
#endif
                                >;

TYPED_TEST_SUITE(NanReprTest, TypeList);

TYPED_TEST(NanReprTest, MakeWithPayload)
{
    auto nan_val = make_nan_with_payload<TypeParam>("32");
    EXPECT_TRUE(std::isnan(nan_val));

    nan_repr<float_kind_for<TypeParam>> repr(nan_val);
    EXPECT_EQ(repr.payload.lo, 32u);
}

TYPED_TEST(NanReprTest, SignComparison_Positive)
{
    const auto val = std::numeric_limits<TypeParam>::quiet_NaN();
    EXPECT_TRUE(check_nan_eq(val, val));
}

TYPED_TEST(NanReprTest, SignComparison_Negative)
{
    const auto val = std::copysign(std::numeric_limits<TypeParam>::quiet_NaN(),
                                   static_cast<TypeParam>(-1.0));
    EXPECT_TRUE(check_nan_eq(val, val));
}

TYPED_TEST(NanReprTest, SignComparison_Differ)
{
    const auto pos = std::numeric_limits<TypeParam>::quiet_NaN();
    const auto neg = std::copysign(std::numeric_limits<TypeParam>::quiet_NaN(),
                                   static_cast<TypeParam>(-1.0));
    EXPECT_FALSE(check_nan_eq(pos, neg));
}

TYPED_TEST(NanReprTest, PayloadComparison_Equal)
{
    const auto val = make_nan_with_payload<TypeParam>("42");
    EXPECT_TRUE(check_nan_eq(val, val));
}

TYPED_TEST(NanReprTest, PayloadComparison_Differ)
{
    const auto nan1 = make_nan_with_payload<TypeParam>("42");
    const auto nan2 = make_nan_with_payload<TypeParam>("43");
    EXPECT_FALSE(check_nan_eq(nan1, nan2));
}

TYPED_TEST(NanReprTest, ToFloatRoundTrip)
{
    nan_repr<float_kind_for<TypeParam>> repr1(
        std::numeric_limits<TypeParam>::quiet_NaN());
    repr1.payload.lo = 48;

    TypeParam nan_val = repr1.template to_float<TypeParam>();
    EXPECT_TRUE(std::isnan(nan_val));

    nan_repr<float_kind_for<TypeParam>> repr2(nan_val);

    EXPECT_EQ(repr2.payload.lo, 48u);
}

#if SCN_HAS_STD_F128
TEST(NanReprF128Test, PayloadRoundTrip)
{
    const auto nan_val = make_nan_with_payload<std::float128_t>("1111");
    EXPECT_TRUE(std::isnan(nan_val));

    nan_repr<float_kind_for<std::float128_t>> repr(nan_val);
    EXPECT_EQ(repr.payload.lo, 1111u);
}

TEST(NanReprF128Test, LargePayload)
{
    nan_repr<float_kind_for<std::float128_t>> repr1(
        std::numeric_limits<std::float128_t>::quiet_NaN());
    repr1.payload.lo = 0xFEDCBA9876543210ULL;
    repr1.payload.hi = 0x123456ULL;

    const auto nan_val = repr1.to_float<std::float128_t>();
    EXPECT_TRUE(std::isnan(nan_val));
    nan_repr<float_kind_for<std::float128_t>> repr2(nan_val);

    EXPECT_EQ(repr2.payload, repr1.payload);
}
#endif

TEST(NanReprParsingTest, HexPayload)
{
    const auto nan_val = make_nan_with_payload<double>("0x1234");
    nan_repr<float_kind_for<double>> repr(nan_val);
    EXPECT_EQ(repr.payload.lo, 0x1234u);
}

TEST(NanReprParsingTest, OctalPayload)
{
    const auto nan_val = make_nan_with_payload<double>("01234");
    nan_repr<float_kind_for<double>> repr(nan_val);
    EXPECT_EQ(repr.payload.lo, 01234u);
}

TEST(NanReprParsingTest, InvalidPayload_FooBar)
{
    const auto nan_val = make_nan_with_payload<double>("Foo_Bar");
    nan_repr<float_kind_for<double>> repr_parsed(nan_val);

    nan_repr<float_kind_for<double>> repr_qnan(
        std::numeric_limits<double>::quiet_NaN());

    EXPECT_EQ(repr_parsed.payload, repr_qnan.payload);
}

TEST(NanReprParsingTest, InvalidPayload_Empty)
{
    auto nan_val = make_nan_with_payload<double>("");
    nan_repr<float_kind_for<double>> repr_parsed(nan_val);

    nan_repr<float_kind_for<double>> repr_qnan(
        std::numeric_limits<double>::quiet_NaN());

    EXPECT_EQ(repr_parsed.payload, repr_qnan.payload);
}
