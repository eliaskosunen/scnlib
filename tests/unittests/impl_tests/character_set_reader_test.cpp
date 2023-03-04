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

#include <gtest/gtest.h>

#include <scn/impl/reader/string/character_set_reader.h>

class CharacterSetFormatParserTest : public ::testing::Test {
protected:
    testing::AssertionResult ParseAndSanitize(std::string_view fmt)
    {
        SCN_EXPECT(fmt.front() == '[');

        if (auto ret = parser.parse(fmt); !ret) {
            return testing::AssertionFailure() << "Parse failed";
        }
        else if (scn::detail::to_address(*ret) !=
                 scn::detail::to_address(fmt.end())) {
            return testing::AssertionFailure() << "Non-exhaustive parse";
        }

        if (!parser.sanitize()) {
            return testing::AssertionFailure() << "Sanitize failed";
        }
        return testing::AssertionSuccess();
    }

    testing::AssertionResult CheckAsciiChar(char ch)
    {
        if (!parser.check_code_point(scn::make_code_point(ch))) {
            return testing::AssertionFailure()
                   << "Char '" << ch << "' not accepted";
        }
        return testing::AssertionSuccess() << "Char '" << ch << "' accepted";
    }

    template <typename Result>
    testing::AssertionResult CheckReadResult(std::string_view source,
                                             int code_units_expected,
                                             Result result)
    {
        const auto expected_result =
            source.substr(0, static_cast<size_t>(code_units_expected));
        if (!result) {
            return testing::AssertionFailure() << "Read failed";
        }
        if (result->iterator != source.begin() + code_units_expected) {
            return testing::AssertionFailure() << "Returned iterator off";
        }
        if (result->value != expected_result) {
            return testing::AssertionFailure() << "Parsed value incorrect";
        }
        return testing::AssertionSuccess();
    }

    testing::AssertionResult CheckRead(std::string_view source,
                                       int code_units_expected,
                                       scn::detail::locale_ref loc = {})
    {
        auto reader = scn::impl::make_character_set_reader(parser);

        {
            auto ret = reader.read(source, loc);
            return CheckReadResult(source, code_units_expected, ret);
        }
    }

    scn::impl::character_set_classic_format_parser<char> parser{};
};

TEST_F(CharacterSetFormatParserTest, Upper_SpelledOut)
{
    EXPECT_TRUE(ParseAndSanitize("[A-Z]"));

    for (char ch = 'A'; ch != 'Z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }

    for (char ch = 'a'; ch != 'z' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
    for (char ch = '0'; ch != '9' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }

    EXPECT_TRUE(CheckRead("ABC", 3));
    EXPECT_TRUE(CheckRead("ABCd", 3));
}
TEST_F(CharacterSetFormatParserTest, Upper_ColonSpecifier)
{
    EXPECT_TRUE(ParseAndSanitize("[:upper:]"));

    for (char ch = 'A'; ch != 'Z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }

    for (char ch = 'a'; ch != 'z' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
    for (char ch = '0'; ch != '9' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
}

TEST_F(CharacterSetFormatParserTest, Alpha_SpelledOut)
{
    EXPECT_TRUE(ParseAndSanitize("[a-zA-Z]"));

    for (char ch = 'A'; ch != 'Z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }
    for (char ch = 'a'; ch != 'z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }

    for (char ch = '0'; ch != '9' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
    for (char ch = '['; ch != '`' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
}
TEST_F(CharacterSetFormatParserTest, Alpha_ColonSpecifier)
{
    EXPECT_TRUE(ParseAndSanitize("[:alpha:]"));

    for (char ch = 'A'; ch != 'Z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }
    for (char ch = 'a'; ch != 'z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }

    for (char ch = '0'; ch != '9' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
    for (char ch = '['; ch != '`' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }

    EXPECT_TRUE(CheckRead("ABC", 3));
    EXPECT_TRUE(CheckRead("ABCd", 4));
    EXPECT_TRUE(CheckRead("ABCd3", 4));
}
TEST_F(CharacterSetFormatParserTest, Alpha_ColonSpecifier_LowerAndUpper)
{
    EXPECT_TRUE(ParseAndSanitize("[:lower::upper:]"));

    for (char ch = 'A'; ch != 'Z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }
    for (char ch = 'a'; ch != 'z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }

    for (char ch = '0'; ch != '9' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
    for (char ch = '['; ch != '`' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
}
TEST_F(CharacterSetFormatParserTest, Alpha_BackslashSpecifier)
{
    EXPECT_TRUE(ParseAndSanitize("[\\l]"));

    for (char ch = 'A'; ch != 'Z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }
    for (char ch = 'a'; ch != 'z' + 1; ++ch) {
        EXPECT_TRUE(CheckAsciiChar(ch));
    }

    for (char ch = '0'; ch != '9' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
    for (char ch = '['; ch != '`' + 1; ++ch) {
        EXPECT_FALSE(CheckAsciiChar(ch));
    }
}
