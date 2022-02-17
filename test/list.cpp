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
#include "test.h"

TEST_CASE("list")
{
    std::vector<int> values;
    auto ret = scn::scan_list("0 1 2 3 42 -1 1024", values);
    CHECK(ret);

    std::vector<int> cmp{0, 1, 2, 3, 42, -1, 1024};
    CHECK(values.size() == cmp.size());
    CHECK(std::equal(values.begin(), values.end(), cmp.begin()));
}

TEST_CASE("comma list")
{
    std::vector<int> values;
    auto ret = scn::scan_list_ex("0, 1, 2, 3, 42, -1, 1024", values,
                                 scn::list_separator(','));
    CHECK(ret);

    std::vector<int> cmp{0, 1, 2, 3, 42, -1, 1024};
    CHECK(values.size() == cmp.size());
    CHECK(std::equal(values.begin(), values.end(), cmp.begin()));
}

TEST_CASE("list until line break")
{
    std::vector<int> values;
    auto ret =
        scn::scan_list_ex("0 1 2 3 42\n-1 1024", values, scn::list_until('\n'));
    CHECK(ret);

    std::vector<int> cmp{0, 1, 2, 3, 42};
    CHECK(values.size() == cmp.size());
    CHECK(std::equal(values.begin(), values.end(), cmp.begin()));

    values.clear();
    ret = scn::scan_list_ex("0 1 2 3 42 \n-1 1024", values,
                            scn::list_until('\n'));
    CHECK(ret);

    CHECK(values.size() == cmp.size());
    CHECK(std::equal(values.begin(), values.end(), cmp.begin()));
}

TEST_CASE("comma list until line break")
{
    std::vector<int> values;
    auto ret = scn::scan_list_ex("0, 1, 2, 3, 42,\n-1, 1024", values,
                                 scn::list_separator_and_until(',', '\n'));
    CHECK(ret);

    std::vector<int> cmp{0, 1, 2, 3, 42};
    CHECK(values.size() == cmp.size());
    CHECK(std::equal(values.begin(), values.end(), cmp.begin()));

    values.clear();
    ret = scn::scan_list_ex("0, 1, 2, 3, 42\n-1, 1024", values,
                            scn::list_separator_and_until(',', '\n'));
    CHECK(ret);

    CHECK(values.size() == cmp.size());
    CHECK(std::equal(values.begin(), values.end(), cmp.begin()));
}
