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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

#include <cmath>

template <typename CharT, typename T>
static scn::scan_result scan_value(scn::method m,
                                   std::string source,
                                   std::string f,
                                   T& value)
{
    return scan_value<CharT>(scn::options::builder{}.float_method(m).make(),
                             std::move(source), std::move(f), value);
}

template <typename CharT, typename T>
static scn::scan_result scanf_value(scn::method m,
                                    std::string source,
                                    std::string f,
                                    T& value)
{
    return scanf_value<CharT>(scn::options::builder{}.float_method(m).make(),
                              std::move(source), std::move(f), value);
}

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

    std::vector<scn::method> methods{scn::method::sto, scn::method::strto};
    if (scn::is_int_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    {
        value_type f{1.0};
        auto e = scan_value<char_type>(method, "0", "{}", f);
        CHECK(f == 0.0);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{1.0};
        auto e = scan_value<char_type>(method, "0.0", "{}", f);
        CHECK(f == 0.0);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "42", "{}", f);
        CHECK(f == doctest::Approx(42));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "3.14", "{}", f);
        CHECK(f == doctest::Approx(3.14));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "-2.22", "{}", f);
        CHECK(f == doctest::Approx(-2.22));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "2.0e4", "{}", f);
        CHECK(f == doctest::Approx(2.0e4));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "0x1.bc70a3d70a3d7p+6", "{}", f);
        CHECK(f == doctest::Approx(111.11));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "inf", "{}", f);
        CHECK(std::isinf(f));
        CHECK(!std::signbit(f));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "-inf", "{}", f);
        CHECK(std::isinf(f));
        CHECK(std::signbit(f));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "nan", "{}", f);
        CHECK(std::isnan(f));
        CHECK(!std::signbit(f));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{};
        auto e = scan_value<char_type>(method, "-0", "{}", f);
        CHECK(f == 0.0);
        CHECK(std::signbit(f));
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type f{1.0};
        auto e = scan_value<char_type>(
            method, "999999999999999.9999999999999e999999", "{}", f);
        CHECK(f == doctest::Approx(1.0));
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::value_out_of_range);
    }
    {
        value_type f{1.0};
        auto e = scan_value<char_type>(method, "str", "{}", f);
        CHECK(f == doctest::Approx(1.0));
        CHECK(!e);
        CHECK(e.value() == 0);
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
    std::vector<scn::method> methods{scn::method::sto, scn::method::strto};
    if (scn::is_float_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    double d{};

    auto ret = scan_value<char>(method, "str", "{}", d);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(d == doctest::Approx(0.0));
}

TEST_CASE("float scanf")
{
    std::vector<scn::method> methods{scn::method::sto, scn::method::strto};
    if (scn::is_float_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    double d{};

    SUBCASE("%f")
    {
        auto ret = scanf_value<char>(method, "1.0", "%f", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }
    SUBCASE("%F")
    {
        auto ret = scanf_value<char>(method, "1.0", "%F", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }
    SUBCASE("%a")
    {
        auto ret = scanf_value<char>(method, "1.0", "%a", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }
    SUBCASE("%A")
    {
        auto ret = scanf_value<char>(method, "1.0", "%A", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }
    SUBCASE("%e")
    {
        auto ret = scanf_value<char>(method, "1.0", "%e", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }
    SUBCASE("%E")
    {
        auto ret = scanf_value<char>(method, "1.0", "%E", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }
    SUBCASE("%g")
    {
        auto ret = scanf_value<char>(method, "1.0", "%g", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }
    SUBCASE("%G")
    {
        auto ret = scanf_value<char>(method, "1.0", "%G", d);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(d == doctest::Approx(1.0));
    }

    SUBCASE("%f /w error")
    {
        auto ret = scanf_value<char>(method, "str", "%f", d);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_scanned_value);
    }
}

#if SCN_CLANG >= SCN_COMPILER(3, 8, 0)
SCN_CLANG_POP
#endif

