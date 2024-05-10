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

using namespace std::string_view_literals;

TEST(FindClassicSpaceNarrowFastTest, ShortInput)
{
    auto src = "foo bar"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.begin() + 3);
}
TEST(FindClassicSpaceNarrowFastTest, ShortInputWithNoSpaces)
{
    auto src = "foobar"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.end());
}
TEST(FindClassicSpaceNarrowFastTest, LongerInput)
{
    auto src = "foobarbazhelloworld123 foo"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.end() - 4);
}
TEST(FindClassicSpaceNarrowFastTest, MultipleSpaces)
{
    auto src = "foo bar baz"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.begin() + 3);
}

TEST(FindClassicSpaceNarrowFastTest, WonkyLongInput)
{
    auto src =
        "\360,l\377\377\377\377\377\377\377\377ን ጉሮሮ ?T  ላU\213\230\263\255\341\341ጋed sample plain-te\341\213\265\341"sv;
    ASSERT_EQ(src.size(), 64);

    auto it = scn::impl::find_classic_space_narrow_fast(src);
    EXPECT_EQ(&*it, src.data() + 14);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 24);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 27);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 28);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 44);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 51);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(scn::detail::to_address(it), src.data() + src.size());
}
TEST(FindClassicSpaceNarrowFastTest, WonkyLongInput2)
{
    auto src =
        "\360,l\215\210ተውን ጉሮሮ ?T  ላU\213\230\263\255\341\341ጋed sample plain\214\211ሮ\265\341"sv;
    ASSERT_EQ(src.size(), 64);

    auto it = scn::impl::find_classic_space_narrow_fast(src);
    EXPECT_EQ(&*it, src.data() + 14);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 24);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 27);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 28);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 44);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(&*it, src.data() + 51);

    ++it;
    it = scn::impl::find_classic_space_narrow_fast(
        scn::detail::make_string_view_from_iterators<char>(it, src.end()));
    EXPECT_EQ(scn::detail::to_address(it), src.data() + src.size());
}
TEST(FindClassicSpaceNarrowFastTest, WonkyInput3)
{
    auto src = "plain\214\211ሮ\265\341"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.end());
}
TEST(FindClassicSpaceNarrowFastTest, WonkyInput4)
{
    auto src = "plain\214\211ሮ\265Ṗn̹\226n̰\002"sv;
    EXPECT_EQ(scn::impl::find_classic_space_narrow_fast(src), src.end());
}

TEST(FindClassicNonspaceNarrowFastTest, EmojiInput)
{
    auto input = "😂\n"sv;

    EXPECT_EQ(
        scn::detail::to_address(
            scn::impl::find_classic_nonspace_narrow_fast(input.substr(0))),
        input.data() + 0);
    EXPECT_EQ(
        scn::detail::to_address(
            scn::impl::find_classic_nonspace_narrow_fast(input.substr(1))),
        input.data() + 1);
    EXPECT_EQ(
        scn::detail::to_address(
            scn::impl::find_classic_nonspace_narrow_fast(input.substr(2))),
        input.data() + 2);
    EXPECT_EQ(
        scn::detail::to_address(
            scn::impl::find_classic_nonspace_narrow_fast(input.substr(3))),
        input.data() + 3);

    EXPECT_EQ(
        scn::detail::to_address(
            scn::impl::find_classic_nonspace_narrow_fast(input.substr(4))),
        input.data() + 5);
}
