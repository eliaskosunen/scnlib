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

#include "test_common.h"

#include <scn/scan.h>
#include <scn/xchar.h>

TEST(CharTest, CharFromNarrow)
{
    auto result = scn::scan<char>("abc", "{}");
    ASSERT_THAT(result, Succeeded());
    auto [ch] = result->values();
    EXPECT_EQ(ch, 'a');
}
TEST(CharTest, CharFromWide)
{
    static_assert(!scn::detail::is_scannable<char, wchar_t>::value);
}

TEST(CharTest, WcharFromNarrow)
{
    auto result = scn::scan<wchar_t>("abc", "{}");
    ASSERT_THAT(result, Succeeded());
    auto [ch] = result->values();
    EXPECT_EQ(ch, L'a');
}
TEST(CharTest, WcharFromWide)
{
    auto result = scn::scan<wchar_t>(L"abc", L"{}");
    ASSERT_THAT(result, Succeeded());
    auto [ch] = result->values();
    EXPECT_EQ(ch, L'a');
}

TEST(CharTest, MultipleChars)
{
    auto result = scn::scan<char, char, char>("abc", "{}{}{}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b, c] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
    EXPECT_EQ(c, 'c');
}

TEST(CharTest, CharWithWhitespace)
{
    auto result = scn::scan<char, char>("a b", "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b] = result->values();
    EXPECT_EQ(a, 'a');
    EXPECT_EQ(b, 'b');
}

TEST(CharTest, SpecialChars)
{
    auto result = scn::scan<char>("\n", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), '\n');

    result = scn::scan<char>("\t", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), '\t');
}

TEST(CharTest, UnicodeChar)
{
    auto result = scn::scan<char32_t>("世", "{}");
    ASSERT_THAT(result, Succeeded());
    // 0x4e16 == U'世', but works on Windows, too
    EXPECT_EQ(result->value(), 0x4e16);
}

TEST(CharTest, EmptyInputChar)
{
    auto result = scn::scan<char>("", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

TEST(CharTest, CharAfterInt)
{
    auto result = scn::scan<int, char>("42a", "{}{}");
    ASSERT_THAT(result, Succeeded());
    auto [num, ch] = result->values();
    EXPECT_EQ(num, 42);
    EXPECT_EQ(ch, 'a');
}

TEST(CharTest, CodePointFromNarrowEmptyInput)
{
    // Whitespace is only skipped as fill, so `{:c}` and `{:.2}` would read it
    constexpr std::array<std::pair<std::string_view, const char*>, 8>
        code_point_cases{{{"", "{:c}"},
                          {"", "{:.2}"},
                          {"", "{:>4}"},
                          {"", "{:>4c}"},
                          {" ", "{:>4}"},
                          {" ", "{:>4c}"},
                          {"    ", "{:>4}"},
                          {"    ", "{:>4c}"}}};
    for (auto [input, fmt] : code_point_cases) {
        EXPECT_THAT(scn::scan<char32_t>(input, scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::end_of_input))
            << '"' << input << "\" " << fmt;
    }
    // `{:c}` isn't currently accepted for wchar_t
    for (auto [input, fmt] :
         {std::pair<std::string_view, const char*>{"", "{:.2}"},
          {"", "{:>4}"},
          {" ", "{:>4}"},
          {"    ", "{:>4}"}}) {
        EXPECT_THAT(scn::scan<wchar_t>(input, scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::end_of_input))
            << '"' << input << "\" " << fmt;
    }
}

TEST(CharTest, CodePointFromNarrowInvalidUtf8)
{
    constexpr std::array<std::string_view, 5> inputs{
        "\xc3\x28",          // bad continuation
        "\xe4\xb8",          // truncated at the end of input
        "\xc0\x80",          // overlong NUL
        "\xed\xa0\x80",      // surrogate U+D800
        "\xf4\x90\x80\x80",  // U+110000
    };
    for (std::string_view input : inputs) {
        EXPECT_THAT(scn::scan<char32_t>(input, "{}"),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
}

TEST(CharTest, WcharFromNarrowInvalidUtf8)
{
    constexpr std::array<std::string_view, 5> inputs{
        "\xc3\x28",          // bad continuation
        "\xe4\xb8",          // truncated at the end of input
        "\xc0\x80",          // overlong NUL
        "\xed\xa0\x80",      // surrogate U+D800
        "\xf4\x90\x80\x80",  // U+110000
    };
    for (std::string_view input : inputs) {
        EXPECT_THAT(scn::scan<wchar_t>(input, "{}"),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
}

TEST(CharTest, CodePointFromNarrowMultiByte)
{
    const std::pair<std::string_view, char32_t> cases[] = {
        {"\xc3\xa4", 0xe4},
        {"\xe4\xb8\x96", 0x4e16},
        {"\xf0\x9f\x98\x80", 0x1f600}};
    constexpr std::array<const char*, 2> formats{"{}", "{:c}"};
    for (const auto& [input, expected] : cases) {
        for (auto fmt : formats) {
            auto result = scn::scan<char32_t>(input, scn::runtime_format(fmt));
            ASSERT_THAT(result, Succeeded()) << fmt;
            EXPECT_EQ(result->value(), expected) << fmt;
            EXPECT_EQ(result->begin(), input.end()) << fmt;
        }
    }
}

TEST(CharTest, WcharFromNarrowMultiByte)
{
    auto result = scn::scan<wchar_t>("\xe4\xb8\x96", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), static_cast<wchar_t>(0x4e16));
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(CharTest, CodePointFromWideInvalid)
{
    // Converted from a variable, because truncating a constant to a 16-bit
    // wchar_t is a warning on MSVC
    constexpr std::array<char32_t, 3> code_points{0xd800, 0xdc00, 0x110000};
    for (char32_t cp : code_points) {
        if (sizeof(wchar_t) == 2 && cp > 0xffff) {
            continue;
        }
        const auto unit = static_cast<wchar_t>(cp);
        EXPECT_THAT(scn::scan<char32_t>(std::wstring{unit}, L"{}"),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << static_cast<uint32_t>(cp);
        EXPECT_THAT(scn::scan<char32_t>(std::wstring{unit, L'a'}, L"{}"),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << static_cast<uint32_t>(cp);
    }
}

TEST(CharTest, CodePointFromWideEmptyInput)
{
    EXPECT_THAT(
        scn::scan<char32_t>(std::wstring_view{}, scn::runtime_format(L"{:c}")),
        FailedWith(scn::scan_error::end_of_input));
}
