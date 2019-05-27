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

#include "test.h"

TEST_CASE("read_into_until_space optimized")
{
    SUBCASE("regular")
    {
        auto stream = scn::make_stream("word another");
        auto locale = scn::basic_default_locale_ref<char>{};
        std::string buf;

        auto ret =
            scn::read_into_until_space(stream, locale, std::back_inserter(buf));

        CHECK(ret);
        CHECK(ret.value() == strlen("word"));
        CHECK(buf == "word");

        stream.read_char(); // skip space

        buf.clear();
        ret =
            scn::read_into_until_space(stream, locale, std::back_inserter(buf));

        CHECK(ret);
        CHECK(ret.value() == strlen("another"));
        CHECK(buf == "another");
    }
    SUBCASE("keep_final")
    {
        auto stream = scn::make_stream("word another");
        auto locale = scn::basic_default_locale_ref<char>{};
        std::string buf;

        auto ret = scn::read_into_until_space(stream, locale,
                                              std::back_inserter(buf), true);

        CHECK(ret);
        CHECK(ret.value() == strlen("word "));
        CHECK(buf == "word ");

        buf.clear();
        ret = scn::read_into_until_space(stream, locale,
                                         std::back_inserter(buf), true);

        CHECK(ret);
        CHECK(ret.value() == strlen("another"));
        CHECK(buf == "another");
    }
}
