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

#include "wrapped_gtest.h"

#include <scn/scan.h>

TEST(FormatStringParserTest, DefaultConstructedSpecs)
{
    auto specs = scn::detail::format_specs{};
    EXPECT_EQ(specs.width, 0);
    EXPECT_EQ(specs.type, scn::detail::presentation_type::none);
    EXPECT_EQ(specs.arbitrary_base, 0);
    EXPECT_EQ(specs.localized, false);
}

TEST(FormatStringParserTest, ParsePresentationType)
{
    EXPECT_EQ(scn::detail::parse_presentation_type('i'),
              scn::detail::presentation_type::int_generic);
    EXPECT_EQ(scn::detail::parse_presentation_type('r'),
              scn::detail::presentation_type::int_arbitrary_base);
    EXPECT_EQ(scn::detail::parse_presentation_type('a'),
              scn::detail::parse_presentation_type('A'));
    EXPECT_EQ(scn::detail::parse_presentation_type('e'),
              scn::detail::parse_presentation_type(L'E'));
    EXPECT_EQ(scn::detail::parse_presentation_type('U'),
              scn::detail::presentation_type::unicode);
    EXPECT_EQ(scn::detail::parse_presentation_type('?'),
              scn::detail::presentation_type::escaped_character);
    EXPECT_EQ(scn::detail::parse_presentation_type('z'),
              scn::detail::presentation_type::none);
}

namespace scn {
SCN_BEGIN_NAMESPACE

namespace detail {
static inline bool operator==(const format_specs& a, const format_specs& b)
{
    return a.width == b.width && a.type == b.type &&
           a.arbitrary_base == b.arbitrary_base && a.localized == b.localized;
}
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn

struct mock_specs_setter : public scn::detail::specs_setter {
    constexpr mock_specs_setter(scn::detail::format_specs& specs) noexcept
        : scn::detail::specs_setter(specs)
    {
    }

    void on_error(const char* msg)
    {
        latest_error = msg;
    }

    const char* latest_error{nullptr};
};

class FormatStringParserWidthTest : public ::testing::Test {
protected:
    scn::detail::format_specs specs{};
    mock_specs_setter handler{specs};
};

TEST_F(FormatStringParserWidthTest, WidthTooLarge)
{
    std::string_view input{"9999999999999999999999999999999999}"};
    std::ignore = scn::detail::parse_width(
        input.data(), input.data() + input.size(), handler);
    ASSERT_NE(handler.latest_error, nullptr);
}

TEST_F(FormatStringParserWidthTest, WidthZero)
{
    std::string_view input{"0}"};
    std::ignore = scn::detail::parse_width(
        input.data(), input.data() + input.size(), handler);
    ASSERT_NE(handler.latest_error, nullptr);
}

TEST_F(FormatStringParserWidthTest, WidthWithLeadingZero)
{
    std::string_view input{"05}"};
    std::ignore = scn::detail::parse_width(
        input.data(), input.data() + input.size(), handler);
    ASSERT_NE(handler.latest_error, nullptr);
    EXPECT_EQ(specs.width, 0);
}

class FormatStringParserPrecisionTest : public ::testing::Test {
protected:
    scn::detail::format_specs specs{};
    mock_specs_setter handler{specs};
};

TEST_F(FormatStringParserPrecisionTest, PrecisionWithoutDigits)
{
    std::string_view input{"}"};
    std::ignore = scn::detail::parse_precision(
        input.data(), input.data() + input.size(), handler);
    ASSERT_NE(handler.latest_error, nullptr);
}

TEST_F(FormatStringParserPrecisionTest, Precision)
{
    std::string_view input{"12}"};
    std::ignore = scn::detail::parse_precision(
        input.data(), input.data() + input.size(), handler);
    EXPECT_EQ(handler.latest_error, nullptr);
    EXPECT_EQ(specs.precision, 12);
}

TEST_F(FormatStringParserPrecisionTest, PrecisionWithLeadingZero)
{
    std::string_view input{"012}"};
    std::ignore = scn::detail::parse_precision(
        input.data(), input.data() + input.size(), handler);
    ASSERT_NE(handler.latest_error, nullptr);
    EXPECT_EQ(specs.precision, 0);
}

class FormatStringParserFormatSpecsTest : public ::testing::Test {
protected:
    scn::detail::format_specs specs{};
    mock_specs_setter handler{specs};
};

TEST_F(FormatStringParserFormatSpecsTest, EmptySpecs)
{
    std::string_view input{"}"};
    auto result = scn::detail::parse_format_specs(
        input.data(), input.data() + input.size(), handler);
    EXPECT_EQ(result, input.data());
    EXPECT_EQ(specs, scn::detail::format_specs{});
    EXPECT_EQ(handler.latest_error, nullptr);
}

#if !SCN_DISABLE_LOCALE
TEST_F(FormatStringParserFormatSpecsTest, Localized)
{
    std::string_view input{"L}"};
    auto result = scn::detail::parse_format_specs(
        input.data(), input.data() + input.size(), handler);
    EXPECT_EQ(result, input.data() + 1);
    EXPECT_EQ(specs.localized, true);
    EXPECT_EQ(handler.latest_error, nullptr);
}
#endif
