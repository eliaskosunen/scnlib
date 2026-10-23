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

#include <array>
#include <deque>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace {
template <typename CharT>
void expect_integer_presentation_matches_signedness()
{
    // `limits` gives the range of the integer type with the same signedness
    // and width as the character type
    using limits = std::numeric_limits<CharT>;
    const auto max = static_cast<long long>(limits::max());
    const auto min = static_cast<long long>(limits::min());

    auto max_result = scn::scan<CharT>(std::to_string(max), "{:d}");
    ASSERT_THAT(max_result, Succeeded());
    EXPECT_EQ(max_result->value(), limits::max());

    auto min_result = scn::scan<CharT>(std::to_string(min), "{:d}");
    ASSERT_THAT(min_result, Succeeded());
    EXPECT_EQ(min_result->value(), limits::min());

    EXPECT_THAT(scn::scan<CharT>(std::to_string(max + 1), "{:d}"),
                FailedWith(scn::scan_error::value_positive_overflow));
    EXPECT_THAT(scn::scan<CharT>(std::to_string(min - 1), "{:d}"), Failed());

    auto minus_one = scn::scan<CharT>("-1", "{:d}");
    if constexpr (std::is_signed_v<CharT>) {
        ASSERT_THAT(minus_one, Succeeded());
        EXPECT_EQ(minus_one->value(), static_cast<CharT>(-1));
    }
    else {
        EXPECT_THAT(minus_one, Failed());
    }
}
}  // namespace

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

