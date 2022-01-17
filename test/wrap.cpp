// Copyright 2017 Elias Kosunen
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

static_assert(SCN_CHECK_CONCEPT(scn::ranges::view<scn::string_view>), "");
static_assert(!SCN_CHECK_CONCEPT(scn::ranges::view<const char*>), "");
static_assert(!SCN_CHECK_CONCEPT(scn::ranges::view<const char (&)[2]>), "");

TEST_CASE("lvalue range_wrapper")
{
    auto wrapped = scn::wrap("123 456");
    auto range = scn::wrap(wrapped);

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}

TEST_CASE("rvalue range_wrapper")
{
    auto range = scn::wrap(scn::wrap("123 456"));

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}

// mapped_file has .wrap() member function
TEST_CASE("lvalue mapped_file")
{
    auto file = scn::mapped_file{};
    auto range = scn::wrap(file);

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}
TEST_CASE("rvalue mapped_file")
{
    auto range = scn::wrap(scn::mapped_file{});

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}

TEST_CASE("string literal")
{
    auto range = scn::wrap("");

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}
TEST_CASE("wide string literal")
{
    auto range = scn::wrap(L"");

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::wstring_view>>::value,
        "");
}

TEST_CASE("lvalue span")
{
    auto source = scn::span<char>{};
    auto range = scn::wrap(source);

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}
TEST_CASE("rvalue span")
{
    auto range = scn::wrap(scn::span<char>{});

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}

TEST_CASE("lvalue string")
{
    auto source = std::string{};
    auto range = scn::wrap(source);

    static_assert(
        std::is_same<decltype(range),
                     scn::detail::range_wrapper<scn::string_view>>::value,
        "");
}
TEST_CASE("rvalue string")
{
    auto range = scn::wrap(std::string{});

    static_assert(std::is_same<decltype(range),
                               scn::detail::range_wrapper<std::string>>::value,
                  "");
}

TEST_CASE("lvalue file")
{
    auto source = scn::file{};
    auto range = scn::wrap(source);

    static_assert(std::is_same<decltype(range),
                               scn::detail::range_wrapper<scn::file&>>::value,
                  "");
}
TEST_CASE("rvalue file")
{
    auto range = scn::wrap(scn::file{});

    static_assert(std::is_same<decltype(range),
                               scn::detail::range_wrapper<scn::file>>::value,
                  "");
}
