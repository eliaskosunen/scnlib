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

TEST_CASE("utf8")
{
    constexpr auto zero = scn::utf8::make_code_point(0);
    constexpr auto latin_small_letter_a =
        scn::utf8::make_code_point(0x61);  // a
    constexpr auto latin_small_letter_a_with_diaeresis =
        scn::utf8::make_code_point(0xe4);  // ä, 2 bytes
    constexpr auto euro_sign =
        scn::utf8::make_code_point(0x20ac);  // €, 3 bytes

    scn::string_view str{"aä€"};
    CHECK(str.length() == 6);

    scn::utf8::code_point cp{};
    auto ret = scn::utf8::parse_code_point(str.begin(), str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 1);
    CHECK(cp == latin_small_letter_a);
    cp = zero;

    ret = scn::utf8::parse_code_point(str.begin() + 1, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 3);
    CHECK(cp == latin_small_letter_a_with_diaeresis);
    cp = zero;

    ret = scn::utf8::parse_code_point(str.begin() + 3, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.end());
    CHECK(cp == euro_sign);
}
