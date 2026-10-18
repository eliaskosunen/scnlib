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

#include <scn/regex.h>
#include <scn/scan.h>
#include <scn/xchar.h>

#include <array>
#include <deque>

using namespace std::string_view_literals;

#if !SCN_DISABLE_REGEX

template <typename CharT>
auto regex_matches_are(
    std::initializer_list<std::basic_string_view<CharT>> elems)
{
    using matcher_t = decltype(testing::Optional(testing::Property(
        &scn::basic_regex_match<CharT>::get, std::basic_string_view<CharT>{})));
    std::vector<matcher_t> matchers;
    matchers.reserve(elems.size());
    for (const auto& elem : elems) {
        matchers.emplace_back(testing::Optional(
            testing::Property(&scn::basic_regex_match<CharT>::get, elem)));
    }
    return testing::ElementsAreArray(matchers.begin(), matchers.end());
}

static_assert(
    std::is_same_v<
        std::iterator_traits<scn::regex_matches::iterator>::iterator_category,
        std::input_iterator_tag>);
#if SCN_STD >= SCN_STD_20
static_assert(
    scn::ranges::random_access_iterator<scn::regex_matches::iterator>);
static_assert(
    scn::ranges::random_access_iterator<scn::wregex_matches::iterator>);
static_assert(scn::ranges::random_access_range<scn::regex_matches>);
#endif

