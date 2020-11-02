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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <scn/istream.h>

#include <istream>
#include <sstream>

#include "test.h"

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
    my_type val{};
    auto ret = scn::scan("123", "{}", val);
    CHECK(ret);
    CHECK(val.value == 123);
}

TEST_CASE("istream fail")
{
    my_type val{};
    auto ret = scn::scan("foo", "{}", val);
    CHECK(!ret);
    CHECK(ret.error().code() == scn::error::invalid_scanned_value);
    CHECK(val.value == 0);
}

TEST_CASE("istream eof")
{
    my_type val{};
    auto ret = scn::scan("", "{}", val);
    CHECK(!ret);
    CHECK(ret.error().code() == scn::error::end_of_range);
    CHECK(val.value == 0);
}

TEST_CASE("istream composite")
{
    auto source = std::string{"foo 123 456"};

    std::string s;
    auto ret = scn::scan_default(source, s);
    CHECK(ret);
    CHECK(s == "foo");

    my_type val{};
    ret = scn::scan_default(ret.range(), val);
    CHECK(ret);
    CHECK(val.value == 123);

    int i;
    ret = scn::scan_default(ret.range(), i);
    CHECK(ret);
    CHECK(i == 456);
    CHECK(ret.empty());
}
TEST_CASE("istream composite error")
{
    auto source = std::string{"123 foo 456"};

    int i;
    auto ret = scn::scan_default(source, i);
    CHECK(ret);
    CHECK(i == 123);

    my_type val{};
    ret = scn::scan_default(ret.range(), val);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(val.value == 0);

    std::string s;
    ret = scn::scan_default(ret.range(), s);
    CHECK(ret);
    CHECK(s == "foo");
}
