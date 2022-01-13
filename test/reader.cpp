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

TEST_CASE("read_char")
{
    SUBCASE("direct")
    {
        auto range = scn::wrap("42");
        auto ret = scn::read_char(range);
        CHECK(ret);
        CHECK(ret.value() == '4');

        CHECK(*range.begin() == '2');
        range.advance();

        ret = scn::read_char(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
#if 0
    SUBCASE("indirect")
    {
        auto range = scn::wrap("42");
        auto ret = scn::read_char(range);
        CHECK(ret);
        CHECK(ret.value() == '4');

        CHECK(range.range()[0] == '2');
        range.advance();

        ret = scn::read_char(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
#endif
}

TEST_CASE("read_zero_copy")
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap("123");
        auto ret = scn::read_zero_copy(range, 2);
        CHECK(ret);
        CHECK(ret.value().size() == 2);
        CHECK(ret.value()[0] == '1');
        CHECK(ret.value()[1] == '2');

        CHECK(*range.begin() == '3');
        range.advance();

        ret = scn::read_zero_copy(range, 1);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE("read_into")
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap("123");
        std::vector<char> data{};
        auto it = std::back_inserter(data);
        auto ret = scn::read_into(range, it, 2);
        CHECK(ret);
        CHECK(data.size() == 2);
        CHECK(data[0] == '1');
        CHECK(data[1] == '2');

        CHECK(*range.begin() == '3');
        range.advance();

        ret = scn::read_into(range, it, 1);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 2);
        CHECK(data[0] == '1');
        CHECK(data[1] == '2');
    }
}

TEST_CASE("read_until_space_zero_copy")
{
    SUBCASE("contiguous & no final space")
    {
        auto range = scn::wrap("123 456");
        auto ret = scn::read_until_space_zero_copy(
            range, [](char ch) { return ch == ' '; }, false);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == '1');
        CHECK(ret.value()[1] == '2');
        CHECK(ret.value()[2] == '3');

        CHECK(*range.begin() == ' ');
        range.advance();

        ret = scn::read_until_space_zero_copy(
            range, [](char ch) { return ch == ' '; }, false);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == '4');
        CHECK(ret.value()[1] == '5');
        CHECK(ret.value()[2] == '6');

        CHECK(range.begin() == range.end());
    }

    SUBCASE("contiguous & keep final space")
    {
        auto range = scn::wrap("123 456");
        auto ret = scn::read_until_space_zero_copy(
            range, [](char ch) { return ch == ' '; }, true);
        CHECK(ret);
        CHECK(ret.value().size() == 4);
        CHECK(ret.value()[0] == '1');
        CHECK(ret.value()[1] == '2');
        CHECK(ret.value()[2] == '3');
        CHECK(ret.value()[3] == ' ');

        ret = scn::read_until_space_zero_copy(
            range, [](char ch) { return ch == ' '; }, true);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == '4');
        CHECK(ret.value()[1] == '5');
        CHECK(ret.value()[2] == '6');

        CHECK(range.begin() == range.end());
    }
}
