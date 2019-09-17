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
#include "test.h"

TEST_CASE("bidirectional iterator stream")
{
    auto data = std::string{"Hello world"};
    auto stream = scn::make_stream(data.begin(), data.end());

    {
        std::string str{};
        auto ret = scn::scan(stream, "{}", str);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(str == "Hello");
    }
    {
        int i{};
        auto ret = scn::scan(stream, "{}", i);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_scanned_value);
        CHECK(i == 0);
    }
    {
        std::string str{};
        auto ret = scn::scan(stream, "{}", str);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(str == "world");
    }
}

TEST_CASE("forward iterator stream")
{
    auto data = std::string{"Hello world"};
    auto stream = scn::basic_forward_iterator_stream<std::string::iterator>(
        data.begin(), data.end());

    {
        std::string str{};
        auto ret = scn::scan(stream, "{}", str);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(str == "Hello");
    }
    {
        int i{};
        auto ret = scn::scan(stream, "{}", i);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_scanned_value);
        CHECK(i == 0);
    }
    {
        std::string str{};
        auto ret = scn::scan(stream, "{}", str);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(str == "world");
    }
}

TEST_CASE("erased stream")
{
    auto stream = scn::make_erased_stream("Hello 42");

    std::string str{};
    int i{};
    auto ret = scn::scan(stream, "{} {}", str, i);
    CHECK(ret);
    CHECK(ret.value() == 2);
    CHECK(str == "Hello");
    CHECK(i == 42);
}
