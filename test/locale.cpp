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

TEST_CASE("static locale")
{
    scn::detail::basic_static_locale_ref<char> loc{};
    scn::detail::basic_static_locale_ref<wchar_t> wloc{};

    SUBCASE("space")
    {
        CHECK(loc.is_space(' '));
        CHECK(loc.is_space('\n'));
        CHECK(loc.is_space('\r'));
        CHECK(loc.is_space('\t'));
        CHECK(loc.is_space('\v'));
        CHECK(loc.is_space('\f'));
        CHECK(!loc.is_space('0'));
        CHECK(!loc.is_space('a'));
        CHECK(!loc.is_space('Z'));
        CHECK(!loc.is_space('Z'));
        CHECK(!loc.is_space('@'));

        CHECK(wloc.is_space(L' '));
        CHECK(wloc.is_space(L'\n'));
        CHECK(wloc.is_space(L'\r'));
        CHECK(wloc.is_space(L'\t'));
        CHECK(wloc.is_space(L'\v'));
        CHECK(wloc.is_space(L'\f'));
        CHECK(!wloc.is_space(L'0'));
        CHECK(!wloc.is_space(L'a'));
        CHECK(!wloc.is_space(L'Z'));
        CHECK(!wloc.is_space(L'Z'));
        CHECK(!wloc.is_space(L'@'));
    }

    SUBCASE("digit")
    {
        for (char i = 0; i != 10; ++i) {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            CHECK(loc.is_digit('0' + i));
            CHECK(wloc.is_digit(L'0' + i));
            SCN_GCC_POP
        }
        CHECK(!loc.is_digit('a'));
        CHECK(!loc.is_digit('Z'));
        CHECK(!loc.is_digit(' '));
        CHECK(!loc.is_digit('@'));

        CHECK(!wloc.is_digit(L'a'));
        CHECK(!wloc.is_digit(L'Z'));
        CHECK(!wloc.is_digit(L' '));
        CHECK(!wloc.is_digit(L'@'));
    }

    SUBCASE("decimal_point & thousands_separator")
    {
        CHECK(loc.decimal_point() == '.');
        CHECK(wloc.decimal_point() == L'.');

        CHECK(loc.thousands_separator() == ',');
        CHECK(wloc.thousands_separator() == L',');
    }

    SUBCASE("truename & falsename")
    {
        CHECK_EQ(
            std::strncmp(loc.truename().data(), "true", loc.truename().size()),
            0);
        CHECK_EQ(std::wcsncmp(wloc.truename().data(), L"true",
                              wloc.truename().size()),
                 0);

        CHECK_EQ(std::strncmp(loc.falsename().data(), "false",
                              loc.falsename().size()),
                 0);
        CHECK_EQ(std::wcsncmp(wloc.falsename().data(), L"false",
                              wloc.falsename().size()),
                 0);
    }
}

TEST_CASE("custom locale")
{
    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
    scn::detail::basic_custom_locale_ref<char> loc{&std::locale::classic()};
    scn::detail::basic_custom_locale_ref<wchar_t> wloc{&std::locale::classic()};
    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

    SUBCASE("space")
    {
        CHECK(loc.is_space(' '));
        CHECK(loc.is_space('\n'));
        CHECK(loc.is_space('\r'));
        CHECK(loc.is_space('\t'));
        CHECK(loc.is_space('\v'));
        CHECK(loc.is_space('\f'));
        CHECK(!loc.is_space('0'));
        CHECK(!loc.is_space('a'));
        CHECK(!loc.is_space('Z'));
        CHECK(!loc.is_space('Z'));
        CHECK(!loc.is_space('@'));

        CHECK(wloc.is_space(L' '));
        CHECK(wloc.is_space(L'\n'));
        CHECK(wloc.is_space(L'\r'));
        CHECK(wloc.is_space(L'\t'));
        CHECK(wloc.is_space(L'\v'));
        CHECK(wloc.is_space(L'\f'));
        CHECK(!wloc.is_space(L'0'));
        CHECK(!wloc.is_space(L'a'));
        CHECK(!wloc.is_space(L'Z'));
        CHECK(!wloc.is_space(L'Z'));
        CHECK(!wloc.is_space(L'@'));
    }

    SUBCASE("digit")
    {
        for (char i = 0; i != 10; ++i) {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            CHECK(loc.is_digit('0' + i));
            CHECK(wloc.is_digit(L'0' + i));
            SCN_GCC_POP
        }
        CHECK(!loc.is_digit('a'));
        CHECK(!loc.is_digit('Z'));
        CHECK(!loc.is_digit(' '));
        CHECK(!loc.is_digit('@'));

        CHECK(!wloc.is_digit(L'a'));
        CHECK(!wloc.is_digit(L'Z'));
        CHECK(!wloc.is_digit(L' '));
        CHECK(!wloc.is_digit(L'@'));
    }

    SUBCASE("decimal_point & thousands_separator")
    {
        CHECK(loc.decimal_point() == '.');
        CHECK(wloc.decimal_point() == L'.');

        CHECK(loc.thousands_separator() == ',');
        CHECK(wloc.thousands_separator() == L',');
    }

    SUBCASE("truename & falsename")
    {
        CHECK_EQ(
            std::strncmp(loc.truename().data(), "true", loc.truename().size()),
            0);
        CHECK_EQ(std::wcsncmp(wloc.truename().data(), L"true",
                              wloc.truename().size()),
                 0);

        CHECK_EQ(std::strncmp(loc.falsename().data(), "false",
                              loc.falsename().size()),
                 0);
        CHECK_EQ(std::wcsncmp(wloc.falsename().data(), L"false",
                              wloc.falsename().size()),
                 0);
    }

#if 0
    SUBCASE("widen & narrow")
    {
        CHECK(loc.widen('a') == 'a');
        CHECK(wloc.widen('a') == L'a');

        CHECK(loc.narrow('a', 0) == 'a');
        CHECK(wloc.narrow(L'a', 0) == 'a');

        CHECK(wloc.narrow(1024, 0) == 0);
    }
#endif

    SUBCASE("read_num")
    {
        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
        std::string str{"42"};
        std::wstring wstr{L"123"};
        int i{};

        auto ret = loc.read_num(i, str, 0);
        CHECK(ret);
        CHECK(i == 42);

        ret = wloc.read_num(i, wstr, 0);
        CHECK(ret);
        CHECK(i == 123);

        str = "456 789";
        ret = loc.read_num(i, str, 0);
        CHECK(ret);
        CHECK(i == 456);
        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
    }
}

TEST_CASE("default localized scanning")
{
    SUBCASE("default")
    {
        int i{};
        double d{};

        auto ret = scn::scan("100,200 100.200", "{:'} {}", i, d);
        CHECK(ret);
        CHECK(i == 100200);
        CHECK(d == doctest::Approx(100.2));
    }
}
