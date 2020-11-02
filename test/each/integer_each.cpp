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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../test.h"

#include <random>

TEST_CASE_TEMPLATE_DEFINE("integer each", T, integer_each_test)
{
    const auto min = std::numeric_limits<T>::min();
    const auto max = std::numeric_limits<T>::max();
    auto check = [&](T val) {
        std::ostringstream oss;
        oss << val;
        auto str = oss.str();

        T tmp{};
        auto ret = scn::scan_default(str, tmp);

        CHECK(ret);

        CHECK(tmp == val);
    };
    auto random_ints = [](size_t n, T a, T b) {
        std::default_random_engine rng(std::random_device{}());
        std::uniform_int_distribution<T> dist(a, b);
        std::vector<T> vec;
        std::generate_n(std::back_inserter(vec), n,
                        [&]() { return dist(rng); });
        return vec;
    };
    for (T i = min; i != min + 1000; ++i) {
        check(i);
    }
    auto ints = random_ints(10000, min, max);
    for (T i : ints) {
        check(i);
    }
    for (T i = max - 1000; i != max; ++i) {
        check(static_cast<T>(i + T{1}));
    }
}

TEST_CASE_TEMPLATE_INSTANTIATE(integer_each_test,
                               short,
                               int,
                               long,
                               long long,
                               unsigned short,
                               unsigned,
                               unsigned long,
                               unsigned long long);
