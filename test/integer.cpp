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

    ret = scn::scan_default("0", i);
    CHECK(ret);
    CHECK(i == 0);
    CHECK(ret.range().empty());
    i = 1;

    auto wret = scn::scan_default(L"0", i);
    CHECK(wret);
    CHECK(i == 0);
    CHECK(wret.range().empty());
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
    CHECK(ret.error() == scn::error::value_out_of_range);

    ret = scn::scan("-32768", "{}", i);
    CHECK(ret);
    CHECK(i == -32768);
    CHECK(ret.range().size() == 0);

    ret = scn::scan("-32769", "{}", i);
    CHECK(!ret);
    CHECK(i == -32768);
    CHECK(ret.error() == scn::error::value_out_of_range);

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
    CHECK(ret.error() == scn::error::invalid_scanned_value);

    ret = scn::scan("-32769", "{}", u);
    CHECK(!ret);
    CHECK(u == 32768);
    CHECK(ret.error() == scn::error::invalid_scanned_value);

    ret = scn::scan("65535", "{}", u);
    CHECK(ret);
    CHECK(u == 65535);
    CHECK(ret.range().size() == 0);

    ret = scn::scan("65536", "{}", u);
    CHECK(!ret);
    CHECK(u == 65535);
    CHECK(ret.error() == scn::error::value_out_of_range);
}

