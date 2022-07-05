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

#include <gtest/gtest.h>

#include <scn/detail/scan.h>

struct mytype {
    int i{}, j{};
};

template <>
struct scn::scanner<mytype, char> : scn::scanner<std::string, char> {
    template <typename Context>
    scn::scan_expected<typename Context::iterator> scan(mytype& val,
                                                        Context& ctx) const
    {
        auto [result, i, j] = scn::scan<int, int>(ctx.range(), "{} {}");
        if (result) {
            val = mytype{i, j};
            return result.range().begin();
        }
        return unexpected(result.error());
    }
};

TEST(CustomTypeTest, Simple)
{
    auto [result, val] = scn::scan<mytype>("123 456", "{}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(val.i, 123);
    EXPECT_EQ(val.j, 456);
}

TEST(CustomTypeTest, Multiple)
{
    auto [result, a, b] = scn::scan<mytype, mytype>("12 34 56 78", "{} {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(a.i, 12);
    EXPECT_EQ(a.j, 34);
    EXPECT_EQ(b.i, 56);
    EXPECT_EQ(b.j, 78);
}

TEST(CustomTypeTest, Surrounded)
{
    auto [result, a, val, b] =
        scn::scan<int, mytype, int>("1 2 3 4", "{} {} {}");
    EXPECT_TRUE(result);
    EXPECT_TRUE(result.range().empty());
    EXPECT_EQ(a, 1);
    EXPECT_EQ(val.i, 2);
    EXPECT_EQ(val.j, 3);
    EXPECT_EQ(b, 4);
}
