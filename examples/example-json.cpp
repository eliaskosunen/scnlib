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

#include <scn/scn.h>

#include <iostream>
#include <vector>

struct string_list {
    std::vector<int> list;
};

namespace scn {
    SCN_BEGIN_NAMESPACE
    template <>
    struct scanner<string_list> : public empty_parser {
        template <typename Context>
        error scan(string_list& val, Context& ctx)
        {
            auto result = scn::scan_list_ex(
                ctx.range(), val.list, scn::list_separator_and_until(',', ']'));
            if (!result) {
                return result.error();
            }
            ctx.range() = std::move(result.range());
            return {};
        }
    };
    SCN_END_NAMESPACE
}  // namespace scn

int main()
{
    string_list val;
    std::string source = R"([1, 2, 3])";
    auto result = scn::scan(source, "[{}", val);

    if (!result) {
        std::cout << result.error().msg() << '\n';
    }
    else {
        for (const auto& e : val.list) {
            std::cout << e << '\n';
        }
    }
}
