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

TEST(BitsTest, CountTrailingZeroes)
{
    EXPECT_EQ(scn::impl::count_trailing_zeroes(0b0001), 0);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(0b1000), 3);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(0b1111), 0);

    EXPECT_EQ(
        scn::impl::count_trailing_zeroes(std::numeric_limits<uint64_t>::max()),
        0);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(
                  std::numeric_limits<uint64_t>::max() - 1),
              1);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(
                  std::numeric_limits<uint64_t>::max() - 2),
              0);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(
                  std::numeric_limits<uint64_t>::max() - 3),
              2);

    EXPECT_EQ(scn::impl::count_trailing_zeroes(
                  static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())),
              0);
    EXPECT_EQ(
        scn::impl::count_trailing_zeroes(
            static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - 1),
        1);
    EXPECT_EQ(
        scn::impl::count_trailing_zeroes(
            static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - 2),
        0);
    EXPECT_EQ(
        scn::impl::count_trailing_zeroes(
            static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - 3),
        2);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(
                  static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) *
                  2,
              0);
}

TEST(BitsTest, HasZeroByte)
{
    EXPECT_TRUE(scn::impl::has_zero_byte(0));
    EXPECT_TRUE(scn::impl::has_zero_byte(0xff));
    EXPECT_FALSE(
        scn::impl::has_zero_byte(std::numeric_limits<uint64_t>::max()));
    EXPECT_TRUE(
        scn::impl::has_zero_byte(std::numeric_limits<uint64_t>::max() - 0xff));
}

TEST(BitsTest, Log2)
{
    EXPECT_EQ(scn::impl::log2_fast(1), 0);
    EXPECT_EQ(scn::impl::log2_fast(2), 1);
    EXPECT_EQ(scn::impl::log2_fast(3), 1);
    EXPECT_EQ(scn::impl::log2_fast(4), 2);
    EXPECT_EQ(scn::impl::log2_fast(7), 2);
    EXPECT_EQ(scn::impl::log2_fast(8), 3);

    EXPECT_EQ(scn::impl::log2_pow2_fast(2), 1);
    EXPECT_EQ(scn::impl::log2_pow2_fast(4), 2);
    EXPECT_EQ(scn::impl::log2_pow2_fast(8), 3);
}
