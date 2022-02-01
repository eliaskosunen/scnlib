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
}

TEST_CASE_TEMPLATE("ignore", CharT, char, wchar_t)
{
    using string_type = std::basic_string<CharT>;
    string_type data = widen<CharT>("line1\nline2");

    SUBCASE("ignore_until")
    {
        string_type s{};
        {
            auto ret = scn::ignore_until(data, 0x0a);  // '\n'
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
        auto ret = scn::ignore_until(data, 0x33);  // '3'
        CHECK(ret);
        CHECK(ret.range().size() == 0);
    }

    SUBCASE("empty range")
    {
        string_type s{};
        auto ret = scn::ignore_until(s, 0x0a);
        CHECK(!ret);
        CHECK(ret.error().code() == scn::error::end_of_range);

        scn::basic_string_view<CharT> sv{};
        auto result = scn::make_result(sv);
        ret = scn::ignore_until(result.range(), 0x00);
        CHECK(!ret);
        CHECK(ret.error().code() == scn::error::end_of_range);
    }
}

TEST_CASE("set parse")
{
    scn::basic_default_locale_ref<char> locale{};
    auto make_parse_ctx = [&](scn::string_view str) {
        return scn::basic_parse_context<scn::basic_default_locale_ref<char>>{
            str, locale};
    };

    using scanner_type = scn::detail::string_scanner;
    using set_parser_type = scn::detail::set_parser_type;

    SUBCASE("empty (s)")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("s}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
    }

    SUBCASE("empty set")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
    }

    SUBCASE("empty set + L")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("L[]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK((scanner.common_options & scanner_type::localized) != 0);
    }

    SUBCASE(":alpha:")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[:alpha:]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(scanner.set_parser.get_option('z'));
        CHECK(scanner.set_parser.get_option('A'));
        CHECK(scanner.set_parser.get_option('Z'));
        CHECK(!scanner.set_parser.get_option('0'));
        CHECK(!scanner.set_parser.get_option('9'));
        CHECK(!scanner.set_parser.get_option('['));
        CHECK(!scanner.set_parser.get_option('`'));
        CHECK(!scanner.set_parser.get_option('@'));
        CHECK(!scanner.set_parser.get_option('{'));
        CHECK(!scanner.set_parser.get_option('/'));
        CHECK(!scanner.set_parser.get_option(':'));
    }

    SUBCASE(":alpha::digit:")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[:alpha::digit:]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::digit));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alnum));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(scanner.set_parser.get_option('z'));
        CHECK(scanner.set_parser.get_option('A'));
        CHECK(scanner.set_parser.get_option('Z'));
        CHECK(scanner.set_parser.get_option('0'));
        CHECK(scanner.set_parser.get_option('9'));
        CHECK(!scanner.set_parser.get_option('_'));
        CHECK(!scanner.set_parser.get_option('['));
        CHECK(!scanner.set_parser.get_option('`'));
        CHECK(!scanner.set_parser.get_option('@'));
        CHECK(!scanner.set_parser.get_option('{'));
        CHECK(!scanner.set_parser.get_option('/'));
        CHECK(!scanner.set_parser.get_option(':'));
    }

    SUBCASE("\\w")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\w]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::alnum_underscore));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(scanner.set_parser.get_option('z'));
        CHECK(scanner.set_parser.get_option('A'));
        CHECK(scanner.set_parser.get_option('Z'));
        CHECK(scanner.set_parser.get_option('0'));
        CHECK(scanner.set_parser.get_option('9'));
        CHECK(scanner.set_parser.get_option('_'));
        CHECK(!scanner.set_parser.get_option('['));
        CHECK(!scanner.set_parser.get_option('`'));
        CHECK(!scanner.set_parser.get_option('@'));
        CHECK(!scanner.set_parser.get_option('{'));
        CHECK(!scanner.set_parser.get_option('/'));
        CHECK(!scanner.set_parser.get_option(':'));
    }

    SUBCASE("\\W")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\W]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::alnum_underscore));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::inverted_alnum_underscore));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option('a'));
        CHECK(!scanner.set_parser.get_option('z'));
        CHECK(!scanner.set_parser.get_option('A'));
        CHECK(!scanner.set_parser.get_option('Z'));
        CHECK(!scanner.set_parser.get_option('0'));
        CHECK(!scanner.set_parser.get_option('9'));
        CHECK(!scanner.set_parser.get_option('_'));
        CHECK(scanner.set_parser.get_option('['));
        CHECK(scanner.set_parser.get_option('`'));
        CHECK(scanner.set_parser.get_option('@'));
        CHECK(scanner.set_parser.get_option('{'));
        CHECK(scanner.set_parser.get_option('/'));
        CHECK(scanner.set_parser.get_option(':'));
    }

    SUBCASE("\\n")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\n]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::alnum_underscore));
        CHECK(scanner.set_parser.get_option('\n'));
        CHECK(!scanner.set_parser.get_option('\t'));
    }

    SUBCASE("literal \\]")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\]]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::alnum_underscore));
        CHECK(scanner.set_parser.get_option(']'));
        CHECK(!scanner.set_parser.get_option('\\'));
    }

    SUBCASE("literal \\^")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\^]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(set_parser_type::flag::inverted));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::alnum_underscore));
        CHECK(scanner.set_parser.get_option('^'));
        CHECK(!scanner.set_parser.get_option('\\'));
        CHECK(!scanner.set_parser.get_option(']'));
    }

    SUBCASE("literal \\:")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\:]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::alnum_underscore));
        CHECK(scanner.set_parser.get_option(':'));
        CHECK(!scanner.set_parser.get_option('\\'));
        CHECK(!scanner.set_parser.get_option(']'));
    }

    SUBCASE("literal \\")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\\\]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::specifier::alnum_underscore));
        CHECK(scanner.set_parser.get_option('\\'));
        CHECK(!scanner.set_parser.get_option(']'));
    }

    SUBCASE("erroneous \\")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\]}");
        auto e = scanner.parse(pctx);
        CHECK(!e);
        CHECK(e == scn::error::invalid_format_string);
    }

    SUBCASE("abc")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[abc]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(scanner.set_parser.get_option('b'));
        CHECK(scanner.set_parser.get_option('c'));
        CHECK(!scanner.set_parser.get_option('A'));
        CHECK(!scanner.set_parser.get_option('d'));
    }

    SUBCASE("-")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[-]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option('-'));
        CHECK(!scanner.set_parser.get_option('a'));
    }

    SUBCASE("a-")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[a-]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option('-'));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(!scanner.set_parser.get_option('b'));
    }

    SUBCASE("-a")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[-a]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option('-'));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(!scanner.set_parser.get_option('b'));
    }

    SUBCASE("a-c")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[a-c]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(scanner.set_parser.get_option('b'));
        CHECK(scanner.set_parser.get_option('c'));
        CHECK(!scanner.set_parser.get_option('-'));
        CHECK(!scanner.set_parser.get_option('d'));
    }

    SUBCASE("a-cA-C")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[a-cA-C]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(scanner.set_parser.get_option('b'));
        CHECK(scanner.set_parser.get_option('c'));
        CHECK(scanner.set_parser.get_option('A'));
        CHECK(scanner.set_parser.get_option('B'));
        CHECK(scanner.set_parser.get_option('C'));
        CHECK(!scanner.set_parser.get_option('-'));
        CHECK(!scanner.set_parser.get_option('d'));
        CHECK(!scanner.set_parser.get_option('D'));
    }
}
