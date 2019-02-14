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

#include "test.h"

TEST_CASE_TEMPLATE_DEFINE("string", CharT, string_test)
{
    using string_type = std::basic_string<CharT>;
    {
        string_type s{}, s2{};
        auto e = scan_value<CharT>("thisisaword nextword", "{} {}", s, s2);
        CHECK(s == widen<CharT>("thisisaword"));
        CHECK(s2 == widen<CharT>("nextword"));
        CHECK(e);
    }
    {
        string_type s{};
        auto e = scan_value<CharT>("WoRdW1th_Special<>Charact3rs", "{}", s);
        CHECK(s == widen<CharT>("WoRdW1th_Special<>Charact3rs"));
        CHECK(e);
    }
}

TEST_CASE_TEMPLATE_DEFINE("getline", CharT, getline_test)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>(
        "firstline\n"
        "Second line with spaces");
    auto stream = scn::make_stream(data);

    {
        string_type s{};
        auto ret = scn::getline(stream, s);
        CHECK(s == widen<CharT>("firstline"));
        CHECK(ret);
    }
    {
        string_type s{};
        auto ret = scn::getline(stream, s);
        CHECK(s == widen<CharT>("Second line with spaces"));
        CHECK(ret);
    }
}

TEST_CASE_TEMPLATE_DEFINE("ignore", CharT, ignore_test)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>("line1\nline2");
    auto stream = scn::make_stream(data);
    auto fstr = widen<CharT>("{}");
    auto f = scn::basic_string_view<CharT>(fstr.data(), fstr.size());

    SUBCASE("ignore_n")
    {
        string_type s{};
        auto ret = scn::ignore_n(stream, 6);
        CHECK(ret);

        ret = scn::scan(stream, f, s);
        CHECK(s == widen<CharT>("line2"));
        CHECK(ret);
    }
    SUBCASE("ignore_until")
    {
        string_type s{};
        auto ret = scn::ignore_until(stream, 0x0a);  // '\n'
        CHECK(ret);

        ret = scn::scan(stream, f, s);
        CHECK(s == widen<CharT>("line2"));
        CHECK(ret);
    }
    SUBCASE("ignore_all")
    {
        string_type s{};
        auto ret = scn::ignore_all(stream);
        CHECK(ret);

        ret = scn::scan(stream, f, s);
        CHECK(!ret);
        if (!ret) {
            CHECK(ret == scn::error::end_of_stream);
        }
    }
}

TEST_CASE_TEMPLATE_INSTANTIATE(string_test, char, wchar_t);
TEST_CASE_TEMPLATE_INSTANTIATE(getline_test, char, wchar_t);
TEST_CASE_TEMPLATE_INSTANTIATE(ignore_test, char, wchar_t);
