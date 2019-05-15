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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../test.h"

#include <random>
#include <sstream>

TEST_CASE_TEMPLATE_DEFINE("integer each", T, integer_each_test)
{
    std::vector<scn::method> methods{scn::method::sto, scn::method::strto,
                                     scn::method::custom};
    if (scn::is_int_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);

    const auto min = std::numeric_limits<T>::min();
    const auto max = std::numeric_limits<T>::max();
    const auto options = scn::options::builder{}.int_method(method).make();
    auto check = [&](T val) {
        std::ostringstream oss;
        oss << val;
        auto str = oss.str();
        auto stream = scn::make_stream(str);

        T tmp{};
        auto ret = scn::scan(options, stream, scn::default_tag, tmp);

        CHECK(ret);
        CHECK(ret.value() == 1);

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
        check(i);
    }
    check(max);
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
