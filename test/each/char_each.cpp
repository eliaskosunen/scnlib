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
#include <cwctype>

#include "../test.h"

TEST_CASE("char each")
{
    std::vector<char> vec;
    for (char ch = 0; ch != 127; ++ch) {
        auto uch = static_cast<unsigned char>(ch);
        if (std::isprint(uch) != 0 && std::isspace(uch) == 0) {
            vec.push_back(ch);
        }
    }

    for (auto ch : vec) {
        std::string source(1, ch);
        char tmp{};
        auto ret = scn::scan_default(source, tmp);

        CHECK(ret);
        CHECK(ch == tmp);
    }
}

TEST_CASE("wchar_t each")
{
    for (wchar_t ch = 0; ch != 65535; ++ch) {
        auto wint = static_cast<std::wint_t>(ch);
        if (std::iswprint(wint) == 0) {
            continue;
        }
        if (std::iswspace(wint) != 0) {
            continue;
        }

        std::wstring source(1, ch);
        wchar_t tmp{};
        auto ret = scn::scan_default(source, tmp);

        CHECK(ret);
        CHECK(ch == tmp);
    }
}
