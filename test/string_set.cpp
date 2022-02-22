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

TEST_CASE("set parse")
{
    scn::locale_ref locale{};
    auto make_parse_ctx = [&](scn::string_view str) {
        return scn::make_parse_context(str, locale);
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

        const auto& loc = pctx.locale();
        CHECK(scanner.set_parser.check_character('a', false, loc));
        CHECK(scanner.set_parser.check_character('b', false, loc));
        CHECK(scanner.set_parser.check_character('c', false, loc));
        CHECK(!scanner.set_parser.check_character('A', false, loc));
        CHECK(!scanner.set_parser.check_character('d', false, loc));
    }

    SUBCASE("^abc")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[^abc]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::enabled));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::use_chars));
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::inverted));
        CHECK(!scanner.set_parser.get_option(
            set_parser_type::flag::use_specifiers));
        CHECK(
            !scanner.set_parser.get_option(set_parser_type::specifier::alpha));
        CHECK(scanner.set_parser.get_option('a'));
        CHECK(scanner.set_parser.get_option('b'));
        CHECK(scanner.set_parser.get_option('c'));
        CHECK(!scanner.set_parser.get_option('A'));
        CHECK(!scanner.set_parser.get_option('d'));
        CHECK(!scanner.set_parser.get_option('^'));

        const auto& loc = pctx.locale();
        CHECK(!scanner.set_parser.check_character('a', false, loc));
        CHECK(!scanner.set_parser.check_character('b', false, loc));
        CHECK(!scanner.set_parser.check_character('c', false, loc));
        CHECK(scanner.set_parser.check_character('A', false, loc));
        CHECK(scanner.set_parser.check_character('d', false, loc));
        CHECK(scanner.set_parser.check_character('^', false, loc));
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

        const auto& loc = pctx.locale();
        CHECK(scanner.set_parser.check_character('a', false, loc));
        CHECK(scanner.set_parser.check_character('b', false, loc));
        CHECK(scanner.set_parser.check_character('c', false, loc));
        CHECK(scanner.set_parser.check_character('A', false, loc));
        CHECK(scanner.set_parser.check_character('B', false, loc));
        CHECK(scanner.set_parser.check_character('C', false, loc));
        CHECK(!scanner.set_parser.check_character('-', false, loc));
        CHECK(!scanner.set_parser.check_character('d', false, loc));
        CHECK(!scanner.set_parser.check_character('D', false, loc));
    }

    SUBCASE(":all:")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[:all:]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::accept_all));

        const auto& loc = pctx.locale();
        CHECK(scanner.set_parser.check_character('a', false, loc));
        CHECK(scanner.set_parser.check_character('Z', false, loc));
        CHECK(scanner.set_parser.check_character('0', false, loc));
        CHECK(scanner.set_parser.check_character('-', false, loc));
        CHECK(scanner.set_parser.check_character(char{0x7f}, false, loc));
    }
    SUBCASE("\\s\\S = all")
    {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\s\\S]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(scanner.set_parser.get_option(set_parser_type::flag::accept_all));
    }
    SUBCASE("\\x00-\\x7f = not all") {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\x00-\\x7f]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());
        CHECK(!scanner.set_parser.get_option(set_parser_type::flag::accept_all));
    }

    SUBCASE(":alnum:") {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[:alnum:]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());

        const auto& loc = pctx.locale();
        CHECK(scanner.set_parser.check_character('a', false, loc));
        CHECK(scanner.set_parser.check_character('Z', false, loc));
        CHECK(scanner.set_parser.check_character('0', false, loc));
        CHECK(!scanner.set_parser.check_character('-', false, loc));
    }
    SUBCASE(":punct:") {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[:punct:]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());

        const auto& loc = pctx.locale();
        CHECK(scanner.set_parser.check_character('.', false, loc));
        CHECK(scanner.set_parser.check_character(',', false, loc));
        CHECK(scanner.set_parser.check_character('-', false, loc));
        CHECK(!scanner.set_parser.check_character('a', false, loc));
        CHECK(!scanner.set_parser.check_character('Z', false, loc));
        CHECK(!scanner.set_parser.check_character('0', false, loc));
    }
    SUBCASE(":xdigit:") {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[:xdigit:]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());

        const auto& loc = pctx.locale();
        CHECK(scanner.set_parser.check_character('a', false, loc));
        CHECK(scanner.set_parser.check_character('F', false, loc));
        CHECK(scanner.set_parser.check_character('0', false, loc));
        CHECK(scanner.set_parser.check_character('9', false, loc));
        CHECK(!scanner.set_parser.check_character('x', false, loc));
        CHECK(!scanner.set_parser.check_character('g', false, loc));
        CHECK(!scanner.set_parser.check_character('-', false, loc));
    }
    SUBCASE("\\l") {
        scanner_type scanner{};
        auto pctx = make_parse_ctx("[\\l]}");
        auto e = scanner.parse(pctx);
        CHECK(e);
        CHECK(pctx.check_arg_end());

        const auto& loc = pctx.locale();
        CHECK(scanner.set_parser.check_character('a', false, loc));
        CHECK(scanner.set_parser.check_character('F', false, loc));
        CHECK(scanner.set_parser.check_character('Z', false, loc));
        CHECK(!scanner.set_parser.check_character('0', false, loc));
        CHECK(!scanner.set_parser.check_character('9', false, loc));
        CHECK(!scanner.set_parser.check_character('-', false, loc));
    }
}

TEST_CASE("set scanning")
{
    SUBCASE("simple")
    {
        std::string str;
        auto ret = scn::scan("foo", "{:[a-z]}", str);
        CHECK(ret);
        CHECK(ret.range().empty());
        CHECK(str == "foo");
    }

    SUBCASE("preceding whitespace")
    {
        std::string str;
        auto ret = scn::scan(" foo", "{:[\\S]}", str);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::invalid_scanned_value);
        CHECK(str.empty());
        str.clear();

        ret = scn::scan(" foo", " {:[\\S]}", str);
        CHECK(ret);
        CHECK(str == "foo");
        str.clear();

        std::string w;
        ret = scn::scan(" foo", "{:[\\s]}{:[\\S]}", w, str);
        CHECK(ret);
        CHECK(w == " ");
        CHECK(str == "foo");
    }

    SUBCASE("ÅÄÖ")
    {
        std::string str;
        auto ret = scn::scan("ÅÄO", "{:[ÅÄÖ]}", str);
        CHECK(ret);
        CHECK(ret.range_as_string() == "O");
        CHECK(str == "ÅÄ");
        str = "";

        ret = scn::scan("ÅÄO", "{:[\\u00c5\\u00C4\\U000000D6]}", str);
        CHECK(ret);
        CHECK(ret.range_as_string() == "O");
        CHECK(str == "ÅÄ");
        str = "";
    }
}
