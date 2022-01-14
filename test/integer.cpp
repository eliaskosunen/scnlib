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
    int i{};
    auto ret = scn::scan("42", "{}", i);
    CHECK(ret);
    CHECK(i == 42);
    CHECK(ret.range().size() == 0);
}

TEST_CASE("short ranges")
{
    // range is (inclusive) from -32768 to 32767
    short i{};

    auto ret = scn::scan("32767", "{}", i);
    CHECK(ret);
    CHECK(i == 32767);
    CHECK(ret.range().size() == 0);

    ret = scn::scan("32768", "{}", i);
    CHECK(!ret);
    CHECK(i == 32767);
    CHECK(ret.error().code() == scn::error::value_out_of_range);

    ret = scn::scan("-32768", "{}", i);
    CHECK(ret);
    CHECK(i == -32768);
    CHECK(ret.range().size() == 0);

    ret = scn::scan("-32769", "{}", i);
    CHECK(!ret);
    CHECK(i == -32768);
    CHECK(ret.error().code() == scn::error::value_out_of_range);

    // range is (inclusive) from 0 to 65535
    unsigned short u{};

    ret = scn::scan("32767", "{}", u);
    CHECK(ret);
    CHECK(u == 32767);
    CHECK(ret.range().size() == 0);

    ret = scn::scan("32768", "{}", u);
    CHECK(ret);
    CHECK(u == 32768);
    CHECK(ret.range().size() == 0);

    ret = scn::scan("-32768", "{}", u);
    CHECK(!ret);
    CHECK(u == 32768);
    CHECK(ret.error().code() == scn::error::value_out_of_range);

    ret = scn::scan("-32769", "{}", u);
    CHECK(!ret);
    CHECK(u == 32768);
    CHECK(ret.error().code() == scn::error::value_out_of_range);

    ret = scn::scan("65535", "{}", u);
    CHECK(ret);
    CHECK(u == 65535);
    CHECK(ret.range().size() == 0);

    ret = scn::scan("65536", "{}", u);
    CHECK(!ret);
    CHECK(u == 65535);
    CHECK(ret.error().code() == scn::error::value_out_of_range);
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

    constexpr bool u = std::is_unsigned<value_type>::value;

    {
        value_type i{1};
        auto e = do_scan<char_type>("0", "{}", i);
        CHECK(i == 0);
        CHECK(e);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("1", "{}", i);
        CHECK(i == 1);
        CHECK(e);
    }

    {
        if (!u) {
            value_type i{};
            auto e = do_scan<char_type>("-1", "{}", i);
            CHECK(i == -1);
            CHECK(e);
        }
        else {
            value_type i{};
            auto e = do_scan<char_type>("-1", "{}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
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
            auto e = do_scan<char_type>("2147483648", "{}", i);
            CHECK(i == 2147483648);
            CHECK(e);
        }
        else {
            value_type i{};
            auto e = do_scan<char_type>("2147483648", "{}", i);
            CHECK(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
        }
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("1011", "{:b2}", i);
        CHECK(i == 11);
        CHECK(e);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("400", "{:o}", i);
        CHECK(i == 0400);
        CHECK(e);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("0400", "{}", i);
        CHECK(i == 0400);
        CHECK(e);
    }

    const bool can_fit_badidea = [=]() { return sizeof(value_type) >= 4; }();
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = do_scan<char_type>("bad1dea", "{:x}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
        }
        else {
            value_type i{};
            auto e = do_scan<char_type>("bad1dea", "{:x}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
        }
    }
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = do_scan<char_type>("0xbad1dea", "{}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
        }
        else {
            value_type i{};
            auto e = do_scan<char_type>("0xbad1dea", "{}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
        }
    }
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = do_scan<char_type>("0xBAD1DEA", "{}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
        }
        else {
            value_type i{};
            auto e = do_scan<char_type>("0xBAD1DEA", "{}", i);
            REQUIRE(!e);
            CHECK(e.error().code() == scn::error::value_out_of_range);
        }
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("ff", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("FF", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
    }
    {
        value_type i{0};
        auto e = do_scan<char_type>("0xff", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
    }
    {
        value_type i{0};
        auto e = do_scan<char_type>("0xFF", "{:b16}", i);
        CHECK(i == 0xff);
        CHECK(e);
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("text", "{}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }

    if (std::is_signed<value_type>::value) {
        value_type i{};
        auto e = do_scan<char_type>("-", "{}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
        CHECK(i == 0);
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("+", "{}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
        CHECK(i == 0);
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("123", "{:b}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("123", "{:ba}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("123", "{:b0}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
}

TEST_CASE("integer decimal separator")
{
    int i{};

    auto ret = scn::scan_default("100.200", i);
    CHECK(ret);
    CHECK(i == 100);

    char ch{};
    auto cret = scn::scan_default(ret.range(), ch);
    CHECK(cret);
    CHECK(ch == '.');

    auto ret2 = scn::scan_default(cret.range(), i);
    CHECK(ret2);
    CHECK(i == 200);
}

TEST_CASE("integer error")
{
    int i{};

    auto ret = scn::scan("str", "{}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 0);
}

TEST_CASE("integer thousands separator")
{
    int a{}, b{};

    SUBCASE("without {'}")
    {
        auto ret = scn::scan("100,200", "{}", a);
        CHECK(ret);
        CHECK(a == 100);

        char ch{};
        auto cret = scn::scan_default(ret.range(), ch);
        CHECK(cret);
        CHECK(ch == ',');

        auto ret2 = scn::scan(cret.range(), "{}", b);
        CHECK(ret2);
        CHECK(b == 200);
    }

    SUBCASE("with {'}")
    {
        auto ret = scn::scan("100,200", "{:'}", a);
        CHECK(ret);
        CHECK(a == 100200);
    }
}

TEST_CASE("parse_integer")
{
    SUBCASE("0")
    {
        scn::string_view source{"0"};
        int i{};
        auto ret = scn::parse_integer<int>(source, i);
        CHECK(ret);
        CHECK(ret.value() == source.end());
        CHECK(i == 0);
    }
    SUBCASE("123")
    {
        scn::string_view source{"123 456"};
        int i{};
        auto ret = scn::parse_integer<int>(source, i);
        CHECK(ret);
        CHECK(ret.value() == source.begin() + 3);
        CHECK(i == 123);
    }
    SUBCASE("-1024")
    {
        scn::string_view source{"-1024 456"};
        int i{};
        auto ret = scn::parse_integer<int>(source, i);
        CHECK(ret);
        CHECK(ret.value() == source.begin() + 5);
        CHECK(i == -1024);
    }
    SUBCASE("int::max()")
    {
        auto oss = std::ostringstream{};
        oss << std::numeric_limits<int>::max();
        auto source = oss.str();

        int i{};
        auto ret = scn::parse_integer<int>(
            scn::string_view{source.data(), source.size()}, i);
        CHECK(ret);
        CHECK(ret.value() == source.data() + source.size());
        CHECK(i == std::numeric_limits<int>::max());
    }
    SUBCASE("int::max() in short")
    {
        auto oss = std::ostringstream{};
        oss << std::numeric_limits<int>::max();
        auto source = oss.str();

        short i{};
        auto ret = scn::parse_integer<short>(
            scn::string_view{source.data(), source.size()}, i);
        CHECK(!ret);
        CHECK(ret.error().code() == scn::error::value_out_of_range);
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

    {
        value_type i{};
        auto e =
            do_scan<char_type>(std::to_string(maxval<value_type>()), "{}", i);
        CHECK(e);
        CHECK(i == maxval<value_type>());
    }
    {
        value_type i{};
        auto e =
            do_scan<char_type>(std::to_string(minval<value_type>()), "{}", i);
        CHECK(e);
        CHECK(i == minval<value_type>());
    }
    {
        value_type i{};
        auto e = do_scan<char_type>(std::to_string(maxval<value_type>() - 1),
                                    "{}", i);
        CHECK(e);
        CHECK(i == maxval<value_type>() - 1);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>(std::to_string(minval<value_type>() + 1),
                                    "{}", i);
        CHECK(e);
        CHECK(i == minval<value_type>() + 1);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>(overstr<value_type>(), "{}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::value_out_of_range);
    }
    {
        if (std::is_signed<value_type>::value) {
            value_type i{};
            auto e = do_scan<char_type>(understr<value_type>(), "{}", i);
            CHECK(!e);
            CHECK(e.error() == scn::error::value_out_of_range);
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

TEST_CASE("trailing")
{
    int i{}, j{};
    auto ret = scn::scan(";42;43;", ";{};{}", i, j);
    CHECK(ret);
    CHECK(i == 42);
    CHECK(j == 43);
    CHECK(ret.range().size() == 1);
    CHECK(ret.range_as_string_view()[0] == ';');
}
