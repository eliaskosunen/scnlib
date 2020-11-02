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

#if SCN_HAS_STRING_VIEW
TEST_CASE("std string_view")
{
    scn::string_view sv = std::string_view("foo");
    CHECK_EQ(std::memcmp(sv.data(), "foo", 3), 0);
}

TEST_CASE("scanning std::string_view")
{
    std::string_view sv;
    auto ret = scn::scan("foo", "{}", sv);
    CHECK(ret);
    CHECK_EQ(std::memcmp(sv.data(), "foo", 3), 0);
}

TEST_CASE("getline std::string_view")
{
    std::string_view sv;
    auto ret = scn::getline("foo\nbar", sv);
    CHECK(ret);
    CHECK_EQ(std::memcmp(sv.data(), "foo", 3), 0);
}

#endif
