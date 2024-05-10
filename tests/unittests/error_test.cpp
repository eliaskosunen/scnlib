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

TEST(ErrorTest, General)
{
    const auto good = scn::scan_error{};
    const auto eof_error =
        scn::scan_error{scn::scan_error::end_of_range, "EOF"};
    const auto invalid_scanned_value_error =
        scn::scan_error{scn::scan_error::invalid_scanned_value, ""};

    EXPECT_TRUE(good);
    EXPECT_FALSE(eof_error);
    EXPECT_FALSE(invalid_scanned_value_error);

    EXPECT_EQ(good.code(), scn::scan_error::good);
    EXPECT_EQ(eof_error.code(), scn::scan_error::end_of_range);
    EXPECT_EQ(invalid_scanned_value_error.code(),
              scn::scan_error::invalid_scanned_value);

    EXPECT_EQ(good, scn::scan_error::good);
    EXPECT_EQ(eof_error, scn::scan_error::end_of_range);
    EXPECT_EQ(invalid_scanned_value_error,
              scn::scan_error::invalid_scanned_value);
}
