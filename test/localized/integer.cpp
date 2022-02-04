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
#include "../test.h"

TEST_CASE("integer ranges")
{
    short a;
    auto ret = scn::scan_localized(std::locale::classic(), "1", "{:L}", a);
    CHECK(ret);
    CHECK(a == 1);
    CHECK(ret.range().empty());
    a = 0;

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "1", "{:L}", a);
    CHECK(ret);
    CHECK(a == 1);
    CHECK(ret.range().empty());
    a = 0;

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "1", "{:L}", a);
    CHECK(ret);
    CHECK(a == 1);
    CHECK(ret.range().empty());
    a = 0;

    ret = scn::scan_localized(std::locale::classic(), "99999", "{:L}", a);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::value_out_of_range);
    CHECK(a == 0);
    a = 0;

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "99999", "{:L}", a);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::value_out_of_range);
    CHECK(a == 0);
    a = 0;

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "99999", "{:L}", a);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::value_out_of_range);
    CHECK(a == 0);
    a = 0;

    ret = scn::scan_localized(std::locale::classic(), "-99999", "{:L}", a);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::value_out_of_range);
    CHECK(a == 0);
    a = 0;

    ret = scn::scan_localized(std::locale{"en_US.UTF-8"}, "-99999", "{:L}", a);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::value_out_of_range);
    CHECK(a == 0);
    a = 0;

    ret = scn::scan_localized(std::locale{"fi_FI.UTF-8"}, "-99999", "{:L}", a);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::value_out_of_range);
    CHECK(a == 0);
    a = 0;
}

TEST_CASE("Option L")
{
    int i{};
    auto fi = std::locale{"fi_FI.UTF-8"};

    // {} with locale -> no effect
    auto ret = scn::scan_localized(fi, "1", "{}", i);
    CHECK(ret);
    CHECK(i == 1);
    i = 0;

    // {:L} with locale -> use supplied locale
    ret = scn::scan_localized(fi, "1", "{:L}", i);
    CHECK(ret);
    CHECK(i == 1);
    i = 0;

    // {:L} without locale -> use global C++ locale
    ret = scn::scan("1", "{:L}", i);
    CHECK(ret);
    CHECK(i == 1);
    i = 0;
}

TEST_CASE("Option n")
{
    int i{};
    auto fi = std::locale{"fi_FI.UTF-8"};

    // {:n} with locale -> implies {:L}, use iostreams
    auto ret = scn::scan_localized(fi, "1", "{:n}", i);
    CHECK(ret);
    CHECK(i == 1);
    i = 0;

    // {:Ln} == {:n}
    ret = scn::scan_localized(fi, "1", "{:Ln}", i);
    CHECK(ret);
    CHECK(i == 1);
    i = 0;

    // {:n} without locale -> use global C++ locale
    ret = scn::scan("1", "{:n}", i);
    CHECK(ret);
    CHECK(i == 1);
    i = 0;
}

TEST_CASE("thsep")
{
    int i{};
    auto en = std::locale{"en_US.UTF-8"};
    auto fi = std::locale{"fi_FI.UTF-8"};

    // {:L'} with locale -> locale thsep, built-in parser
    auto ret = scn::scan_localized(en, "100,200", "{:L'}", i);
    CHECK(ret);
    CHECK(i == 100200);
    i = 0;

    // {:n'} with locale -> locale thsep, use iostreams
    ret = scn::scan_localized(en, "100,200", "{:n'}", i);
    CHECK(ret);
    CHECK(i == 100200);
    i = 0;

    // {:Ln'} == {:n'}
    ret = scn::scan_localized(en, "100,200", "{:Ln'}", i);
    CHECK(ret);
    CHECK(i == 100200);
    i = 0;

    // {:'} with locale -> default thsep
    ret = scn::scan_localized(fi, "100,200", "{:'}", i);
    CHECK(ret);
    CHECK(i == 100200);
    i = 0;

    // {:L'} without locale -> use global locale
    ret = scn::scan("100,200", "{:L'}", i);
    CHECK(ret);
    CHECK(i == 100200);
    i = 0;
}

