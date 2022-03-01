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
    char ch[6] = {0};
    auto ret = scn::scan(" ab123", "{}{}{:c}{}{:c}{:i}", ch[0], ch[1], ch[2],
                         ch[3], ch[4], ch[5]);

    CHECK(ret);
    CHECK(ch[0] == ' ');
    CHECK(ch[1] == 'a');
    CHECK(ch[2] == 'b');
    CHECK(ch[3] == '1');
    CHECK(ch[4] == '2');
    CHECK(ch[5] == 3);
    std::fill(ch, ch + 6, 0);

    ret = scn::scan(" ab", " {}{}", ch[0], ch[1]);
    CHECK(ret);
    CHECK(ch[0] == 'a');
    CHECK(ch[1] == 'b');
}

TEST_CASE("signed + unsigned char")
{
    signed char s1, s2, s3;

    auto ret = scn::scan(" -1 2", "{}{:c}{:c}", s1, s2, s3);
    CHECK(ret);
    CHECK(s1 == -1);
    CHECK(s2 == ' ');
    CHECK(s3 == '2');

    unsigned char u1, u2, u3;
    ret = scn::scan(" 1 2", "{}{:c}{:c}", u1, u2, u3);
    CHECK(ret);
    CHECK(u1 == 1);
    CHECK(u2 == ' ');
    CHECK(u3 == '2');
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

    ret = do_scan<char>("1", "{:i}", ch);
    CHECK(ret);
    CHECK(ch == 1);

    ret = do_scan<char>("a", "{:", ch);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);

    ret = do_scan<char>("a", "{:a}", ch);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
}

TEST_CASE("character type interop")
{
    char ch{};
    signed char sch{};
    unsigned char uch{};
    wchar_t wch{};

    auto ret = scn::scan("1 2 3 4", "{} {} {}", ch, sch, uch);
    CHECK(ret);
    CHECK(ret.range_as_string() == " 4");
    CHECK(ch == '1');
    CHECK(sch == 2);
    CHECK(uch == 3);

    auto wret = scn::scan(L"5 6 7 8", L"{} {} {}", wch, sch, uch);
    CHECK(wret);
    CHECK(wret.range_as_string() == L" 8");
    CHECK(wch == L'5');
    CHECK(sch == 6);
    CHECK(uch == 7);

    ret = scn::scan("1", "{}", wch);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_operation);

    wret = scn::scan(L"1", L"{}", ch);
    CHECK(!wret);
    CHECK(wret.error() == scn::error::invalid_operation);
}
