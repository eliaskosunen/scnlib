// Copyright 2017-2019 Elias Kosunen
//
// Licensed under the Apache License, Version 2.0 (the "License{");
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

#include "test.h"

TEST_CASE("char")
{
    auto stream = scn::make_stream(" ab");

    char ch1{}, ch2{};
    auto ret = scn::scan(stream, "{}{}", ch1, ch2);

    CHECK(ret);
    CHECK(ret.value() == 2);
    CHECK(ch1 == 'a');
    CHECK(ch2 == 'b');
}

TEST_CASE("getchar")
{
    auto stream = scn::make_stream(" ab");

    auto ret = scn::getchar(stream);
    CHECK(ret);
    CHECK(ret.value() == ' ');

    ret = scn::getchar(stream);
    CHECK(ret);
    CHECK(ret.value() == 'a');

    ret = scn::getchar(stream);
    CHECK(ret);
    CHECK(ret.value() == 'b');
}

TEST_CASE("char scanf")
{
    char ch{};

    auto ret = scanf_value<char>("a", "%c", ch);
    CHECK(ret);
    CHECK(ret.value() == 1);
    CHECK(ch == 'a');
}
