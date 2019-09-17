// Copyright 2017-2019 Elias Kosunen
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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

#include <scn/istream.h>
#include <istream>
#include <sstream>

TEST_CASE("istream stream")
{
    std::istringstream ss("123");
    auto stream = scn::make_stream(ss);

    int i{};
    auto ret = scn::scan(stream, "{}", i);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(i == 123);
}

TEST_CASE("istream stream fail")
{
    SUBCASE("at eof")
    {
        int i{};
        std::istringstream ss{};
        ss >> i;
        CHECK(ss.eof());

        auto stream = scn::make_stream(ss);
        auto ret = scn::scan(stream, "{}", i);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::end_of_stream);
        CHECK(i == 0);
    }
    SUBCASE("parsing failed")
    {
        std::istringstream ss("foo");
        auto stream = scn::make_stream(ss);

        int i{};
        auto ret = scn::scan(stream, "{}", i);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_scanned_value);
        CHECK(i == 0);
        CHECK(stream);

        std::string str{};
        ret = scn::scan(stream, "{}", str);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(str == "foo");
    }
}

struct my_type {
    int value{};

    friend std::istream& operator>>(std::istream& is, my_type& val)
    {
        is >> val.value;
        return is;
    }
};

TEST_CASE("istream value")
{
    auto stream = scn::make_stream("123");

    my_type val{};
    auto ret = scn::scan(stream, "{}", val);
    CHECK(ret.value() == 1);
    CHECK(val.value == 123);
}
