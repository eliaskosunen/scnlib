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

#ifndef SCN_BENCHMARK_FLOAT_H
#define SCN_BENCHMARK_FLOAT_H

#include "../benchmark.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <sstream>
#include <vector>

#define FLOAT_DATA_N (static_cast<size_t>(2 << 12))

template <typename T>
T generate_single_float()
{
    static std::uniform_int_distribution<int> int_dist(-16, 16);
    static std::uniform_real_distribution<T> float_dist(T(0.0), T(1.0));
    auto f = float_dist(get_rng());
    auto exp = int_dist(get_rng());
    return std::scalbn(f, exp);
}

template <typename Float>
std::vector<std::string> stringified_floats_list(size_t n = FLOAT_DATA_N)
{
    std::vector<std::string> ret;
    for (size_t i = 0; i < n; ++i) {
        std::ostringstream oss;
        oss << generate_single_float<Float>();
        ret.push_back(std::move(oss).str());
    }
    return ret;
}

template <typename Float>
std::string stringified_float_list(size_t n = FLOAT_DATA_N,
                                   const char* delim = " ")
{
    std::ostringstream oss;
    for (size_t i = 0; i < n; ++i) {
        oss << generate_single_float<Float>() << delim;
    }
    return oss.str();
}

inline int scanf_float(const char* ptr, float& f)
{
    return sscanf(ptr, "%f", &f);
}
inline int scanf_float(const char* ptr, double& f)
{
    return sscanf(ptr, "%lf", &f);
}
inline int scanf_float(const char* ptr, long double& f)
{
    return sscanf(ptr, "%Lf", &f);
}

inline int scanf_float_n(char*& ptr, float& f)
{
    int n;
    auto ret = sscanf(ptr, "%f%n", &f, &n);
    ptr += n + 1;
    return ret;
}
inline int scanf_float_n(char*& ptr, double& f)
{
    int n;
    auto ret = sscanf(ptr, "%lf%n", &f, &n);
    ptr += n + 1;
    return ret;
}
inline int scanf_float_n(char*& ptr, long double& f)
{
    int n;
    auto ret = sscanf(ptr, "%Lf%n", &f, &n);
    ptr += n + 1;
    return ret;
}

#endif  // SCN_BENCHMARK_FLOAT_H
