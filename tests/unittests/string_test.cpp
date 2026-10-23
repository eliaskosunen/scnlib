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

#include <scn/istream.h>
#include <scn/scan.h>
#include <scn/xchar.h>

#include "test_common.h"

#include <algorithm>
#include <array>
#include <deque>
#include <iterator>
#include <sstream>

#if __has_include(<format>)
#include <format>
#endif

namespace {
template <typename T, typename = void>
struct has_set_debug_format : std::false_type {};
template <typename T>
struct has_set_debug_format<
    T,
    std::void_t<
        decltype(std::declval<scn::scanner<T, char>&>().set_debug_format())>>
    : std::true_type {};

template <typename CharT>
struct debug_string {
    std::basic_string<CharT> value;
};
}  // namespace

template <>
struct scn::scanner<debug_string<char>, char>
    : scn::scanner<std::string, char> {
    constexpr scanner()
    {
        set_debug_format();
    }

    template <typename Context>
    auto scan(debug_string<char>& val, Context& ctx) const
    {
        return scn::scanner<std::string, char>::scan(val.value, ctx);
    }
};

TEST(StringTest, DefaultNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, DefaultWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, DefaultNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, DefaultWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, StringPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:s}");
    EXPECT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, StringPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:s}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, StringPresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{:s}");
    EXPECT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, StringPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{:s}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, CharacterPresentationRequiresWidthOrPrecision)
{
    auto result =
        scn::scan<std::string>("abc def", scn::runtime_format("{:c}"));
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(StringTest, CharacterPresentationWithWidth)
{
    auto result = scn::scan<std::string>("abc def", "{:5c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), "ef");
    EXPECT_EQ(result->value(), "abc d");

    auto with_precision = scn::scan<std::string>("abc def", "{:2.5c}");
    ASSERT_THAT(with_precision, Succeeded());
    EXPECT_STREQ(with_precision->begin(), "ef");
    EXPECT_EQ(with_precision->value(), "abc d");
}

// U+00E4 is two bytes, and `c` counts them separately
TEST(StringTest, CharacterPresentationWithWidthFromNonContiguousSource)
{
    const auto input = std::string_view{
        "a\xc3\xa4"
        "bcd"};
    const auto source = std::deque<char>(input.begin(), input.end());
    auto result = scn::scan<std::string>(source, "{:4c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(),
              "a\xc3\xa4"
              "b");
    EXPECT_EQ(std::distance(source.begin(), result->begin()), 4);

    const auto short_source = std::deque<char>{'a', 'b', 'c'};
    EXPECT_THAT(scn::scan<std::string>(short_source, "{:4c}"),
                FailedWith(scn::scan_error::length_too_short));
}

#if !SCN_DISABLE_IOSTREAM
TEST(StringTest, CharacterPresentationWithWidthFromIstream)
{
    std::istringstream stream{
        "a\xc3\xa4"
        "bcd"};
    auto result = scn::scan<std::string>(stream, "{:4c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(),
              "a\xc3\xa4"
              "b");
    std::string rest;
    std::getline(stream, rest);
    EXPECT_EQ(rest, "cd");

    std::istringstream short_stream{"abc"};
    EXPECT_THAT(scn::scan<std::string>(short_stream, "{:4c}"),
                FailedWith(scn::scan_error::length_too_short));
}
#endif

TEST(StringTest, CharacterPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:.4c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), "def");
    EXPECT_EQ(result->value(), "abc ");
}
TEST(StringTest, CharacterPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:.4c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L"def");
    EXPECT_EQ(result->value(), L"abc ");
}

// `c` doesn't transcode
TEST(StringTest, CharacterPresentationNarrowStringFromWideSource)
{
    for (auto fmt : {L"{:1c}", L"{:.4c}"}) {
        EXPECT_THAT(
            scn::scan<std::string>(L"abc def", scn::runtime_format(fmt)),
            FailedWith(scn::scan_error::invalid_format_string));
        EXPECT_THAT(scn::scan<std::string>(L"", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string));
    }
}
TEST(StringTest, CharacterPresentationWideStringFromNarrowSource)
{
    for (auto fmt : {"{:1c}", "{:.4c}"}) {
        EXPECT_THAT(
            scn::scan<std::wstring>("abc def", scn::runtime_format(fmt)),
            FailedWith(scn::scan_error::invalid_format_string));
        EXPECT_THAT(scn::scan<std::wstring>("", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string));
    }
}

