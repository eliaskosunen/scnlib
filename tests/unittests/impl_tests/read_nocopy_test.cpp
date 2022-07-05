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

#include <scn/impl/algorithms/read_nocopy.h>

TEST(ReadAllNocopyTest, General)
{
    std::string source{"abcdef"};
    auto result = scn::impl::read_all_nocopy(source);
    EXPECT_EQ(result.iterator, source.end());
    EXPECT_EQ(result.value.data(), source.data());
    EXPECT_EQ(result.value.size(), source.size());
    EXPECT_EQ(result.value, source);
}
TEST(ReadAllNocopyTest, Dangling)
{
    auto result = scn::impl::read_all_nocopy(std::string{"abcdef"});
    static_assert(
        std::is_same_v<decltype(result),
                       scn::impl::iterator_value_result<
                           scn::ranges::dangling, scn::ranges::dangling>>);
}
TEST(ReadAllNocopyTest, StringLiteral)
{
    auto result = scn::impl::read_all_nocopy("abcdef");
    auto result_str = std::string{result.value.data(), result.value.size()};
    EXPECT_STREQ(result_str.c_str(), "abcdef");
}

TEST(ReadNNocopyTest, SmallerN)
{
    std::string source{"abcdef"};
    auto result = scn::impl::read_n_nocopy(source, 3);
    EXPECT_EQ(result.iterator, source.begin() + 3);
    EXPECT_EQ(result.value.data(), source.data());
    EXPECT_EQ(result.value.size(), 3);
    EXPECT_EQ(result.value, "abc");
}
TEST(ReadNNocopyTest, LargerN)
{
    std::string source{"abcdef"};
    auto result = scn::impl::read_n_nocopy(source, 12);
    EXPECT_EQ(result.iterator, source.end());
    EXPECT_EQ(result.value.data(), source.data());
    EXPECT_EQ(result.value.size(), source.size());
    EXPECT_EQ(result.value, source);
}

TEST(ReadUntilAsciiNocopyTest, UntilSpace)
{
    std::string source{"foo bar"};
    auto result = scn::impl::read_until_classic_space_nocopy(source);

    EXPECT_EQ(result.iterator, source.begin() + 3);
    EXPECT_EQ(*result.iterator, ' ');

    EXPECT_EQ(result.value.data(), source.data());
    EXPECT_EQ(result.value.size(), 3);
    EXPECT_EQ(result.value, "foo");
}

TEST(ReadUntilCodeUnitsNocopyTest, UntilOE)
{
    std::string source{"aäö "};
    auto result = scn::impl::read_until_code_units_nocopy(
        source,
        std::vector<char>{static_cast<char>(0xc3), static_cast<char>(0xb6)});

    EXPECT_EQ(result.iterator, source.begin() + 3);
    EXPECT_EQ(*result.iterator, static_cast<char>(0xc3));

    EXPECT_EQ(result.value.data(), source.data());
    EXPECT_EQ(result.value.size(), 3);
    EXPECT_EQ(result.value, "aä");
}
