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

TEST_CASE("char")
{
    char ch[3] = {};
    auto ret = scn::scan(" ab", "{}{}{}", ch[0], ch[1], ch[2]);

    CHECK(ret);
    CHECK(ch[0] == ' ');
    CHECK(ch[1] == 'a');
    CHECK(ch[2] == 'b');
    std::fill(ch, ch + 3, 0);

    ret = scn::scan(" ab", " {}{}", ch[0], ch[1]);
    CHECK(ret);
    CHECK(ch[0] == 'a');
    CHECK(ch[1] == 'b');
}

TEST_CASE("char format string")
{
    char ch{};

    auto ret = do_scan<char>("a", "{}", ch);
    CHECK(ret);
    CHECK(ch == 'a');

    ret = do_scan<char>("a", "{:c}", ch);
    CHECK(ret);
    CHECK(ch == 'a');

    ret = do_scan<char>("a", "{:", ch);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);

    ret = do_scan<char>("a", "{:a}", ch);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
}