TEST(RegexTest, InvalidRegexString)
{
    auto r = scn::scan<std::string>("foobar123", "{:/[a/}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(RegexTest, InvalidRegexStringView)
{
    auto r = scn::scan<std::string_view>("foobar123", "{:/[a/}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(RegexTest, InvalidRegexMatches)
{
    auto r = scn::scan<scn::regex_matches>("foobar123", "{:/[a/}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(RegexTest, String)
{
    auto r = scn::scan<std::string>("foobar123", "{:/([a-zA-Z]+)/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, StringView)
{
    auto r = scn::scan<std::string_view>("foobar123", "{:/([a-zA-Z]+)/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, Matches)
{
    auto r =
        scn::scan<scn::regex_matches>("foobar123", "{:/([a-zA-Z]+)([0-9]+)/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());
    EXPECT_THAT(r->value(),
                regex_matches_are<char>({"foobar123", "foobar", "123"}));
}

TEST(RegexTest, MatchesIteratorArithmetic)
{
    auto r =
        scn::scan<scn::regex_matches>("foobar123", "{:/([a-zA-Z]+)([0-9]+)/}");
    ASSERT_THAT(r, Succeeded());
    const auto& matches = r->value();

    const auto first = matches.begin();
    const auto last = matches.end();
    EXPECT_EQ(last - first, 3);
    EXPECT_EQ(first - last, -3);
    EXPECT_TRUE(first < last);
    EXPECT_TRUE(first <= last);
    EXPECT_TRUE(last > first);
    EXPECT_TRUE(last >= first);
    EXPECT_FALSE(first > last);
    EXPECT_TRUE(first <= first);
    EXPECT_TRUE(first >= first);

    EXPECT_EQ(first + 3, last);
    EXPECT_EQ(3 + first, last);
    EXPECT_EQ(last - 3, first);
    EXPECT_EQ((first + 2) - first, 2);

    EXPECT_EQ(first[1]->get(), "foobar");
    EXPECT_EQ(first[2]->get(), "123");
    EXPECT_EQ((last - 1)[0]->get(), "123");
}

#if SCN_REGEX_SUPPORTS_NAMED_CAPTURES
TEST(RegexTest, NamedString)
{
    auto r = scn::scan<std::string>("foobar123",
                                    "{:/(?<prefix>[a-zA-Z]+)([0-9]+)/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());
    EXPECT_EQ(r->value(), "foobar123");
}

TEST(RegexTest, NamedMatches)
{
    auto r = scn::scan<scn::regex_matches>("foobar123",
                                           "{:/(?<prefix>[a-zA-Z]+)([0-9]+)/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());

    ASSERT_TRUE(r->value()[0]);
    EXPECT_EQ(r->value()[0]->get(), "foobar123");
    EXPECT_FALSE(r->value()[0]->name());

    ASSERT_TRUE(r->value()[1]);
    EXPECT_EQ(r->value()[1]->get(), "foobar");
    ASSERT_TRUE(r->value()[1]->name());
    EXPECT_EQ(*r->value()[1]->name(), "prefix");

    ASSERT_TRUE(r->value()[2]);
    EXPECT_EQ(r->value()[2]->get(), "123");
    EXPECT_FALSE(r->value()[2]->name());
}
#endif

#if SCN_REGEX_SUPPORTS_WIDE_STRINGS
TEST(RegexTest, WideStringView)
{
    auto r = scn::scan<std::wstring_view>(L"foobar123", L"{:/[a-zA-Z]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), L"foobar");
}

TEST(RegexTest, WideString)
{
    auto r = scn::scan<std::wstring>(L"foobar123", L"{:/[a-zA-Z]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), L"foobar");
}

TEST(RegexTest, WideMatches)
{
    auto r = scn::scan<scn::wregex_matches>(L"foobar123",
                                            L"{:/([a-zA-Z]+)([0-9]+)/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());
    EXPECT_THAT(r->value(),
                regex_matches_are<wchar_t>({L"foobar123", L"foobar", L"123"}));
}
#endif

TEST(RegexTest, TranscodeStringNarrowToWide)
{
    auto r = scn::scan<std::wstring>("foobar123", "{:/[a-zA-Z]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), L"foobar");
}

#if SCN_REGEX_SUPPORTS_WIDE_STRINGS
TEST(RegexTest, TranscodeStringWideToNarrow)
{
    auto r = scn::scan<std::string>(L"foobar123", L"{:/[a-zA-Z]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "foobar");
}
#endif

TEST(RegexTest, AlphaCharacterClass)
{
    auto r = scn::scan<std::string_view>("foobar123", "{:/[[:alpha:]]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, AlphaCharacterClassWithNonAscii)
{
#if SCN_REGEX_BOOST_USE_ICU
    // [[:alpha:]] uses the ICU with Boost.Regex + ICU
    auto r = scn::scan<std::string_view>("fööbär123", "{:/[[:alpha:]]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "fööbär");
#else
    // [[:alpha:]] is ASCII only with
    // std::regex and re2, and Boost.Regex without ICU
    auto r = scn::scan<std::string_view>("fööbär123", "{:/[[:alpha:]]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "f");
#endif
}

#if SCN_REGEX_BACKEND != SCN_REGEX_BACKEND_STD
// std::regex doesnt support Unicode character classes
TEST(RegexTest, LetterUnicodeCharacterClass)
{
    // L = Letter
    auto r = scn::scan<std::string_view>("foobar123", "{:/\\pL+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "foobar");
}

TEST(RegexTest, LetterUnicodeCharacterClassWithNonAscii)
{
#if SCN_REGEX_SUPPORTS_UTF8_CLASSIFICATION
    auto r = scn::scan<std::string_view>("fööbär123", "{:/\\pL+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "fööbär");
#else
    auto r = scn::scan<std::string_view>("fööbär123", "{:/\\pL+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "f");
#endif
}

TEST(RegexTest, EmojiWithSoUnicodeCharacterClass)
{
#if SCN_REGEX_SUPPORTS_UTF8_CLASSIFICATION
    // U+1F600 "GRINNING FACE" and U+1F601 "GRINNING FACE WITH SMILING EYES"
    auto r = scn::scan<std::string_view>("\xf0\x9f\x98\x80\xf0\x9f\x98\x81 abc",
                                         "{:/\\p{So}+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "\xf0\x9f\x98\x80\xf0\x9f\x98\x81");
#else
    auto r = scn::scan<std::string_view>("\xf0\x9f\x98\x80\xf0\x9f\x98\x81 abc",
                                         "{:/\\p{So}+/}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_format_string));
#endif
}
#endif

TEST(RegexTest, NoCaseFlagStringView)
{
    auto r = scn::scan<std::string_view>("FooBar123", "{:/[a-z]+/i}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "FooBar");
}

TEST(RegexTest, NoCaseFlagMatches)
{
    auto r = scn::scan<scn::regex_matches>("FooBar123", "{:/([a-z]+)/i}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_THAT(r->value(), regex_matches_are<char>({"FooBar", "FooBar"}));
}

TEST(RegexTest, NoCaseAndNoCaptureFlagStringView)
{
    auto r = scn::scan<std::string_view>("FooBar123", "{:/[a-z]+/in}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "FooBar");
}

TEST(RegexTest, NoCaseAndNoCaptureFlagMatches)
{
    auto r =
        scn::scan<scn::regex_matches>("FooBar123", "{:/([a-z]+)([0-9]+)/in}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());
    EXPECT_THAT(r->value(), regex_matches_are<char>({"FooBar123"}));
}

TEST(RegexTest, EscapedSlashInPattern)
{
    auto r = scn::scan<std::string_view>("foo/bar", "{:/[a-z]+\\/[a-z]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), IsEmptyRange());
    EXPECT_EQ(r->value(), "foo/bar");
}

TEST(RegexTest, PatternEndingInEscapedBackslash)
{
    auto r = scn::scan<std::string_view>("a\\b", "{:/a\\\\/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "a\\");
    EXPECT_STREQ(r->begin(), "b");
}

TEST(RegexTest, EscapedSlashAfterEscapedBackslash)
{
    auto r = scn::scan<std::string_view>("a\\/b", "{:/a\\\\\\/b/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "a\\/b");
    EXPECT_THAT(r->range(), IsEmptyRange());
}

TEST(RegexTest, EmptyGroup)
{
    auto r = scn::scan<scn::regex_matches>("b", "{:/(a*)b/}");
    ASSERT_THAT(r, Succeeded());
    ASSERT_EQ(r->value().size(), 2);
    ASSERT_TRUE(r->value()[1]);
    EXPECT_EQ(r->value()[1]->get(), "");
    EXPECT_THAT(r->range(), IsEmptyRange());
}

TEST(RegexTest, UnmatchedOptionalGroup)
{
    auto r = scn::scan<scn::regex_matches>("b", "{:/(a)?b/}");
    ASSERT_THAT(r, Succeeded());
    ASSERT_EQ(r->value().size(), 2);
    EXPECT_FALSE(r->value()[1]);
    EXPECT_THAT(r->range(), IsEmptyRange());
}

TEST(RegexTest, MultilineFlagAbsent)
{
    auto r = scn::scan<std::string_view>("a\nb", "{:/a$/}");
#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD && SCN_STDLIB_MS_STL && \
    _MSVC_STL_UPDATE < 202506L
    // Older MSVC STL versions always match in multiline mode
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "a");
#else
    EXPECT_THAT(r, FailedWith(scn::scan_error::invalid_scanned_value));
#endif
}

TEST(RegexTest, MultilineFlag)
{
#if SCN_REGEX_BACKEND != SCN_REGEX_BACKEND_STD || SCN_HAS_STD_REGEX_MULTILINE
    auto r = scn::scan<std::string_view>("a\nb", "{:/a$/m}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "a");
    EXPECT_STREQ(r->begin(), "\nb");
#else
    auto r =
        scn::scan<std::string_view>("a\nb", scn::runtime_format("{:/a$/m}"));
    EXPECT_THAT(r, FailedWith(scn::scan_error::invalid_format_string));
#endif
}

TEST(RegexTest, SinglelineFlagAbsent)
{
    auto r = scn::scan<std::string_view>("a\nb", "{:/a.b/}");
    EXPECT_THAT(r, FailedWith(scn::scan_error::invalid_scanned_value));
}

TEST(RegexTest, SinglelineFlag)
{
#if SCN_REGEX_BACKEND != SCN_REGEX_BACKEND_STD
    auto r = scn::scan<std::string_view>("a\nb", "{:/a.b/s}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_EQ(r->value(), "a\nb");
    EXPECT_THAT(r->range(), IsEmptyRange());
#else
    auto r =
        scn::scan<std::string_view>("a\nb", scn::runtime_format("{:/a.b/s}"));
    EXPECT_THAT(r, FailedWith(scn::scan_error::invalid_format_string));
#endif
}

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD || \
    SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST

TEST(RegexTest, NonContiguousSource)
{
    auto source = std::deque<char>{'F', 'o', 'o', '4', '2'};
    auto r = scn::scan<std::string>(source, "{:/[a-zA-Z]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_EQ(r->value(), "Foo");
}

TEST(RegexTest, NonContiguousSourceWithMatches)
{
    auto source = std::deque<char>{'F', 'o', 'o', '4', '2'};
    auto r = scn::scan<scn::regex_matches>(source, "{:/[a-zA-Z]+/}");
    ASSERT_THAT(r, Succeeded());
    EXPECT_THAT(r->range(), ::testing::Not(IsEmptyRange()));
    EXPECT_THAT(r->value(), regex_matches_are<char>({"Foo"}));
}

#endif

TEST(RegexTest, RegexMatchesWithRuntimeFormatWithoutRegexFails)
{
    constexpr std::array<const char*, 3> formats{"{}", "{:5}", "{:L}"};
    for (auto fmt : formats) {
        auto r = scn::scan<scn::regex_matches>("abc", scn::runtime_format(fmt));
        EXPECT_THAT(r, FailedWith(scn::scan_error::invalid_format_string))
            << fmt;
    }
}

#endif  // !SCN_DISABLE_REGEX
