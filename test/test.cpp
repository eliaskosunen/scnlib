// Copyright 2017-2019 Elias Kosunen
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
    CHECK(r.value() == 3);

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
    auto ret = scn::scan(scn::make_view(data), "test {{}} {} {} {} {:a}", i, d,
                         span, b);

    CHECK(data == copy);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
    CHECK(ret);
    CHECK(ret.value() == 4);

    ret = scn::scan(ret.range(), "{}", i);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::end_of_stream);
}

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
        CHECK(view[0] == ' ');

        int i;
        ret = scn::scan(view, "{}", i);
        CHECK(i == 42);
        CHECK(ret);
        CHECK(ret.range().empty());
        CHECK(view.empty());
    }
}

TEST_CASE("empty format")
{
    int i{0};
    double d{};
    std::string s(6, '\0');
    bool b{};
    auto ret = scn::scan("42 3.14 foobar true", scn::default_tag, i, d, s, b);

    CHECK(ret);
    CHECK(ret.value() == 4);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
}

TEST_CASE("scanf")
{
    int i{0};
    double d{};
    std::string s(6, '\0');
    bool b{};
    auto ret = scn::scanf("test % 42 3.14 foobar true", "test %% %i %f %s %b",
                          i, d, s, b);

    CHECK(ret);
    CHECK(ret.value() == 4);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
}

TEST_CASE("temporary")
{
    struct temporary {
        temporary(int&& val) : value(std::move(val)) {}
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

    auto ret = scn::scan("42", scn::default_tag, temporary{0}());

    CHECK(ret);
    CHECK(ret.value() == 1);
}

TEST_CASE("enumerated arguments")
{
    int i{};
    std::string s{};
    auto ret = scn::scan("42 text", "{1} {0}", s, i);

    CHECK(ret);
    CHECK(ret.value() == 2);

    CHECK(i == 42);
    CHECK(s == "text");
}

TEST_CASE("format string literal mismatch")
{
    std::string str;
    auto ret = scn::scan("abc", "z{}", str);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(str.empty());
}

TEST_CASE("format string argument count mismatch")
{
    std::string s1, s2;
    auto ret = scn::scan("foo bar baz biz whatevz", "{} {}", s1);
    CHECK(!ret);
    CHECK(ret.value() == 1);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(s1 == "foo");

    ret = scn::scan(ret.range(), "{}", s1, s2);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(s1 == "bar");
    CHECK(s2.empty());
}

TEST_CASE("brace mismatch")
{
    std::string s1, s2;
    auto ret = scn::scan("foo bar baz biz whatevz", "{} {", s1, s2);
    CHECK(!ret);
    CHECK(ret.value() == 1);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(s1 == "foo");
}

TEST_CASE("empty span")
{
    scn::span<char> s{};
    auto ret = scn::scan("abc", scn::default_tag, s);
    CHECK(ret);
    CHECK(ret.value() == 1);
}

TEST_CASE("empty input")
{
    int i{};
    auto ret = scn::scan("", "{}", i);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(i == 0);
    CHECK(ret.error() == scn::error::end_of_stream);
}
TEST_CASE("empty format string")
{
    int i{};
    auto ret = scn::scan("", "", i);
    CHECK(ret);
    CHECK(ret.value() == 0);
    CHECK(i == 0);
}
TEST_CASE("unpacked arguments")
{
    std::array<int, 16> a{{0}};
    auto ret =
        scn::scan("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15", scn::default_tag,
                  a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9],
                  a[10], a[11], a[12], a[13], a[14], a[15]);
    CHECK(ret);
    CHECK(ret.value() == 16);
    for (int i = 0; i < 16; ++i) {
        CHECK(a[static_cast<size_t>(i)] == i);
    }
}
