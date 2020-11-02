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
#include <scn/istream.h>
#include <istream>
#include "../test.h"

TEST_CASE("file")
{
    scn::owning_file file{"./test/file/testfile.txt", "r"};
    REQUIRE(file.is_open());

    SUBCASE("basic")
    {
        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);
    }
    SUBCASE("entire file")
    {
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

    SUBCASE("getline")
    {
        std::string line;
        auto result = scn::getline(file, line);
        CHECK(result);
        CHECK(line == "123");

        result = scn::getline(result.range(), line);
        CHECK(result);
        CHECK(line == "word another");

        result = scn::getline(result.range(), line);
        CHECK(!result);
        CHECK(result.error().code() == scn::error::end_of_range);
        CHECK(line == "word another");
    }
}

TEST_CASE("mapped file")
{
    scn::mapped_file file{"./test/file/testfile.txt"};
    REQUIRE(file.valid());

    SUBCASE("basic")
    {
        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);
    }
    SUBCASE("entire file")
    {
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
}

struct int_and_string {
    int i;
    std::string s;
};
struct two_strings {
    std::string first, second;
};
struct istream_int_and_string {
    int i;
    std::string s;

    friend std::istream& operator>>(std::istream& is,
                                    istream_int_and_string& val)
    {
        is >> val.i >> val.s;
        return is;
    }
};

namespace scn {
    template <typename CharT>
    struct scanner<CharT, int_and_string> : public scn::empty_parser {
        template <typename Context>
        error scan(int_and_string& val, Context& ctx)
        {
            auto r = scn::scan(ctx.range(), "{} {}", val.i, val.s);
            ctx.range() = std::move(r.range());
            return r.error();
        }
    };

    template <typename CharT>
    struct scanner<CharT, two_strings> : public scn::empty_parser {
        template <typename Context>
        error scan(two_strings& val, Context& ctx)
        {
            auto r = scn::scan(ctx.range(), "{} {}", val.first, val.second);
            ctx.range() = std::move(r.range());
            return r.error();
        }
    };
}  // namespace scn

TEST_CASE("file usertype")
{
    scn::owning_file file{"./test/file/testfile.txt", "r"};
    REQUIRE(file.is_open());

    SUBCASE("int_and_string")
    {
        int_and_string val{};
        auto result = scn::scan_default(file, val);
        CHECK(result);
        CHECK(val.i == 123);
        CHECK(val.s == "word");

        std::string s;
        result = scn::scan_default(result.range(), s);
        CHECK(result);
        CHECK(s == "another");
    }

    SUBCASE("int_and_string failure")
    {
        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);

        int_and_string val{};
        result = scn::scan_default(result.range(), val);
        CHECK(!result);
        CHECK(result.error().code() == scn::error::invalid_scanned_value);

        std::string s;
        result = scn::scan_default(result.range(), s);
        CHECK(result);
        CHECK(s == "word");
    }

    SUBCASE("two_strings")
    {
        two_strings val{};
        auto result = scn::scan_default(file, val);
        CHECK(result);
        CHECK(val.first == "123");
        CHECK(val.second == "word");

        std::string s;
        result = scn::scan_default(result.range(), s);
        CHECK(result);
        CHECK(s == "another");
    }

    SUBCASE("istream")
    {
        istream_int_and_string val{};
        auto result = scn::scan_default(file, val);
        CHECK(result);
        CHECK(val.i == 123);
        CHECK(val.s == "word");

        std::string s;
        result = scn::scan_default(result.range(), s);
        CHECK(result);
        CHECK(s == "another");
    }
    SUBCASE("istream failure")
    {
        int i;
        auto result = scn::scan_default(file, i);
        CHECK(result);
        CHECK(i == 123);

        istream_int_and_string val{};
        result = scn::scan_default(result.range(), val);
        CHECK(!result);
        CHECK(result.error().code() == scn::error::invalid_scanned_value);

        std::string s;
        result = scn::scan_default(result.range(), s);
        CHECK(result);
        CHECK(s == "word");
    }
}
