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
