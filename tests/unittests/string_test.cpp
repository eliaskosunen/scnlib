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

#include <array>
#include <deque>
#include <iterator>
#include <sstream>

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
