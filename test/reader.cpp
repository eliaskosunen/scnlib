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

TEST_CASE_TEMPLATE("read_code_unit", CharT, char, wchar_t)
{
    SUBCASE("direct")
    {
        auto range = scn::wrap(widen<CharT>("42"));
        auto ret = scn::read_code_unit(range, false);
        CHECK(ret);
        CHECK(ret.value() == scn::detail::ascii_widen<CharT>('4'));

        ret = scn::read_code_unit(range);
        CHECK(ret.value() == scn::detail::ascii_widen<CharT>('4'));

        CHECK(*range.begin() == scn::detail::ascii_widen<CharT>('2'));
        range.advance();

        ret = scn::read_code_unit(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
    SUBCASE("indirect")
    {
        auto range = scn::wrap(get_indirect<CharT>(widen<CharT>("42")));
        auto ret = scn::read_code_unit(range, false);
        CHECK(ret);
        CHECK(ret.value() == scn::detail::ascii_widen<CharT>('4'));

        ret = scn::read_code_unit(range);
        CHECK(ret.value() == scn::detail::ascii_widen<CharT>('4'));

        CHECK((*range.begin()).value() == scn::detail::ascii_widen<CharT>('2'));
        range.advance();

        ret = scn::read_code_unit(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

// assumes wchar_t = utf-32
#if SCN_POSIX
TEST_CASE("read_code_point")
{
    unsigned char buf[4] = {0};

    SUBCASE("contiguous")
    {
        auto range = scn::wrap("aäa");

        auto ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == 'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 2);
        CHECK(ret.value().cp == scn::make_code_point(0xe4));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == 'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(range.empty());
    }
    SUBCASE("non-direct")
    {
        auto range = scn::wrap(get_indirect<char>("aäa"));

        auto ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == 'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 2);
        CHECK(ret.value().cp == scn::make_code_point(0xe4));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == 'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(range.size() == 1);
        CHECK(range.begin()->error().code() == scn::error::end_of_range);
    }

    SUBCASE("wide contiguous")
    {
        auto range = scn::wrap(L"aäa");

        auto ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == L'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().cp == scn::make_code_point(0xe4));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == L'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(range.empty());
    }
    SUBCASE("wide non-direct")
    {
        auto range = scn::wrap(get_indirect<wchar_t>(L"aäa"));

        auto ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == L'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().cp == scn::make_code_point(0xe4));
        CHECK(!range.empty());

        ret = scn::read_code_point(range, scn::make_span(buf, 4));
        CHECK(ret);
        CHECK(ret.value().chars.size() == 1);
        CHECK(ret.value().chars[0] == L'a');
        CHECK(ret.value().cp == scn::make_code_point(0x61));
        CHECK(range.size() == 1);
        CHECK(range.begin()->error().code() == scn::error::end_of_range);
    }
}
#endif

TEST_CASE_TEMPLATE("read_zero_copy", CharT, char, wchar_t)
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap(widen<CharT>("123"));
        auto ret = scn::read_zero_copy(range, 2);
        CHECK(ret);
        CHECK(ret.value().size() == 2);
        CHECK(ret.value()[0] == scn::detail::ascii_widen<CharT>('1'));
        CHECK(ret.value()[1] == scn::detail::ascii_widen<CharT>('2'));

        CHECK(*range.begin() == scn::detail::ascii_widen<CharT>('3'));
        range.advance();

        ret = scn::read_zero_copy(range, 1);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque<CharT>());
        auto ret = scn::read_zero_copy(range, 2);
        CHECK(ret);
        CHECK(ret.value().size() == 0);
        CHECK(range.size() == 3);

        range = scn::wrap(get_empty_deque<CharT>());
        ret = scn::read_zero_copy(range, 2);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE_TEMPLATE("read_all_zero_copy", CharT, char, wchar_t)
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap(widen<CharT>("123"));
        auto ret = scn::read_all_zero_copy(range);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == scn::detail::ascii_widen<CharT>('1'));
        CHECK(ret.value()[1] == scn::detail::ascii_widen<CharT>('2'));
        CHECK(ret.value()[2] == scn::detail::ascii_widen<CharT>('3'));

        CHECK(range.begin() == range.end());
        ret = scn::read_all_zero_copy(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque<CharT>());
        auto ret = scn::read_all_zero_copy(range);
        CHECK(ret);
        CHECK(ret.value().size() == 0);
        CHECK(range.size() == 3);

        range = scn::wrap(get_empty_deque<CharT>());
        ret = scn::read_all_zero_copy(range);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE_TEMPLATE("read_into", CharT, char, wchar_t)
{
    SUBCASE("contiguous + direct")
    {
        auto range = scn::wrap(widen<CharT>("123"));
        std::vector<CharT> data{};
        auto it = std::back_inserter(data);
        auto ret = scn::read_into(range, it, 2);
        CHECK(ret);
        CHECK(data.size() == 2);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('1'));
        CHECK(data[1] == scn::detail::ascii_widen<CharT>('2'));

        data.clear();
        ret = scn::read_into(range, it, 2);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('3'));

        ret = scn::read_into(range, it, 1);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('3'));
    }

    SUBCASE("direct")
    {
        auto range = scn::wrap(get_deque<CharT>());
        std::vector<CharT> data{};
        auto it = std::back_inserter(data);
        auto ret = scn::read_into(range, it, 2);
        CHECK(ret);
        CHECK(data.size() == 2);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('1'));
        CHECK(data[1] == scn::detail::ascii_widen<CharT>('2'));

        data.clear();
        ret = scn::read_into(range, it, 2);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('3'));

        ret = scn::read_into(range, it, 1);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('3'));
    }

    SUBCASE("indirect")
    {
        auto range = scn::wrap(get_indirect<CharT>(widen<CharT>("123")));
        std::vector<CharT> data{};
        auto it = std::back_inserter(data);
        auto ret = scn::read_into(range, it, 2);
        CHECK(ret);
        CHECK(data.size() == 2);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('1'));
        CHECK(data[1] == scn::detail::ascii_widen<CharT>('2'));

        data.clear();
        ret = scn::read_into(range, it, 2);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('3'));

        ret = scn::read_into(range, it, 1);
        CHECK(!ret);
        CHECK(ret == scn::error::end_of_range);
        CHECK(data.size() == 1);
        CHECK(data[0] == scn::detail::ascii_widen<CharT>('3'));
    }
}

