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

TEST_CASE("error")
{
    SUBCASE("default construct")
    {
        auto e = scn::error{};
        CHECK(e);
        CHECK(e == scn::error::good);
        CHECK(e.code() == scn::error::good);
        CHECK(e.is_recoverable());
        CHECK(e == scn::error{});
    }
    SUBCASE("general")
    {
        auto e = scn::error{scn::error::end_of_range, "EOF"};
        CHECK(!e);
        CHECK(e == scn::error::end_of_range);
        CHECK(e.code() == scn::error::end_of_range);
        CHECK(e.is_recoverable());
    }
    SUBCASE("general")
    {
        auto e = scn::error{scn::error::unrecoverable_source_error, ""};
        CHECK(!e);
        CHECK(e == scn::error::unrecoverable_source_error);
        CHECK(!e.is_recoverable());
    }
}
TEST_CASE("erased_storage")
{
    scn::detail::erased_storage<int> val{42};
    CHECK(*val == 42);
    auto cp = val;
    CHECK(*cp == 42);
}
TEST_CASE("expected")
{
    SUBCASE("success")
    {
        scn::expected<int> r{42};
        CHECK(r);
        CHECK(r.has_value());
        CHECK(r.value() == 42);
        CHECK(r.error() == scn::error::good);
    }
    SUBCASE("error")
    {
        scn::expected<int> r{scn::error(scn::error::end_of_range, "EOF")};
        CHECK(!r);
        CHECK(!r.has_value());
        CHECK(r.error() == scn::error::end_of_range);
    }
    struct not_default_constructible {
        not_default_constructible(int v) : val(v) {}

        int val;
    };
    SUBCASE("complex success")
    {
        scn::expected<not_default_constructible> r{42};
        CHECK(r);
        CHECK(r.has_value());
        CHECK(r.value().val == 42);
        CHECK(r.error() == scn::error::good);
    }
    SUBCASE("complex error")
    {
        scn::expected<not_default_constructible> r{
            scn::error(scn::error::end_of_range, "EOF")};
        CHECK(!r);
        CHECK(!r.has_value());
        CHECK(r.error() == scn::error::end_of_range);
    }
}
