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
        return scn::scan<int, int>(ctx.range(), "{} {}")
            .transform([&](auto result) {
                std::tie(val.i, val.j) = result.values();
                return result.begin();
            });
    }
};

struct mytype2 {
    char ch{};
};

template <>
struct scn::scanner<mytype2, char> : scn::scanner<std::string, char> {
    template <typename Context>
    scn::scan_expected<typename Context::iterator> scan(mytype2& val,
                                                        Context& ctx) const
    {
        return scn::scan<scn::discard<char>, char>(ctx.range(), "{} {}")
            .transform([&](auto result) {
                std::tie(std::ignore, val.ch) = result.values();
                return result.begin();
            });
    }
};

TEST(CustomTypeTest, Simple)
{
    auto result = scn::scan<mytype>("123 456", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto& val = std::get<0>(result->values());
    EXPECT_EQ(val.i, 123);
    EXPECT_EQ(val.j, 456);
}

TEST(CustomTypeTest, DISABLED_CustomFormatString)
{
    auto input = std::string_view{"123 456"};
    auto result = scn::scan<mytype>(input, "{:6}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->begin(), input.end() - 1);
    EXPECT_EQ(result->value().i, 123);
    EXPECT_EQ(result->value().j, 45);
}

TEST(CustomTypeTest, Multiple)
{
    auto result = scn::scan<mytype, mytype>("12 34 56 78", "{} {}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto& [a, b] = result->values();
    EXPECT_EQ(a.i, 12);
    EXPECT_EQ(a.j, 34);
    EXPECT_EQ(b.i, 56);
    EXPECT_EQ(b.j, 78);
}

TEST(CustomTypeTest, Surrounded)
{
    auto result = scn::scan<int, mytype, int>("1 2 3 4", "{} {} {}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto& [a, val, b] = result->values();
    EXPECT_EQ(a, 1);
    EXPECT_EQ(val.i, 2);
    EXPECT_EQ(val.j, 3);
    EXPECT_EQ(b, 4);
}

TEST(CustomTypeTest, WhiteSpaceNotSkipped)
{
    auto result = scn::scan<mytype2>(" abc", "{}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->range().data(), "bc");

    EXPECT_EQ(result->value().ch, 'a');
}
