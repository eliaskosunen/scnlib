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

TEST_CASE_TEMPLATE("string test", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    {
        string_type s{}, s2{};
        auto e = do_scan<CharT>("thisisaword nextword", "{} {}", s, s2);
        CHECK(s == widen<CharT>("thisisaword"));
        CHECK(s2 == widen<CharT>("nextword"));
        CHECK(e);
    }
    {
        string_type s{};
        auto e = do_scan<CharT>("WoRdW1th_Special<>Charact3rs!?", "{}", s);
        CHECK(s == widen<CharT>("WoRdW1th_Special<>Charact3rs!?"));
        CHECK(e);
    }
    {
        string_type s{};
        auto e = do_scan<CharT>("foo", "{:s}", s);
        CHECK(s == widen<CharT>("foo"));
        CHECK(e);
    }
    {
        string_type s{};
        auto e = do_scan<CharT>("foo", "{:a}", s);
        CHECK(s.empty());
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
    }
}

TEST_CASE_TEMPLATE("getline", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>(
        "firstline\n"
        "Second line with spaces");

    SUBCASE("test")
    {
        string_type s{};
        auto ret = scn::getline(data, s);
        CHECK(s == widen<CharT>("firstline"));
        CHECK(ret);

        ret = scn::getline(ret.range(), s);
        CHECK(s == widen<CharT>("Second line with spaces"));
        CHECK(ret);
    }
}

TEST_CASE_TEMPLATE("ignore", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>("line1\nline2");

    SUBCASE("ignore_until")
    {
        string_type s{};
        {
            auto ret = scn::ignore_until(data, 0x0a);  // '\n'
            CHECK(ret);
            data.assign(ret.string());
        }

        {
            auto ret = scn::scan(data, scn::default_tag, s);
            CHECK(s == widen<CharT>("line2"));
            CHECK(ret);
        }
    }
}

TEST_CASE("string scanf")
{
    std::string str{};

    auto ret = do_scanf<char>("str", "%s", str);
    CHECK(ret);
    CHECK(str == "str");
}