TEST(StringTest, UnicodePresentationRequiresWidthOrPrecision)
{
    EXPECT_THAT(scn::scan<std::string>("abc def", scn::runtime_format("{:U}")),
                FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_THAT(
        scn::scan<std::wstring>(L"abc def", scn::runtime_format(L"{:U}")),
        FailedWith(scn::scan_error::invalid_format_string));
}

TEST(StringTest, UnicodePresentationWithWidth)
{
    auto result = scn::scan<std::string>("abc def", "{:5U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), "ef");
    EXPECT_EQ(result->value(), "abc d");

    auto with_precision = scn::scan<std::string>("abc def", "{:2.5U}");
    ASSERT_THAT(with_precision, Succeeded());
    EXPECT_STREQ(with_precision->begin(), "ef");
    EXPECT_EQ(with_precision->value(), "abc d");
}

// U+00E4 is two bytes, and `U` counts it as one
TEST(StringTest, UnicodePresentationWithWidthFromNonContiguousSource)
{
    const auto input = std::string_view{
        "a\xc3\xa4"
        "bcd"};
    const auto source = std::deque<char>(input.begin(), input.end());
    auto result = scn::scan<std::string>(source, "{:4U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(),
              "a\xc3\xa4"
              "bc");
    EXPECT_EQ(std::distance(source.begin(), result->begin()), 5);

    const auto short_source = std::deque<char>{'a', 'b', 'c'};
    EXPECT_THAT(scn::scan<std::string>(short_source, "{:4U}"),
                FailedWith(scn::scan_error::length_too_short));
}

#if !SCN_DISABLE_IOSTREAM
TEST(StringTest, UnicodePresentationWithWidthFromIstream)
{
    std::istringstream stream{
        "a\xc3\xa4"
        "bcd"};
    auto result = scn::scan<std::string>(stream, "{:4U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(),
              "a\xc3\xa4"
              "bc");
    std::string rest;
    std::getline(stream, rest);
    EXPECT_EQ(rest, "d");

    std::istringstream short_stream{"abc"};
    EXPECT_THAT(scn::scan<std::string>(short_stream, "{:4U}"),
                FailedWith(scn::scan_error::length_too_short));
}
#endif

TEST(StringTest, UnicodePresentationDoesNotSkipLeadingWhitespace)
{
    auto result = scn::scan<std::string>(" a b", "{:3U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), " a ");
}

TEST(StringTest, UnicodePresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:.4U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), "def");
    EXPECT_EQ(result->value(), "abc ");
}
TEST(StringTest, UnicodePresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:.4U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L"def");
    EXPECT_EQ(result->value(), L"abc ");
}

// Unlike `c`, `U` transcodes
TEST(StringTest, UnicodePresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"a\u00e4\u4e16x", L"{:3U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a\xc3\xa4\xe4\xb8\x96");
    EXPECT_STREQ(result->begin(), L"x");
}
TEST(StringTest, UnicodePresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>(
        "a\xc3\xa4\xe4\xb8\x96"
        "x",
        "{:3U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L"a\u00e4\u4e16");
    EXPECT_STREQ(result->begin(), "x");
}

// Behaves like a character set of every valid code point
TEST(StringTest, UnicodePresentationStopsAtInvalidCodePoint)
{
    const std::string_view input =
        "ab\xff"
        "cd";
    auto result = scn::scan<std::string>(input, "{:.5U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "ab");
    EXPECT_EQ(result->begin(), input.begin() + 2);

    auto exact = scn::scan<std::string>(input, "{:2U}");
    ASSERT_THAT(exact, Succeeded());
    EXPECT_EQ(exact->value(), "ab");

    EXPECT_THAT(scn::scan<std::string>(input, "{:5U}"),
                FailedWith(scn::scan_error::length_too_short));
    EXPECT_THAT(scn::scan<std::string>(input, "{:3.5U}"),
                FailedWith(scn::scan_error::length_too_short));
}

TEST(StringTest, UnicodePresentationDoesNotMatchLeadingInvalidCodePoint)
{
    for (auto fmt : {"{:.5U}", "{:2U}", "{:1.5U}"}) {
        auto result = scn::scan<std::string>(
            "\xff"
            "cd",
            scn::runtime_format(fmt));
        ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value))
            << fmt;
        EXPECT_STREQ(result.error().msg(), "Invalid encoding in scanned string")
            << fmt;
    }
}

