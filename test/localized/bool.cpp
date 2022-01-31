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
        auto e = do_scan_localized<CharT>(locale, "true", "{:L}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "false", "{:L}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "bool", "{:L}", b);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "0", "{:Ls}", b);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "0", "{:L}", b);
        CHECK(!b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "1", "{:L}", b);
        CHECK(b);
        CHECK(e);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "2", "{:L}", b);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "true", "{:Li}", b);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }
    {
        bool b{};
        auto e = do_scan_localized<CharT>(locale, "1", "{:Ln}", b);
        CHECK(e);
        CHECK(b);
    }
}
