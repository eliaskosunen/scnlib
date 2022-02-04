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

TEST_CASE("simple")
{
    int i;
    std::string s;
    double d;
    auto r = scn::scan("42 foo 3.14", "{} {} {}", i, s, d);

    CHECK(r);

    CHECK(i == 42);
    CHECK(s == std::string{"foo"});
    CHECK(d == doctest::Approx(3.14));
}

TEST_CASE("general")
{
    std::string data{"test {} 42 3.14 foobar true"};
    std::string copy = data;

    int i{0};
    double d{};
    std::string s(6, '\0');
    auto span = scn::make_span(&s[0], &s[0] + s.size());
    bool b{};
    auto ret = scn::scan(data, "test {{}} {} {} {} {}", i, d, span, b);

    CHECK(data == copy);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
    CHECK(ret);

    ret = scn::scan(ret.range(), "{}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::end_of_range);
}

#if 0
TEST_CASE("range wrapping")
{
    SUBCASE("rvalue view")
    {
        auto view = "hello 42";

        std::string str;
        auto ret = scn::scan(scn::string_view(view), "{}", str);
        CHECK(str == "hello");
        CHECK(ret);
        CHECK(ret.range()[0] == ' ');
        CHECK(view[0] == 'h');

        int i;
        ret = scn::scan(ret.range(), "{}", i);
        CHECK(i == 42);
        CHECK(ret);
        CHECK(ret.range().empty());
        CHECK(view[0] == 'h');
    }

    SUBCASE("lvalue view")
    {
        auto view = scn::make_view("hello 42");

        std::string str;
        auto ret = scn::scan(view, "{}", str);
        CHECK(str == "hello");
        CHECK(ret);
        CHECK(ret.range()[0] == ' ');

        int i;
        ret = scn::scan(view, "{}", i);
        CHECK(i == 42);
        CHECK(ret);
        CHECK(ret.range().empty());
    }
}
#endif

TEST_CASE("empty format")
{
    int i{0};
    double d{};
    std::string s(6, '\0');
    bool b{};
    auto ret = scn::scan_default("42 3.14 foobar true", i, d, s, b);

    CHECK(ret);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
}

TEST_CASE("value")
{
    auto ret = scn::scan_value<int>("42");
    CHECK(ret);
    CHECK(ret.value() == 42);

    auto ret2 = scn::scan_value<int>("foo");
    CHECK(!ret2);
    CHECK(ret2.range_as_string() == "foo");
}

TEST_CASE("temporary")
{
    struct temporary {
        explicit temporary(int&& val) : value(val) {}
        ~temporary()
        {
            CHECK(value == 42);
        }

        int& operator()() &&
        {
            return value;
        }

        int value;
    };

    auto ret = scn::scan_default("42", temporary{0}());

    CHECK(ret);
}

TEST_CASE("discard")
{
    auto ret = scn::scan_default("123 456", scn::discard<int>());
    CHECK(ret);
    CHECK(ret.range_as_string() == " 456");
}

TEST_CASE("enumerated arguments")
{
    int i{};
    std::string s{};
    auto ret = scn::scan("42 text", "{1} {0}", s, i);

    CHECK(ret);

    CHECK(i == 42);
    CHECK(s == "text");
}

TEST_CASE("format string literal mismatch")
{
    std::string str;
    auto ret = scn::scan("abc", "z{}", str);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(str.empty());
}

TEST_CASE("format string argument count mismatch")
{
    std::string s1, s2;
    auto ret = scn::scan("foo bar baz biz whatevz", "{} {}", s1);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(s1 == "foo");

    ret = scn::scan(ret.range(), "{}", s1, s2);
    CHECK(ret);
    CHECK(s1 == "bar");
    CHECK(s2.empty());
}

TEST_CASE("brace mismatch")
{
    std::string s1, s2;
    auto ret = scn::scan("foo bar baz biz whatevz", "{} {", s1, s2);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(s1 == "foo");
}

TEST_CASE("empty span")
{
    scn::span<char> s{};
    auto ret = scn::scan_default("abc", s);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
}

TEST_CASE("empty input")
{
    int i{};
    auto ret = scn::scan("", "{}", i);
    CHECK(!ret);
    CHECK(i == 0);
    CHECK(ret.error() == scn::error::end_of_range);
}
TEST_CASE("empty format string")
{
    int i{};
    auto ret = scn::scan("", "", i);
    CHECK(ret);
    CHECK(i == 0);
}
TEST_CASE("unpacked arguments")
{
    std::array<int, 16> a{{0}};
    auto ret = scn::scan_default(
        "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", a[0], a[1], a[2], a[3], a[4],
        a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);
    CHECK(ret);
    for (int i = 0; i < 16; ++i) {
        CHECK(a[static_cast<size_t>(i)] == i);
    }
}

TEST_CASE("partial success = fail")
{
    int i, j;
    auto ret = scn::scan("123 foo", "{} {}", i, j);
    CHECK(!ret);
    CHECK(i == 123);
    // j is undefined
}

TEST_CASE("argument amount")
{
    SUBCASE("1")
    {
        int i;
        auto ret = scn::scan_default("0", i);
        CHECK(ret);
        CHECK(i == 0);
    }
#define MAKE_ARGUMENT_AMOUNT_TEST(str, n, ...)      \
    int i[n];                                       \
    auto ret = scn::scan_default(str, __VA_ARGS__); \
    CHECK(ret);                                     \
    for (int j = 0; j < n; ++j) {                   \
        CHECK(i[j] == j);                           \
    }                                               \
    do {                                            \
    } while (false)
    SUBCASE("2")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1", 2, i[0], i[1]);
    }
    SUBCASE("3")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2", 3, i[0], i[1], i[2]);
    }
    SUBCASE("4")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3", 4, i[0], i[1], i[2], i[3]);
    }
    SUBCASE("5")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4", 5, i[0], i[1], i[2], i[3], i[4]);
    }
    SUBCASE("6")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5", 6, i[0], i[1], i[2], i[3],
                                  i[4], i[5]);
    }
    SUBCASE("7")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6", 7, i[0], i[1], i[2], i[3],
                                  i[4], i[5], i[6]);
    }
    SUBCASE("8")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7", 8, i[0], i[1], i[2], i[3],
                                  i[4], i[5], i[6], i[7]);
    }
    SUBCASE("9")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8", 9, i[0], i[1], i[2],
                                  i[3], i[4], i[5], i[6], i[7], i[8]);
    }
    SUBCASE("10")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8 9", 10, i[0], i[1], i[2],
                                  i[3], i[4], i[5], i[6], i[7], i[8], i[9]);
    }
    SUBCASE("11")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8 9 10", 11, i[0], i[1],
                                  i[2], i[3], i[4], i[5], i[6], i[7], i[8],
                                  i[9], i[10]);
    }
    // 12 should be the limit for packing arguments
    SUBCASE("12")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8 9 10 11", 12, i[0], i[1],
                                  i[2], i[3], i[4], i[5], i[6], i[7], i[8],
                                  i[9], i[10], i[11]);
    }
    SUBCASE("13")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8 9 10 11 12", 13, i[0],
                                  i[1], i[2], i[3], i[4], i[5], i[6], i[7],
                                  i[8], i[9], i[10], i[11], i[12]);
    }
    SUBCASE("14")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8 9 10 11 12 13", 14, i[0],
                                  i[1], i[2], i[3], i[4], i[5], i[6], i[7],
                                  i[8], i[9], i[10], i[11], i[12], i[13]);
    }
    SUBCASE("15")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14", 15,
                                  i[0], i[1], i[2], i[3], i[4], i[5], i[6],
                                  i[7], i[8], i[9], i[10], i[11], i[12], i[13],
                                  i[14]);
    }
    SUBCASE("16")
    {
        MAKE_ARGUMENT_AMOUNT_TEST("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", 16,
                                  i[0], i[1], i[2], i[3], i[4], i[5], i[6],
                                  i[7], i[8], i[9], i[10], i[11], i[12], i[13],
                                  i[14], i[15]);
    }
}
