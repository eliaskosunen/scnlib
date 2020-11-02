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
#include <cmath>

#include "test.h"

template <typename CharT, typename T>
struct fpair {
    using char_type = CharT;
    using value_type = T;
};

#if SCN_CLANG >= SCN_COMPILER(3, 8, 0)
SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wdouble-promotion")
#endif

TEST_CASE_TEMPLATE_DEFINE("floating point", T, floating_test)
{
    using value_type = typename T::value_type;
    using char_type = typename T::char_type;

    {
        value_type f{1.0};
        auto e = do_scan<char_type>("0", "{}", f);
        CHECK(f == 0.0);
        CHECK(e);
    }
    {
        value_type f{1.0};
        auto e = do_scan<char_type>("0.0", "{}", f);
        CHECK(f == 0.0);
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("42", "{}", f);
        CHECK(f == doctest::Approx(42));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("3.14", "{}", f);
        CHECK(f == doctest::Approx(3.14));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("-2.22", "{}", f);
        CHECK(f == doctest::Approx(-2.22));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("2.0e4", "{}", f);
        CHECK(f == doctest::Approx(2.0e4));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("0x1.bc70a3d70a3d7p+6", "{}", f);
        CHECK(f == doctest::Approx(111.11));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("inf", "{}", f);
        CHECK(std::isinf(f));
        CHECK(!std::signbit(f));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("-inf", "{}", f);
        CHECK(std::isinf(f));
        CHECK(std::signbit(f));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("nan", "{}", f);
        CHECK(std::isnan(f));
        CHECK(!std::signbit(f));
        CHECK(e);
    }
    {
        value_type f{};
        auto e = do_scan<char_type>("-0", "{}", f);
        CHECK(f == 0.0);
        CHECK(std::signbit(f));
        CHECK(e);
    }
    {
        value_type f{1.0};
        auto e =
            do_scan<char_type>("999999999999999.9999999999999e999999", "{}", f);
        CHECK(f == doctest::Approx(1.0));
        CHECK(!e);
        CHECK(e.error() == scn::error::value_out_of_range);
    }
    {
        value_type f{1.0};
        auto e = do_scan<char_type>("str", "{}", f);
        CHECK(f == doctest::Approx(1.0));
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }
}

template <typename T>
using char_fpair = fpair<char, T>;
template <typename T>
using wchar_fpair = fpair<wchar_t, T>;

TYPE_TO_STRING(char_fpair<float>);
TYPE_TO_STRING(char_fpair<double>);
TYPE_TO_STRING(char_fpair<long double>);
TYPE_TO_STRING(wchar_fpair<float>);
TYPE_TO_STRING(wchar_fpair<double>);
TYPE_TO_STRING(wchar_fpair<long double>);

TEST_CASE_TEMPLATE_INSTANTIATE(floating_test,
                               char_fpair<float>,
                               char_fpair<double>,
                               char_fpair<long double>,
                               wchar_fpair<float>,
                               wchar_fpair<double>,
                               wchar_fpair<long double>);

TEST_CASE("float error")
{
    double d{};
    auto ret = do_scan<char>("str", "{}", d);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(d == doctest::Approx(0.0));
}

TEST_CASE("parse_float")
{
    scn::string_view source = "3.14 123";
    double d{};
    auto ret = scn::parse_float(source, d);
    CHECK(ret);
    CHECK(ret.value() == source.begin() + 4);
    CHECK(d == doctest::Approx(3.14));
}

#if SCN_CLANG >= SCN_COMPILER(3, 8, 0)
SCN_CLANG_POP
#endif
