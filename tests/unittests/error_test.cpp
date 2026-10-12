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

TEST(ErrorTest, General)
{
    const auto eof_error =
        scn::scan_error{scn::scan_error::end_of_input, "EOF"};
    const auto invalid_scanned_value_error =
        scn::scan_error{scn::scan_error::invalid_scanned_value, ""};

    const scn::scan_error::code_t eof_code = eof_error.code();
    EXPECT_EQ(eof_code, scn::scan_error::end_of_input);
    EXPECT_EQ(invalid_scanned_value_error.code(),
              scn::scan_error::invalid_scanned_value);

    EXPECT_EQ(eof_error, scn::scan_error::end_of_input);
    EXPECT_EQ(invalid_scanned_value_error,
              scn::scan_error::invalid_scanned_value);
}

TEST(ErrorTest, ToErrc)
{
    EXPECT_EQ((scn::scan_error{scn::scan_error::end_of_input, ""}.to_errc()),
              std::errc::invalid_argument);
    EXPECT_EQ(
        (scn::scan_error{scn::scan_error::invalid_source_state, ""}.to_errc()),
        std::errc::io_error);
    EXPECT_EQ((scn::scan_error{scn::scan_error::value_positive_overflow, ""}
                   .to_errc()),
              std::errc::result_out_of_range);
}

TEST(ErrorTest, ToErrcUnknownCode)
{
    EXPECT_EQ((scn::scan_error{scn::scan_error::max_error, ""}.to_errc()),
              std::errc::invalid_argument);
    // Not an enumerator, but within the value range of the enumeration
    const auto unknown = static_cast<scn::scan_error::code_t>(
        static_cast<int>(scn::scan_error::max_error) + 1);
    EXPECT_EQ((scn::scan_error{unknown, ""}.to_errc()),
              std::errc::invalid_argument);
}

TEST(ErrorTest, ExpectedVoid)
{
    const auto good = scn::scan_expected<void>{};
    const auto eof_error = scn::scan_expected<void>{
        scn::unexpected(scn::scan_error{scn::scan_error::end_of_input, "EOF"})};
    const auto invalid_scanned_value_error =
        scn::scan_expected<void>{scn::detail::unexpected_scan_error(
            scn::scan_error::invalid_scanned_value, "")};

    EXPECT_THAT(good, Succeeded());
    EXPECT_THAT(eof_error, Failed());
    EXPECT_THAT(invalid_scanned_value_error, Failed());

    EXPECT_EQ(eof_error.error().code(), scn::scan_error::end_of_input);
    EXPECT_EQ(invalid_scanned_value_error.error().code(),
              scn::scan_error::invalid_scanned_value);

    EXPECT_EQ(eof_error.error(), scn::scan_error::end_of_input);
    EXPECT_EQ(invalid_scanned_value_error.error(),
              scn::scan_error::invalid_scanned_value);
}