TEST_CASE("base L")
{
    int i{};
    auto en = std::locale{"en_US.UTF-8"};
    auto fi = std::locale{"fi_FI.UTF-8"};

    // o with prefix
    auto ret = scn::scan_localized(en, "010", "{:Lo}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "010", "{:Lo}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    ret = scn::scan_localized(en, "0o10", "{:Lo}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "0o10", "{:Lo}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // o without prefix
    ret = scn::scan_localized(en, "10", "{:Lo}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:Lo}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // x with prefix
    ret = scn::scan_localized(en, "0x10", "{:Lx}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;
    ret = scn::scan_localized(fi, "0x10", "{:Lx}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;

    // x without prefix
    ret = scn::scan_localized(en, "10", "{:Lx}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:Lx}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;

    // i base detect -> binary
    ret = scn::scan_localized(en, "0b10", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 2);
    i = 0;
    ret = scn::scan_localized(fi, "0b10", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 2);
    i = 0;

    // i base detect -> octal
    ret = scn::scan_localized(en, "010", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "010", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // i base detect -> octal 0o
    ret = scn::scan_localized(en, "0o10", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "0o10", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // i base detect -> hex
    ret = scn::scan_localized(en, "0x10", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;
    ret = scn::scan_localized(fi, "0x10", "{:Li}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;

    // b with prefix
    ret = scn::scan_localized(en, "0b10", "{:Lb}", i);
    CHECK(ret);
    CHECK(i == 2);
    i = 0;
    ret = scn::scan_localized(fi, "0b10", "{:Lb}", i);
    CHECK(ret);
    CHECK(i == 2);
    i = 0;

    // b without prefix
    ret = scn::scan_localized(en, "10", "{:Lb}", i);
    CHECK(ret);
    CHECK(i == 2);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:Lb}", i);
    CHECK(ret);
    CHECK(i == 2);
    i = 0;

    // u - signed -> fail
    ret = scn::scan_localized(en, "-10", "{:Lu}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
    i = 0;
    ret = scn::scan_localized(fi, "-10", "{:Lu}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
    i = 0;

    // u + signed
    ret = scn::scan_localized(en, "+10", "{:Lu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;
    ret = scn::scan_localized(fi, "+10", "{:Lu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;

    // u, no sign
    ret = scn::scan_localized(en, "10", "{:Lu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:Lu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;

    // B__
    ret = scn::scan_localized(en, "10", "{:LB11}", i);
    CHECK(ret);
    CHECK(i == 11);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:LB11}", i);
    CHECK(ret);
    CHECK(i == 11);
    i = 0;
}

TEST_CASE("base n")
{
    int i{};
    auto en = std::locale{"en_US.UTF-8"};
    auto fi = std::locale{"fi_FI.UTF-8"};

    // o with prefix
    auto ret = scn::scan_localized(en, "010", "{:no}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "010", "{:no}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // 0o prefix
    ret = scn::scan_localized(en, "0o10", "{:no}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "0o10", "{:no}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // o without prefix
    ret = scn::scan_localized(en, "10", "{:no}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:no}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // x with prefix
    ret = scn::scan_localized(en, "0x10", "{:nx}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;
    ret = scn::scan_localized(fi, "0x10", "{:nx}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;

    // x without prefix
    ret = scn::scan_localized(en, "10", "{:nx}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:nx}", i);
    CHECK(ret);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;

    // i base detect -> binary -> fail
    ret = scn::scan_localized(en, "0b10", "{:ni}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
    i = 0;
    ret = scn::scan_localized(fi, "0b10", "{:ni}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
    i = 0;

    // i base detect -> octal
    ret = scn::scan_localized(en, "010", "{:ni}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "010", "{:ni}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // i base detect -> octal 0o
    ret = scn::scan_localized(en, "0o10", "{:ni}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;
    ret = scn::scan_localized(fi, "0o10", "{:ni}", i);
    CHECK(ret);
    CHECK(i == 8);
    i = 0;

    // i base detect -> hex
    ret = scn::scan_localized(en, "0x10", "{:ni}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;
    ret = scn::scan_localized(fi, "0x10", "{:ni}", i);
    CHECK(ret);
    CHECK(i == 16);
    i = 0;

    // b with prefix -> fail
    ret = scn::scan_localized(en, "0b10", "{:nb}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 0);
    i = 0;
    ret = scn::scan_localized(fi, "0b10", "{:nb}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 0);
    i = 0;

    // b without prefix -> fail
    ret = scn::scan_localized(en, "10", "{:nb}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 0);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:nb}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 0);
    i = 0;

    // u - signed -> fail
    ret = scn::scan_localized(en, "-10", "{:nu}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
    i = 0;
    ret = scn::scan_localized(fi, "-10", "{:nu}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
    i = 0;

    // u + signed
    ret = scn::scan_localized(en, "+10", "{:nu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;
    ret = scn::scan_localized(fi, "+10", "{:nu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;

    // u, no sign
    ret = scn::scan_localized(en, "10", "{:nu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:nu}", i);
    CHECK(ret);
    CHECK(i == 10);
    i = 0;

    // B__ -> fail
    ret = scn::scan_localized(en, "10", "{:nB11}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 0);
    i = 0;
    ret = scn::scan_localized(fi, "10", "{:nB11}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 0);
    i = 0;
}
