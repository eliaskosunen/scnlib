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
