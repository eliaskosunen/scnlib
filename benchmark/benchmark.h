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

#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

#if SCN_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

#include <benchmark/benchmark.h>

#include <cctype>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

inline std::string generate_data(size_t len)
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

#if SCN_GCC
#pragma GCC diagnostic pop
#endif

#if SCN_CLANG
#pragma clang diagnostic pop
#endif

#endif  // SCN_BENCHMARK_BENCHMARK_H
