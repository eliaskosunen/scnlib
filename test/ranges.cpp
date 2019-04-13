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
            scn::ranges::scan(data, "test {{}} {} {} {} {a}", i, d, span, b);

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
              data.begin() + static_cast<std::ptrdiff_t>(str.length()) + 1);

        ret = scn::ranges::scan(
            scn::ranges::subrange_from(ret.iterator(), data), "{}", str);

        CHECK(str == "world");
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(ret.iterator() == data.end());
    }
}