TEST(StringTest, UnicodePresentationStopsAtLoneSurrogateWide)
{
    std::wstring input = L"ab";
    input.push_back(static_cast<wchar_t>(0xdc00));
    input += L"cd";
    auto result = scn::scan<std::wstring>(input, L"{:.5U}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L"ab");
    EXPECT_EQ(result->begin(), input.begin() + 2);

    EXPECT_THAT(scn::scan<std::wstring>(input, L"{:5U}"),
                FailedWith(scn::scan_error::length_too_short));
    EXPECT_THAT(scn::scan<std::wstring>(input.substr(2), L"{:.5U}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromNarrowSource)
{
    auto result = scn::scan<std::string>("abc def", "{:[a-z]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromWideSource)
{
    auto result = scn::scan<std::wstring>(L"abc def", L"{:[a-z]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), L"abc");
}

TEST(StringTest, CharacterSetPresentationNarrowStringFromWideSource)
{
    auto result = scn::scan<std::string>(L"abc def", L"{:[a-z]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), L" def");
    EXPECT_EQ(result->value(), "abc");
}
TEST(StringTest, CharacterSetPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>("abc def", "{:[a-z]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), " def");
    EXPECT_EQ(result->value(), L"abc");
}

// ü (U+00FC) is not in [a-ä] (U+0061..U+00E4)
TEST(StringTest, CharacterSetRangeAcrossAsciiBoundaryNarrow)
{
    auto result =
        scn::scan<std::string>("bcd\xc3\xa4\xc3\xbcz", "{:[a-\xc3\xa4]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "bcd\xc3\xa4");
    EXPECT_STREQ(result->begin(), "\xc3\xbcz");
}

TEST(StringTest, CharacterSetRangeAcrossAsciiBoundaryWide)
{
    auto result =
        scn::scan<std::wstring>(L"bcd\u00e4\u00fcz", L"{:[a-\u00e4]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L"bcd\u00e4");
    EXPECT_STREQ(result->begin(), L"\u00fcz");
}

TEST(StringTest, CharacterSetRangeAcrossAsciiBoundaryInverted)
{
    // Split in two, because `\xa4b` would be read as one escape sequence
    auto result = scn::scan<std::string>(
        "XY\xc3\xa4"
        "b",
        "{:[^a-\xc3\xa4]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "XY");
    EXPECT_STREQ(result->begin(),
                 "\xc3\xa4"
                 "b");
}

// An invalid code point is in no set: a non-inverted set stops before it,
// with or without non-ASCII members
TEST(StringTest, CharacterSetStopsAtInvalidCodePoint)
{
    const std::string_view input =
        "ab\xff"
        "cd";
    for (auto fmt : {"{:[a-z]}", "{:[a-z\xc3\xa4]}"}) {
        auto result = scn::scan<std::string>(input, scn::runtime_format(fmt));
        ASSERT_THAT(result, Succeeded()) << fmt;
        EXPECT_EQ(result->value(), "ab") << fmt;
        EXPECT_EQ(result->begin(), input.begin() + 2) << fmt;

        auto view =
            scn::scan<std::string_view>(input, scn::runtime_format(fmt));
        ASSERT_THAT(view, Succeeded()) << fmt;
        EXPECT_EQ(view->value(), "ab") << fmt;
    }
}

// Fails because nothing matched, not because of the invalid encoding
TEST(StringTest, CharacterSetDoesNotMatchLeadingInvalidCodePoint)
{
    for (auto fmt : {"{:[a-z]}", "{:[a-z\xc3\xa4]}"}) {
        auto result = scn::scan<std::string>(
            "\xff"
            "cd",
            scn::runtime_format(fmt));
        ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value))
            << fmt;
        EXPECT_STREQ(result.error().msg(),
                     "No characters matched in [character set]")
            << fmt;
    }
}

// An inverted set consumes an invalid code point, and the result fails
// validation
TEST(StringTest, InvertedCharacterSetRejectsInvalidCodePoint)
{
    for (auto fmt : {"{:[^,]}", "{:[^,\xc3\xa4]}"}) {
        EXPECT_THAT(scn::scan<std::string>("ab\xff"
                                           "cd,x",
                                           scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << fmt;
        EXPECT_THAT(scn::scan<std::string_view>("\xff"
                                                "cd,x",
                                                scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << fmt;
    }
}

// A low surrogate can't start a UTF-16 code point, unlike a high surrogate
// followed by something else, so this was skipped with a 16-bit wchar_t
TEST(StringTest, CharacterSetStopsAtLoneSurrogateWide)
{
    std::wstring input = L"ab";
    input.push_back(static_cast<wchar_t>(0xdc00));
    input += L"cd,x";
    for (auto fmt : {L"{:[a-z]}", L"{:[a-zä]}"}) {
        auto result = scn::scan<std::wstring>(input, scn::runtime_format(fmt));
        ASSERT_THAT(result, Succeeded());
        EXPECT_EQ(result->value(), L"ab");
        EXPECT_EQ(result->begin(), input.begin() + 2);
    }
    for (auto fmt : {L"{:[^,]}", L"{:[^,ä]}"}) {
        EXPECT_THAT(scn::scan<std::wstring>(input, scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
}

TEST(StringTest, CharacterSetLeadingBracket)
{
    auto result = scn::scan<std::string>("a]bc]d", "{:[]abc]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a]bc]");
    EXPECT_STREQ(result->begin(), "d");
}
TEST(StringTest, CharacterSetLeadingBracketRuntime)
{
    auto result =
        scn::scan<std::string>("a]bc]d", scn::runtime_format("{:[]abc]}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a]bc]");
    EXPECT_STREQ(result->begin(), "d");
}
TEST(StringTest, CharacterSetLeadingBracketOnly)
{
    auto result = scn::scan<std::string>("]]a", "{:[]]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "]]");
    EXPECT_STREQ(result->begin(), "a");
}
TEST(StringTest, CharacterSetLeadingBracketInverted)
{
    auto result = scn::scan<std::string>("xyz]abc", "{:[^]abc]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "xyz");
    EXPECT_STREQ(result->begin(), "]abc");
}
TEST(StringTest, CharacterSetLeadingBracketInvertedRuntime)
{
    auto result =
        scn::scan<std::string>("xyz]abc", scn::runtime_format("{:[^]abc]}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "xyz");
    EXPECT_STREQ(result->begin(), "]abc");
}
TEST(StringTest, CharacterSetLeadingBracketInvertedOnly)
{
    auto result = scn::scan<std::string>("ab]c", "{:[^]]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "ab");
    EXPECT_STREQ(result->begin(), "]c");
}
TEST(StringTest, CharacterSetLeadingBracketStartsRange)
{
    // ']' (0x5d) .. 'a' (0x61) includes ^ _ `
    auto result = scn::scan<std::string>("]^_`a-b", "{:[]-a]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "]^_`a");
    EXPECT_STREQ(result->begin(), "-b");
}
TEST(StringTest, CharacterSetLeadingBracketStartsRangeRuntime)
{
    auto result =
        scn::scan<std::string>("]^_`a-b", scn::runtime_format("{:[]-a]}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "]^_`a");
    EXPECT_STREQ(result->begin(), "-b");
}
TEST(StringTest, CharacterSetLeadingBracketStartsRangeInverted)
{
    auto result = scn::scan<std::string>("-b_c", "{:[^]-a]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "-b");
    EXPECT_STREQ(result->begin(), "_c");
}
TEST(StringTest, CharacterSetLeadingBracketTrailingDash)
{
    auto result = scn::scan<std::string>("]-^", "{:[]-]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "]-");
    EXPECT_STREQ(result->begin(), "^");
}
TEST(StringTest, CharacterSetLeadingBracketNonAsciiMember)
{
    auto result = scn::scan<std::string>("\xc3\xa4]\xc3\xb6", "{:[]\xc3\xa4]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "\xc3\xa4]");
    EXPECT_STREQ(result->begin(), "\xc3\xb6");
}
TEST(StringTest, CharacterSetLeadingBracketNonAsciiMemberWide)
{
    auto result = scn::scan<std::wstring>(L"\u00e4]\u00f6", L"{:[]\u00e4]}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L"\u00e4]");
    EXPECT_STREQ(result->begin(), L"\u00f6");
}
TEST(StringTest, CharacterSetLeadingBracketNotClosed)
{
    for (auto fmt : {"{:[]}", "{:[^]}"}) {
        auto result = scn::scan<std::string>("]a", scn::runtime_format(fmt));
        EXPECT_THAT(result, Failed()) << fmt;
    }
}
TEST(StringTest, CharacterSetStrayBracketAfterSet)
{
    auto result = scn::scan<std::string>("a]", scn::runtime_format("{:[a]]}"));
    EXPECT_THAT(result, Failed());
}

TEST(StringTest, CharacterSetOnEmptyInput)
{
    constexpr std::array<const char*, 4> formats{"{:[a-z]}", "{:[^a-z]}",
                                                 "{:[\xc3\xa4-\xc3\xb6]}",
                                                 "{:[^\xc3\xa4-\xc3\xb6]}"};
    for (auto fmt : formats) {
        auto result = scn::scan<std::string>("", scn::runtime_format(fmt));
        EXPECT_THAT(result, FailedWith(scn::scan_error::end_of_input)) << fmt;

        auto deque_source = std::deque<char>{};
        auto from_deque =
            scn::scan<std::string>(deque_source, scn::runtime_format(fmt));
        EXPECT_THAT(from_deque, FailedWith(scn::scan_error::end_of_input))
            << fmt;
    }
}

TEST(StringTest, CharacterSetNonMatchingInputIsNotEndOfInput)
{
    auto result = scn::scan<std::string>("\n", "{:[^\n]}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}

// U+4E00 is 3 bytes long in UTF-8. Precision counts code points.
TEST(StringTest, PrecisionOfOneMultibyteCodePoint)
{
    const std::string wide_char = "\xe4\xb8\x80";

    constexpr std::array<const char*, 3> formats{"{:.1}", "{:.1[^x]}",
                                                 "{:.1s}"};
    for (auto fmt : formats) {
        const auto input = wide_char + "a";
        auto result = scn::scan<std::string>(input, scn::runtime_format(fmt));
        ASSERT_THAT(result, Succeeded()) << fmt;
        EXPECT_EQ(result->value(), wide_char) << fmt;
        EXPECT_EQ(result->begin(), input.begin() + 3) << fmt;

        auto deque_source =
            std::deque<char>(wide_char.begin(), wide_char.end());
        deque_source.push_back('a');
        auto from_deque =
            scn::scan<std::string>(deque_source, scn::runtime_format(fmt));
        ASSERT_THAT(from_deque, Succeeded()) << fmt;
        EXPECT_EQ(from_deque->value(), wide_char) << fmt;
        EXPECT_EQ(from_deque->begin(), deque_source.begin() + 3) << fmt;
    }
}

TEST(StringTest, PrecisionOfOneMultibyteCodePointStringView)
{
    const std::string wide_char = "\xe4\xb8\x80";

    auto result =
        scn::scan<std::string_view>(wide_char, scn::runtime_format("{:.1}"));
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), wide_char);
}

// `c` counts code units, and can split a code point
TEST(StringTest, CharacterPresentationSplitsMultibyteCodePoint)
{
    const std::string input = "\xe4\xb8\x80";

    auto result = scn::scan<std::string>(input, "{:.1c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "\xe4");
    EXPECT_EQ(result->begin(), input.begin() + 1);

    auto view = scn::scan<std::string_view>(input, "{:2c}");
    ASSERT_THAT(view, Succeeded());
    EXPECT_EQ(view->value(), "\xe4\xb8");
    EXPECT_EQ(view->begin(), input.begin() + 2);
}

TEST(StringTest, PrecisionLongerThanInput)
{
    const std::string wide_char = "\xe4\xb8\x80";

    constexpr std::array<const char*, 2> formats{"{:.2}", "{:.2[^x]}"};
    for (auto fmt : formats) {
        auto result =
            scn::scan<std::string>(wide_char, scn::runtime_format(fmt));
        ASSERT_THAT(result, Succeeded()) << fmt;
        EXPECT_EQ(result->value(), wide_char) << fmt;
    }
}

TEST(StringTest, CharacterSetMatchingNothing)
{
    auto result = scn::scan<std::string>("123", "{:[a-z]}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));

    auto inverted = scn::scan<std::string>("abc", "{:[^a-z]}");
    EXPECT_THAT(inverted, FailedWith(scn::scan_error::invalid_scanned_value));

    auto nonascii = scn::scan<std::string>("abc", "{:[\xc3\xa4-\xc3\xb6]}");
    EXPECT_THAT(nonascii, FailedWith(scn::scan_error::invalid_scanned_value));

    auto nonascii_inverted =
        scn::scan<std::string>("\xc3\xa4", "{:[^\xc3\xa4-\xc3\xb6]}");
    EXPECT_THAT(nonascii_inverted,
                FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, CharacterSetMatchingNothingStringView)
{
    auto result = scn::scan<std::string_view>("123", "{:[a-z]}");
    EXPECT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, WonkyInput)
{
    const char source[] = {'o', ' ', '\x0f', '\n', '\n', '\xc3'};
    auto input = std::string_view{source, sizeof(source)};

    // `c` doesn't validate the encoding
    auto result = scn::scan<std::string>(input, "{:.64c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), input);
    EXPECT_THAT(result->range(), IsEmptyRange());
}

TEST(StringTest, WonkyInputAndFormatWithTranscoding)
{
    const char source[] = {'a', ']', 'c', '{', '}', '\xdf', ':', '\xb1'};
    auto input = std::string_view{source, sizeof(source)};

    auto result = scn::scan<std::wstring>(input, scn::runtime_format(input));
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, WonkyInput2)
{
    const auto input =
        std::string_view{"\303 \245å\377åä\3035\377ååíääccccc\307c\244c"};

    auto result = scn::scan<std::string_view>(input, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, WonkyInput3)
{
    const char source[] = {
        '\216', '\030', 0,      0,      0,      0,      0,      0,      0,
        '\216', '\'',   'a',    '\216', '\216', '\216', '\216', '\216', '\216',
        '\216', '\216', '\216', '\216', '\216', '\360', '\237', '\237'};
    auto input = std::string_view{source, sizeof(source)};

    auto result = scn::scan<std::string>(input, "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, EmptyString)
{
    auto result = scn::scan<std::string>("", "{}");
    ASSERT_THAT(result, FailedWith(scn::scan_error::end_of_input));
}

TEST(StringTest, StringWithWidth)
{
    auto result = scn::scan<std::string>("hello world", "{:5}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "hello");
    EXPECT_STREQ(result->begin(), " world");
}

TEST(StringTest, MultipleStrings)
{
    auto result =
        scn::scan<std::string, std::string, std::string>("a b c", "{} {} {}");
    ASSERT_THAT(result, Succeeded());
    auto [a, b, c] = result->values();
    EXPECT_EQ(a, "a");
    EXPECT_EQ(b, "b");
    EXPECT_EQ(c, "c");
}

TEST(StringTest, StringWithWhitespace)
{
    auto result = scn::scan<std::string>("   leading", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "leading");
}

TEST(StringTest, UnicodeString)
{
    auto result = scn::scan<std::string>("你好世界 next", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "你好世界");
}

TEST(StringTest, StringThenInt)
{
    auto result = scn::scan<std::string, int>("hello 42", "{} {}");
    ASSERT_THAT(result, Succeeded());
    auto [str, num] = result->values();
    EXPECT_EQ(str, "hello");
    EXPECT_EQ(num, 42);
}

namespace {
void expect_escaped_value(std::string_view input,
                          const std::string& expected,
                          std::string_view expected_rest = "")
{
    expect_value<std::string>(input, "{:?}", expected, expected_rest);
}

void expect_escaped_error(std::string_view input,
                          scn::scan_error::code_t expected)
{
    expect_error<std::string>(input, "{:?}", expected);

    // Widening non-ASCII bytes one by one gives other characters
    if (std::all_of(input.begin(), input.end(), [](char ch) {
            return static_cast<unsigned char>(ch) < 128;
        })) {
        const std::wstring wide(input.begin(), input.end());
        EXPECT_THAT(scn::scan<std::wstring>(wide, L"{:?}"),
                    FailedWith(expected))
            << input;
    }
}
}  // namespace

TEST(StringTest, EscapedPresentationPlain)
{
    expect_escaped_value("\"abc def\"", "abc def");
    expect_escaped_value("\"\"", "");
    expect_escaped_value("\"a\xc3\xa4\xe4\xb8\x96\xf0\x9f\x98\x80\"",
                         "a\xc3\xa4\xe4\xb8\x96\xf0\x9f\x98\x80");
}

TEST(StringTest, EscapedPresentationEscapes)
{
    expect_escaped_value(R"("\t\n\r\"\'\\")", "\t\n\r\"'\\");
    expect_escaped_value(R"("a\nb")", "a\nb");
}

TEST(StringTest, EscapedPresentationBothQuotesInEitherKind)
{
    expect_escaped_value(R"("it's")", "it's");
    expect_escaped_value(R"("it\'s")", "it's");
    expect_escaped_value(R"("say \"hi\"")", "say \"hi\"");
}

TEST(StringTest, EscapedPresentationUnicodeEscape)
{
    expect_escaped_value(R"("\u{41}")", "A");
    expect_escaped_value(R"("\u{e4}")", "\xc3\xa4");
    expect_escaped_value(R"("\u{4E16}")", "\xe4\xb8\x96");
    expect_escaped_value(R"("\u{1f600}")", "\xf0\x9f\x98\x80");
    expect_escaped_value(R"("\u{10ffff}")", "\xf4\x8f\xbf\xbf");
    expect_escaped_value(R"("\u{0000000041}")", "A");
    expect_escaped_value(R"("\u{0}")", std::string(1, '\0'));
}

TEST(StringTest, EscapedPresentationHexEscape)
{
    expect_escaped_value(R"("\x{41}")", "A");
    expect_escaped_value(R"("\x{ff}\x{c3}")", "\xff\xc3");
    expect_escaped_value(R"("\x{FF}")", "\xff");
}

TEST(StringTest, EscapedPresentationRawControlCharacters)
{
    expect_escaped_value("\"a\tb\rc\x01\"", "a\tb\rc\x01");
}

TEST(StringTest, EscapedPresentationSkipsLeadingWhitespace)
{
    expect_escaped_value(" \t\n\"abc\"", "abc");
}

TEST(StringTest, EscapedPresentationLeavesTrailingData)
{
    expect_escaped_value("\"abc\" def", "abc", " def");
    expect_escaped_value("\"abc\"\"def\"", "abc", "\"def\"");
}

TEST(StringTest, EscapedPresentationEmptySource)
{
    expect_escaped_error("", scn::scan_error::end_of_input);
    expect_escaped_error(" \n", scn::scan_error::end_of_input);
}

TEST(StringTest, EscapedPresentationMissingOpeningQuote)
{
    expect_escaped_error("abc\"", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("'abc'", scn::scan_error::invalid_scanned_value);
}

TEST(StringTest, EscapedPresentationUnterminated)
{
    expect_escaped_error("\"", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"abc", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"abc\\", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"abc\\\"", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"\\u{41", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"\\u", scn::scan_error::invalid_scanned_value);
}

TEST(StringTest, EscapedPresentationRawNewline)
{
    expect_escaped_error("\"abc\ndef\"",
                         scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"abc\n", scn::scan_error::invalid_scanned_value);
}

TEST(StringTest, EscapedPresentationInvalidEscape)
{
    for (auto input : {R"("\a")",
                       R"("\b")",
                       R"("\f")",
                       R"("\v")",
                       R"("\?")",
                       R"("\e")",
                       R"("\0")",
                       R"("\101")",
                       R"("\o{101}")",
                       R"("\x41")",
                       R"("\u0041")",
                       R"("\U00000041")",
                       R"("\N{LATIN CAPITAL LETTER A}")",
                       R"("\q")",
                       R"("\ ")",
                       R"("\x")",
                       R"("\u{}")",
                       R"("\x{}")",
                       R"("\u{4g}")",
                       R"("\u{ 41}")",
                       R"("\u{-1}")",
                       R"("\u{+1}")",
                       R"("\u{0x41}")"}) {
        expect_escaped_error(input, scn::scan_error::invalid_scanned_value);
    }
}

TEST(StringTest, EscapedPresentationOutOfRangeEscape)
{
    for (std::string_view input :
         {R"("\u{110000}")", R"("\u{ffffffff}")", R"("\u{100000000}")",
          R"("\u{d800}")", R"("\u{dfff}")", R"("\u{DC00}")", R"("\x{100}")",
          R"("\x{ffffffff}")", R"("\x{100000000}")",
          R"("\x{ffffffffffffffffffff}")",
          R"("\u{fffffffffffffffffffffffffffff}")"}) {
        EXPECT_THAT(scn::scan<std::string>(input, "{:?}"),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << input;
    }
}

TEST(StringTest, EscapedPresentationOutOfRangeEscapeWide)
{
    for (std::wstring_view input :
         {LR"("\u{110000}")", LR"("\u{d800}")", LR"("\x{100000000}")"}) {
        EXPECT_THAT(scn::scan<std::wstring>(input, L"{:?}"),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
    // Raw string literals with quotes inside them are misread by MSVC in
    // macro arguments
    std::string_view narrow_input = R"("\u{d800}")";
    EXPECT_THAT(scn::scan<std::wstring>(narrow_input, "{:?}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, EscapedPresentationRawInvalidEncoding)
{
    expect_escaped_error("\"a\xff\"", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"a\xc3\"", scn::scan_error::invalid_scanned_value);
    expect_escaped_error("\"a\xed\xa0\x80\"",
                         scn::scan_error::invalid_scanned_value);
}

#if !SCN_DISABLE_IOSTREAM
TEST(StringTest, EscapedPresentationFromIstream)
{
    std::istringstream stream{"\"a\\n\\u{e4}b\" rest"};
    auto result = scn::scan<std::string>(stream, "{:?}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(),
              "a\n\xc3\xa4"
              "b");
    std::string rest;
    std::getline(stream, rest);
    EXPECT_EQ(rest, " rest");

    std::istringstream unterminated{"\"abc"};
    EXPECT_THAT(scn::scan<std::string>(unterminated, "{:?}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
}
#endif

TEST(StringTest, EscapedPresentationWideStringFromWideSource)
{
    auto result =
        scn::scan<std::wstring>(L"\"a\\n\\u{e4}\\u{1f600}\\x{41}\" x", L"{:?}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L"a\n\u00e4\U0001F600A");
    EXPECT_STREQ(result->begin(), L" x");
}

TEST(StringTest, EscapedPresentationNarrowStringFromWideSource)
{
    auto result =
        scn::scan<std::string>(L"\"a\u00e4\u4e16\\u{1f600}\" x", L"{:?}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), "a\xc3\xa4\xe4\xb8\x96\xf0\x9f\x98\x80");
    EXPECT_STREQ(result->begin(), L" x");
}

TEST(StringTest, EscapedPresentationWideStringFromNarrowSource)
{
    auto result = scn::scan<std::wstring>(
        "\"a\xc3\xa4\xe4\xb8\x96\\u{1f600}\" x", "{:?}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), L"a\u00e4\u4e16\U0001F600");
    EXPECT_STREQ(result->begin(), " x");
}

TEST(StringTest, EscapedPresentationHexEscapeNotTranscoded)
{
    for (std::string_view input : {R"("\x{41}")", R"("a\x{ff}")"}) {
        EXPECT_THAT(scn::scan<std::wstring>(input, "{:?}"),
                    FailedWith(scn::scan_error::invalid_scanned_value))
            << input;
    }
    for (std::wstring_view input : {LR"("\x{41}")", LR"("a\x{d800}")"}) {
        EXPECT_THAT(scn::scan<std::string>(input, L"{:?}"),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
}

TEST(StringTest, EscapedPresentationWideSourceRawInvalidEncoding)
{
    std::wstring input = L"\"a";
    input.push_back(static_cast<wchar_t>(0xdc00));
    input += L"\"";
    EXPECT_THAT(scn::scan<std::string>(input, L"{:?}"),
                FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(StringTest, EscapedPresentationHexEscapeWideCodeUnit)
{
    auto result = scn::scan<std::wstring>(L"\"\\x{ffff}\"", L"{:?}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value(), std::wstring(1, static_cast<wchar_t>(0xffff)));

    if constexpr (sizeof(wchar_t) == 2) {
        EXPECT_THAT(scn::scan<std::wstring>(L"\"\\x{10000}\"", L"{:?}"),
                    FailedWith(scn::scan_error::invalid_scanned_value));
    }
}

TEST(StringTest, EscapedPresentationRejectsWidthAndPrecision)
{
    for (auto fmt : {"{:5?}", "{:.5?}", "{:2.5?}"}) {
        EXPECT_THAT(scn::scan<std::string>("\"abc\"", scn::runtime_format(fmt)),
                    FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
    for (auto fmt : {L"{:5?}", L"{:.5?}"}) {
        EXPECT_THAT(
            scn::scan<std::wstring>(L"\"abc\"", scn::runtime_format(fmt)),
            FailedWith(scn::scan_error::invalid_format_string));
    }
}

TEST(StringTest, EscapedPresentationRejectsLocalized)
{
    EXPECT_THAT(scn::scan<std::string>("\"abc\"", scn::runtime_format("{:L?}")),
                FailedWith(scn::scan_error::invalid_format_string));
}

TEST(StringTest, EscapedPresentationInTuple)
{
    auto result = scn::scan<std::string, int, std::string>(
        "\"a b\" 42 \"c\\\"d\"", "{:?} {} {:?}");
    ASSERT_THAT(result, Succeeded());
    auto [first, number, second] = result->values();
    EXPECT_EQ(first, "a b");
    EXPECT_EQ(number, 42);
    EXPECT_EQ(second, "c\"d");
}

TEST(StringTest, SetDebugFormat)
{
    static_assert(
        std::is_same_v<decltype(std::declval<scn::scanner<std::string, char>&>()
                                    .set_debug_format()),
                       void>);

    scn::scanner<std::string, char> narrow;
    narrow.set_debug_format();
    scn::scanner<std::wstring, wchar_t> wide;
    wide.set_debug_format();
    scn::scanner<char, char> narrow_char;
    narrow_char.set_debug_format();
    scn::scanner<wchar_t, wchar_t> wide_char;
    wide_char.set_debug_format();
    scn::scanner<char32_t, char> code_point;
    code_point.set_debug_format();
}

TEST(StringTest, SetDebugFormatOnlyForStringsAndCharacters)
{
    static_assert(has_set_debug_format<std::string>::value);
    static_assert(has_set_debug_format<char>::value);
    static_assert(has_set_debug_format<wchar_t>::value);
    static_assert(has_set_debug_format<char32_t>::value);
    static_assert(!has_set_debug_format<std::string_view>::value);
    static_assert(!has_set_debug_format<int>::value);
    static_assert(!has_set_debug_format<bool>::value);
    static_assert(!has_set_debug_format<double>::value);
}

TEST(StringTest, SetDebugFormatInDerivedScanner)
{
    auto result = scn::scan<debug_string<char>>("\"a b\"", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().value, "a b");

    auto spaced = scn::scan<debug_string<char>>("a b", "{}");
    EXPECT_THAT(spaced, FailedWith(scn::scan_error::invalid_scanned_value));
}

// std::format's `?` for strings and characters is part of P2286
#if defined(__cpp_lib_format_ranges)
namespace {
// Without /utf-8, MSVC's std::format treats narrow strings as the ANSI code
// page, and writes bytes that are invalid UTF-8 unescaped
bool std_format_escapes_invalid_utf8()
{
    return std::format("{:?}", '\xff') == "'\\x{ff}'";
}

void expect_round_trip(const std::string& input)
{
    const auto formatted = std::format("{:?}", input);
    auto result = scn::scan<std::string>(formatted, "{:?}");
    ASSERT_THAT(result, Succeeded()) << formatted;
    EXPECT_EQ(result->value(), input) << formatted;
    EXPECT_THAT(result->range(), IsEmptyRange()) << formatted;

    auto source = std::deque<char>(formatted.begin(), formatted.end());
    auto noncontiguous = scn::scan<std::string>(source, "{:?}");
    ASSERT_THAT(noncontiguous, Succeeded()) << formatted;
    EXPECT_EQ(noncontiguous->value(), input) << formatted;
}
}  // namespace

TEST(StringTest, EscapedPresentationRoundTripsStdFormat)
{
    const std::string inputs[] = {
        "",
        "abc def",
        "tab\there\nnewline\rcarriage",
        "quote\"double'single\\backslash",
        "\"'\\",
        std::string("nul\0byte", 8),
        "\x01\x02\x1f\x7f",
        "a\xc3\xa4\xe4\xb8\x96\xf0\x9f\x98\x80",
        "\xc2\x85\xe2\x80\xa8",
        "}{}{ ?",
    };
    for (const auto& input : inputs) {
        expect_round_trip(input);
    }

    if (std_format_escapes_invalid_utf8()) {
        for (const auto& input :
             {"invalid\xff\xc3", "\xed\xa0\x80", "\xf4\x90\x80\x80"}) {
            expect_round_trip(input);
        }
    }
}

TEST(StringTest, EscapedPresentationRoundTripsStdFormatWide)
{
    const std::wstring inputs[] = {
        L"",
        L"abc def",
        L"tab\there\nnewline\rcarriage",
        L"quote\"double'single\\backslash",
        L"a\u00e4\u4e16\U0001F600",
        L"\x01\x1f\x7f",
    };
    for (const auto& input : inputs) {
        const auto formatted = std::format(L"{:?}", input);
        auto result = scn::scan<std::wstring>(formatted, L"{:?}");
        ASSERT_THAT(result, Succeeded());
        EXPECT_EQ(result->value(), input);
        EXPECT_THAT(result->range(), IsEmptyRange());
    }
}

TEST(StringTest, EscapedPresentationCharRoundTripsStdFormat)
{
    const int end = std_format_escapes_invalid_utf8() ? 256 : 128;
    for (int value = 0; value < end; ++value) {
        const auto ch = static_cast<char>(value);
        const auto formatted = std::format("{:?}", ch);
        auto result = scn::scan<char>(formatted, "{:?}");
        ASSERT_THAT(result, Succeeded()) << formatted;
        EXPECT_EQ(result->value(), ch) << formatted;
        EXPECT_THAT(result->range(), IsEmptyRange()) << formatted;
    }
    for (wchar_t ch : {L'a', L'\'', L'"', L'\\', L'\n', L'\t', L'\0', L'\u00e4',
                       L'\u4e16'}) {
        const auto formatted = std::format(L"{:?}", ch);
        auto result = scn::scan<wchar_t>(formatted, L"{:?}");
        ASSERT_THAT(result, Succeeded());
        EXPECT_EQ(result->value(), ch);
    }
}
#endif
