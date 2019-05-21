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

#include "test.h"

template <typename CharT, typename T>
static scn::result<int> scan_value(scn::method m,
                                   std::string source,
                                   std::string f,
                                   T& value)
{
    return scan_value<CharT>(scn::options::builder{}.int_method(m).make(),
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
        auto e = scan_value<char_type>(
            method, std::to_string(maxval<value_type>() / 10), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == maxval<value_type>() / 10);
    }
    {
        value_type i{};
        auto e = scan_value<char_type>(
            method, std::to_string(minval<value_type>() / 10), "{}", i);
        CHECK(e);
        CHECK(e.value() == 1);
        CHECK(i == minval<value_type>() / 10);
    }
    if (sizeof(value_type) < 8) {
        {
            value_type i{};
            auto e =
                scan_value<char_type>(method, overstr<value_type>(), "{}", i);
            CHECK(!e);
            CHECK(e.value() == 0);
        }
        if (std::is_signed<value_type>::value) {
            value_type i{};
            auto e =
                scan_value<char_type>(method, understr<value_type>(), "{}", i);
            CHECK(!e);
            CHECK(e.value() == 0);
        }
    }
}

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