TEST_CASE_TEMPLATE("read_until_space_zero_copy no final space",
                   CharT,
                   char,
                   wchar_t)
{
    auto locale = scn::make_default_locale_ref<CharT>();
    auto pred = scn::detail::make_is_space_predicate(locale, false);

    SUBCASE("contiguous")
    {
        auto range = scn::wrap(widen<CharT>("123 456"));
        auto ret = scn::read_until_space_zero_copy(range, pred, false);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == scn::detail::ascii_widen<CharT>('1'));
        CHECK(ret.value()[1] == scn::detail::ascii_widen<CharT>('2'));
        CHECK(ret.value()[2] == scn::detail::ascii_widen<CharT>('3'));

        CHECK(*range.begin() == ' ');
        range.advance();

        ret = scn::read_until_space_zero_copy(range, pred, false);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == scn::detail::ascii_widen<CharT>('4'));
        CHECK(ret.value()[1] == scn::detail::ascii_widen<CharT>('5'));
        CHECK(ret.value()[2] == scn::detail::ascii_widen<CharT>('6'));

        CHECK(range.begin() == range.end());
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque<CharT>(widen<CharT>("123 456")));
        auto ret = scn::read_until_space_zero_copy(range, pred, false);
        CHECK(ret);
        CHECK(ret.value().size() == 0);

        range.advance(7);
        ret = scn::read_until_space_zero_copy(range, pred, false);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE_TEMPLATE("read_until_space_zero_copy keep final space",
                   CharT,
                   char,
                   wchar_t)
{
    auto locale = scn::make_default_locale_ref<CharT>();
    auto pred = scn::detail::make_is_space_predicate(locale, false);

    SUBCASE("contiguous")
    {
        auto range = scn::wrap(widen<CharT>("123 456"));
        auto ret = scn::read_until_space_zero_copy(range, pred, true);
        CHECK(ret);
        CHECK(ret.value().size() == 4);
        CHECK(ret.value()[0] == scn::detail::ascii_widen<CharT>('1'));
        CHECK(ret.value()[1] == scn::detail::ascii_widen<CharT>('2'));
        CHECK(ret.value()[2] == scn::detail::ascii_widen<CharT>('3'));
        CHECK(ret.value()[3] == scn::detail::ascii_widen<CharT>(' '));

        ret = scn::read_until_space_zero_copy(range, pred, true);
        CHECK(ret);
        CHECK(ret.value().size() == 3);
        CHECK(ret.value()[0] == scn::detail::ascii_widen<CharT>('4'));
        CHECK(ret.value()[1] == scn::detail::ascii_widen<CharT>('5'));
        CHECK(ret.value()[2] == scn::detail::ascii_widen<CharT>('6'));

        CHECK(range.begin() == range.end());
    }

    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque<CharT>(widen<CharT>("123 456")));
        auto ret = scn::read_until_space_zero_copy(range, pred, true);
        CHECK(ret);
        CHECK(ret.value().size() == 0);

        range.advance(7);
        ret = scn::read_until_space_zero_copy(range, pred, true);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::end_of_range);
    }
}

TEST_CASE("putback_n")
{
    SUBCASE("contiguous")
    {
        auto range = scn::wrap("abc");
        range.advance(2);

        auto e = scn::putback_n(range, 2);
        CHECK(e);
        CHECK(range.data() == std::string{"abc"});
    }
    SUBCASE("non-contiguous")
    {
        auto range = scn::wrap(get_deque<char>("abc"));
        range.advance(2);

        auto e = scn::putback_n(range, 2);
        CHECK(e);
        CHECK(range.size() == 3);
        CHECK(*range.begin() == 'a');
    }
}
