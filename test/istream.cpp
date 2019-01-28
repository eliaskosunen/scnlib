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

#include <doctest.h>
#include <scn/istream.h>
#include <scn/scn.h>
#include <sstream>

TEST_CASE("istream stream")
{
    std::string str{"123"};
    std::istringstream ss(str);
    auto stream = scn::make_stream(ss);

    int i{};
    auto ret = scn::scan(stream, "{}", i);
    CHECK(ret);
    if (!ret) {
        CHECK(ret.get_code() == scn::error::good);
    }
    CHECK(i == 123);
}

struct my_type {
    int value;

    friend std::istream& operator>>(std::istream& is, my_type& val)
    {
        is >> val.value;
        return is;
    }
};

TEST_CASE("istream value")
{
    std::string str{"123"};
    auto stream = scn::make_stream(str);

    my_type val{};
    auto ret = scn::scan(stream, "{}", val);
    CHECK(ret);
    if (!ret) {
        CHECK(ret.get_code() == scn::error::good);
    }
    CHECK(val.value == 123);
}
