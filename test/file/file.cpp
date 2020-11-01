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
#include "../test.h"

template <typename T>
struct debug;

TEST_CASE("file")
{
    SUBCASE("basic")
    {
        scn::owning_file file{"./test/file/testfile.txt", "r"};
        REQUIRE(file.is_open());

        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);
    }
    SUBCASE("entire file")
    {
        scn::owning_file file{"./test/file/testfile.txt", "r"};
        REQUIRE(file.is_open());

        auto result = scn::make_result(file);

        int i;
        result = scn::scan_default(result.range(), i);
        CHECK(result);
        CHECK(i == 123);

        std::string word;
        result = scn::scan_default(result.range(), word);
        CHECK(result);
        CHECK(word == "word");

        result = scn::scan_default(result.range(), word);
        CHECK(result);
        CHECK(word == "another");

        result = scn::scan_default(result.range(), word);
        CHECK(!result);
        CHECK(result.error().code() == scn::error::end_of_range);
        CHECK(word == "another");
    }

    SUBCASE("syncing")
    {
        scn::owning_file file{"./test/file/testfile.txt", "r"};
        REQUIRE(file.is_open());

        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);
        file.sync();

        std::string word;
        result = scn::scan_default(file, word);
        CHECK(result);
        CHECK(word == "word");
        file.sync();

        std::string cmp{"another"};
        word.clear();
        word.resize(cmp.size());
        auto n = std::fread(&word[0], 1, word.size(), file.handle());
        CHECK(n == cmp.size());
        CHECK(word == cmp);
    }

    SUBCASE("not syncing")
    {
        scn::owning_file file{"./test/file/testfile.txt", "r"};
        REQUIRE(file.is_open());

        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);

        // syncing required to use original `file`
        std::string word;
        result = scn::scan_default(file, word);
        CHECK(result);
        CHECK(word == "123");

        // syncing required to use the file handle
        std::string cmp{"word"};
        word.clear();
        word.resize(cmp.size());
        auto n = std::fread(&word[0], 1, word.size(), file.handle());
        CHECK(n == cmp.size());
        CHECK(word == cmp);
    }

    SUBCASE("error")
    {
        scn::owning_file file{"./test/file/testfile.txt", "r"};
        REQUIRE(file.is_open());

        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);

        result = scn::scan_default(result.range(), i);
        CHECK(!result);
        CHECK(result.error().code() == scn::error::invalid_scanned_value);
        CHECK(i == 123);

        // can still read again
        std::string word;
        result = scn::scan_default(result.range(), word);
        CHECK(result);
        CHECK(word == "word");
    }
}
