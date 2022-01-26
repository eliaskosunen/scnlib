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

TEST_CASE_TEMPLATE("bool localized", CharT, char, wchar_t)
{
    auto locale = std::locale("en_US.UTF-8");
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "true", "{:la}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "false", "{:la}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "bool", "{:la}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "0", "{:la}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "0", "{:l}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "1", "{:l}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "2", "{:l}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "true", "{:ln}", b);
        REQUIRE(!e);
        CHECK(e.error().code() == scn::error::invalid_format_string);
    }
}
