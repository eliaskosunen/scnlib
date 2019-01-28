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

#include <doctest.h>
#include <scn/scn.h>

TEST_CASE("string")
{
    std::string data{"thisisaword nextword WoRdW1th_Special<>Charact3rs"};
    auto stream = scn::make_stream(data);
    std::string s{}, s2{};

    {
        auto ret = scn::scan(stream, "{} {}", s, s2);
        CHECK(s == "thisisaword");
        CHECK(s2 == "nextword");
        CHECK(ret);
        s.clear();
        s2.clear();
    }
    {
        auto ret = scn::scan(stream, "{}", s);
        CHECK(s == "WoRdW1th_Special<>Charact3rs");
        CHECK(ret);
        s.clear();
    }
}

TEST_CASE("getline")
{
    std::string data{
        "firstline\n"
        "Second line with spaces"};
    auto stream = scn::make_stream(data);
    std::string s{};

    {
        auto ret = scn::getline(stream, s);
        CHECK(s == "firstline");
        CHECK(ret);
        s.clear();
    }
    {
        auto ret = scn::getline(stream, s);
        CHECK(s == "Second line with spaces");
        CHECK(ret);
        s.clear();
    }
}

TEST_CASE("ignore")
{
    std::string data{"line1\nline2"};
    auto stream = scn::make_stream(data);
    std::string s{};

    SUBCASE("ignore_n")
    {
        auto ret = scn::ignore_n(stream, 6);
        CHECK(ret);

        ret = scn::scan(stream, "{}", s);
        CHECK(s == "line2");
        CHECK(ret);
    }
    SUBCASE("ignore_until")
    {
        auto ret = scn::ignore_until(stream, '\n');
        CHECK(ret);

        ret = scn::scan(stream, "{}", s);
        CHECK(s == "line2");
        CHECK(ret);
    }
    SUBCASE("ignore_all")
    {
        auto ret = scn::ignore_all(stream);
        CHECK(ret);

        ret = scn::scan(stream, "{}", s);
        CHECK(!ret);
        if (!ret) {
            CHECK(ret == scn::error::end_of_stream);
        }
    }
}
