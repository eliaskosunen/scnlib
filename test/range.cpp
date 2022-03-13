// Copyright 2017 Elias Kosunen
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

template <typename T>
struct debug;

TEST_CASE("string lvalue")
{
    int a;
    std::string source{"123 456"};
    auto ret = scn::scan(source, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.reconstruct() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::non_reconstructed_scan_result<
                                   scn::detail::range_wrapper<scn::string_view>,
                                   std::string, scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}
TEST_CASE("string rvalue")
{
    int a;
    auto ret = scn::scan(std::string{"123 456"}, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.reconstruct() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::non_reconstructed_scan_result<
                                   scn::detail::range_wrapper<std::string>,
                                   std::string, scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}

TEST_CASE("string_view lvalue")
{
    int a;
    scn::string_view source{"123 456"};
    auto ret = scn::scan(source, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.range_as_string() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::reconstructed_scan_result<
                                   scn::detail::range_wrapper<scn::string_view>,
                                   scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}
TEST_CASE("string_view rvalue")
{
    int a;
    auto ret = scn::scan(scn::string_view{"123 456"}, "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.range_as_string() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::reconstructed_scan_result<
                                   scn::detail::range_wrapper<scn::string_view>,
                                   scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}

TEST_CASE("string literal")
{
    int a;
    auto ret = scn::scan("123 456", "{}", a);
    CHECK(ret);
    CHECK(a == 123);
    CHECK(ret.range_as_string() == " 456");
    static_assert(std::is_same<decltype(ret),
                               scn::detail::reconstructed_scan_result<
                                   scn::detail::range_wrapper<scn::string_view>,
                                   scn::wrapped_error>>::value,
                  "");

    ret = scn::scan(ret.range(), "{}", a);
    CHECK(ret);
    CHECK(a == 456);
    CHECK(ret.range().empty());
}

TEST_CASE("mapped_file")
{
    auto file = scn::mapped_file{};
    auto result = scn::make_result(file);
    static_assert(
        std::is_same<decltype(result),
                     scn::detail::non_reconstructed_scan_result<
                         scn::detail::range_wrapper<scn::string_view>,
                         scn::mapped_file, scn::wrapped_error>>::value,
        "");
}

TEST_CASE("get_buffer")
{
    std::string first{"123"}, second{"456"};
    std::vector<scn::span<const char>> buf{scn::make_span(first),
                                           scn::make_span(second)};
    auto spanbuf{scn::make_span(buf)};

    auto ret = scn::detail::get_buffer(spanbuf, spanbuf[0].begin());
    CHECK(ret.data() == first.data());
    CHECK(ret.size() == 3);

    ret = scn::detail::get_buffer(spanbuf, spanbuf[0].begin() + 1);
    CHECK(ret.data() == first.data() + 1);
    CHECK(ret.size() == 2);

    ret = scn::detail::get_buffer(spanbuf, spanbuf[0].end());
    CHECK(ret.data() == second.data());
    CHECK(ret.size() == 3);

    ret = scn::detail::get_buffer(spanbuf, spanbuf[1].begin());
    CHECK(ret.data() == second.data());
    CHECK(ret.size() == 3);

    ret = scn::detail::get_buffer(spanbuf, spanbuf[1].end());
    CHECK(ret.data() == nullptr);
    CHECK(ret.size() == 0);

    ret = scn::detail::get_buffer(spanbuf, spanbuf[0].begin(), 2);
    CHECK(ret.data() == first.data());
    CHECK(ret.size() == 2);
}

#if 0
// ranges must be default-constructible

struct non_default_constructible_range {
    using iterator = std::string::iterator;
    using const_iterator = std::string::const_iterator;
    using pointer = std::string::pointer;
    using const_pointer = std::string::const_pointer;
    using size_type = std::string::size_type;

    non_default_constructible_range() = delete;
    non_default_constructible_range(const char* s) : str(s) {}

    iterator begin() {
        return str.begin();
    }
    const_iterator begin() const {
        return str.begin();
    }

    iterator end() {
        return str.end();
    }
    const_iterator end() const {
        return str.end();
    }

    pointer data() {
        return &str[0];
    }
    const_pointer data() const {
        return str.data();
    }

    size_type size() const {
        return str.size();
    }

    std::string str;
};

TEST_CASE("non default constructible range: lvalue")
{
    int i;
    auto ret = scn::scan(non_default_constructible_range{"123 456"}, "{}", i);
    CHECK(ret);
    CHECK(i == 123);
    CHECK(ret.string() == "456");

    ret = scn::scan(ret.range(), "{}", i);
    CHECK(ret);
    CHECK(i == 456);
    CHECK(ret.range().empty());
}
TEST_CASE("non default constructible range: rvalue")
{
    int i;
    auto source = non_default_constructible_range{"123 456"};
    auto ret = scn::scan(source, "{}", i);
    CHECK(ret);
    CHECK(i == 123);
    CHECK(ret.string() == "456");

    ret = scn::scan(ret.range(), "{}", i);
    CHECK(ret);
    CHECK(i == 456);
    CHECK(ret.range().empty());
}
#endif
