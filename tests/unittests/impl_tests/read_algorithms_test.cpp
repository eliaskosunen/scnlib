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

#include "../wrapped_gtest.h"

#include <scn/detail/erased_range.h>
#include <scn/impl/algorithms/read.h>

using namespace std::string_view_literals;

// read_all

TEST(ReadAllTest, Contiguous)
{
    auto src = "foo"sv;
    auto it = scn::impl::read_all(src);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadAllTest, NonContiguous)
{
    auto src = scn::erased_range{"foo"sv};
    auto it = scn::impl::read_all(src);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadAllTest, NonBorrowed)
{
    auto it = scn::impl::read_all(scn::erased_range{"foo"sv});
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(it),
                                 scn::scan_expected<scn::ranges::dangling>>);
}

// read_code_unit

TEST(ReadCodeUnitTest, Contiguous)
{
    auto src = "foo"sv;
    auto it = scn::impl::read_code_unit(src);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin() + 1);
}
TEST(ReadCodeUnitTest, NonContiguous)
{
    auto src = scn::erased_range{"foo"sv};
    auto it = scn::impl::read_code_unit(src);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin()));
}
TEST(ReadCodeUnitTest, NonBorrowed)
{
    auto it = scn::impl::read_code_unit(scn::erased_range{"foo"sv});
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(it),
                                 scn::scan_expected<scn::ranges::dangling>>);
}
TEST(ReadCodeUnitTest, ContiguousEnd)
{
    auto src = ""sv;
    auto it = scn::impl::read_code_unit(src);
    ASSERT_FALSE(it);
}
TEST(ReadCodeUnitTest, NonContiguousEnd)
{
    auto src = scn::erased_range{""sv};
    auto it = scn::impl::read_code_unit(src);
    ASSERT_FALSE(it);
}
TEST(ReadCodeUnitTest, NonBorrowedEnd)
{
    auto it = scn::impl::read_code_unit(scn::erased_range{""sv});
    ASSERT_FALSE(it);
}

// read_exactly_n_code_units

TEST(ReadExactlyNCodeUnitsTest, ReadAllContiguous)
{
    auto src = "foo"sv;
    auto it = scn::impl::read_exactly_n_code_units(src, 3);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadExactlyNCodeUnitsTest, ReadAllNonContiguous)
{
    auto src = scn::erased_range{"foo"sv};
    auto it = scn::impl::read_exactly_n_code_units(src, 3);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadExactlyNCodeUnitsTest, ReadAllNonBorrowed)
{
    auto it =
        scn::impl::read_exactly_n_code_units(scn::erased_range{"foo"sv}, 3);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(it),
                                 scn::scan_expected<scn::ranges::dangling>>);
}
TEST(ReadExactlyNCodeUnitsTest, ReadLessContiguous)
{
    auto src = "foo"sv;
    auto it = scn::impl::read_exactly_n_code_units(src, 2);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin() + 2);
}
TEST(ReadExactlyNCodeUnitsTest, ReadLessNonContiguous)
{
    auto src = scn::erased_range{"foo"sv};
    auto it = scn::impl::read_exactly_n_code_units(src, 2);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin(), 2));
}
TEST(ReadExactlyNCodeUnitsTest, ReadLessNonBorrowed)
{
    auto it =
        scn::impl::read_exactly_n_code_units(scn::erased_range{"foo"sv}, 2);
    ASSERT_TRUE(it);
}
TEST(ReadExactlyNCodeUnitsTest, ReadMoreContiguous)
{
    auto src = "foo"sv;
    auto it = scn::impl::read_exactly_n_code_units(src, 4);
    ASSERT_FALSE(it);
}
TEST(ReadExactlyNCodeUnitsTest, ReadMoreNonContiguous)
{
    auto src = scn::erased_range{"foo"sv};
    auto it = scn::impl::read_exactly_n_code_units(src, 4);
    ASSERT_FALSE(it);
}
TEST(ReadExactlyNCodeUnitsTest, ReadMoreNonBorrowed)
{
    auto it =
        scn::impl::read_exactly_n_code_units(scn::erased_range{"foo"sv}, 4);
    ASSERT_FALSE(it);
}

