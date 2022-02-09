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

TEST_CASE("buffer")
{
    std::string data{"data moredata"};
    std::string s(6, '\0');
    auto span = scn::make_span(s);

    auto ret = scn::scan(data, "{}", span);
    CHECK(ret);
    CHECK(span.size() == 4);
    CHECK(data.substr(0, 4) == s.substr(0, 4));
}

TEST_CASE("non-contiguous")
{
    auto source = get_deque<char>("123");
    std::string dest(3, '\0');
    auto span = scn::make_span(dest);

    auto ret = scn::scan(source, "{}", span);
    CHECK(ret);
    CHECK(dest == "123");
}
