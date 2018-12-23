// Copyright 2017-2018 Elias Kosunen
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

#include <doctest.h>
#include <scn/scn.h>
#include <cmath>

TEST_CASE("floating point")
{
    std::string data{
        "42 3.14 -2.22 2.0e4 0x1.bc70a3d70a3d7p+6 inf -inf nan -0"};
    auto stream = scn::make_stream(data);
    double d{};

    {
        // Integer ("42") to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(d == doctest::Approx(42.0));
        CHECK(ret);
        d = 0.0;
    }
    {
        // Basic float ("3.14") to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(d == doctest::Approx(3.14));
        CHECK(ret);
        d = 0.0;
    }
    {
        // Negative float ("-2.22") to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(d == doctest::Approx(-2.22));
        CHECK(ret);
        d = 0.0;
    }
    {
        // Float with exponent ("2.0e4") to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(d == doctest::Approx(2.0e4));
        CHECK(ret);
        d = 0.0;
    }
    {
        // Binary float to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(d == doctest::Approx(111.11));
        CHECK(ret);
        d = 0.0;
    }
    {
        // +inf to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(std::isinf(d));
        CHECK(!std::signbit(d));
        CHECK(ret);
        d = 0.0;
    }
    {
        // -inf to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(std::isinf(d));
        CHECK_FALSE(!std::signbit(d));
        CHECK(ret);
        d = 0.0;
    }
    {
        // nan to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(std::isnan(d));
        CHECK(!std::signbit(d));
        CHECK(ret);
        d = 0.0;
    }
    {
        // Negative zero to double
        auto ret = scn::scan(stream, "{}", d);
        CHECK(d == doctest::Approx(0.0));
        CHECK_FALSE(!std::signbit(d));
        CHECK(ret);
        d = 0.0;
    }
    {
        // Scan from EOF
        // Should fail
        auto ret = scn::scan(stream, "{}", d);
        CHECK(d == doctest::Approx(0.0));
        CHECK(!ret);
        if (!ret) {
            CHECK(ret.error() == scn::error::end_of_stream);
        }
        d = 0.0;
    }
}
