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

struct user_type {
    int val1{}, val2{};
};
struct user_type2 {
    int val1{}, val2{};
};
struct user_type3 {
    int val1{}, val2{};
};

namespace scn {
    template <>
    struct scanner<user_type> : public scn::empty_parser {
        template <typename Context>
        error scan(user_type& val, Context& ctx)
        {
            return scan_usertype(ctx, "[{}, {}]", val.val1, val.val2);
        }
    };
    template <>
    struct scanner<user_type2> : public scn::empty_parser {
        template <typename Context>
        error scan(user_type2& val, Context& ctx)
        {
            using pctx_type =
                basic_parse_context<typename Context::locale_type>;
            auto args = make_args<Context, pctx_type>(val.val1, val.val2);
            return vscan_usertype(ctx, "[{}, {}]", {args});
        }
    };
    template <>
    struct scanner<user_type3> : public scn::empty_parser {
        template <typename Context>
        error scan(user_type3& val, Context& ctx)
        {
            int i, j;
            auto format = string_view{"[{}, {}]"};
            auto newctx = make_context(ctx.range());
            auto pctx = make_parse_context(format, newctx.locale());
            auto args = make_args_for(newctx.range(), format, i, j);
            auto err = visit(newctx, pctx, {args});
            ctx.range() = std::move(newctx.range());
            if (err) {
                val.val1 = i;
                val.val2 = j;
                return {};
            }
            return err;
        }
    };
}  // namespace scn

TEST_CASE_TEMPLATE_DEFINE("user type", T, user_type_test)
{
    T ut{};

    SUBCASE("regular")
    {
        auto ret = scn::scan("[4, 20]", "{}", ut);
        CHECK(ret);
        CHECK(ut.val1 == 4);
        CHECK(ut.val2 == 20);
    }
    SUBCASE("format string error")
    {
        auto ret = scn::scan("[4, 20]", "{", ut);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::invalid_format_string);

        ret = scn::scan(ret.range(), "{:a}", ut);
        CHECK(!ret);
        CHECK(ret.error() == scn::error::invalid_format_string);
    }
    SUBCASE("mixed")
    {
        int i, j;
        auto ret = scn::scan("123 [4, 20] 456", "{} {} {}", i, ut, j);
        CHECK(ret);
        CHECK(i == 123);
        CHECK(ut.val1 == 4);
        CHECK(ut.val2 == 20);
        CHECK(j == 456);
        CHECK(ret.empty());
    }
}
TYPE_TO_STRING(user_type);
TYPE_TO_STRING(user_type2);
TYPE_TO_STRING(user_type3);
TEST_CASE_TEMPLATE_INSTANTIATE(user_type_test,
                               user_type,
                               user_type2,
                               user_type3);

struct non_default_construct {
    non_default_construct() = delete;

    explicit non_default_construct(int val) : value(val) {}

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

TEST_CASE("non_default_construct")
{
    scn::optional<non_default_construct> val;
    auto ret = scn::scan("42", "{}", val);

    CHECK(ret);

    REQUIRE(val);
    CHECK(val->value == 42);
}
