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

TEST(FloatTest, FloatWithDoubleSign) {
    auto result = scn::scan<double>("--4", "{}");
    ASSERT_FALSE(result);
}
