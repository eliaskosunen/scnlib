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

#include <scn/impl/algorithms/read_copying.h>
#include <scn/impl/util/ascii_ctype.h>

TEST(ReadUntilAsciiCopyingTest, ContiguousSourceAndDest)
{
    auto src = std::string_view{"foo bar"};
    auto dst = std::string(8, '\0');

    auto [in, out] = scn::impl::read_until_classic_copying(
        src, dst, [](char ch) SCN_NOEXCEPT { return scn::impl::is_ascii_space(ch); });

    EXPECT_EQ(in, src.begin() + 3);
    EXPECT_EQ(out, dst.begin() + 3);
    {
        auto a = std::string_view{dst.data(),
                                  static_cast<size_t>(out - dst.begin())};
        EXPECT_EQ(a, "foo");
    }
    EXPECT_EQ(*in, ' ');
}

TEST(ReadUntilCodePointCopyingTest, ContiguousSourceAndDest)
{
    auto src = std::string_view{"aäö "};
    auto dst = std::string{};

    auto result = scn::impl::read_until_code_point_copying(
        src, scn::impl::back_insert(dst),
        [](scn::code_point cp) { return cp == scn::make_code_point(0xf6); });
    EXPECT_TRUE(result);
    auto [in, out] = result.value();

    EXPECT_EQ(in, src.begin() + 3);
    EXPECT_NE(out, scn::ranges_std::unreachable_sentinel_t{});

    EXPECT_EQ(dst, std::string{"aä"});
    EXPECT_EQ(static_cast<unsigned char>(*in), 0xc3);
    EXPECT_EQ(static_cast<unsigned char>(*(in + 1)), 0xb6);
}
