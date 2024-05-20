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

#include <scn/scan.h>

#include "bench_helpers.h"

#include <random>
#include <sstream>
#include <vector>

template <typename Int>
std::vector<std::string> make_integer_list(std::size_t n)
{
    static std::uniform_int_distribution<Int> dist(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    std::vector<std::string> result{};
    for (size_t i = 0; i < n; ++i) {
        std::ostringstream oss;
        oss << dist(get_rng());
        result.push_back(SCN_MOVE(oss.str()));
    }
    return result;
}

template <typename Int>
const auto& get_integer_list()
{
    static auto list = make_integer_list<Int>(2 << 12);
    return list;
}

template <typename Int>
std::string make_integer_string(std::size_t n)
{
    static std::uniform_int_distribution<Int> dist(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());

    std::ostringstream oss;
    for (size_t i = 0; i < n; ++i) {
        oss << dist(get_rng()) << " ";
    }
    return oss.str();
}

template <typename Int>
const std::string& get_integer_string()
{
    static auto str = make_integer_string<Int>(2 << 12);
    return str;
}

inline int sscanf_integral(const char* ptr, int& i)
{
    return std::sscanf(ptr, "%d", &i);
}
inline int sscanf_integral(const char* ptr, long long& i)
{
    return std::sscanf(ptr, "%lld", &i);
}
inline int sscanf_integral(const char* ptr, unsigned& i)
{
    return std::sscanf(ptr, "%u", &i);
}

inline int sscanf_integral_n(const char*& ptr, int& i)
{
    int n{};
    auto ret = std::sscanf(ptr, "%d%n", &i, &n);
    ptr += n + 1;
    return ret;
}
inline int sscanf_integral_n(const char*& ptr, long long& i)
{
    int n{};
    auto ret = std::sscanf(ptr, "%lld%n", &i, &n);
    ptr += n + 1;
    return ret;
}
inline int sscanf_integral_n(const char*& ptr, unsigned& i)
{
    int n{};
    auto ret = std::sscanf(ptr, "%u%n", &i, &n);
    ptr += n + 1;
    return ret;
}

inline bool strtol_integral(const char* ptr, int& i)
{
    char* endptr{};
    auto tmp = std::strtol(ptr, &endptr, 0);
    i = static_cast<int>(tmp);
    return endptr != ptr;
}
inline bool strtol_integral(const char* ptr, long long& i)
{
    char* endptr{};
    i = std::strtoll(ptr, &endptr, 0);
    return endptr != ptr;
}
inline bool strtol_integral(const char* ptr, unsigned& i)
{
    char* endptr{};
    auto tmp = std::strtoul(ptr, &endptr, 0);
    i = static_cast<unsigned>(tmp);
    return endptr != ptr;
}

inline int strtol_integral_n(const char*& ptr, int& i)
{
    char* endptr{};
    auto tmp = std::strtol(ptr, &endptr, 0);
    if (*ptr == '\0') {
        return EOF;
    }
    if (endptr == ptr) {
        return 1;
    }
    ptr = endptr;
    i = static_cast<int>(tmp);
    return 0;
}
inline int strtol_integral_n(const char*& ptr, long long& i)
{
    char* endptr{};
    i = std::strtoll(ptr, &endptr, 0);
    if (*ptr == '\0') {
        return EOF;
    }
    if (endptr == ptr) {
        return 1;
    }
    ptr = endptr;
    return 0;
}
inline int strtol_integral_n(const char*& ptr, unsigned& i)
{
    char* endptr{};
    auto tmp = std::strtoul(ptr, &endptr, 0);
    if (*ptr == '\0') {
        return EOF;
    }
    if (endptr == ptr) {
        return 1;
    }
    ptr = endptr;
    i = static_cast<unsigned>(tmp);
    return 0;
}
