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

TEST_CASE("max_digits")
{
    CHECK(scn::detail::max_digits<int>(10) ==
          std::numeric_limits<int>::digits10 + 1);
    CHECK(scn::detail::max_digits<unsigned>(10) ==
          std::numeric_limits<unsigned>::digits10);
    CHECK(scn::detail::max_digits<long long>(10) ==
          std::numeric_limits<long long>::digits10 + 1);

    auto radix = std::numeric_limits<int>::radix;
    CHECK(scn::detail::max_digits<int>(radix) ==
          std::numeric_limits<int>::digits + 1);
    CHECK(scn::detail::max_digits<unsigned>(radix) ==
          std::numeric_limits<unsigned>::digits);
    CHECK(scn::detail::max_digits<long long>(radix) ==
          std::numeric_limits<long long>::digits + 1);

    CHECK(scn::detail::max_digits<int>(8) == 12);
    CHECK(scn::detail::max_digits<unsigned>(8) == 11);
    CHECK(scn::detail::max_digits<long long>(8) == 22);

    CHECK(scn::detail::max_digits<int>(4) == 17);
    CHECK(scn::detail::max_digits<unsigned>(4) == 16);
    CHECK(scn::detail::max_digits<long long>(4) == 33);

    CHECK(scn::detail::max_digits<int>(0) ==
          scn::detail::max_digits<int>(2) + 2);
    CHECK(scn::detail::max_digits<unsigned>(0) ==
          scn::detail::max_digits<unsigned>(2) + 2);
    CHECK(scn::detail::max_digits<long long>(0) ==
          scn::detail::max_digits<long long>(2) + 2);
}

TEST_CASE("ascii_widen")
{
    CHECK(scn::detail::ascii_widen<char>('a') == 'a');
    CHECK(scn::detail::ascii_widen<wchar_t>('a') == L'a');
}

TEST_CASE("minmax")
{
    CHECK(scn::detail::min(1, 2) == 1);
    CHECK(scn::detail::min(1, 1) == 1);

    CHECK(scn::detail::max(1, 2) == 2);
    CHECK(scn::detail::max(1, 1) == 1);
}

TEST_CASE("unique_ptr")
{
    auto ptr = scn::detail::make_unique<int>(0);
    CHECK(ptr);
    CHECK(ptr.get() != nullptr);
    CHECK(*(ptr.get()) == 0);
    CHECK(*(ptr.get()) == *ptr);

    ptr = scn::detail::unique_ptr<int>();
    CHECK(!ptr);
    CHECK(ptr.get() == nullptr);
}

TEST_CASE("erased_storage")
{
    auto val = scn::detail::erased_storage<int>{};
    CHECK(!val);
    CHECK(!val.has_value());
    CHECK(!val.operator->());

    val = scn::detail::erased_storage<int>{42};
    CHECK(val);
    CHECK(val.has_value());
    CHECK(val.operator->());
    CHECK(*val == 42);
    CHECK(val.get() == 42);

    auto copy = val;
    CHECK(copy);
    CHECK(*copy == 42);
    CHECK(val);
    CHECK(*val == 42);

    auto move = std::move(val);
    CHECK(move);
    CHECK(*move == 42);
    CHECK(!val);

    *copy = 123;
    move = std::move(copy);
    CHECK(move);
    CHECK(*move == 123);
    CHECK(!copy);
}
