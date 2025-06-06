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

#include <scn/scan.h>

#include <iostream>

#include "wrapped_gtest.h"

template <typename T>
static std::optional<T> read_scn()
{
    auto r = scn::input<T>("{}");
    if (!r) {
        return std::nullopt;
    }
    return r->value();
}

template <typename T>
static std::optional<T> read_scanf()
{
    if constexpr (std::is_same_v<T, int>) {
        int i{};
        if (std::scanf("%d", &i) != 1) {
            return std::nullopt;
        }
        return i;
    }
    else if constexpr (std::is_same_v<T, std::string>) {
        std::string val{};
        val.resize(3);
        if (std::scanf(" %3c", &val[0]) != 1) {
            return std::nullopt;
        }
        return SCN_MOVE(val);
    }
    else if constexpr (std::is_same_v<T, char>) {
        char val{};
        if (std::scanf("%c", &val) != 1) {
            return std::nullopt;
        }
        return SCN_MOVE(val);
    }
    else {
        static_assert(scn::detail::dependent_false<T>::value, "");
    }
}

template <typename T>
static std::optional<T> read_cin()
{
    T i{};
    if (!(std::cin >> i)) {
        return std::nullopt;
    }
    return SCN_MOVE(i);
}

using testing::Optional;

TEST(Stdin, Test)
{
    using namespace std::string_literals;

    EXPECT_THAT(read_scn<int>(), Optional(100));
    EXPECT_THAT(read_scn<int>(), Optional(101));
    EXPECT_THAT(read_scanf<int>(), Optional(102));
    EXPECT_THAT(read_scn<int>(), Optional(103));
    EXPECT_THAT(read_cin<int>(), Optional(104));
    EXPECT_THAT(read_scn<int>(), Optional(105));

    EXPECT_EQ(read_scn<int>(), std::nullopt);
    EXPECT_THAT(read_scn<std::string>(), Optional("aaa"s));

    EXPECT_EQ(read_scn<int>(), std::nullopt);
    EXPECT_THAT(read_scanf<std::string>(), Optional("bbb"s));

    EXPECT_EQ(read_scn<int>(), std::nullopt);
    EXPECT_THAT(read_cin<std::string>(), Optional("ccc"s));

    EXPECT_THAT(read_scn<char>(), Optional('\n'));
    EXPECT_THAT(read_scn<char>(), Optional('d'));
    EXPECT_THAT(read_scn<char>(), Optional('\n'));
    EXPECT_THAT(read_cin<char>(), Optional('e'));
}
