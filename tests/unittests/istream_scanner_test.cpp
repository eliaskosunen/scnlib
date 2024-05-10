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

#include <scn/istream.h>

#include "wrapped_gtest.h"

#include <istream>

struct has_istream_operator {
    int i{};

    friend std::istream& operator>>(std::istream& is, has_istream_operator& val)
    {
        return is >> val.i;
    }
};
template <typename CharT>
struct scn::scanner<has_istream_operator, CharT>
    : public scn::basic_istream_scanner<CharT> {};

TEST(IstreamScannerTest, HasIstreamOperator)
{
    auto result = scn::scan<has_istream_operator>("42", "{}");
    ASSERT_TRUE(result);
    const auto& [val] = result->values();
    EXPECT_EQ(val.i, 42);
}
TEST(IstreamScannerTest, OtherValues)
{
    auto result = scn::scan<has_istream_operator, has_istream_operator,
                            has_istream_operator>("123 456 789", "{} {} {}");
    ASSERT_TRUE(result);
    const auto& [a, b, c] = result->values();
    EXPECT_EQ(a.i, 123);
    EXPECT_EQ(b.i, 456);
    EXPECT_EQ(c.i, 789);
}
