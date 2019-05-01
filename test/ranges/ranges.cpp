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
#include "../test.h"

#include <scn/ranges.h>

TEST_CASE("ranges")
{
    SUBCASE("general")
    {
        std::string data{"test {} 42 3.14 foobar true"};

        int i{0};
        double d{};
        std::string s(6, '\0');
        auto span = scn::make_span(&s[0], &s[0] + s.size());
        bool b{};
        auto ret =
            scn::ranges::scan(data, "test {{}} {} {} {} {:a}", i, d, span, b);

        CHECK(i == 42);
        CHECK(d == doctest::Approx(3.14));
        CHECK(s == "foobar");
        CHECK(b);

        CHECK(ret);
        CHECK(ret.value() == 4);
        CHECK(ret.iterator() == data.end());
    }
    SUBCASE("subrange")
    {
        std::string data{"Hello world"};

        std::string str{};
        auto ret = scn::ranges::scan(data, "{}", str);

        CHECK(str == "Hello");
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(ret.iterator() ==
              data.begin() + static_cast<std::ptrdiff_t>(str.length()));

        ret = scn::ranges::scan(ret.view(), "{}", str);

        CHECK(str == "world");
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(ret.iterator() == data.end());
    }
#if 0
    SUBCASE("action")
    {
        std::string data{"42 foo"};

        scn::error err;
        int i;
        auto ret = data | scn::ranges::action::scan(i, err);
    }
#endif
}

TEST_CASE_TEMPLATE_DEFINE("ranges getline", CharT, getline_test)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>(
        "firstline\n"
        "Second line with spaces");

    {
        string_type s{};
        auto ret = scn::ranges::getline(data, s);
        CHECK(s == widen<CharT>("firstline"));
        CHECK(ret);
        CHECK(ret.value() == data.begin() + s.size());
        data.erase(data.begin(), ret.value());
    }
    {
        string_type s{};
        auto ret = scn::ranges::getline(data, s);
        CHECK(s == widen<CharT>("Second line with spaces"));
        CHECK(ret);
        CHECK(ret.value() == data.end());
    }
}

TEST_CASE("scanf")
{
    std::string data{"test % 42 3.14 foobar true"};

    int i{0};
    double d{};
    std::string s(6, '\0');
    bool b{};
    auto ret = scn::ranges::scanf(data, "test %% %i %f %s %b", i, d, s, b);

    CHECK(ret);
    CHECK(ret.value() == 4);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
}
