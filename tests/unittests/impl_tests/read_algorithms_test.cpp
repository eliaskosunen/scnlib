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

#include <scn/impl.h>

#include <deque>

using namespace std::string_view_literals;

// read_all

static auto make_non_contiguous_buffer_range(std::string_view in)
{
    static std::deque<char> mem;
    mem.clear();
    std::copy(in.begin(), in.end(), std::back_inserter(mem));

    static std::optional<
        scn::detail::basic_scan_forward_buffer_impl<std::deque<char>>>
        buffer{};
    buffer.reset();
    buffer.emplace(mem);

    return scn::ranges::subrange{
        scn::detail::basic_scan_buffer<char>::forward_iterator{&*buffer, 0},
        scn::ranges::default_sentinel};
}

TEST(ReadAllTest, Contiguous)
{
    auto src = "foo"sv;
    auto it = scn::impl::read_all(src);
    EXPECT_EQ(it, src.end());
}
TEST(ReadAllTest, NonContiguous)
{
    auto src = make_non_contiguous_buffer_range("foo");
    auto it = scn::impl::read_all(src);
    EXPECT_EQ(it, src.end());
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
    auto src = make_non_contiguous_buffer_range("foo");
    auto it = scn::impl::read_code_unit(src);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin()));
}
TEST(ReadCodeUnitTest, ContiguousEnd)
{
    auto src = ""sv;
    auto it = scn::impl::read_code_unit(src);
    ASSERT_FALSE(it);
}
TEST(ReadCodeUnitTest, NonContiguousEnd)
{
    auto src = make_non_contiguous_buffer_range("");
    auto it = scn::impl::read_code_unit(src);
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
    auto src = make_non_contiguous_buffer_range("foo");
    auto it = scn::impl::read_exactly_n_code_units(src, 3);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
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
    auto src = make_non_contiguous_buffer_range("foo");
    auto it = scn::impl::read_exactly_n_code_units(src, 2);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin(), 2));
}
TEST(ReadExactlyNCodeUnitsTest, ReadMoreContiguous)
{
    auto src = "foo"sv;
    auto it = scn::impl::read_exactly_n_code_units(src, 4);
    ASSERT_FALSE(it);
}
TEST(ReadExactlyNCodeUnitsTest, ReadMoreNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("foo");
    auto it = scn::impl::read_exactly_n_code_units(src, 4);
    ASSERT_FALSE(it);
}

// read_code_point(_into)

