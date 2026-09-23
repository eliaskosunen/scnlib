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
    EXPECT_EQ(scn::impl::count_trailing_zeroes(0b0001u), 0);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(0b1000u), 3);
    EXPECT_EQ(scn::impl::count_trailing_zeroes(0b1111u), 0);

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

TEST(BitsTest, Uint128PolyfillAddSub)
{
    using u128 = scn::impl::uint128_polyfill;
    const auto make = [](std::uint64_t high, std::uint64_t low) {
        return (u128{high} << 64u) | u128{low};
    };
    constexpr auto max64 = std::numeric_limits<std::uint64_t>::max();

    EXPECT_EQ(make(1, 5) - make(0, 1), make(1, 4));
    EXPECT_EQ(u128{5u} - u128{5u}, u128{0u});

    EXPECT_EQ(make(1, 0) - make(0, 1), make(0, max64));
    EXPECT_EQ(make(3, 1) - make(1, 2), make(1, max64));
    EXPECT_EQ((u128{1u} << 105u) - u128{1u}, make((1ull << 41u) - 1u, max64));

    EXPECT_EQ(make(1, 4) + make(0, 1), make(1, 5));

    EXPECT_EQ(make(0, max64) + make(0, 1), make(1, 0));
    EXPECT_EQ(make(1, max64) + make(1, 2), make(3, 1));
}
