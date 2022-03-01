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

TEST_CASE_TEMPLATE("char localized", CharT, char, wchar_t)
{
    SUBCASE("single code unit")
    {
        CharT ch{};
        auto ret = scn::scan(widen<CharT>("a"), widen<CharT>("{:L}"), ch);
        CHECK(ret);
        CHECK(ch == scn::detail::ascii_widen<CharT>('a'));
        CHECK(ret.range().empty());

        ret = scn::scan_localized(std::locale{}, widen<CharT>("b"),
                                  widen<CharT>("{}"), ch);
        CHECK(ret);
        CHECK(ch == scn::detail::ascii_widen<CharT>('b'));
        CHECK(ret.range().empty());

        ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, widen<CharT>("c"),
                                  widen<CharT>("{}"), ch);
        CHECK(ret);
        CHECK(ch == scn::detail::ascii_widen<CharT>('c'));
        CHECK(ret.range().empty());

        ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, widen<CharT>("d"),
                                  widen<CharT>("{}"), ch);
        CHECK(ret);
        CHECK(ch == scn::detail::ascii_widen<CharT>('d'));
        CHECK(ret.range().empty());

        ret = scn::scan_localized(std::locale{}, widen<CharT>("e"),
                                  widen<CharT>("{:L}"), ch);
        CHECK(ret);
        CHECK(ch == scn::detail::ascii_widen<CharT>('e'));
        CHECK(ret.range().empty());

        ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, widen<CharT>("f"),
                                  widen<CharT>("{:L}"), ch);
        CHECK(ret);
        CHECK(ch == scn::detail::ascii_widen<CharT>('f'));
        CHECK(ret.range().empty());

        ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, widen<CharT>("g"),
                                  widen<CharT>("{:L}"), ch);
        CHECK(ret);
        CHECK(ch == scn::detail::ascii_widen<CharT>('g'));
        CHECK(ret.range().empty());
    }
}

TEST_CASE("char code unit in code point")
{
    char ch{};
    auto cmp = static_cast<char>(0xc3);

    // ä == c3 a4
    auto ret = scn::scan("ä", "{:L}", ch);
    CHECK(ret);
    CHECK(ch == cmp);
    CHECK(ret.range_as_string() == "\xa4");
    ch = 0;

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "ä", "{}", ch);
    CHECK(ret);
    CHECK(ch == cmp);
    CHECK(ret.range_as_string() == "\xa4");
    ch = 0;

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "ä", "{}", ch);
    CHECK(ret);
    CHECK(ch == cmp);
    CHECK(ret.range_as_string() == "\xa4");
    ch = 0;

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "ä", "{:L}", ch);
    CHECK(ret);
    CHECK(ch == cmp);
    CHECK(ret.range_as_string() == "\xa4");
    ch = 0;

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "ä", "{:L}", ch);
    CHECK(ret);
    CHECK(ch == cmp);
    CHECK(ret.range_as_string() == "\xa4");
    ch = 0;
}

TEST_CASE("code point")
{
    scn::code_point cp{};
    auto cmp = scn::make_code_point(0xe4);

    auto ret = scn::scan("ä", "{:L}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "ä", "{}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "ä", "{}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "ä", "{:L}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "ä", "{:L}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};
}

TEST_CASE("wide code point")
{
    scn::code_point cp{};
    auto cmp = scn::make_code_point(0xe4);

    auto ret = scn::scan(L"ä", L"{:L}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, L"ä", L"{}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, L"ä", L"{}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, L"ä", L"{:L}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, L"ä", L"{:L}", cp);
    CHECK(ret);
    CHECK(cp == cmp);
    CHECK(ret.range().empty());
    cp = scn::code_point{};
}

TEST_CASE("signed + unsigned char")
{
    signed char s1, s2, s3;

    auto ret =
        scn::scan_localized(std::locale{}, " -1 2", "{}{:c}{:c}", s1, s2, s3);
    CHECK(ret);
    CHECK(s1 == -1);
    CHECK(s2 == ' ');
    CHECK(s3 == '2');

    unsigned char u1, u2, u3;
    ret = scn::scan_localized(std::locale{}, " 1 2", "{:L}{:Lc}{:Lc}", u1, u2,
                              u3);
    CHECK(ret);
    CHECK(u1 == 1);
    CHECK(u2 == ' ');
    CHECK(u3 == '2');
}