// read_code_point(_into)

TEST(ReadCodePointIntoTest, SingleCodeUnitCodePointFromContiguous)
{
    auto src = "ab"sv;
    auto ret = scn::impl::read_code_point_into(src);
    ASSERT_TRUE(ret);
    auto [it, cp] = *ret;
    EXPECT_EQ(it, src.begin() + 1);
    EXPECT_EQ(cp.view(), "a"sv);
    EXPECT_FALSE(cp.stores_allocated_string());
}
TEST(ReadCodePointIntoTest, SingleCodeUnitCodePointFromNonContiguous)
{
    auto src = scn::erased_range{"ab"sv};
    auto ret = scn::impl::read_code_point_into(src);
    ASSERT_TRUE(ret);
    auto [it, cp] = *ret;
    EXPECT_EQ(it, scn::ranges::next(src.begin()));
    EXPECT_EQ(cp.view(), "a"sv);
    EXPECT_TRUE(cp.stores_allocated_string());
}
TEST(ReadCodePointIntoTest, SingleCodeUnitCodePointFromNonBorrowed)
{
    auto ret = scn::impl::read_code_point_into(scn::erased_range{"ab"sv});
    ASSERT_TRUE(ret);
    auto [it, cp] = *ret;
    static_assert(std::is_same_v<decltype(it), scn::ranges::dangling>);
    EXPECT_EQ(cp.view(), "a"sv);
    EXPECT_TRUE(cp.stores_allocated_string());
}
TEST(ReadCodePointIntoTest, MultipleCodeUnitCodePointFromContiguous)
{
    auto src = "äö"sv;
    auto ret = scn::impl::read_code_point_into(src);
    ASSERT_TRUE(ret);
    auto [it, cp] = *ret;
    EXPECT_EQ(it, src.begin() + 2);
    EXPECT_EQ(cp.view(), "ä"sv);
    EXPECT_FALSE(cp.stores_allocated_string());
}
TEST(ReadCodePointIntoTest, MultipleCodeUnitCodePointFromNonContiguous)
{
    auto src = scn::erased_range{"äö"sv};
    auto ret = scn::impl::read_code_point_into(src);
    ASSERT_TRUE(ret);
    auto [it, cp] = *ret;
    EXPECT_EQ(it, scn::ranges::next(src.begin(), 2));
    EXPECT_EQ(cp.view(), "ä"sv);
    EXPECT_TRUE(cp.stores_allocated_string());
}
TEST(ReadCodePointIntoTest, MultipleCodeUnitCodePointFromNonBorrowed)
{
    auto ret = scn::impl::read_code_point_into(scn::erased_range{"äö"sv});
    ASSERT_TRUE(ret);
    auto [it, cp] = *ret;
    static_assert(std::is_same_v<decltype(it), scn::ranges::dangling>);
    EXPECT_EQ(cp.view(), "ä"sv);
    EXPECT_TRUE(cp.stores_allocated_string());
}

// read_exactly_n_code_points

TEST(ReadExactlyNCodePointsTest, ReadAllContiguous)
{
    auto src = "aäö"sv;
    auto it = scn::impl::read_exactly_n_code_points(src, 3);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadExactlyNCodePointsTest, ReadAllNonContiguous)
{
    auto src = scn::erased_range{"aäö"sv};
    auto it = scn::impl::read_exactly_n_code_points(src, 3);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadExactlyNCodePointsTest, ReadAllNonBorrowed)
{
    auto it =
        scn::impl::read_exactly_n_code_points(scn::erased_range{"aäö"sv}, 3);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(it),
                                 scn::scan_expected<scn::ranges::dangling>>);
}
TEST(ReadExactlyNCodePointsTest, ReadLessContiguous)
{
    auto src = "aäö"sv;
    auto it = scn::impl::read_exactly_n_code_points(src, 2);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin() + 3);
}
TEST(ReadExactlyNCodePointsTest, ReadLessNonContiguous)
{
    auto src = scn::erased_range{"aäö"sv};
    auto it = scn::impl::read_exactly_n_code_points(src, 2);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin(), 3));
}
TEST(ReadExactlyNCodePointsTest, ReadLessNonBorrowed)
{
    auto it =
        scn::impl::read_exactly_n_code_points(scn::erased_range{"aäö"sv}, 2);
    ASSERT_TRUE(it);
}
TEST(ReadExactlyNCodePointsTest, ReadMoreContiguous)
{
    auto src = "aäö"sv;
    auto it = scn::impl::read_exactly_n_code_points(src, 4);
    ASSERT_FALSE(it);
}
TEST(ReadExactlyNCodePointsTest, ReadMoreNonContiguous)
{
    auto src = scn::erased_range{"aäö"sv};
    auto it = scn::impl::read_exactly_n_code_points(src, 4);
    ASSERT_FALSE(it);
}
TEST(ReadExactlyNCodePointsTest, ReadMoreNonBorrowed)
{
    auto it =
        scn::impl::read_exactly_n_code_points(scn::erased_range{"aäö"sv}, 4);
    ASSERT_FALSE(it);
}

