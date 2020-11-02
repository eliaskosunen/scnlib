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

TEST_CASE("string_view scan")
{
    using string_type = scn::string_view;
    {
        string_type s{}, s2{};
        auto e = scn::scan("thisisaword nextword", "{} {}", s, s2);
        CHECK(std::strncmp("thisisaword", s.data(), s.size()) == 0);
        CHECK(std::strncmp("nextword", s2.data(), s2.size()) == 0);
        CHECK(e);
    }
    {
        string_type s{};
        auto e = scn::scan("WoRdW1th_Special<>Charact3rs!?", "{}", s);
        CHECK(std::strncmp("WoRdW1th_Special<>Charact3rs!?", s.data(),
                           s.size()) == 0);
        CHECK(e);
    }
    {
        string_type s{};
        auto e = scn::scan("foo", "{:s}", s);
        CHECK(std::strncmp("foo", s.data(), s.size()) == 0);
        CHECK(e);
    }
    {
        string_type s{};
        auto e = scn::scan("foo", "{:a}", s);
        CHECK(s.empty());
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
    }
}
