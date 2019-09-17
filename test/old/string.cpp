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
        auto e = scan_value<CharT>("thisisaword nextword", "{} {}", s, s2);
        CHECK(s == widen<CharT>("thisisaword"));
        CHECK(s2 == widen<CharT>("nextword"));
        CHECK(e);
        CHECK(e.value() == 2);
    }
    {
        string_type s{};
        auto e = scan_value<CharT>("WoRdW1th_Special<>Charact3rs!?", "{}", s);
        CHECK(s == widen<CharT>("WoRdW1th_Special<>Charact3rs!?"));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        string_type s{};
        auto e = scan_value<CharT>("foo", "{:s}", s);
        CHECK(s == widen<CharT>("foo"));
        CHECK(e);
        CHECK(e.value());
    }
    {
        string_type s{};
        auto e = scan_value<CharT>("foo", "{:a}", s);
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
    auto stream = scn::make_stream(data);

    SUBCASE("test")
    {
        string_type s{};
        auto ret = scn::getline(stream, s);
        CHECK(s == widen<CharT>("firstline"));
        CHECK(ret);

        ret = scn::getline(stream, s);
        CHECK(s == widen<CharT>("Second line with spaces"));
        CHECK(ret);
    }
}

TEST_CASE_TEMPLATE("ignore", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>("line1\nline2");
    auto stream = scn::make_stream(data);
    auto fstr = widen<CharT>("{}");
    auto f = scn::basic_string_view<CharT>(fstr.data(), fstr.size());

    SUBCASE("ignore_n")
    {
        string_type s{};
        {
            auto ret = scn::ignore_n(stream, 6);
            CHECK(ret);
        }

        {
            auto ret = scn::scan(stream, f, s);
            CHECK(s == widen<CharT>("line2"));
            CHECK(ret);
        }
    }
    SUBCASE("ignore_until")
    {
        string_type s{};
        {
            auto ret = scn::ignore_until(stream, 0x0a);  // '\n'
            CHECK(ret);
        }

        {
            auto ret = scn::scan(stream, f, s);
            CHECK(s == widen<CharT>("line2"));
            CHECK(ret);
        }
    }
    SUBCASE("ignore_all")
    {
        string_type s{};
        {
            auto ret = scn::ignore_all(stream);
            CHECK(ret);
        }

        {
            auto ret = scn::scan(stream, f, s);
            CHECK(!ret);
            if (!ret) {
                CHECK(ret.error() == scn::error::end_of_stream);
            }
        }
    }
}

TEST_CASE("string scanf")
{
    std::string str{};

    auto ret = scanf_value<char>("str", "%s", str);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(str == "str");
}
