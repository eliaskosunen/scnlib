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

#ifndef SCN_BENCHMARK_BENCHMARK_H
#define SCN_BENCHMARK_BENCHMARK_H

#include <scn/scn.h>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wpadded")
SCN_CLANG_IGNORE("-Wweak-vtables")
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wused-but-marked-unused")
SCN_CLANG_IGNORE("-Wexit-time-destructors")

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wswitch-default")
SCN_GCC_IGNORE("-Wredundant-decls")

#include <benchmark/benchmark.h>

#include <cctype>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

#define STRTO_METHOD (static_cast<int64_t>(scn::method::strto))
#define STO_METHOD (static_cast<int64_t>(scn::method::sto))
#define FROM_CHARS_METHOD (static_cast<int64_t>(scn::method::from_chars))
#define CUSTOM_METHOD (static_cast<int64_t>(scn::method::custom))

inline std::string generate_buffer(size_t len)
{
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(std::numeric_limits<char>::min(),
                                         std::numeric_limits<char>::max());

    std::string data;
    data.reserve(len);
    std::generate(data.begin(), data.end(), [&]() { return dist(rng); });
    return data;
}

template <typename Char>
inline std::basic_string<Char> generate_data(size_t)
{
}

template <>
inline std::basic_string<char> generate_data<char>(size_t len)
{
    static const std::vector<char> chars = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'A',  'B',
        'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M',  'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',  'Y',  'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',  'k',  'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',  'w',  'x',
        'y', 'z', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\n', '\n', '\t'};
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size() - 1));

    std::string data;
    for (std::size_t i = 0; i < len; ++i) {
        data.push_back(chars[static_cast<size_t>(dist(rng))]);
    }
    return data;
}
template <>
inline std::basic_string<wchar_t> generate_data<wchar_t>(size_t len)
{
    static const std::vector<wchar_t> chars = {
        L'0', L'1', L'2', L'3',  L'4',  L'5', L'6', L'7', L'8', L'9', L'A',
        L'B', L'C', L'D', L'E',  L'F',  L'G', L'H', L'I', L'J', L'K', L'L',
        L'M', L'N', L'O', L'P',  L'Q',  L'R', L'S', L'T', L'U', L'V', L'W',
        L'X', L'Y', L'Z', L'a',  L'b',  L'c', L'd', L'e', L'f', L'g', L'h',
        L'i', L'j', L'k', L'l',  L'm',  L'n', L'o', L'p', L'q', L'r', L's',
        L't', L'u', L'v', L'w',  L'x',  L'y', L'z', L' ', L' ', L' ', L' ',
        L' ', L' ', L' ', L'\n', L'\n', L'\t'};
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size() - 1));

    std::wstring data;
    for (std::size_t i = 0; i < len; ++i) {
        data.push_back(chars[static_cast<size_t>(dist(rng))]);
    }
    return data;
}

template <typename Int>
std::string generate_int_data(size_t n)
{
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<Int> dist(std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());

    std::ostringstream oss;
    for (size_t i = 0; i < n; ++i) {
        oss << dist(rng) << ' ';
    }
    return oss.str();
}
template <typename Float>
std::string generate_float_data(size_t n)
{
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<int> int_dist(-16, 16);
    std::uniform_real_distribution<Float> float_dist(Float(0.0), Float(1.0));

    std::ostringstream oss;
    for (size_t i = 0; i < n; ++i) {
        auto f = float_dist(rng);
        auto exp = int_dist(rng);
        f = std::scalbn(f, exp);
        oss << f << ' ';
    }
    return oss.str();
}

SCN_GCC_POP
SCN_CLANG_POP

#endif  // SCN_BENCHMARK_BENCHMARK_H
