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
    if (!result &&
        result.error().code() == scn::scan_error::type_not_supported) {
        GTEST_SKIP();
    }
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_F32
TEST(FloatTest, Float32)
{
    auto result = scn::scan<std::float32_t>("3.14", "{}");
    if (!result &&
        result.error().code() == scn::scan_error::type_not_supported) {
        GTEST_SKIP();
    }
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_F64
TEST(FloatTest, Float64)
{
    auto result = scn::scan<std::float64_t>("3.14", "{}");
    if (!result &&
        result.error().code() == scn::scan_error::type_not_supported) {
        GTEST_SKIP();
    }
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_F128
TEST(FloatTest, Float128)
{
    auto result = scn::scan<std::float128_t>("3.14", "{}");
    if (!result &&
        result.error().code() == scn::scan_error::type_not_supported) {
        GTEST_SKIP();
    }
    ASSERT_TRUE(result);
}
#endif
#if SCN_HAS_STD_BF16
TEST(FloatTest, BFloat16)
{
    auto result = scn::scan<std::bfloat16_t>("3.14", "{}");
    if (!result &&
        result.error().code() == scn::scan_error::type_not_supported) {
        GTEST_SKIP();
    }
    ASSERT_TRUE(result);
}
#endif
