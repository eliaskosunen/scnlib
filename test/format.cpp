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

struct mytype {
    char ch;
};
namespace scn {
    template <>
    struct scanner<mytype> : common_parser_default {
        template <typename Context>
        error scan(mytype& val, Context& ctx)
        {
            return scan_usertype(ctx, "{}", val.ch);
        }
    };
}  // namespace scn

TEST_CASE("mytype")
{
    mytype a{};
    auto e = scn::scan("a", "{}", a);
    CHECK(e);
    CHECK(a.ch == 'a');

    e = scn::scan_default("b", a);
    CHECK(e);
    CHECK(a.ch == 'b');
}

TEST_CASE("align")
{
    mytype a{};
    SUBCASE("left")
    {
        auto e = scn::scan("a   b", "{:<}", a);
        CHECK(e);
        CHECK(e.range_as_string() == "b");
        CHECK(a.ch == 'a');
    }
    SUBCASE("right")
    {
        auto e = scn::scan("   a b", "{:>}", a);
        CHECK(e);
        CHECK(e.range_as_string() == " b");
        CHECK(a.ch == 'a');
    }
    SUBCASE("center")
    {
        auto e = scn::scan("   a   b", "{:^}", a);
        CHECK(e);
        CHECK(e.range_as_string() == "b");
        CHECK(a.ch == 'a');
    }
    SUBCASE("center, non-contiguous")
    {
        auto e = scn::scan(get_deque<char>("   a   b"), "{:^}", a);
        CHECK(e);
        CHECK(e.range().size() == 1);
        CHECK(*e.range().begin() == 'b');
        CHECK(a.ch == 'a');
    }
}
TEST_CASE("align + fill")
{
    mytype a{};
    SUBCASE("left")
    {
        auto e = scn::scan("a*** b", "{:*<}", a);
        CHECK(e);
        CHECK(e.range_as_string() == " b");
        CHECK(a.ch == 'a');
    }
    SUBCASE("right")
    {
        auto e = scn::scan("***a b", "{:*>}", a);
        CHECK(e);
        CHECK(e.range_as_string() == " b");
        CHECK(a.ch == 'a');
    }
    SUBCASE("center")
    {
        auto e = scn::scan("***a*** b", "{:*^}", a);
        CHECK(e);
        CHECK(e.range_as_string() == " b");
        CHECK(a.ch == 'a');
    }
}

TEST_CASE("width")
{
    SUBCASE("string over")
    {
        std::string str;
        auto e = scn::scan("foo", "{:2}", str);
        CHECK(e);
        CHECK(e.range_as_string() == "o");
        CHECK(str == "fo");
    }
    SUBCASE("string under")
    {
        std::string str;
        auto e = scn::scan("foo", "{:4}", str);
        CHECK(e);
        CHECK(e.empty());
        CHECK(str == "foo");
    }
    SUBCASE("int over")
    {
        int i;
        auto e = scn::scan("123", "{:2}", i);
        CHECK(e);
        CHECK(e.range_as_string() == "3");
        CHECK(i == 12);
    }
    SUBCASE("int under")
    {
        int i;
        auto e = scn::scan("123", "{:4}", i);
        CHECK(e);
        CHECK(e.empty());
        CHECK(i == 123);
    }
}

TEST_CASE("utf8 literal")
{
    SUBCASE("code points")
    {
        scn::code_point a, b;
        auto e = scn::scan("åäö", "{}ä{}", a, b);
        CHECK(e);
        CHECK(e.empty());
        CHECK(a == 0xe5);
        CHECK(b == 0xf6);
    }
    SUBCASE("code units")
    {
        char a1, a2, b1, b2;
        auto e = scn::scan("åäö", "{}{}ä{}{}", a1, a2, b1, b2);
        CHECK(e);
        CHECK(e.empty());
        CHECK(a1 == static_cast<char>(0xc3));
        CHECK(a2 == static_cast<char>(0xa5));
        CHECK(b1 == static_cast<char>(0xc3));
        CHECK(b2 == static_cast<char>(0xb6));
    }
}