TEST(ReadCodePointIntoTest, SingleCodeUnitCodePointFromContiguous)
{
    auto src = "ab"sv;
    auto [it, cp] = scn::impl::read_code_point_into(src);
    EXPECT_EQ(it, src.begin() + 1);
    EXPECT_EQ(cp, "a");
}
TEST(ReadCodePointIntoTest, SingleCodeUnitCodePointFromNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("ab");
    auto [it, cp] = scn::impl::read_code_point_into(src);
    EXPECT_EQ(it, scn::ranges::next(src.begin()));
    EXPECT_EQ(cp, "a"sv);
}
TEST(ReadCodePointIntoTest, MultipleCodeUnitCodePointFromContiguous)
{
    auto src = "äö"sv;
    auto [it, cp] = scn::impl::read_code_point_into(src);
    EXPECT_EQ(it, src.begin() + 2);
    EXPECT_EQ(cp, "ä"sv);
}
TEST(ReadCodePointIntoTest, MultipleCodeUnitCodePointFromNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("äö");
    auto [it, cp] = scn::impl::read_code_point_into(src);
    EXPECT_EQ(it, scn::ranges::next(src.begin(), 2));
    EXPECT_EQ(cp, "ä"sv);
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
    auto src = make_non_contiguous_buffer_range("aäö");
    auto it = scn::impl::read_exactly_n_code_points(src, 3);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, src.end());
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
    auto src = make_non_contiguous_buffer_range("aäö");
    auto it = scn::impl::read_exactly_n_code_points(src, 2);
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin(), 3));
}
TEST(ReadExactlyNCodePointsTest, ReadMoreContiguous)
{
    auto src = "aäö"sv;
    auto it = scn::impl::read_exactly_n_code_points(src, 4);
    ASSERT_FALSE(it);
}
TEST(ReadExactlyNCodePointsTest, ReadMoreNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("aäö");
    auto it = scn::impl::read_exactly_n_code_points(src, 4);
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
    EXPECT_EQ(*it, ' ');
}
TEST(ReadUntilCodeUnit, ReadSomeNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("a b");
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    EXPECT_EQ(*it, ' ');
}
TEST(ReadUntilCodeUnit, ReadNoneContiguous)
{
    auto src = " ab"sv;
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(*it, ' ');
}
TEST(ReadUntilCodeUnit, ReadNoneNonContiguous)
{
    auto src = make_non_contiguous_buffer_range(" ab");
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(*it, ' ');
}
TEST(ReadUntilCodeUnit, ReadAllContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    EXPECT_EQ(it, src.end());
}
TEST(ReadUntilCodeUnit, ReadAllNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("abc");
    auto it = scn::impl::read_until_code_unit(src, is_literal_space);
    EXPECT_EQ(it, src.end());
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
    EXPECT_EQ(*it, ' ');
}
TEST(ReadWhileCodeUnit, ReadSomeNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("a b");
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    EXPECT_EQ(*it, ' ');
}
TEST(ReadWhileCodeUnit, ReadNoneContiguous)
{
    auto src = " ab"sv;
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(*it, ' ');
}
TEST(ReadWhileCodeUnit, ReadNoneNonContiguous)
{
    auto src = make_non_contiguous_buffer_range(" ab");
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(*it, ' ');
}
TEST(ReadWhileCodeUnit, ReadAllContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    EXPECT_EQ(it, src.end());
}
TEST(ReadWhileCodeUnit, ReadAllNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("abc");
    auto it = scn::impl::read_while_code_unit(src, is_not_literal_space);
    EXPECT_EQ(it, src.end());
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

constexpr bool is_smiling_emoji(char32_t cp)
{
    return cp == 0x1f60a;  // 😊
}

TEST(ReadUntilCodePoint, ReadSomeContiguous)
{
    auto src = "a😊b"sv;
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    EXPECT_EQ(it, src.begin() + 1);
}
TEST(ReadUntilCodePoint, ReadSomeNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("a😊b");
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    EXPECT_EQ(it, scn::ranges::next(src.begin(), 1));
}
TEST(ReadUntilCodePoint, ReadNoneContiguous)
{
    auto src = "😊ab"sv;
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    EXPECT_EQ(it, src.begin());
}
TEST(ReadUntilCodePoint, ReadNoneNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("😊ab");
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    EXPECT_EQ(it, src.begin());
}
TEST(ReadUntilCodePoint, ReadAllContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    EXPECT_EQ(it, src.end());
}
TEST(ReadUntilCodePoint, ReadAllNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("abc");
    auto it = scn::impl::read_until_code_point(src, is_smiling_emoji);
    EXPECT_EQ(it, src.end());
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
    auto src = make_non_contiguous_buffer_range("abc");
    auto it = scn::impl::read_matching_code_unit(src, 'a');
    ASSERT_TRUE(it);
    EXPECT_EQ(*it, scn::ranges::next(src.begin(), 1));
    EXPECT_EQ(**it, 'b');
}
TEST(ReadMatchingCodeUnit, NoMatchContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_matching_code_unit(src, 'b');
    ASSERT_FALSE(it);
}
TEST(ReadMatchingCodeUnit, NoMatchNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("abc");
    auto it = scn::impl::read_matching_code_unit(src, 'b');
    ASSERT_FALSE(it);
}

TEST(ReadWhileClassicSpace, SingleMatchContiguous)
{
    auto src = " abc"sv;
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.begin() + 1);
    EXPECT_EQ(*it, 'a');
}
TEST(ReadWhileClassicSpace, SingleMatchNonContiguous)
{
    auto src = make_non_contiguous_buffer_range(" abc");
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, scn::ranges::next(src.begin(), 1));
    EXPECT_EQ(*it, 'a');
}
TEST(ReadWhileClassicSpace, NoMatchContiguous)
{
    auto src = "abc"sv;
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(*it, 'a');
}
TEST(ReadWhileClassicSpace, NoMatchNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("abc");
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(*it, 'a');
}
TEST(ReadWhileClassicSpace, MatchAllContiguous)
{
    auto src = "   "sv;
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.end());
}
TEST(ReadWhileClassicSpace, MatchAllNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("   ");
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.end());
}
TEST(ReadWhileClassicSpace, EmptyContiguous)
{
    auto src = ""sv;
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(it, src.end());
}
TEST(ReadWhileClassicSpace, EmptyNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("");
    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(it, src.end());
}
TEST(ReadWhileClassicSpace, RepeatedNonContiguous)
{
    auto src = make_non_contiguous_buffer_range("0\n0");

    auto it = scn::impl::read_while_classic_space(src);
    EXPECT_EQ(it, src.begin());
    EXPECT_EQ(*it, '0');

    ++it;
    it = scn::impl::read_while_classic_space(
        scn::ranges::subrange{it, src.end()});
    EXPECT_NE(it, src.end());
    EXPECT_EQ(*it, '0');

    ++it;
    it = scn::impl::read_while_classic_space(
        scn::ranges::subrange{it, src.end()});
    EXPECT_EQ(it, src.end());
}
