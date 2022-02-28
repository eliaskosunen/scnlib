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

TEST_CASE("C locale")
{
    std::setlocale(LC_NUMERIC, "C");
    float a, b;
    auto ret = scn::scan("3.14 3,14", "{} {}", a, b);
    CHECK(ret);
    CHECK(std::string{ret.begin(), ret.end()} == ",14");
    CHECK(a == doctest::Approx(3.14));
    CHECK(b == doctest::Approx(3.0));
}

TEST_CASE("Changing C locale")
{
    std::setlocale(LC_NUMERIC, "en_US.UTF-8");
    float a, b;
    auto ret = scn::scan("3.14 3,14", "{} {}", a, b);
    CHECK(ret);
    CHECK(std::string{ret.begin(), ret.end()} == ",14");
    CHECK(a == doctest::Approx(3.14));
    CHECK(b == doctest::Approx(3.0));
    a = b = 0;

    // C locale change not affecting behavior
    std::setlocale(LC_NUMERIC, "fi_FI.UTF-8");
    ret = scn::scan("3.14 3,14", "{} {}", a, b);
    CHECK(ret);
    CHECK(std::string{ret.begin(), ret.end()} == ",14");
    CHECK(a == doctest::Approx(3.14));
    CHECK(b == doctest::Approx(3.0));

    // Also not affecting when using scan_localized
    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "3.14 3,14", "{} {}",
                              a, b);
    CHECK(ret);
    CHECK(std::string{ret.begin(), ret.end()} == ",14");
    CHECK(a == doctest::Approx(3.14));
    CHECK(b == doctest::Approx(3.0));

    std::setlocale(LC_NUMERIC, "C");
}

TEST_CASE("Changing global C++ locale")
{
    std::locale::global(std::locale("en_US.UTF-8"));
    float a, b;
    auto ret = scn::scan("3.14 3,14", "{} {}", a, b);
    CHECK(ret);
    CHECK(std::string{ret.begin(), ret.end()} == ",14");
    CHECK(a == doctest::Approx(3.14));
    CHECK(b == doctest::Approx(3.0));
    a = b = 0;

    // Global C++ locale change not affecting behavior
    std::locale::global(std::locale("fi_FI.UTF-8"));
    ret = scn::scan("3.14 3,14", "{} {}", a, b);
    CHECK(ret);
    CHECK(std::string{ret.begin(), ret.end()} == ",14");
    CHECK(a == doctest::Approx(3.14));
    CHECK(b == doctest::Approx(3.0));

    // Also not affecting when using scan_localized
    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "3.14 3,14", "{} {}",
                              a, b);
    CHECK(ret);
    CHECK(std::string{ret.begin(), ret.end()} == ",14");
    CHECK(a == doctest::Approx(3.14));
    CHECK(b == doctest::Approx(3.0));

    std::locale::global(std::locale::classic());
}

TEST_CASE("custom_locale_ref")
{
    auto to_locale = [](const void* l) {
        return *static_cast<const std::locale*>(l);
    };

    SUBCASE("basic value operations")
    {
        SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
        auto loc = scn::detail::basic_custom_locale_ref<char>();
        CHECK(to_locale(loc.get_locale()).name() == "C");
        CHECK(to_locale(loc.get_locale()).name() ==
              to_locale(loc.make_classic().get_locale()).name());
        loc.convert_to_global();
        CHECK(to_locale(loc.get_locale()).name() ==
              to_locale(loc.make_classic().get_locale()).name());

        auto enus = std::locale{"en_US.UTF-8"};
        auto other = scn::detail::basic_custom_locale_ref<char>(&enus);
        loc = SCN_MOVE(other);
        CHECK(to_locale(loc.get_locale()) == enus);

        auto other2 = SCN_MOVE(loc);
        CHECK(to_locale(other2.get_locale()) == enus);
        SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
    }
}
