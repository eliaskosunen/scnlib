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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <scn/detail/format_string_parser.h>

TEST(FormatStringParserTest, DefaultConstructedSpecs)
{
    auto specs = scn::detail::basic_format_specs<char>{};
    EXPECT_EQ(specs.width, 0);
    EXPECT_EQ(specs.fill, " ");
    EXPECT_EQ(specs.type, scn::detail::presentation_type::none);
    EXPECT_EQ(specs.arbitrary_base, 0);
    EXPECT_EQ(specs.align, scn::detail::align_type::none);
    EXPECT_EQ(specs.localized, false);
    EXPECT_EQ(specs.thsep, false);
}

TEST(FormatStringParserTest, ParsePresentationType)
{
    EXPECT_EQ(scn::detail::parse_presentation_type('i'),
              scn::detail::presentation_type::int_generic);
    EXPECT_EQ(scn::detail::parse_presentation_type('B'),
              scn::detail::presentation_type::int_arbitrary_base);
    EXPECT_EQ(scn::detail::parse_presentation_type('a'),
              scn::detail::parse_presentation_type('A'));
    EXPECT_EQ(scn::detail::parse_presentation_type('e'),
              scn::detail::parse_presentation_type(L'E'));
    EXPECT_EQ(scn::detail::parse_presentation_type('z'),
              scn::detail::presentation_type::none);
}

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        static inline bool operator==(const basic_format_specs<char>& a,
                                      const basic_format_specs<char>& b)
        {
            return a.width == b.width && a.fill == b.fill && a.type == b.type &&
                   a.arbitrary_base == b.arbitrary_base && a.align == b.align &&
                   a.localized == b.localized && a.thsep == b.thsep;
        }
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn

struct mock_specs_setter : public scn::detail::specs_setter<char> {
    mock_specs_setter(scn::detail::basic_format_specs<char>& specs)
        : scn::detail::specs_setter<char>{specs}
    {
    }

    void on_error(const char* msg)
    {
        latest_error = msg;
    }

    const char* latest_error{nullptr};
};

class FormatStringParserAlignTest : public testing::Test {
protected:
    scn::detail::basic_format_specs<char> specs{};
    mock_specs_setter handler{specs};
};

TEST_F(FormatStringParserAlignTest, NoAlignNoFill)
{
    std::string_view input{"}"};
    auto result = scn::detail::parse_align(input.begin(), input.end(), handler);
    EXPECT_EQ(specs.fill, " ");
    EXPECT_EQ(specs.align, scn::detail::align_type::none);
    EXPECT_EQ(specs, scn::detail::basic_format_specs<char>{});
    EXPECT_EQ(result, input.begin());
    EXPECT_EQ(handler.latest_error, nullptr);
}

TEST_F(FormatStringParserAlignTest, LeftAlignNoFill)
{
    std::string_view input{"<}"};
    auto result = scn::detail::parse_align(input.begin(), input.end(), handler);
    EXPECT_EQ(specs.fill, " ");
    EXPECT_EQ(specs.align, scn::detail::align_type::left);
    EXPECT_EQ(result, input.begin() + 1);
    EXPECT_EQ(handler.latest_error, nullptr);
}

TEST_F(FormatStringParserAlignTest, RightAlignWithFill)
{
    std::string_view input{"_>}"};
    auto result = scn::detail::parse_align(input.begin(), input.end(), handler);
    EXPECT_EQ(specs.fill, "_");
    EXPECT_EQ(specs.align, scn::detail::align_type::right);
    EXPECT_EQ(result, input.begin() + 2);
    EXPECT_EQ(handler.latest_error, nullptr);
}

TEST_F(FormatStringParserAlignTest, InvalidFillCharacter)
{
    std::string_view input{"{^}"};
    std::ignore = scn::detail::parse_align(input.begin(), input.end(), handler);
    EXPECT_EQ(specs, scn::detail::basic_format_specs<char>{});
    EXPECT_NE(handler.latest_error, nullptr);
}

class FormatStringParserWidthTest : public ::testing::Test {
    // TODO
};

TEST_F(FormatStringParserWidthTest, Test)
{
    SUCCEED();
}

class FormatStringParserFormatSpecsTest : public ::testing::Test {
protected:
    scn::detail::basic_format_specs<char> specs{};
    mock_specs_setter handler{specs};
};

TEST_F(FormatStringParserFormatSpecsTest, EmptySpecs)
{
    std::string_view input{"}"};
    auto result =
        scn::detail::parse_format_specs(input.begin(), input.end(), handler);
    EXPECT_EQ(result, input.begin());
    EXPECT_EQ(specs, scn::detail::basic_format_specs<char>{});
    EXPECT_EQ(handler.latest_error, nullptr);
}

TEST_F(FormatStringParserFormatSpecsTest, Localized)
{
    std::string_view input{"L}"};
    auto result =
        scn::detail::parse_format_specs(input.begin(), input.end(), handler);
    EXPECT_EQ(result, input.begin() + 1);
    EXPECT_EQ(specs.localized, true);
    EXPECT_EQ(handler.latest_error, nullptr);
}
