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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "test.h"

#include <scn/tuple_return.h>

static std::tuple<scn::result<int>> just_result()
{
    return {{1}};
}
static std::tuple<scn::result<int>, int> result_and_int()
{
    return {{1}, {2}};
}

TEST_CASE("structured_bindings tie")
{
    SUBCASE("just_result")
    {
        scn::result<int> r(0);
        std::tie(r) = just_result();
        CHECK(r.value() == 1);
    }
    SUBCASE("result_and_int")
    {
        scn::result<int> r(0);
        int i(0);
        std::tie(r, i) = result_and_int();
        CHECK(r.value() == 1);
        CHECK(i == 2);
    }
}
TEST_CASE("structured_bindings")
{
    SUBCASE("just_result")
    {
        auto [r] = just_result();
        CHECK(r.value() == 1);
    }
    SUBCASE("result_and_int")
    {
        auto [r, i] = result_and_int();
        CHECK(r.value() == 1);
        CHECK(i == 2);
    }
}
