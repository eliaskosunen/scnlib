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

TEST_CASE("basic")
{
    std::string a, b;
    auto ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "abc åäö",
                                   "{:L} {}", a, b);
    CHECK(ret);
    CHECK(a == "abc");
    CHECK(b == "åäö");

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "abc åäö", "{:L} {}",
                              a, b);
    CHECK(ret);
    CHECK(a == "abc");
    CHECK(b == "åäö");
}

TEST_CASE(":alpha:")
{
    std::string str;
    auto ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "aä",
                                   "{:[:alpha:]}", str);
    CHECK(ret);
    CHECK(str == "a");
    CHECK(ret.range_as_string() == "ä");
    str.clear();

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "aä", "{:L[:alpha:]}",
                              str);
    CHECK(ret);
    CHECK(str == "aä");
    CHECK(ret.range().empty());
    str.clear();

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "aä", "{:[:alpha:]}",
                              str);
    CHECK(ret);
    CHECK(str == "a");
    CHECK(ret.range_as_string() == "ä");
    str.clear();

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "aä", "{:L[:alpha:]}",
                              str);
    CHECK(ret);
    CHECK(str == "aä");
    CHECK(ret.empty());
    str.clear();
}
