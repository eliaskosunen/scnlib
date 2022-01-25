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

#ifndef SCN_BENCHMARK_INTEGER_H
#define SCN_BENCHMARK_INTEGER_H

#include "../benchmark.h"

#include <cstdio>
#include <limits>
#include <sstream>
#include <vector>

#define INT_DATA_N (static_cast<size_t>(2 << 12))

template <typename Int>
std::vector<std::string> stringified_integers_list(size_t n = INT_DATA_N)
{
    static std::uniform_int_distribution<Int> dist(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    std::vector<std::string> ret;
    for (size_t i = 0; i < n; ++i) {
        std::ostringstream oss;
        oss << dist(get_rng());
        ret.push_back(std::move(oss).str());
    }
    return ret;
}

template <typename Int>
std::string stringified_integer_list(size_t n = INT_DATA_N,
                                     const char* delim = " ")
{
    static std::uniform_int_distribution<Int> dist(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());

    std::ostringstream oss;
    for (size_t i = 0; i < n; ++i) {
        oss << dist(get_rng()) << delim;
    }
    return oss.str();
}

inline int scanf_integral(const char* ptr, int& i)
{
    return sscanf(ptr, "%d", &i);
}
inline int scanf_integral(const char* ptr, long long& i)
{
    return sscanf(ptr, "%lld", &i);
}
inline int scanf_integral(const char* ptr, unsigned& i)
{
    return sscanf(ptr, "%u", &i);
}

inline int scanf_integral_n(char*& ptr, int& i)
{
    int n;
    auto ret = sscanf(ptr, "%d%n", &i, &n);
    ptr += n + 1;
    return ret;
}
inline int scanf_integral_n(char*& ptr, long long& i)
{
    int n;
    auto ret = sscanf(ptr, "%lld%n", &i, &n);
    ptr += n + 1;
    return ret;
}
inline int scanf_integral_n(char*& ptr, unsigned& i)
{
    int n;
    auto ret = sscanf(ptr, "%u%n", &i, &n);
    ptr += n + 1;
    return ret;
}

#endif  // SCN_BENCHMARK_INTEGER_H
