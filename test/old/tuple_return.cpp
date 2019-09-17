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

TEST_CASE("tuple_return")
{
    auto stream = scn::make_stream("42 foo");

    int i;
    std::string s;
    scn::result<int> r(0);
    std::tie(r, i, s) = scn::scan<int, std::string>(stream, "{} {}");

    CHECK(r);
    CHECK(r.value() == 2);

    CHECK(i == 42);
    CHECK(s == std::string{"foo"});
}

TEST_CASE("tuple_return int")
{
    auto stream = scn::make_stream("42");

    int i;
    scn::result<int> r(0);
    std::tie(r, i) = scn::scan<int>(stream, scn::default_tag);

    CHECK(r);
    CHECK(r.value() == 1);

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
    template <typename CharT>
    struct scanner<CharT, wrap_default<non_default_construct>>
        : public scanner<CharT, int> {
        template <typename Context>
        error scan(wrap_default<non_default_construct>& val, Context& ctx)
        {
            int tmp{};
            auto ret = scanner<CharT, int>::scan(tmp, ctx);
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
    auto stream = scn::make_stream("42");

    scn::wrap_default<non_default_construct> val;
    scn::result<int> ret(0);
    std::tie(ret, val) = scn::scan<scn::wrap_default<non_default_construct>>(
        stream, scn::default_tag);

    CHECK(ret);
    CHECK(ret.value() == 1);

    REQUIRE(val);
    CHECK(val->value == 42);
}
