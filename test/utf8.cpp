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

TEST_CASE("utf8")
{
    constexpr auto latin_small_letter_a = scn::make_code_point(0x61);  // a
    constexpr auto latin_small_letter_a_with_diaeresis =
        scn::make_code_point(0xe4);                           // ä, 2 bytes
    constexpr auto euro_sign = scn::make_code_point(0x20ac);  // €, 3 bytes
    constexpr auto slightly_smiling_face =
        scn::make_code_point(0x1f642);  // 🙂, 4 bytes

    scn::string_view str{"aä€🙂"};
    CHECK(str.length() == 10);

    scn::code_point cp{};
    auto ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 1);
    CHECK(cp == latin_small_letter_a);
    cp = zero;

    ret = scn::parse_code_point(str.begin() + 1, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 3);
    CHECK(cp == latin_small_letter_a_with_diaeresis);
    cp = zero;

    ret = scn::parse_code_point(str.begin() + 3, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 6);
    CHECK(cp == euro_sign);
    cp = zero;

    ret = scn::parse_code_point(str.begin() + 6, str.end(), cp);
    CHECK(ret);
    CHECK(ret.value() == str.begin() + 10);
    CHECK(cp == slightly_smiling_face);
    cp = zero;
}

TEST_CASE("read_code_point")
{
    unsigned char buf[4] = {0};
    auto bufspan = scn::make_span(buf, 4);

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque<char>("aä€🙂"));
        CHECK(range.size() == 10);

        auto ret = scn::read_code_point(range, bufspan);
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == 'a');
        CHECK(ret.value().cp == scn::make_code_point('a'));
        CHECK(range.size() == 9);

        ret = scn::read_code_point(range, bufspan);
        CHECK(ret);
        CHECK(ret.value().chars.size() == 2);
        CHECK(ret.value().chars[0] == static_cast<char>(0xc3));
        CHECK(ret.value().chars[1] == static_cast<char>(0xa4));
        CHECK(ret.value().cp == scn::make_code_point(0xe4));
        CHECK(range.size() == 7);

        ret = scn::read_code_point(range, bufspan);
        CHECK(ret);
        CHECK(ret.value().chars.size() == 3);
        CHECK(ret.value().cp == scn::make_code_point(0x20ac));
        CHECK(range.size() == 4);

        ret = scn::read_code_point(range, bufspan);
        CHECK(ret);
        CHECK(ret.value().chars.size() == 4);
        CHECK(ret.value().cp == scn::make_code_point(0x1f642));
        CHECK(range.empty());

        ret = scn::read_code_point(range, bufspan);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE("invalid utf8")
{
    scn::code_point cp{};

    // partial code point: 0xc1 would be the first byte in a 2-byte code point
    scn::string_view str{"\xc1"};
    auto ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_encoding);
    cp = zero;

    // partial code point: 0xf0 would mean 4 code units, only 3 given
    str = "\xf1\x81\x81";
    ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_encoding);
    cp = zero;

    // trailing code point: 0x81 can't be a leading code unit
    str = "\x81";
    ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_encoding);
    cp = zero;

    // invalid leading code unit: 0xf9 = 11111001b (5 bytes?)
    str = "\xf9\x81\x81\x81\x81";
    ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_encoding);
    cp = zero;

    // overlong sequence
    str = "\xf0\x82\x82\xac";
    ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_encoding);
    cp = zero;

    // surrogate U+D800
    // 1101 100000 000000 ->
    // 11101101 10100000 10000000 ->
    // 0xed 0xa0 0x80
    str = "\xed\xa0\x80";
    ret = scn::parse_code_point(str.begin(), str.end(), cp);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_encoding);
    cp = zero;
}
