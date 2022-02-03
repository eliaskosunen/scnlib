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
#include <scn/tuple_return.h>

#include "test.h"

/*
#if (defined(__cpp_structured_bindings) &&                                    \
     __cpp_structured_bindings >= 201606) ||                                  \
    SCN_GCC >= SCN_COMPILER(7, 0, 0) || SCN_CLANG >= SCN_COMPILER(4, 0, 0) || \
    SCN_MSVC >= SCN_COMPILER(19, 11, 0)
*/

#if defined(__cpp_structured_bindings) && __cpp_structured_bindings >= 201606
TEST_CASE("tuple_return")
{
    auto [r, i, s] = scn::scan_tuple<int, std::string>("42 foo", "{} {}");

    CHECK(r);

    CHECK(i == 42);
    CHECK(s == std::string{"foo"});
}

TEST_CASE("tuple_return int")
{
    auto [r, i] = scn::scan_tuple_default<int>("42");

    CHECK(r);

    CHECK(i == 42);
}

struct non_default_construct {
    non_default_construct() = delete;

    non_default_construct(int val) : value(val) {}

    int value{};
};

namespace scn {
    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")
    template <>
    struct scanner<optional<non_default_construct>> : public scanner<int> {
        template <typename Context>
        error scan(optional<non_default_construct>& val, Context& ctx)
        {
            int tmp{};
            auto ret = scanner<int>::scan(tmp, ctx);
            if (!ret) {
                return ret;
            }
            val = non_default_construct(tmp);
            return {};
        }
    };
    SCN_CLANG_POP
}  // namespace scn

TEST_CASE("tuple_return non_default_construct")
{
    auto [ret, val] =
        scn::scan_tuple_default<scn::optional<non_default_construct>>("42");

    CHECK(ret);

    REQUIRE(val);
    CHECK(val->value == 42);
}

#endif
