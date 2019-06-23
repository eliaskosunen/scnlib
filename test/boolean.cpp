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

TEST_CASE_TEMPLATE("boolean", CharT, char, wchar_t)
{
    {
        bool b{};
        auto e = scan_value<CharT>("true", "{:a}", b);
        CHECK(b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>("false", "{:a}", b);
        CHECK(!b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>("bool", "{:a}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
        CHECK(e.value() == 0);
    }
    {
        bool b{};
        auto e = scan_value<CharT>("0", "{:a}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
        CHECK(e.value() == 0);
    }
    {
        bool b{};
        auto e = scan_value<CharT>("0", "{}", b);
        CHECK(!b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>("1", "{}", b);
        CHECK(b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>("2", "{}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
        CHECK(e.value() == 0);
    }
    {
        bool b{};
        auto e = scan_value<CharT>("true", "{:n}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
        CHECK(e.value() == 0);
    }
}

#if !SCN_MSVC
TEST_CASE_TEMPLATE("bool localized", CharT, char, wchar_t)
{
    auto locale = std::locale("en_US");
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "true", "{:la}", b);
        CHECK(b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "false", "{:la}", b);
        CHECK(!b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "bool", "{:la}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
        CHECK(e.value() == 0);
    }
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "0", "{:la}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
        CHECK(e.value() == 0);
    }
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "0", "{:l}", b);
        CHECK(!b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "1", "{:l}", b);
        CHECK(b);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "2", "{:l}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
        CHECK(e.value() == 0);
    }
    {
        bool b{};
        auto e = scan_value<CharT>(locale, "true", "{:ln}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_format_string);
        CHECK(e.value() == 0);
    }
}
#endif

TEST_CASE("bool scanf")
{
    bool b{};

    // 1 (default)
    auto ret = scanf_value<char>("1", "%b", b);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(b);

    // false (alpha)
    ret = scanf_value<char>("false", "%a", b);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(!b);

    // 0 (numeric)
    ret = scanf_value<char>("0", "%n", b);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(!b);

    // 1 (alpha => error)
    ret = scanf_value<char>("1", "%a", b);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_scanned_value);

    // true (numeric => error)
    ret = scanf_value<char>("true", "%n", b);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_scanned_value);

    // %d /w bool => error
    ret = scanf_value<char>("1", "%d", b);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_format_string);

    // 2 (invalid value => error)
    ret = scanf_value<char>("2", "%b", b);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
}
