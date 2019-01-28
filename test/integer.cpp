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
#include <scn/scn.h>

TEST_CASE("integer")
{
    std::string data{
        "0 1 -1 2147483648\n"
        "1011 400 0400 bad1dea 0xbad1dea"};

    int i{};
    unsigned u{};
    // char c{};
    int64_t l{};
    auto stream = scn::make_stream(data);

    {
        // 0 to int
        auto ret = scn::scan(stream, "{}", i);
        CHECK(i == 0);
        CHECK(ret);
        i = 0;
    }
    {
        // 1 to uint
        auto ret = scn::scan(stream, "{}", u);
        CHECK(u == 1);
        CHECK(ret);
        u = 0;
    }
    {
        // -1 to uint
        // should fail
        auto ret = scn::scan(stream, "{}", u);
        // shouldn't modify input
        CHECK(u == 0);
        CHECK(!ret);
        if (!ret) {
            CHECK(ret == scn::error::value_out_of_range);
        }
        //u = 0;
    }
    {
        // -1 to int
        auto ret = scn::scan(stream, "{}", i);
        CHECK(i == -1);
        CHECK(ret);
        i = 0;
    }
    {
        // 2^31 to int
        // should fail (overflow)
        // max value for 32-bit signed 2's complement is 2^31 - 1
        auto ret = scn::scan(stream, "{}", i);
        CHECK(i == 0);
        CHECK(!ret);
        if (!ret) {
            CHECK(ret == scn::error::value_out_of_range);
        }
        i = 0;
    }
    {
        // 2^31 to int64
        auto ret = scn::scan(stream, "{}", l);
        CHECK(l == 2147483648);
        CHECK(ret);
        l = 0;
    }
    {
        // 0b1011 to int
        auto ret = scn::scan(stream, "{b2}", i);
        CHECK(i == 11);
        CHECK(ret);
        i = 0;
    }
    {
        // 0400 to int
        auto ret = scn::scan(stream, "{o}", i);
        CHECK(i == 0400);
        CHECK(ret);
        i = 0;
    }
    {
        // 0400 to int (detect base)
        auto ret = scn::scan(stream, "{}", i);
        CHECK(i == 0400);
        CHECK(ret);
        i = 0;
    }
    {
        // 0xbad1dea to int64
        auto ret = scn::scan(stream, "{x}", l);
        CHECK(l == 0xbad1dea);
        CHECK(ret);
        l = 0;
    }
    {
        // 0xbad1dea to int64 (detect base)
        auto ret = scn::scan(stream, "{}", l);
        CHECK(l == 0xbad1dea);
        CHECK(ret);
        //l = 0;
    }
    {
        // Scan from EOF
        // Should fail
        auto ret = scn::scan(stream, "{}", i);
        CHECK(i == 0);
        CHECK(!ret);
        if (!ret) {
            CHECK(ret.get_code() == scn::error::end_of_stream);
        }
        //i = 0;
    }
}