TEST(CharTest, WcharFromNarrowRejected)
{
    for (auto fmt : {"{}", "{:}", "{:c}"}) {
        EXPECT_THAT(scn::scan<wchar_t>("abc", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
        EXPECT_THAT(scn::scan<wchar_t>(std::deque<char>{'a', 'b'},
                                       scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
    // Rejected before the end of input is checked
    for (auto fmt : {"{}", "{:}", "{:c}"}) {
        EXPECT_THAT(scn::scan<wchar_t>("", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
        EXPECT_THAT(
            scn::scan<wchar_t>(std::deque<char>{}, scn::runtime_format(fmt)),
            FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
    EXPECT_THAT((scn::scan<int, wchar_t>("1", scn::runtime_format("{}{}"))),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT((scn::scan<int, wchar_t>("1", scn::runtime_format("{} {}"))),
                FailedWith(scn::scan_error::invalid_format_string));
}
TEST(CharTest, WcharFromNarrowRejectedInVscanValue)
{
    wchar_t ch{};
    auto result = scn::vscan_value(
        std::string_view{"abc"}, scn::detail::make_arg<scn::scan_context>(ch));
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}
TEST(CharTest, WcharFromNarrowWithIntegerPresentation)
{
    auto result = scn::scan<wchar_t>("42", "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), static_cast<wchar_t>(42));
    EXPECT_THAT(result->range(), IsEmptyRange());
}

namespace {
struct wchar_wrapper {
    wchar_t value{};
};
}  // namespace

template <>
struct scn::scanner<wchar_wrapper, char> : scn::scanner<wchar_t, char> {
    template <typename Context>
    auto scan(wchar_wrapper& val, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        return scn::scanner<wchar_t, char>::scan(val.value, ctx);
    }
};

TEST(CharTest, WcharScannerSubclassFromNarrowWithIntegerPresentation)
{
    auto result = scn::scan<wchar_wrapper>("42", scn::runtime_format("{:d}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().value, static_cast<wchar_t>(42));

    for (auto fmt : {"{}", "{:c}"}) {
        EXPECT_THAT(scn::scan<wchar_wrapper>("a", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
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
    EXPECT_THAT(scn::scan<char32_t>("", scn::runtime_format("{:U}")),
                FailedWith(scn::scan_error::end_of_input));
}

TEST(CharTest, CharacterPresentationNarrow)
{
    auto result = scn::scan<char>("a", "{:c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 'a');
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(CharTest, CharacterPresentationWide)
{
    auto result = scn::scan<wchar_t>(L"a", L"{:c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L'a');
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(CharTest, UnicodePresentationCodePointFromNarrow)
{
    auto result = scn::scan<char32_t>("\xc3\xa4", "{:U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), char32_t{0xe4});
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(CharTest, UnicodePresentationCodePointFromWide)
{
    auto result = scn::scan<char32_t>(L"a", L"{:U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), char32_t{'a'});
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(CharTest, CodePointRejectsCharacterPresentation)
{
    EXPECT_THAT(scn::scan<char32_t>("a", scn::runtime_format("{:c}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(scn::scan<char32_t>(L"a", scn::runtime_format(L"{:c}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(CharTest, UnicodePresentationRejectedForOtherTypes)
{
    EXPECT_THAT(scn::scan<char>("a", scn::runtime_format("{:U}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(scn::scan<wchar_t>(L"a", scn::runtime_format(L"{:U}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(scn::scan<int>("1", scn::runtime_format("{:U}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(scn::scan<bool>("1", scn::runtime_format("{:U}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(scn::scan<double>("1", scn::runtime_format("{:U}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(CharTest, CodePointRejectsIntegerPresentation)
{
    constexpr std::array<const char*, 4> formats{"{:d}", "{:x}", "{:b}",
                                                 "{:s}"};
    for (auto fmt : formats) {
        EXPECT_THAT(scn::scan<char32_t>("a", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}

TEST(CharTest, CharRejectsArbitraryBasePresentation)
{
    EXPECT_THAT(scn::scan<char>("1", scn::runtime_format("{:r16}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(CharTest, CharIntegerPresentationUsesSignednessOfChar)
{
    expect_integer_presentation_matches_signedness<char>();

    auto result = scn::scan<char>("200", "{:d}");
    EXPECT_EQ(static_cast<bool>(result), !std::is_signed_v<char>);
}

TEST(CharTest, WcharIntegerPresentationUsesSignednessOfWchar)
{
    expect_integer_presentation_matches_signedness<wchar_t>();

    // Doesn't fit in a signed 16-bit integer
    auto result = scn::scan<wchar_t>("40000", "{:d}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), static_cast<wchar_t>(40000));
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

TEST(CharTest, CodePointFromNarrowMultiByte)
{
    const std::pair<std::string_view, char32_t> cases[] = {
        {"\xc3\xa4", 0xe4},
        {"\xe4\xb8\x96", 0x4e16},
        {"\xf0\x9f\x98\x80", 0x1f600}};
    constexpr std::array<const char*, 2> formats{"{}", "{:U}"};
    for (const auto& [input, expected] : cases) {
        for (auto fmt : formats) {
            auto result = scn::scan<char32_t>(input, scn::runtime_format(fmt));
            ASSERT_THAT(result, Succeeded()) << fmt;
            EXPECT_EQ(result->value(), expected) << fmt;
            EXPECT_EQ(result->begin(), input.end()) << fmt;
        }
    }
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
            << static_cast<std::uint32_t>(cp);
        EXPECT_THAT(scn::scan<char32_t>(std::wstring{unit, L'a'}, L"{}"),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << static_cast<std::uint32_t>(cp);
    }
}

TEST(CharTest, CodePointFromWideEmptyInput)
{
    EXPECT_THAT(
        scn::scan<char32_t>(std::wstring_view{}, scn::runtime_format(L"{:U}")),
        FailedWith(scn::scan_error::end_of_input));
}

TEST(CharTest, WidthAndPrecisionRejectedWithoutIntegerPresentation)
{
    for (auto fmt :
         {"{:3}", "{:.3}", "{:3c}", "{:.3c}", "{:2.3}", "{:3?}", "{:.3?}"}) {
        EXPECT_THAT(scn::scan<char>("abcd", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
        EXPECT_THAT(scn::scan<wchar_t>("abcd", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
    for (auto fmt : {L"{:3}", L"{:.3}", L"{:3c}", L"{:.3c}", L"{:2.3}",
                     L"{:3?}", L"{:.3?}"}) {
        EXPECT_THAT(scn::scan<wchar_t>(L"abcd", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string));
    }
}

TEST(CharTest, WidthAndPrecisionRejectedForCodePoint)
{
    for (auto fmt : {"{:3}", "{:.3}", "{:3U}", "{:.3U}", "{:3?}", "{:.3?}"}) {
        EXPECT_THAT(scn::scan<char32_t>("abcd", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
    EXPECT_THAT(scn::scan<char32_t>(L"abcd", scn::runtime_format(L"{:3}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(CharTest, WidthWithIntegerPresentationAllowed)
{
    auto result = scn::scan<char>("42", scn::runtime_format("{:2d}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 42);
}

TEST(CharTest, IntegerPresentationSkipsLeadingWhitespace)
{
    auto narrow = scn::scan<char>(" 42", "{:d}");
    ASSERT_THAT(narrow, Succeeded());
    EXPECT_EQ(narrow->value(), 42);

    auto wide = scn::scan<wchar_t>(L" 42", L"{:d}");
    ASSERT_THAT(wide, Succeeded());
    EXPECT_EQ(wide->value(), 42);

    auto hex = scn::scan<char>(" \t2a", "{:x}");
    ASSERT_THAT(hex, Succeeded());
    EXPECT_EQ(hex->value(), 0x2a);

    const auto source = std::deque<char>{' ', '2', 'a', 'z'};
    auto non_contiguous = scn::scan<char>(source, "{:x}");
    ASSERT_THAT(non_contiguous, Succeeded());
    EXPECT_EQ(non_contiguous->value(), 0x2a);
    EXPECT_EQ(*non_contiguous->begin(), 'z');
}

TEST(CharTest, IntegerPresentationOnWhitespaceOnlyIsEndOfInput)
{
    EXPECT_THAT(scn::scan<char>("  ", "{:d}"),
                FailedWith(scn::scan_error::end_of_input));
    EXPECT_THAT(scn::scan<char>(std::deque<char>{' ', '\n'}, "{:x}"),
                FailedWith(scn::scan_error::end_of_input));
    EXPECT_THAT(scn::scan<wchar_t>(L" ", L"{:d}"),
                FailedWith(scn::scan_error::end_of_input));
}

TEST(CharTest, CharacterPresentationKeepsLeadingWhitespace)
{
    auto result = scn::scan<char>(" a", "{:c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), ' ');
    EXPECT_STREQ(result->begin(), "a");
}

TEST(CharTest, EscapedPresentationChar)
{
    auto plain = scn::scan<char>("'a' rest", "{:?}");
    ASSERT_THAT(plain, Succeeded());
    EXPECT_EQ(plain->value(), 'a');
    EXPECT_STREQ(plain->begin(), " rest");

    constexpr std::array<std::pair<std::string_view, char>, 14> cases{{
        {R"('\t')", '\t'},
        {R"('\n')", '\n'},
        {R"('\r')", '\r'},
        {R"('\"')", '"'},
        {R"('\'')", '\''},
        {R"('\\')", '\\'},
        {R"('"')", '"'},
        {R"('\u{41}')", 'A'},
        {R"('\u{7f}')", '\x7f'},
        {R"('\x{41}')", 'A'},
        {R"('\x{ff}')", '\xff'},
        {R"('\x{c3}')", '\xc3'},
        {"'\t'", '\t'},
        {"'\r'", '\r'},
    }};
    for (const auto& [input, expected] : cases) {
        auto result = scn::scan<char>(input, "{:?}");
        ASSERT_THAT(result, Succeeded()) << input;
        EXPECT_EQ(result->value(), expected) << input;
        EXPECT_THAT(result->range(), IsEmptyRange()) << input;

        auto source = std::deque<char>(input.begin(), input.end());
        auto noncontiguous = scn::scan<char>(source, "{:?}");
        ASSERT_THAT(noncontiguous, Succeeded()) << input;
        EXPECT_EQ(noncontiguous->value(), expected) << input;
    }
}

TEST(CharTest, EscapedPresentationSkipsLeadingWhitespace)
{
    auto result = scn::scan<char>(" \n 'a'", "{:?}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), 'a');
}

TEST(CharTest, EscapedPresentationCharEmptyInput)
{
    EXPECT_THAT(scn::scan<char>("", scn::runtime_format("{:?}")),
                FailedWith(scn::scan_error::end_of_input));
    EXPECT_THAT(scn::scan<char>("  ", scn::runtime_format("{:?}")),
                FailedWith(scn::scan_error::end_of_input));
}

TEST(CharTest, EscapedPresentationCharErrors)
{
    for (std::string_view input :
         {"a", "\"a\"", "'", "'a", "'a\\", "''", "'ab'", "'\\'", "'\\a'",
          "'\\b'", "'\\f'", "'\\v'", "'\\?'", "'\\e'", "'\\0'", "'\\101'",
          "'\\o{101}'", "'\\x41'", "'\\u0041'", "'\\U00000041'",
          "'\\N{LATIN CAPITAL LETTER A}'", "'\\u{}'", "'\\u{110000}'",
          "'\\u{d800}'", "'\\x{100}'", "'\\x{}'", "'\n'", "'\xff'", "'\xc3'",
          // Does not fit one code unit
          "'\\u{e4}'", "'\xc3\xa4'", "'\\u{1f600}'"}) {
        EXPECT_THAT(scn::scan<char>(input, scn::runtime_format("{:?}")),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << input;
    }
}

TEST(CharTest, EscapedPresentationWideChar)
{
    auto plain = scn::scan<wchar_t>(L"'a' rest", L"{:?}");
    ASSERT_THAT(plain, Succeeded());
    EXPECT_EQ(plain->value(), L'a');
    EXPECT_STREQ(plain->begin(), L" rest");

    auto escaped = scn::scan<wchar_t>(L"'\\u{e4}'", L"{:?}");
    ASSERT_THAT(escaped, Succeeded());
    EXPECT_EQ(escaped->value(), L'\u00e4');

    auto raw = scn::scan<wchar_t>(L"'\u4e16'", L"{:?}");
    ASSERT_THAT(raw, Succeeded());
    EXPECT_EQ(raw->value(), L'\u4e16');

    auto hex = scn::scan<wchar_t>(L"'\\x{ffff}'", L"{:?}");
    ASSERT_THAT(hex, Succeeded());
    EXPECT_EQ(hex->value(), static_cast<wchar_t>(0xffff));

    auto newline = scn::scan<wchar_t>(L"'\\n'", L"{:?}");
    ASSERT_THAT(newline, Succeeded());
    EXPECT_EQ(newline->value(), L'\n');
}

TEST(CharTest, EscapedPresentationWideCharErrors)
{
    for (std::wstring_view input : {L"''", L"'ab'", L"\"a\"", L"'\n'"}) {
        EXPECT_THAT(scn::scan<wchar_t>(input, scn::runtime_format(L"{:?}")),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
}

TEST(CharTest, EscapedPresentationWideCharEmptyInput)
{
    EXPECT_THAT(scn::scan<wchar_t>(L"", scn::runtime_format(L"{:?}")),
                FailedWith(scn::scan_error::end_of_input));
}

TEST(CharTest, EscapedPresentationWideCharAstral)
{
    auto result =
        scn::scan<wchar_t>(L"'\\u{1f600}'", scn::runtime_format(L"{:?}"));
    if constexpr (sizeof(wchar_t) == 2) {
        // Needs a surrogate pair
        EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
    }
    else {
        ASSERT_THAT(result, Succeeded());
        EXPECT_EQ(result->value(), static_cast<wchar_t>(0x1f600));
    }
}

TEST(CharTest, EscapedPresentationWideCharFromNarrowSourceRejected)
{
    EXPECT_THAT(scn::scan<wchar_t>("'a'", scn::runtime_format("{:?}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(CharTest, EscapedPresentationCodePoint)
{
    auto plain = scn::scan<char32_t>("'a' rest", "{:?}");
    ASSERT_THAT(plain, Succeeded());
    EXPECT_EQ(plain->value(), U'a');
    EXPECT_STREQ(plain->begin(), " rest");

    constexpr std::array<std::pair<std::string_view, char32_t>, 5> cases{{
        {R"('\n')", U'\n'},
        {R"('\'')", U'\''},
        {R"('\u{e4}')", 0xe4},
        {R"('\u{1f600}')", 0x1f600},
        {"'\xf0\x9f\x98\x80'", 0x1f600},
    }};
    for (const auto& [input, expected] : cases) {
        auto result = scn::scan<char32_t>(input, "{:?}");
        ASSERT_THAT(result, Succeeded()) << input;
        EXPECT_EQ(result->value(), expected) << input;
    }
}

TEST(CharTest, EscapedPresentationCodePointFromWide)
{
    auto result = scn::scan<char32_t>(L"'\u00e4'", L"{:?}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), char32_t{0xe4});
}

TEST(CharTest, EscapedPresentationCodePointHexEscapeFromWide)
{
    if constexpr (sizeof(wchar_t) == 4) {
        // A 32-bit wchar_t is UTF-32, like char32_t, so `\x{..}` isn't
        // transcoded
        const std::array<std::pair<std::wstring_view, char32_t>, 3> cases{{
            {LR"('\x{41}')", U'A'},
            {LR"('\x{d800}')", 0xd800},
            {LR"('\x{ffffffff}')", 0xffffffff},
        }};
        for (const auto& [input, expected] : cases) {
            auto result = scn::scan<char32_t>(input, L"{:?}");
            ASSERT_THAT(result, Succeeded());
            EXPECT_EQ(result->value(), expected);
        }
    }
    else {
        EXPECT_THAT(scn::scan<char32_t>(LR"('\x{41}')", L"{:?}"),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
}

TEST(CharTest, EscapedPresentationCodePointErrors)
{
    for (std::string_view input :
         {"a", "''", "'ab'", "'\\u{d800}'", "'\\u{110000}'", "'\\x{41}'",
          "'\\x{100000000}'", "'\xff'", "'\n'"}) {
        EXPECT_THAT(scn::scan<char32_t>(input, scn::runtime_format("{:?}")),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << input;
    }
}

TEST(CharTest, EscapedPresentationCodePointEmptyInput)
{
    EXPECT_THAT(scn::scan<char32_t>("", scn::runtime_format("{:?}")),
                FailedWith(scn::scan_error::end_of_input));
}

TEST(CharTest, EscapedPresentationRejectsLocalized)
{
    EXPECT_THAT(scn::scan<char>("'a'", scn::runtime_format("{:L?}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(CharTest, EscapedPresentationRejectedForOtherTypes)
{
    EXPECT_THAT(scn::scan<int>("1", scn::runtime_format("{:?}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(scn::scan<bool>("1", scn::runtime_format("{:?}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(scn::scan<double>("1", scn::runtime_format("{:?}")),
                FailedWith(scn::scan_error::invalid_format_string));
}
