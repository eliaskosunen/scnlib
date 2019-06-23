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

template <typename CharT, typename T>
static scn::scan_result scan_value(scn::method m,
                                   std::string source,
                                   std::string f,
                                   T& value)
{
    return scan_value<CharT>(scn::options::builder{}.int_method(m).make(),
                             std::move(source), std::move(f), value);
}

template <typename CharT, typename T>
static scn::scan_result scan_value(const std::locale& loc,
                                   std::string source,
                                   std::string f,
                                   T& value)
{
    return scan_value<CharT>(scn::options::builder{}.locale(loc).make(),
                             std::move(source), std::move(f), value);
}

template <typename CharT, typename T>
static scn::scan_result scanf_value(scn::method m,
                                    std::string source,
                                    std::string f,
                                    T& value)
{
    return scanf_value<CharT>(scn::options::builder{}.int_method(m).make(),
                              std::move(source), std::move(f), value);
}

template <typename CharT, typename T>
struct intpair {
    using char_type = CharT;
    using value_type = T;
};

TEST_CASE_TEMPLATE_DEFINE("integer", T, integer_test)
{
    using value_type = typename T::value_type;
    using char_type = typename T::char_type;

    const bool u = std::is_unsigned<value_type>::value;

    std::vector<scn::method> methods{scn::method::sto, scn::method::strto,
                                     scn::method::custom};
    if (scn::is_int_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    {
        value_type i{1};
        auto e = scan_value<char_type>(method, "0", "{}", i);
        CHECK(i == 0);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(method, "1", "{}", i);
        CHECK(i == 1);
        CHECK(e);
        CHECK(e.value() == 1);
    }

    {
        if (!u) {
            value_type i{};
            auto e = scan_value<char_type>(method, "-1", "{}", i);
            CHECK(i == -1);
            CHECK(e);
            CHECK(e.value() == 1);
        }
        else {
            value_type i{};
            auto e = scan_value<char_type>(method, "-1", "{}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
            CHECK(e.value() == 0);
        }
    }

    {
        const bool can_fit_2pow31 = [=]() {
            if (u) {
                return sizeof(value_type) >= 4;
            }
            return sizeof(value_type) >= 8;
        }();
        if (can_fit_2pow31) {
            value_type i{};
            auto e = scan_value<char_type>(method, "2147483648", "{}", i);
            CHECK(i == 2147483648);
            CHECK(e);
            CHECK(e.value() == 1);
        }
        else {
            value_type i{};
            auto e = scan_value<char_type>(method, "2147483648", "{}", i);
            CHECK(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
            CHECK(e.value() == 0);
        }
    }

    {
        value_type i{};
        auto e = scan_value<char_type>(method, "1011", "{:b2}", i);
        CHECK(i == 11);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(method, "400", "{:o}", i);
        CHECK(i == 0400);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(method, "0400", "{}", i);
        CHECK(i == 0400);
        CHECK(e);
        CHECK(e.value() == 1);
    }

    const bool can_fit_badidea = [=]() { return sizeof(value_type) >= 4; }();
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = scan_value<char_type>(method, "bad1dea", "{:x}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
            CHECK(e.value() == 1);
        }
        else {
            value_type i{};
            auto e = scan_value<char_type>(method, "bad1dea", "{:x}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
            CHECK(e.value() == 0);
        }
    }
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = scan_value<char_type>(method, "0xbad1dea", "{}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
            CHECK(e.value() == 1);
        }
        else {
            value_type i{};
            auto e = scan_value<char_type>(method, "0xbad1dea", "{}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
            CHECK(e.value() == 0);
        }
    }
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = scan_value<char_type>(method, "0xBAD1DEA", "{}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
            CHECK(e.value() == 1);
        }
        else {
            value_type i{};
            auto e = scan_value<char_type>(method, "0xBAD1DEA", "{}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
            CHECK(e.value() == 0);
        }
    }

    {
        value_type i{};
        auto e = scan_value<char_type>(method, "ff", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(method, "FF", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type i{0};
        auto e = scan_value<char_type>(method, "0xff", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
        CHECK(e.value() == 1);
    }
    {
        value_type i{0};
        auto e = scan_value<char_type>(method, "0xFF", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
        CHECK(e.value() == 1);
    }

    {
        value_type i{};
        auto e = scan_value<char_type>(method, "text", "{}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }

    if (std::is_signed<value_type>::value) {
        value_type i{};
        auto e = scan_value<char_type>(method, "-", "{}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::invalid_scanned_value);
        CHECK(i == 0);
    }

    {
        value_type i{};
        auto e = scan_value<char_type>(method, "+", "{}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::invalid_scanned_value);
        CHECK(i == 0);
    }

    {
        value_type i{};
        auto e = scan_value<char_type>(method, "123", "{:b}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(method, "123", "{:ba}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(method, "123", "{:b0}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
}

TEST_CASE("integer decimal separator")
{
    auto stream = scn::make_stream("100.200");
    int i{};

    auto ret = scn::scan(stream, scn::default_tag, i);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(i == 100);

    auto cret = scn::getchar(stream);
    CHECK(cret);
    CHECK(cret.value() == '.');

    ret = scn::scan(stream, scn::default_tag, i);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(i == 200);
}

TEST_CASE("integer error")
{
    std::vector<scn::method> methods{scn::method::sto, scn::method::strto,
                                     scn::method::custom};
    if (scn::is_int_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    int i{};

    auto ret = scan_value<char>(method, "str", "{}", i);
    CHECK(!ret);
    CHECK(ret.value() == 0);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
}

TEST_CASE("integer thousands separator")
{
    auto stream = scn::make_stream("100,200");
    int a{}, b{};

    SUBCASE("without {'}")
    {
        auto ret = scn::scan(stream, "{}", a);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(a == 100);

        {
            auto cret = scn::getchar(stream);
            CHECK(cret);
            CHECK(cret.value() == ',');
        }

        ret = scn::scan(stream, "{}", b);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(b == 200);
    }

    SUBCASE("with {'}")
    {
        auto ret = scn::scan(stream, "{:'}", a);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(a == 100200);
    }
}

template <typename T>
T maxval()
{
    return std::numeric_limits<T>::max();
}
template <typename T>
std::string overstr()
{
    using type = typename std::conditional<std::is_unsigned<T>::value,
                                           unsigned long long, long long>::type;
    return std::to_string(static_cast<type>(std::numeric_limits<T>::max()) + 1);
}

template <typename T,
          typename U = typename std::conditional<std::is_unsigned<T>::value,
                                                 unsigned long long,
                                                 long long>::type,
          typename std::enable_if<sizeof(T) == sizeof(U)>::type* = nullptr>
std::string overstr_64()
{
    using type = typename std::conditional<std::is_unsigned<T>::value,
                                           unsigned long long, long long>::type;
    auto str = std::to_string(std::numeric_limits<type>::max());
    if (str.back() < '9') {
        ++str.back();
    }
    else {
        str.back() = '0';
        ++(*(str.rbegin() + 1));
    }
    return str;
}
template <typename T,
          typename U = typename std::conditional<std::is_unsigned<T>::value,
                                                 unsigned long long,
                                                 long long>::type,
          typename std::enable_if<sizeof(T) != sizeof(U)>::type* = nullptr>
std::string overstr_64()
{
    return std::to_string(static_cast<U>(std::numeric_limits<T>::max()) + 1);
}
template <>
std::string overstr<long long>()
{
    return overstr_64<long long>();
}
template <>
std::string overstr<unsigned long long>()
{
    return overstr_64<unsigned long long>();
}
template <>
std::string overstr<long>()
{
    return overstr_64<long>();
}
template <>
std::string overstr<unsigned long>()
{
    return overstr_64<unsigned long>();
}

template <typename T>
T minval()
{
    return std::numeric_limits<T>::min();
}

template <typename T>
std::string understr()
{
    using type = typename std::conditional<std::is_unsigned<T>::value,
                                           unsigned long long, long long>::type;
    return std::to_string(static_cast<type>(std::numeric_limits<T>::min()) - 1);
}
template <typename T,
          typename U = typename std::conditional<std::is_unsigned<T>::value,
                                                 unsigned long long,
                                                 long long>::type,
          typename std::enable_if<sizeof(T) == sizeof(U)>::type* = nullptr>
std::string understr_64()
{
    auto str = std::to_string(std::numeric_limits<long long>::min());
    if (str.back() < '9') {
        ++str.back();
    }
    else {
        str.back() = '0';
        ++(*(str.rbegin() + 1));
    }
    return str;
}
template <typename T,
          typename U = typename std::conditional<std::is_unsigned<T>::value,
                                                 unsigned long long,
                                                 long long>::type,
          typename std::enable_if<sizeof(T) != sizeof(U)>::type* = nullptr>
std::string understr_64()
{
    return std::to_string(static_cast<U>(std::numeric_limits<T>::min()) - 1);
}

template <>
std::string understr<long long>()
{
    return understr_64<long long>();
}
template <>
std::string understr<unsigned long long>()
{
    return "";
}
template <>
std::string understr<long>()
{
    return understr_64<long>();
}
template <>
std::string understr<unsigned long>()
{
    return "";
}

TEST_CASE_TEMPLATE_DEFINE("integer range", T, integer_range_test)
{
    using value_type = typename T::value_type;
    using char_type = typename T::char_type;

    std::vector<scn::method> methods{scn::method::sto, scn::method::strto,
                                     scn::method::custom};
    if (scn::is_int_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    {
        value_type i{};
        auto e = scan_value<char_type>(
            method, std::to_string(maxval<value_type>()), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == maxval<value_type>());
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(
            method, std::to_string(minval<value_type>()), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == minval<value_type>());
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(
            method, std::to_string(maxval<value_type>() - 1), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == maxval<value_type>() - 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(
            method, std::to_string(minval<value_type>() + 1), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == minval<value_type>() + 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(method, overstr<value_type>(), "{}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::value_out_of_range);
    }
    {
        if (std::is_signed<value_type>::value) {
            value_type i{};
            auto e =
                scan_value<char_type>(method, understr<value_type>(), "{}", i);
            CHECK(!e);
            CHECK(e.value() == 0);
            CHECK(e.error() == scn::error::value_out_of_range);
        }
    }
}

#if !SCN_MSVC
TEST_CASE_TEMPLATE_DEFINE("integer range localized",
                          T,
                          integer_range_localized_test)
{
    using value_type = typename T::value_type;
    using char_type = typename T::char_type;

    auto locale = std::locale("en_US");

    {
        value_type i{};
        auto e = scan_value<char_type>(
            locale, std::to_string(maxval<value_type>()), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == maxval<value_type>());
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(
            locale, std::to_string(minval<value_type>()), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == minval<value_type>());
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(
            locale, std::to_string(maxval<value_type>() - 1), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == maxval<value_type>() - 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(
            locale, std::to_string(minval<value_type>() + 1), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == minval<value_type>() + 1);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(locale, overstr<value_type>(), "{}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::value_out_of_range);
    }
    if (std::is_signed<value_type>::value) {
        value_type i{};
        auto e = scan_value<char_type>(locale, understr<value_type>(), "{}", i);
        CHECK(!e);
        CHECK(e.value() == 0);
        CHECK(e.error() == scn::error::value_out_of_range);
    }
}
#endif

template <typename T>
using char_intpair = intpair<char, T>;
template <typename T>
using wchar_intpair = intpair<wchar_t, T>;

TYPE_TO_STRING(char_intpair<short>);
TYPE_TO_STRING(char_intpair<int>);
TYPE_TO_STRING(char_intpair<long>);
TYPE_TO_STRING(char_intpair<long long>);
TYPE_TO_STRING(char_intpair<unsigned short>);
TYPE_TO_STRING(char_intpair<unsigned int>);
TYPE_TO_STRING(char_intpair<unsigned long>);
TYPE_TO_STRING(char_intpair<unsigned long long>);
TYPE_TO_STRING(wchar_intpair<int>);
TYPE_TO_STRING(wchar_intpair<long>);
TYPE_TO_STRING(wchar_intpair<long long>);
TYPE_TO_STRING(wchar_intpair<unsigned int>);
TYPE_TO_STRING(wchar_intpair<unsigned long>);
TYPE_TO_STRING(wchar_intpair<unsigned long long>);

TEST_CASE_TEMPLATE_INSTANTIATE(integer_test,
                               char_intpair<short>,
                               char_intpair<int>,
                               char_intpair<long>,
                               char_intpair<long long>,
                               char_intpair<unsigned short>,
                               char_intpair<unsigned int>,
                               char_intpair<unsigned long>,
                               char_intpair<unsigned long long>,
                               wchar_intpair<int>,
                               wchar_intpair<long>,
                               wchar_intpair<long long>,
                               wchar_intpair<unsigned int>,
                               wchar_intpair<unsigned long>,
                               wchar_intpair<unsigned long long>);
TEST_CASE_TEMPLATE_INSTANTIATE(integer_range_test,
                               char_intpair<short>,
                               char_intpair<int>,
                               char_intpair<long>,
                               char_intpair<long long>,
                               char_intpair<unsigned short>,
                               char_intpair<unsigned int>,
                               char_intpair<unsigned long>,
                               char_intpair<unsigned long long>,
                               wchar_intpair<int>,
                               wchar_intpair<long>,
                               wchar_intpair<long long>,
                               wchar_intpair<unsigned int>,
                               wchar_intpair<unsigned long>,
                               wchar_intpair<unsigned long long>);
#if !SCN_MSVC
TEST_CASE_TEMPLATE_INSTANTIATE(integer_range_localized_test,
                               char_intpair<short>,
                               char_intpair<int>,
                               char_intpair<long>,
                               char_intpair<long long>,
                               char_intpair<unsigned short>,
                               char_intpair<unsigned int>,
                               char_intpair<unsigned long>,
                               char_intpair<unsigned long long>,
                               wchar_intpair<int>,
                               wchar_intpair<long>,
                               wchar_intpair<long long>,
                               wchar_intpair<unsigned int>,
                               wchar_intpair<unsigned long>,
                               wchar_intpair<unsigned long long>);
#endif

TEST_CASE("integer scanf")
{
    std::vector<scn::method> methods{scn::method::sto, scn::method::strto,
                                     scn::method::custom};
    if (scn::is_int_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    int i{};
    unsigned u{};

    SUBCASE("%d")
    {
        auto ret = scanf_value<char>(method, "1", "%d", i);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(i == 1);
    }
    SUBCASE("%x")
    {
        auto ret = scanf_value<char>(method, "f", "%x", i);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(i == 0xf);
    }
    SUBCASE("%o")
    {
        auto ret = scanf_value<char>(method, "10", "%o", i);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(i == 010);
    }
    SUBCASE("%b2")
    {
        auto ret = scanf_value<char>(method, "10", "%b2", i);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(i == 2);
    }
    SUBCASE("%i")
    {
        auto ret = scanf_value<char>(method, "1", "%i", i);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(i == 1);
    }
    SUBCASE("%u")
    {
        auto ret = scanf_value<char>(method, "1", "%u", u);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(u == 1);
    }

    SUBCASE("%'d")
    {
        if (method == scn::method::custom) {
            auto ret = scanf_value<char>(method, "1,000", "%'d", i);
            CHECK(ret);
            CHECK(ret.value() == 1);
            CHECK(i == 1000);
        }
    }

    SUBCASE("%i /w unsigned")
    {
        auto ret = scanf_value<char>(method, "1", "%i", u);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_format_string);
    }
    SUBCASE("%u /w signed")
    {
        auto ret = scanf_value<char>(method, "1", "%u", i);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_format_string);
    }
}