// read_until_code_unit

constexpr bool is_literal_space(char ch)
{
    return ch == ' ';
}

TEST(ReadUntilCodeUnit, ReadSomeContiguous)
{
    auto src = "a b"sv;
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(**it, ' ');
}
TEST(ReadUntilCodeUnit, ReadSomeNonContiguous)
{
    auto src = scn::erased_range{"a b"sv};
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(**it, ' ');
}
TEST(ReadUntilCodeUnit, ReadSomeNonBorrowed)
{
    auto it = scn::impl::read_until_code_unit(scn::erased_range{"a b"sv},
                                              is_literal_space);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
TEST(ReadUntilCodeUnit, ReadNoneContiguous)
{
    auto src = " ab"sv;
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin());
    EXPECT_EQ(**it, ' ');
}
TEST(ReadUntilCodeUnit, ReadNoneNonContiguous)
{
    auto src = scn::erased_range{" ab"sv};
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin());
    EXPECT_EQ(**it, ' ');
}
TEST(ReadUntilCodeUnit, ReadNoneNonBorrowed)
{
    auto it = scn::impl::read_until_code_unit(scn::erased_range{" ab"sv},
                                              is_literal_space);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
TEST(ReadUntilCodeUnit, ReadAllContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadUntilCodeUnit, ReadAllNonContiguous)
{
    auto src = scn::erased_range{"abc"sv};
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadUntilCodeUnit, ReadAllNonBorrowed)
{
    auto it = scn::impl::read_until_code_unit(scn::erased_range{"abc"sv},
                                              is_literal_space);
    ASSERT_TRUE(it);
}

// read_while_code_unit

constexpr bool is_not_literal_space(char ch)
{
    return ch != ' ';
}

TEST(ReadWhileCodeUnit, ReadSomeContiguous)
{
    auto src = "a b"sv;
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(**it, ' ');
}
TEST(ReadWhileCodeUnit, ReadSomeNonContiguous)
{
    auto src = scn::erased_range{"a b"sv};
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(**it, ' ');
}
TEST(ReadWhileCodeUnit, ReadSomeNonBorrowed)
{
    auto it = scn::impl::read_while_code_unit(scn::erased_range{"a b"sv},
                                              is_not_literal_space);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
TEST(ReadWhileCodeUnit, ReadNoneContiguous)
{
    auto src = " ab"sv;
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin());
    EXPECT_EQ(**it, ' ');
}
TEST(ReadWhileCodeUnit, ReadNoneNonContiguous)
{
    auto src = scn::erased_range{" ab"sv};
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin());
    EXPECT_EQ(**it, ' ');
}
TEST(ReadWhileCodeUnit, ReadNoneNonBorrowed)
{
    auto it = scn::impl::read_while_code_unit(scn::erased_range{" ab"sv},
                                              is_not_literal_space);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
TEST(ReadWhileCodeUnit, ReadAllContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadWhileCodeUnit, ReadAllNonContiguous)
{
    auto src = scn::erased_range{"abc"sv};
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadWhileCodeUnit, ReadAllNonBorrowed)
{
    auto it = scn::impl::read_while_code_unit(scn::erased_range{"abc"sv},
                                              is_not_literal_space);
    ASSERT_TRUE(it);
}

// read_until1_code_unit

TEST(ReadUntil1CodeUnit, ReadAll)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_until1_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadUntil1CodeUnit, ReadOne)
{
    auto src = "a b"sv;
    auto it = scn::impl::read_until1_code_unit(src, is_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin() + 1);
    EXPECT_EQ(**it, ' ');
}
TEST(ReadUntil1CodeUnit, ReadNone)
{
    auto src = " ab"sv;
    auto it = scn::impl::read_until1_code_unit(src, is_literal_space);
    ASSERT_FALSE(it);
}

