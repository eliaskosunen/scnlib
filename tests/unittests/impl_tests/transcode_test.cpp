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

#include "../wrapped_gtest.h"

#include <scn/impl.h>

#include <fstream>
#include <sstream>

namespace {
std::string get_file_contents(const std::string& sourcefile)
{
    std::ifstream fstr{sourcefile};
    std::stringstream ss;
    ss << fstr.rdbuf();
    return ss.str();
}
}  // namespace

using namespace std::string_view_literals;

TEST(TranscodeTest, HelloWorld)
{
    auto in = "Hello world"sv;

    std::wstring widened{};
    scn::impl::transcode_to_string(in, widened);
    EXPECT_EQ(widened, L"Hello world");

    std::string narrowed{};
    scn::impl::transcode_to_string(
        std::wstring_view{widened.data(), widened.size()}, narrowed);
    EXPECT_EQ(narrowed, "Hello world");

    widened.clear();
    scn::impl::transcode_valid_to_string(in, widened);
    EXPECT_EQ(widened, L"Hello world");

    narrowed.clear();
    scn::impl::transcode_valid_to_string(
        std::wstring_view{widened.data(), widened.size()}, narrowed);
    EXPECT_EQ(narrowed, "Hello world");
}

TEST(TranscodeTest, Lipsum)
{
    auto in = get_file_contents("lipsum.txt");

    std::wstring widened{};
    scn::impl::transcode_to_string(std::string_view{in.data(), in.size()},
                                   widened);

    std::string narrowed{};
    scn::impl::transcode_to_string(
        std::wstring_view{widened.data(), widened.size()}, narrowed);

    EXPECT_EQ(narrowed, in);

    widened.clear();
    scn::impl::transcode_valid_to_string(std::string_view{in.data(), in.size()},
                                         widened);

    narrowed.clear();
    scn::impl::transcode_valid_to_string(
        std::wstring_view{widened.data(), widened.size()}, narrowed);

    EXPECT_EQ(narrowed, in);
}

TEST(TranscodeTest, Unicode)
{
    auto in = get_file_contents("unicode.txt");

    std::wstring widened{};
    scn::impl::transcode_to_string(std::string_view{in.data(), in.size()},
                                   widened);

    std::string narrowed{};
    scn::impl::transcode_to_string(
        std::wstring_view{widened.data(), widened.size()}, narrowed);

    EXPECT_EQ(narrowed, in);

    widened.clear();
    scn::impl::transcode_valid_to_string(std::string_view{in.data(), in.size()},
                                         widened);

    narrowed.clear();
    scn::impl::transcode_valid_to_string(
        std::wstring_view{widened.data(), widened.size()}, narrowed);

    EXPECT_EQ(narrowed, in);
}

TEST(TranscodeTest, Utf16SurrogatePair)
{
    // U+1F600 GRINNING FACE: D83D DE00 in UTF-16
    const auto utf8 = "a\xf0\x9f\x98\x80z"sv;
    const auto utf16 = u"a\U0001F600z"sv;
    const auto utf32 = U"a\U0001F600z"sv;

    std::u16string to16{};
    scn::impl::transcode_to_string(utf8, to16);
    EXPECT_EQ(to16, utf16);
    to16.clear();
    scn::impl::transcode_valid_to_string(utf8, to16);
    EXPECT_EQ(to16, utf16);
    to16.clear();
    scn::impl::transcode_to_string(utf32, to16);
    EXPECT_EQ(to16, utf16);

    std::u32string to32{};
    scn::impl::transcode_to_string(utf16, to32);
    EXPECT_EQ(to32, utf32);
    to32.clear();
    scn::impl::transcode_valid_to_string(utf16, to32);
    EXPECT_EQ(to32, utf32);

    std::string to8{};
    scn::impl::transcode_to_string(utf16, to8);
    EXPECT_EQ(to8, utf8);
}
