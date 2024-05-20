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

#include "bench_helpers.h"

#include <scn/scan.h>

#include <random>
#include <sstream>
#include <vector>

template <typename T>
T generate_single_float()
{
    static std::uniform_int_distribution<uint16_t> dist(
        static_cast<uint16_t>(std::numeric_limits<uint8_t>::min()),
        static_cast<uint16_t>(std::numeric_limits<uint8_t>::max()));

    unsigned char bytes[sizeof(T)]{};

    auto make_float = [&]() {
        T f{};
        std::memcpy(&f, bytes, sizeof(T));
        return f;
    };
    auto is_good_float = [](T f) { return std::isnormal(f); };

    T f{};
    do {
        for (auto ptr = bytes; ptr != bytes + sizeof(T); ++ptr) {
            auto rand = dist(get_rng());
            std::memcpy(ptr, &rand, 1);
        }
        f = make_float();
    } while (!is_good_float(f));
    return f;
}
template <>
inline long double generate_single_float()
{
    return static_cast<long double>(generate_single_float<double>());
}

template <typename Float>
std::vector<std::string> make_float_list(std::size_t n)
{
    std::vector<std::string> result{};
    for (size_t i = 0; i < n; ++i) {
        std::ostringstream oss;
        oss << generate_single_float<Float>();
        result.push_back(SCN_MOVE(oss.str()));
    }
    return result;
}

template <typename Float>
const auto& get_float_list()
{
    static auto list = make_float_list<Float>(2 << 12);
    return list;
}

template <typename Float>
std::string make_float_string(std::size_t n)
{
    std::ostringstream oss;
    for (size_t i = 0; i < n; ++i) {
        oss << generate_single_float<Float>() << ' ';
    }
    return oss.str();
}

template <typename Float>
const std::string& get_float_string()
{
    static auto str = make_float_string<Float>(2 << 12);
    return str;
}

inline int sscanf_float(const char* ptr, float& f)
{
    return std::sscanf(ptr, "%f", &f);
}
inline int sscanf_float(const char* ptr, double& f)
{
    return std::sscanf(ptr, "%lf", &f);
}
inline int sscanf_float(const char* ptr, long double& f)
{
    return std::sscanf(ptr, "%Lf", &f);
}

inline int sscanf_float_n(const char*& ptr, float& f)
{
    int n{};
    auto ret = std::sscanf(ptr, "%f%n", &f, &n);
    ptr += n + 1;
    return ret;
}
inline int sscanf_float_n(const char*& ptr, double& f)
{
    int n{};
    auto ret = std::sscanf(ptr, "%lf%n", &f, &n);
    ptr += n + 1;
    return ret;
}
inline int sscanf_float_n(const char*& ptr, long double& f)
{
    int n{};
    auto ret = std::sscanf(ptr, "%Lf%n", &f, &n);
    ptr += n + 1;
    return ret;
}

inline bool strtod_float(const char* ptr, float& f)
{
    char* endptr{};
    f = std::strtof(ptr, &endptr);
    return endptr != ptr;
}
inline bool strtod_float(const char* ptr, double& f)
{
    char* endptr{};
    f = std::strtod(ptr, &endptr);
    return endptr != ptr;
}
inline bool strtod_float(const char* ptr, long double& f)
{
    char* endptr{};
    f = std::strtold(ptr, &endptr);
    return endptr != ptr;
}

inline int strtod_float_n(const char*& ptr, float& f)
{
    char* endptr{};
    f = std::strtof(ptr, &endptr);
    if (*ptr == '\0') {
        return EOF;
    }
    if (endptr == ptr) {
        return 1;
    }
    ptr = endptr;
    return 0;
}
inline int strtod_float_n(const char*& ptr, double& f)
{
    char* endptr{};
    f = std::strtod(ptr, &endptr);
    if (*ptr == '\0') {
        return EOF;
    }
    if (endptr == ptr) {
        return 1;
    }
    ptr = endptr;
    return 0;
}
inline int strtod_float_n(const char*& ptr, long double& f)
{
    char* endptr{};
    f = std::strtold(ptr, &endptr);
    if (*ptr == '\0') {
        return EOF;
    }
    if (endptr == ptr) {
        return 1;
    }
    ptr = endptr;
    return 0;
}
