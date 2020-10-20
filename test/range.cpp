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

TEST_CASE("string lvalue")
{
    int a;
    std::string source{"123 456"};
    auto ret = scn::scan(source, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.reconstruct() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::non_reconstructed_scan_result<
                                   scn::detail::range_wrapper<scn::string_view>,
                                   std::string, scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}
TEST_CASE("string rvalue")
{
    int a;
    auto ret = scn::scan(std::string{"123 456"}, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.reconstruct() == " 456");
    static_assert(
        std::is_same<decltype(ret), scn::detail::reconstructed_scan_result<
                                        scn::detail::range_wrapper<std::string>,
                                        scn::wrapped_error>>::value,
        "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}

TEST_CASE("string_view lvalue")
{
    int a;
    scn::string_view source{"123 456"};
    auto ret = scn::scan(source, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.string() == " 456");
    static_assert(
        std::is_same<decltype(ret),
                     scn::detail::non_reconstructed_scan_result<
                         scn::detail::range_wrapper<scn::string_view>,
                         scn::string_view, scn::wrapped_error>>::value,
        "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}
TEST_CASE("string_view rvalue")
{
    int a;
    auto ret = scn::scan(scn::string_view{"123 456"}, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.string() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::reconstructed_scan_result<
                                   scn::detail::range_wrapper<scn::string_view>,
                                   scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}

TEST_CASE("string literal")
{
    int a;
    auto ret = scn::scan("123 456", "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.string() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::reconstructed_scan_result<
                                   scn::detail::range_wrapper<scn::string_view>,
                                   scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}
