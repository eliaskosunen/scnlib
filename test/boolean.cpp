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
        auto e = do_scan<CharT>("true", "{:a}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>("false", "{:a}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>("bool", "{:a}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan<CharT>("0", "{:a}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan<CharT>("0", "{}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>("1", "{}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>("2", "{}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan<CharT>("true", "{:n}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
}

#if 0
TEST_CASE_TEMPLATE("bool localized", CharT, char, wchar_t)
{
    auto locale = std::locale("en_US");
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "true", "{:la}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "false", "{:la}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "bool", "{:la}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "0", "{:la}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "0", "{:l}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "1", "{:l}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "2", "{:l}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan<CharT>(locale, "true", "{:ln}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_format_string);
    }
}
#endif

TEST_CASE("bool scanf")
{
    bool b{};

    // 1 (default)
    auto ret = do_scanf<char>("1", "%b", b);
    CHECK(ret);
    CHECK(b);

    // false (alpha)
    ret = do_scanf<char>("false", "%a", b);
    CHECK(ret);
    CHECK(!b);

    // 0 (numeric)
    ret = do_scanf<char>("0", "%n", b);
    CHECK(ret);
    CHECK(!b);

    // 1 (alpha => error)
    ret = do_scanf<char>("1", "%a", b);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);

    // true (numeric => error)
    ret = do_scanf<char>("true", "%n", b);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);

    // %d /w bool => error
    ret = do_scanf<char>("1", "%d", b);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);

    // 2 (invalid value => error)
    ret = do_scanf<char>("2", "%b", b);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
}
