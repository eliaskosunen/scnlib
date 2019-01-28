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

TEST_CASE("error")
{
    SUBCASE("default construct")
    {
        auto e = scn::error{};
        CHECK(e);
        CHECK(e == scn::error::good);
        CHECK(e.get_code() == scn::error::good);
        CHECK(e.is_recoverable());
        CHECK(e == scn::error{});
    }
    SUBCASE("general")
    {
        auto e = scn::error{scn::error::end_of_stream};
        CHECK(!e);
        CHECK(e == scn::error::end_of_stream);
        CHECK(e == scn::error{scn::error::end_of_stream});
        CHECK(e.get_code() == scn::error::end_of_stream);
        CHECK(e.is_recoverable());
    }
    SUBCASE("general")
    {
        auto e = scn::error{scn::error::unrecoverable_stream_error};
        CHECK(!e);
        CHECK(e == scn::error::unrecoverable_stream_error);
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
TEST_CASE("result")
{
    SUBCASE("success")
    {
        scn::result<int> r{42};
        CHECK(r);
        CHECK(r.has_value());
        CHECK(r.value() == 42);
        CHECK(r.get_error() == scn::error::good);
    }
    SUBCASE("error")
    {
        scn::result<int> r{scn::make_error(scn::error::end_of_stream)};
        CHECK(!r);
        CHECK(!r.has_value());
        CHECK(r.get_error() == scn::error::end_of_stream);
    }
    struct not_default_constructible {
        not_default_constructible(int v) : val(v) {}

        int val;
    };
    SUBCASE("complex success")
    {
        scn::result<not_default_constructible> r{42};
        CHECK(r);
        CHECK(r.has_value());
        CHECK(r.value().val == 42);
        CHECK(r.get_error() == scn::error::good);
    }
    SUBCASE("complex error")
    {
        scn::result<not_default_constructible> r{
            scn::make_error(scn::error::end_of_stream)};
        CHECK(!r);
        CHECK(!r.has_value());
        CHECK(r.get_error() == scn::error::end_of_stream);
    }
}
