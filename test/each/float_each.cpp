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

TEST_CASE_TEMPLATE_DEFINE("float each", T, float_each_test)
{
    std::vector<scn::method> methods{scn::method::sto, scn::method::strto};
    if (scn::is_int_from_chars_available()) {
        methods.push_back(scn::method::from_chars);
    }
    scn::method method{};

    DOCTEST_VALUE_PARAMETERIZED_DATA(method, methods);
    using limit = std::numeric_limits<T>;

    const auto list = [&]() {
        std::vector<T> vec;
        vec.push_back(limit::infinity());
        vec.push_back(limit::quiet_NaN());
        vec.push_back(-limit::infinity());
        vec.push_back(-limit::quiet_NaN());
        vec.push_back(T{+0.0});
        vec.push_back(T{-0.0});

        std::default_random_engine rng(std::random_device{}());
        std::uniform_real_distribution<T> float_dist(T(0.0), T(1.0));
        std::uniform_int_distribution<int> int_dist(limit::min_exponent10,
                                                    limit::max_exponent10);
        std::generate_n(std::back_inserter(vec), 256, [&]() {
            auto f = float_dist(rng);
            auto exp = int_dist(rng);
            return std::scalbn(f, exp);
        });
        return vec;
    }();

    const auto options = scn::options::builder{}.float_method(method).make();
    auto float_eq = [](T x, T y) {
        if (x == y) {
            return true;
        }
        if (std::isnan(x) && std::isnan(y)) {
            return true;
        }
        return std::abs(x - y) <=
                   std::numeric_limits<T>::epsilon() * std::abs(x + y) ||
               std::abs(x - y) < std::numeric_limits<T>::min();
    };
    auto check = [&](T val) {
        std::ostringstream oss;
        oss << val;
        auto str = oss.str();
        auto stream = scn::make_stream(str);

        T tmp{};
        auto ret = scn::scan_default(options, stream, tmp);

        CHECK(ret);
        CHECK(ret.value() == 1);

        CHECK(float_eq(tmp, val));
    };
    for (T f : list) {
        check(f);
    }
}

TEST_CASE_TEMPLATE_INSTANTIATE(float_each_test, float, double, long double);
