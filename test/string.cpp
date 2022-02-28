// Copyright 2017 Elias Kosunen
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

TEST_CASE_TEMPLATE("string test", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    {
        string_type s{}, s2{};
        auto e = do_scan<CharT>("thisisaword nextword", "{} {}", s, s2);
        CHECK(s == widen<CharT>("thisisaword"));
        CHECK(s2 == widen<CharT>("nextword"));
        CHECK(e);
    }
    {
        string_type s{};
        auto e = do_scan<CharT>("WoRdW1th_Special<>Charact3rs!?", "{}", s);
        CHECK(s == widen<CharT>("WoRdW1th_Special<>Charact3rs!?"));
        CHECK(e);
    }
    {
        string_type s{};
        auto e = do_scan<CharT>("foo", "{:s}", s);
        CHECK(s == widen<CharT>("foo"));
        CHECK(e);
    }
    {
        string_type s{};
        auto e = do_scan<CharT>("foo", "{:a}", s);
        CHECK(s.empty());
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
    }
    {
        string_type s{};

        auto e = do_scan<CharT>(" space", "{}", s);
        CHECK(e);
        CHECK(s == widen<CharT>("space"));
        s.clear();

        e = do_scan<CharT>(" space", " {}", s);
        CHECK(e);
        CHECK(s == widen<CharT>("space"));
    }
}

TEST_CASE_TEMPLATE("getline", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>(
        "firstline\n"
        "Second line with spaces");

    using string_view_type = scn::basic_string_view<CharT>;

    SUBCASE("string")
    {
        string_type s{};
        auto ret = scn::getline(data, s, scn::detail::ascii_widen<CharT>('\n'));
        CHECK(ret);
        CHECK(s == widen<CharT>("firstline"));

        ret = scn::getline(ret.range(), s);
        CHECK(ret);
        CHECK(ret.empty());
        CHECK(s == widen<CharT>("Second line with spaces"));
    }
    SUBCASE("string_view")
    {
        string_view_type s{};
        auto ret = scn::getline(data, s, scn::detail::ascii_widen<CharT>('\n'));
        CHECK(ret);
        CHECK(string_type{s.data(), s.size()} == widen<CharT>("firstline"));
        CHECK(!ret.empty());

        ret = scn::getline(ret.range(), s);
        CHECK(ret);
        CHECK(string_type{s.data(), s.size()} ==
              widen<CharT>("Second line with spaces"));
        CHECK(ret.empty());
    }
    SUBCASE("non-contiguous")
    {
        string_type s{};
        auto source = get_deque<CharT>(data);
        auto ret =
            scn::getline(source, s, scn::detail::ascii_widen<CharT>('\n'));
        CHECK(ret);
        CHECK(s == widen<CharT>("firstline"));

        ret = scn::getline(ret.range(), s);
        CHECK(ret);
        CHECK(ret.empty());
        CHECK(s == widen<CharT>("Second line with spaces"));
    }
}

TEST_CASE_TEMPLATE("ignore", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>("line1\nline2");

    SUBCASE("ignore_until")
    {
        string_type s{};
        {
            auto ret = scn::ignore_until(data, CharT{0x0a});  // '\n'
            CHECK(ret);
            data.assign(ret.range_as_string());
        }

        {
            auto ret = scn::scan_default(data, s);
            CHECK(s == widen<CharT>("line2"));
            CHECK(ret);
        }
    }

    SUBCASE("not found")
    {
        auto ret = scn::ignore_until(data, CharT{0x33});  // '3'
        CHECK(ret);
        CHECK(ret.range().size() == 0);
    }

    SUBCASE("empty range")
    {
        string_type s{};
        auto ret = scn::ignore_until(s, CharT{0x0a});
        CHECK(!ret);
        CHECK(ret.error().code() == scn::error::end_of_range);

        scn::basic_string_view<CharT> sv{};
        auto result = scn::make_result(sv);
        ret = scn::ignore_until(result.range(), CharT{0x00});
        CHECK(!ret);
        CHECK(ret.error().code() == scn::error::end_of_range);
    }
}
