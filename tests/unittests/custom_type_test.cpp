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

// Simple wrapper over an `int`, inherits all its scanning properties
struct integer_wrapper {
    int value{};
};

template <>
struct scn::scanner<integer_wrapper, char> : scn::scanner<int, char> {
    template <typename Context>
    scn::scan_expected<typename Context::iterator> scan(integer_wrapper& val,
                                                        Context& ctx) const
    {
        return scn::scanner<int, char>::scan(val.value, ctx);
    }
};

TEST(CustomTypeTest, IntegerWrapperWithDefaultFormatString)
{
    auto result = scn::scan<integer_wrapper>("123", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto val = result->value().value;
    EXPECT_EQ(val, 123);
}

TEST(CustomTypeTest, IntegerWrapperWithCustomFormatString)
{
    auto result = scn::scan<integer_wrapper>("123", "{:x}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto val = result->value().value;
    EXPECT_EQ(val, 0x123);
}

// Wrapper over a variant,
// with fully custom format string parsing
struct variant_wrapper {
    std::variant<int, char, double, std::string> value{};
};

template <>
struct scn::scanner<variant_wrapper, char> {
    template <typename ParseContext>
    constexpr scn::scan_expected<typename ParseContext::iterator> parse(
        ParseContext& pctx)
    {
        if (pctx.begin() == pctx.end() || *pctx.begin() == '}') {
            return scn::unexpected(pctx.on_error(
                "Invalid format string: format specifier required"));
        }

        auto it = pctx.begin();
        switch (*it) {
            case 'i':
                format = format_int;
                break;
            case 'c':
                format = format_char;
                break;
            case 'f':
                format = format_double;
                break;
            case 's':
                format = format_string;
                break;
            default:
                return scn::unexpected(pctx.on_error(
                    "Invalid format string: invalid format specifier"));
        }
        return ++it;
    }

    template <typename Context>
    scn::scan_expected<typename Context::iterator> scan(variant_wrapper& val,
                                                        Context& ctx) const
    {
        switch (format) {
            case format_int: {
                return scn::scanner<int, char>{}.scan(val.value.emplace<int>(),
                                                      ctx);
            }
            case format_char:
                return scn::scanner<char, char>{}.scan(
                    val.value.emplace<char>(), ctx);
            case format_double:
                return scn::scanner<double, char>{}.scan(
                    val.value.emplace<double>(), ctx);
            case format_string:
                return scn::scanner<std::string, char>{}.scan(
                    val.value.emplace<std::string>(), ctx);
        }

        SCN_UNREACHABLE;
    }

private:
    enum {
        format_int,
        format_char,
        format_double,
        format_string,
    } format{};
};

TEST(CustomTypeTest, VariantWrapperWithDefaultFormatString)
{
    auto result = scn::scan<variant_wrapper>("123", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}

TEST(CustomTypeTest, VariantWrapperWithIntegerFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:i}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<int>(val));
    EXPECT_EQ(std::get<int>(val), 123);
}

TEST(CustomTypeTest, VariantWrapperWithCharFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:c}");
    ASSERT_TRUE(result);
    EXPECT_STREQ(result->begin(), "23");

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<char>(val));
    EXPECT_EQ(std::get<char>(val), '1');
}

TEST(CustomTypeTest, VariantWrapperWithDoubleFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:f}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<double>(val));
    EXPECT_DOUBLE_EQ(std::get<double>(val), 123.0);
}

TEST(CustomTypeTest, VariantWrapperWithStringFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:s}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<std::string>(val));
    EXPECT_EQ(std::get<std::string>(val), "123");
}

TEST(CustomTypeTest, VariantWrapperInvalidFormat)
{
    auto result =
        scn::scan<variant_wrapper>("123", scn::runtime_format("{:d}"));
    ASSERT_FALSE(result);

    result = scn::scan<variant_wrapper>("123", scn::runtime_format("{}"));
    ASSERT_FALSE(result);
}
