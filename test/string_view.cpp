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

TEST_CASE("string_view")
{
    std::string str{"Hello world!"};

    SUBCASE("single-arg constructor")
    {
        scn::string_view sv(str.data());
        REQUIRE(sv.data() == str.data());
        REQUIRE(sv.size() == str.size());
    }
    SUBCASE("double-arg constructor")
    {
        scn::string_view sv(str.data(), str.size());
        REQUIRE(sv.data() == str.data());
        REQUIRE(sv.size() == str.size());
    }
    SUBCASE("iterator")
    {
        scn::string_view sv(str.data());
        CHECK(sv.begin() != sv.end());
        CHECK(sv.begin() + sv.size() == sv.end());
        CHECK(sv.begin() == sv.cbegin());
        CHECK(*sv.begin() == *str.begin());
        CHECK(*(sv.begin() + 1) == *(str.begin() + 1));
        CHECK(*(sv.end() - 1) == *(str.end() - 1));
        CHECK(*sv.begin() == sv.front());

        {
            auto it = sv.begin();
            ++it;
            CHECK(it == sv.begin() + 1);
        }
    }
}

TEST_CASE_TEMPLATE("string_view scan", CharT, char, wchar_t)
{
    using string_type = scn::basic_string_view<CharT>;
    {
        string_type s{}, s2{};
        auto e = scan_value<CharT>("thisisaword nextword", "{} {}", s, s2);
        CHECK(s.compare(widen<CharT>("thisisaword").c_str()) == 0);
        CHECK(s2.compare(widen<CharT>("nextword").c_str()) == 0);
        CHECK(e);
        CHECK(e.value() == 2);
    }
    {
        string_type s{};
        auto e = scan_value<CharT>("WoRdW1th_Special<>Charact3rs!?", "{}", s);
        CHECK(s.compare(
                  widen<CharT>("WoRdW1th_Special<>Charact3rs!?").c_str()) == 0);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        string_type s{};
        auto e = scan_value<CharT>("foo", "{:s}", s);
        CHECK(s.compare(widen<CharT>("foo").c_str()) == 0);
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
