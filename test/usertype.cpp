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

#include <doctest.h>
#include <scn/scn.h>

struct user_type {
    int val1{}, val2{};
};

namespace scn {
    template <typename CharT>
    struct basic_value_scanner<CharT, user_type>
        : public detail::empty_parser<CharT> {
        template <typename Context>
        error scan(user_type& val, Context& ctx)
        {
            return scn::scan(ctx.stream(), "[{}, {}]", val.val1, val.val2);
        }
    };
}  // namespace scn

TEST_CASE("user type")
{
    std::string source{"[4, 20]"};
    auto stream = scn::make_stream(source);

    user_type ut{};
    auto ret = scn::scan(stream, "{}", ut);
    CHECK(ret);
    CHECK(ut.val1 == 4);
    CHECK(ut.val2 == 20);
}
