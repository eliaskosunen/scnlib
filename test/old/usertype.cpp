// Copyright 2017-2019 Elias Kosunen
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

struct user_type {
    int val1{}, val2{};
};
struct user_type2 {
    int val1{}, val2{};
};

namespace scn {
    template <typename CharT>
    struct scanner<CharT, user_type> : public scn::empty_parser<CharT> {
        template <typename Context>
        error scan(user_type& val, Context& ctx)
        {
            auto r = scn::scan(ctx.stream(), "[{}, {}]", val.val1, val.val2);
            if (r) {
                return {};
            }
            return r.error();
        }
    };
    template <typename CharT>
    struct scanner<CharT, user_type2> : public scn::empty_parser<CharT> {
        template <typename Context>
        error scan(user_type2& val, Context& ctx)
        {
            auto args = make_args<Context>(val.val1, val.val2);
            auto newctx = Context(ctx.stream(), "[{}, {}]", args);
            auto r = vscan(newctx);
            if (r) {
                return {};
            }
            return r.error();
        }
    };
}  // namespace scn

TEST_CASE_TEMPLATE_DEFINE("user type", T, user_type_test)
{
    std::string source{"[4, 20]"};
    auto stream = scn::make_stream(source);
    user_type ut{};

    SUBCASE("regular")
    {
        auto ret = scn::scan(stream, "{}", ut);
        CHECK(ret);
        CHECK(ret.value() == 1);
        CHECK(ut.val1 == 4);
        CHECK(ut.val2 == 20);
    }
    SUBCASE("format string error")
    {
        auto ret = scn::scan(stream, "{", ut);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_format_string);

        ret = scn::scan(stream, "{:a}", ut);
        CHECK(!ret);
        CHECK(ret.value() == 0);
        CHECK(ret.error() == scn::error::invalid_format_string);
    }
}
TEST_CASE_TEMPLATE_INSTANTIATE(user_type_test, user_type, user_type2);

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

TEST_CASE("non_default_construct")
{
    auto stream = scn::make_stream("42");

    scn::wrap_default<non_default_construct> val;
    auto ret = scn::scan(stream, "{}", val);

    CHECK(ret);
    CHECK(ret.value() == 1);

    REQUIRE(val);
    CHECK(val->value == 42);
}
