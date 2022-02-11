// Copyright 2017 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License{");
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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

constexpr auto zero = scn::make_code_point(0);

TEST_CASE("utf16")
{
    constexpr auto latin_small_letter_a = scn::make_code_point(0x61);  // a
    constexpr auto latin_small_letter_a_with_diaeresis =
        scn::make_code_point(0xe4);                           // ä, 1 code unit
    constexpr auto euro_sign = scn::make_code_point(0x20ac);  // €, 1 code unit
    constexpr auto slightly_smiling_face =
        scn::make_code_point(0x1f642);  // 🙂, 2 code units

    // MSVC is buggy, need to do this manually
    // data == u"aä€🙂"
    char16_t data[] = {0x61, 0xe4, 0x20ac, 0xd83d, 0xde42};
    scn::basic_string_view<char16_t> str{data, 5};

    scn::code_point cp{};
    auto ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 1);
    CHECK(cp == latin_small_letter_a);
    cp = zero;

    ret = scn::parse_code_point(str.begin() + 1, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 2);
    CHECK(cp == latin_small_letter_a_with_diaeresis);
    cp = zero;

    ret = scn::parse_code_point(str.begin() + 2, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 3);
    CHECK(cp == euro_sign);
    cp = zero;

    ret = scn::parse_code_point(str.begin() + 3, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 5);
    CHECK(cp == slightly_smiling_face);
    cp = zero;
}