TEST_CASE("format string")
{
    int i{};

    // Default = d, decimal
    auto ret = scn::scan("1", "{:d}", i);
    CHECK(ret);
    CHECK(i == 1);

    // u = unsigned, negative numbers not allowed
    ret = scn::scan("2", "{:u}", i);
    CHECK(ret);
    CHECK(i == 2);

    // negative number with 'u'
    ret = scn::scan("-3", "{:u}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_scanned_value);
    CHECK(i == 2);

    // i base detect
    ret = scn::scan("4", "{:i}", i);
    CHECK(ret);
    CHECK(i == 4);

    // Starts with 0b -> binary
    ret = scn::scan("0b101", "{:i}", i);
    CHECK(ret);
    CHECK(i == 5);

    // Starts with 0B -> binary
    ret = scn::scan("0b110", "{:i}", i);
    CHECK(ret);
    CHECK(i == 6);

    // Starts with 0o -> octal
    ret = scn::scan("0o7", "{:i}", i);
    CHECK(ret);
    CHECK(i == 7);

    // Starts with 0O -> octal
    ret = scn::scan("0O10", "{:i}", i);
    CHECK(ret);
    CHECK(i == 8);

    // Starts with 0 -> octal
    ret = scn::scan("011", "{:i}", i);
    CHECK(ret);
    CHECK(i == 9);

    // Starts with 0 -> octal
    ret = scn::scan("012", "{:i}", i);
    CHECK(ret);
    CHECK(i == 10);

    // Starts with 0x -> hex
    ret = scn::scan("0xb", "{:i}", i);
    CHECK(ret);
    CHECK(i == 11);

    // Starts with 0X -> hex
    ret = scn::scan("0XC", "{:i}", i);
    CHECK(ret);
    CHECK(i == 12);

    // Hex case irrelevant
    ret = scn::scan("0xD", "{:i}", i);
    CHECK(ret);
    CHECK(i == 13);

    // Hex case irrelevant
    ret = scn::scan("0Xe", "{:i}", i);
    CHECK(ret);
    CHECK(i == 14);

    // Just 0
    ret = scn::scan("0", "{:i}", i);
    CHECK(ret);
    CHECK(i == 0);

    // Again, decimal by default
    ret = scn::scan("15", "{:i}", i);
    CHECK(ret);
    CHECK(i == 15);

    // b = binary
    ret = scn::scan("10000", "{:b}", i);
    CHECK(ret);
    CHECK(i == 16);

    // allow 0b prefix
    ret = scn::scan("0b10001", "{:b}", i);
    CHECK(ret);
    CHECK(i == 17);

    // allow 0B prefix
    ret = scn::scan("0b10010", "{:b}", i);
    CHECK(ret);
    CHECK(i == 18);

    // o = octal
    ret = scn::scan("23", "{:o}", i);
    CHECK(ret);
    CHECK(i == 19);

    // allow 0o prefix
    ret = scn::scan("0o24", "{:o}", i);
    CHECK(ret);
    CHECK(i == 20);

    // allow 0O prefix
    ret = scn::scan("0O25", "{:o}", i);
    CHECK(ret);
    CHECK(i == 21);

    // allow 0 prefix
    ret = scn::scan("026", "{:o}", i);
    CHECK(ret);
    CHECK(i == 22);

    // x = hex
    ret = scn::scan("17", "{:x}", i);
    CHECK(ret);
    CHECK(i == 23);

    // allow 0x prefix
    ret = scn::scan("0x18", "{:x}", i);
    CHECK(ret);
    CHECK(i == 24);

    // allow 0X prefix
    ret = scn::scan("0x19", "{:x}", i);
    CHECK(ret);
    CHECK(i == 25);

    // B2 == binary
    ret = scn::scan("11010", "{:B2}", i);
    CHECK(ret);
    CHECK(i == 26);

    // B02 -> fail
    ret = scn::scan("11010", "{:B02}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 26);

    // Don't allow prefix with B__ -> 0 only parsed
    ret = scn::scan("0b11010", "{:B2}", i);
    CHECK(ret);
    CHECK(ret.range_as_string() == "b11010");
    CHECK(i == 0);

    // B3 == ternary
    ret = scn::scan("1000", "{:B3}", i);
    CHECK(ret);
    CHECK(i == 27);

    // B36 == base-36
    ret = scn::scan("S", "{:B36}", i);
    CHECK(ret);
    CHECK(i == 28);

    // B36 == base-36
    ret = scn::scan("t", "{:B36}", i);
    CHECK(ret);
    CHECK(i == 29);

    // Base over 36, 2 digits
    ret = scn::scan("10001", "{:B37}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 29);

    // Base over 36, 3 digits
    ret = scn::scan("10001", "{:B100}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::invalid_format_string);
    CHECK(i == 29);
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
            CHECK(e.error() == scn::error::invalid_scanned_value);
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
            CHECK(e.error() == scn::error::value_out_of_range);
        }
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("1011", "{:B2}", i);
        CHECK(i == 11);
        CHECK(e);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("10", "{:o}", i);
        CHECK(i == 010);
        CHECK(e);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("010", "{:i}", i);
        CHECK(i == 010);
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
            CHECK(e.error() == scn::error::value_out_of_range);
        }
    }
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = do_scan<char_type>("0xbad1dea", "{:i}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
        }
        else {
            value_type i{};
            auto e = do_scan<char_type>("0xbad1dea", "{:i}", i);
            REQUIRE(!e);
            CHECK(e.error() == scn::error::value_out_of_range);
        }
    }
    {
        if (can_fit_badidea) {
            value_type i{};
            auto e = do_scan<char_type>("0xBAD1DEA", "{:i}", i);
            CHECK(i == 0xbad1dea);
            CHECK(e);
        }
        else {
            value_type i{};
            auto e = do_scan<char_type>("0xBAD1DEA", "{:i}", i);
            CHECK(!e);
            CHECK(e.error() == scn::error::value_out_of_range);
        }
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("2f", "{:B16}", i);
        CHECK(i == 0x2f);
        CHECK(e);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("2F", "{:B16}", i);
        CHECK(i == 0x2f);
        CHECK(e);
    }
    {
        value_type i{0};
        auto e = do_scan<char_type>("0x2f", "{:B16}", i);
        CHECK(e);
        CHECK(i == 0);
    }
    {
        value_type i{0};
        auto e = do_scan<char_type>("0x2F", "{:B16}", i);
        CHECK(e);
        CHECK(i == 0);
    }

    {
        value_type i{};
        auto e = do_scan<char_type>("text", "{}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_scanned_value);
    }

    {
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
        auto e = do_scan<char_type>("123", "{:B}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("123", "{:Ba}", i);
        CHECK(!e);
        CHECK(e.error() == scn::error::invalid_format_string);
        CHECK(i == 0);
    }
    {
        value_type i{};
        auto e = do_scan<char_type>("123", "{:B0}", i);
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
        CHECK(ret.error() == scn::error::value_out_of_range);
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

TYPE_TO_STRING(char_intpair<signed char>);
TYPE_TO_STRING(char_intpair<short>);
TYPE_TO_STRING(char_intpair<int>);
TYPE_TO_STRING(char_intpair<long>);
TYPE_TO_STRING(char_intpair<long long>);
TYPE_TO_STRING(char_intpair<unsigned char>);
TYPE_TO_STRING(char_intpair<unsigned short>);
TYPE_TO_STRING(char_intpair<unsigned int>);
TYPE_TO_STRING(char_intpair<unsigned long>);
TYPE_TO_STRING(char_intpair<unsigned long long>);
TYPE_TO_STRING(wchar_intpair<signed char>);
TYPE_TO_STRING(wchar_intpair<short>);
TYPE_TO_STRING(wchar_intpair<int>);
TYPE_TO_STRING(wchar_intpair<long>);
TYPE_TO_STRING(wchar_intpair<long long>);
TYPE_TO_STRING(wchar_intpair<unsigned char>);
TYPE_TO_STRING(wchar_intpair<unsigned short>);
TYPE_TO_STRING(wchar_intpair<unsigned int>);
TYPE_TO_STRING(wchar_intpair<unsigned long>);
TYPE_TO_STRING(wchar_intpair<unsigned long long>);

TEST_CASE_TEMPLATE_INSTANTIATE(integer_test,
                               char_intpair<signed char>,
                               char_intpair<short>,
                               char_intpair<int>,
                               char_intpair<long>,
                               char_intpair<long long>,
                               char_intpair<unsigned char>,
                               char_intpair<unsigned short>,
                               char_intpair<unsigned int>,
                               char_intpair<unsigned long>,
                               char_intpair<unsigned long long>,
                               wchar_intpair<signed char>,
                               wchar_intpair<short>,
                               wchar_intpair<int>,
                               wchar_intpair<long>,
                               wchar_intpair<long long>,
                               wchar_intpair<unsigned char>,
                               wchar_intpair<unsigned short>,
                               wchar_intpair<unsigned int>,
                               wchar_intpair<unsigned long>,
                               wchar_intpair<unsigned long long>);
TEST_CASE_TEMPLATE_INSTANTIATE(integer_range_test,
                               char_intpair<signed char>,
                               char_intpair<short>,
                               char_intpair<int>,
                               char_intpair<long>,
                               char_intpair<long long>,
                               char_intpair<unsigned char>,
                               char_intpair<unsigned short>,
                               char_intpair<unsigned int>,
                               char_intpair<unsigned long>,
                               char_intpair<unsigned long long>,
                               wchar_intpair<signed char>,
                               wchar_intpair<short>,
                               wchar_intpair<int>,
                               wchar_intpair<long>,
                               wchar_intpair<long long>,
                               wchar_intpair<unsigned char>,
                               wchar_intpair<unsigned short>,
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

TEST_CASE("consistency")
{
    SUBCASE("simple")
    {
        {
            std::string source{"123 456"};
            int i{};
            auto ret = consistency_iostream(source, i);
            CHECK(ret);
            CHECK(i == 123);
            CHECK(source == " 456");
        }
        {
            std::string source{"123 456"};
            int i{};
            auto ret = consistency_scanf(source, "%d", i);
            CHECK(ret);
            CHECK(i == 123);
            CHECK(source == " 456");
        }
        {
            int i{};
            auto ret = scn::scan("123 456", "{}", i);
            CHECK(ret);
            CHECK(i == 123);
            CHECK(ret.range_as_string() == " 456");
        }
    }

    SUBCASE("preceding whitespace")
    {
        {
            std::string source{" \n123 456"};
            int i{};
            auto ret = consistency_iostream(source, i);
            CHECK(ret);
            CHECK(i == 123);
            CHECK(source == " 456");
        }
        {
            std::string source{" \n123 456"};
            int i{};
            auto ret = consistency_scanf(source, "%d", i);
            CHECK(ret);
            CHECK(i == 123);
            CHECK(source == " 456");
        }
        {
            int i{};
            auto ret = scn::scan(" \n123 456", "{}", i);
            CHECK(ret);
            CHECK(i == 123);
            CHECK(ret.range_as_string() == " 456");
        }
    }

    SUBCASE("unexpected float")
    {
        {
            std::string source{"1.23 456"};
            int i{};
            auto ret = consistency_iostream(source, i);
            CHECK(ret);
            CHECK(i == 1);
            CHECK(source == ".23 456");
        }
        {
            std::string source{"1.23 456"};
            int i{};
            auto ret = consistency_scanf(source, "%d", i);
            CHECK(ret);
            CHECK(i == 1);
            CHECK(source == ".23 456");
        }
        {
            int i{};
            auto ret = scn::scan("1.23 456", "{}", i);
            CHECK(ret);
            CHECK(i == 1);
            CHECK(ret.range_as_string() == ".23 456");
        }
    }

    SUBCASE("unexpected char")
    {
        {
            std::string source{"1foo bar"};
            int i{};
            auto ret = consistency_iostream(source, i);
            CHECK(ret);
            CHECK(i == 1);
            CHECK(source == "foo bar");
        }
        {
            std::string source{"1foo bar"};
            int i{};
            auto ret = consistency_scanf(source, "%d", i);
            CHECK(ret);
            CHECK(i == 1);
            CHECK(source == "foo bar");
        }
        {
            int i{};
            auto ret = scn::scan("1foo bar", "{}", i);
            CHECK(ret);
            CHECK(i == 1);
            CHECK(ret.range_as_string() == "foo bar");
        }
    }
}

TEST_CASE("deque + L")
{
    auto source = get_deque<char>("123");
    int i{};
    auto ret = scn::scan(source, "{:L}", i);
    CHECK(ret);
    CHECK(i == 123);
    CHECK(ret.range().empty());
    i = 0;

    ret = scn::scan(ret.range(), "{:L}", i);
    CHECK(!ret);
    CHECK(ret.error() == scn::error::end_of_range);
    CHECK(i == 0);
}

TEST_CASE("signed char, -3") {
    signed char sch{};
    auto ret = scn::scan("-3", "{}", sch);
    CHECK(ret);
    CHECK(sch == -3);
    CHECK(ret.range().empty());
}

TEST_CASE("int as c") {
    int i{};
    auto ret = scn::scan("1", "{:c}", i);
    CHECK(ret);
    CHECK(i == '1');
    CHECK(ret.range().empty());
}