// read_while1_code_unit

TEST(ReadWhile1CodeUnit, ReadAll)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_while1_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadWhile1CodeUnit, ReadOne)
{
    auto src = "a b"sv;
    auto it = scn::impl::read_while1_code_unit(src, is_not_literal_space);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin() + 1);
    EXPECT_EQ(**it, ' ');
}
TEST(ReadWhile1CodeUnit, ReadNone)
{
    auto src = " ab"sv;
    auto it = scn::impl::read_while1_code_unit(src, is_not_literal_space);
    ASSERT_FALSE(it);
}

// read_until_code_point

constexpr bool is_smiling_emoji(scn::code_point cp)
{
    return cp == scn::code_point{0x1f60a};  // 😊
}

TEST(ReadUntilCodePoint, ReadSomeContiguous)
{
    auto src = "a😊b"sv;
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin() + 1);
}
TEST(ReadUntilCodePoint, ReadSomeNonContiguous)
{
    auto src = scn::erased_range{"a😊b"sv};
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin(), 1));
}
TEST(ReadUntilCodePoint, ReadSomeNonBorrowed)
{
    auto it = scn::impl::read_until_code_point(scn::erased_range{"a😊b"sv},
                                               is_smiling_emoji);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
TEST(ReadUntilCodePoint, ReadNoneContiguous)
{
    auto src = "😊ab"sv;
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin());
}
TEST(ReadUntilCodePoint, ReadNoneNonContiguous)
{
    auto src = scn::erased_range{"😊ab"sv};
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin());
}
TEST(ReadUntilCodePoint, ReadNoneNonBorrowed)
{
    auto it = scn::impl::read_until_code_point(scn::erased_range{"😊ab"sv},
                                               is_smiling_emoji);
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
TEST(ReadUntilCodePoint, ReadAllContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadUntilCodePoint, ReadAllNonContiguous)
{
    auto src = scn::erased_range{"abc"sv};
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
}
TEST(ReadUntilCodePoint, ReadAllNonBorrowed)
{
    auto it = scn::impl::read_until_code_point(scn::erased_range{"abc"sv},
                                               is_smiling_emoji);
    ASSERT_TRUE(it);
}

// read_matching_code_unit

TEST(ReadMatchingCodeUnit, MatchContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_matching_code_unit(src, 'a');
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.begin() + 1);
    EXPECT_EQ(**it, 'b');
}
TEST(ReadMatchingCodeUnit, MatchNonContiguous)
{
    auto src = scn::erased_range{"abc"sv};
    auto it = scn::impl::read_matching_code_unit(src, 'a');
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin(), 1));
    EXPECT_EQ(**it, 'b');
}
TEST(ReadMatchingCodeUnit, MatchNonBorrowed)
{
    auto it =
        scn::impl::read_matching_code_unit(scn::erased_range{"abc"sv}, 'a');
    ASSERT_TRUE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
TEST(ReadMatchingCodeUnit, NoMatchContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_matching_code_unit(src, 'b');
    ASSERT_FALSE(it);
}
TEST(ReadMatchingCodeUnit, NoMatchNonContiguous)
{
    auto src = scn::erased_range{"abc"sv};
    auto it = scn::impl::read_matching_code_unit(src, 'b');
    ASSERT_FALSE(it);
}
TEST(ReadMatchingCodeUnit, NoMatchNonBorrowed)
{
    auto it =
        scn::impl::read_matching_code_unit(scn::erased_range{"abc"sv}, 'b');
    ASSERT_FALSE(it);
    static_assert(std::is_same_v<decltype(*it), scn::ranges::dangling&>);
}
