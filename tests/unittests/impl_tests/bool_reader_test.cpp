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

#include "reader_test_common.h"

#include <scn/impl.h>

using namespace std::string_view_literals;

struct classic_tag {};
struct localized_tag {};

template <typename T>
class BoolReaderTest : public testing::Test {
protected:
    static constexpr bool is_localized = std::is_same_v<T, localized_tag>;

    static auto read_default(std::string_view src, bool& val)
    {
#if !SCN_DISABLE_LOCALE
        if constexpr (is_localized) {
            return scn::impl::bool_reader<char>{}.read_localized(src, {}, val);
        }
        else
#endif
        {
            return scn::impl::bool_reader<char>{}.read_classic(src, val);
        }
    }
};

using type_list = testing::Types<classic_tag, localized_tag>;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

TYPED_TEST_SUITE(BoolReaderTest, type_list);

SCN_CLANG_POP

TYPED_TEST(BoolReaderTest, DefaultTextualTrue)
{
    auto src = "true abc"sv;
    bool val{};
    auto ret = this->read_default(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 4);
    EXPECT_TRUE(val);
}
TYPED_TEST(BoolReaderTest, DefaultTextualFalse)
{
    auto src = "false abc"sv;
    bool val{};
    auto ret = this->read_default(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 5);
    EXPECT_FALSE(val);
}
TYPED_TEST(BoolReaderTest, DefaultTextualNonsense)
{
    auto src = "foobar abc"sv;
    bool val{};
    auto ret = this->read_default(src, val);

    ASSERT_FALSE(ret);
}

TYPED_TEST(BoolReaderTest, DefaultNumericTrue)
{
    auto src = "1 abc"sv;
    bool val{};
    auto ret = this->read_default(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 1);
    EXPECT_TRUE(val);
}
TYPED_TEST(BoolReaderTest, DefaultNumericFalse)
{
    auto src = "0 abc"sv;
    bool val{};
    auto ret = this->read_default(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 1);
    EXPECT_FALSE(val);
}
TYPED_TEST(BoolReaderTest, DefaultNumericFalsePrefix)
{
    auto src = "01abc"sv;
    bool val{};
    auto ret = this->read_default(src, val);

    ASSERT_TRUE(ret);
    EXPECT_EQ(*ret, src.begin() + 1);
    EXPECT_FALSE(val);
}
TYPED_TEST(BoolReaderTest, DefaultNumericNonsense)
{
    auto src = "2 abc"sv;
    bool val{};
    auto ret = this->read_default(src, val);

    ASSERT_FALSE(ret);
}
