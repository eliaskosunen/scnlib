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
    auto stream = scn::make_stream("42 foo");

    int i;
    std::string s;
    auto r = scn::scan(stream, "{} {}", i, s);

    CHECK(r);
    CHECK(r.value() == 2);

    CHECK(i == 42);
    CHECK(s == std::string{"foo"});
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
    auto stream = scn::make_stream(data.begin(), data.end());
    auto ret = scn::scan(stream, "test {{}} {} {} {} {:a}", i, d, span, b);

    CHECK(data == copy);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
    CHECK(ret);
    CHECK(ret.value() == 4);

    ret = scn::scan(stream, "{}", i);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::end_of_stream);

    auto stream2 = nonsized_stream<decltype(stream)>{
        scn::make_stream(data.begin(), data.end())};
    ret = scn::scan(stream2, "test {{}} {} {} {} {:a}", i, d, span, b);

    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
    CHECK(ret);
    CHECK(ret.value() == 4);
}

TEST_CASE("empty format")
{
    std::string data{"42 3.14 foobar true"};
    auto stream = scn::make_stream(data);

    int i{0};
    double d{};
    std::string s(6, '\0');
    bool b{};
    auto ret = scn::scan(stream, scn::default_tag, i, d, s, b);

    CHECK(ret);
    CHECK(ret.value() == 4);
    CHECK(i == 42);
    CHECK(d == doctest::Approx(3.14));
    CHECK(s == "foobar");
    CHECK(b);
}

TEST_CASE("scanf")
{
    std::string data{"test % 42 3.14 foobar true"};
    auto stream = scn::make_stream(data);

    int i{0};
    double d{};
    std::string s(6, '\0');
    bool b{};
    auto ret = scn::scanf(stream, "test %% %i %f %s %b", i, d, s, b);

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

    std::string data{"42"};
    auto stream = scn::make_stream(data);

    auto ret = scn::scan(stream, scn::default_tag, temporary{0}());

    CHECK(ret);
    CHECK(ret.value() == 1);
}

TEST_CASE("enumerated arguments")
{
    std::string source{"42 text"};
    auto stream = scn::make_stream(source);

    int i{};
    std::string s{};
    auto ret = scn::scan(stream, "{1} {0}", s, i);

    CHECK(ret);
    CHECK(ret.value() == 2);

    CHECK(i == 42);
    CHECK(s == "text");
}

TEST_CASE("get_value")
{
    auto stream = scn::make_stream("42 foo 3.14");

    auto i = scn::get_value<int>(stream);
    CHECK(i);
    CHECK(i.value() == 42);

    auto str = scn::get_value<std::string>(stream);
    CHECK(str);
    CHECK(str.value() == "foo");

    auto d = scn::get_value<double>(stream);
    CHECK(d);
    CHECK(d.value() == 3.14);
}

TEST_CASE("format string literal mismatch")
{
    auto stream = scn::make_stream("abc");

    std::string str;
    auto ret = scn::scan(stream, "z{}", str);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(str.empty());
}

TEST_CASE("format string argument count mismatch")
{
    auto stream = scn::make_stream("foo bar baz biz whatevz");

    std::string s1, s2;
    auto ret = scn::scan(stream, "{} {}", s1);
    CHECK(!ret);
    CHECK(ret.value() == 1);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(s1 == "foo");

    ret = scn::scan(stream, "{}", s1, s2);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(s1 == "bar");
    CHECK(s2.empty());
}

TEST_CASE("brace mismatch")
{
    auto stream = scn::make_stream("foo bar baz biz whatevz");

    std::string s1, s2;
    auto ret = scn::scan(stream, "{} {", s1, s2);
    CHECK(!ret);
    CHECK(ret.value() == 1);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(s1 == "foo");
}

TEST_CASE("empty span")
{
    auto stream = scn::make_stream("abc");
    scn::span<char> s{};

    auto ret = scn::scan(stream, scn::default_tag, s);
    CHECK(ret);
    CHECK(ret.value() == 1);
}
