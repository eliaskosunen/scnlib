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

// Simple wrapper over a `char`, doesn't allow a custom format string
struct char_wrapper {
    char value{};
};

template <>
struct scn::scanner<char_wrapper, char> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(char_wrapper& val, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        return scn::scan<char>(ctx.range(), "{}")
            .transform([&val](const auto& result) noexcept {
                val.value = result.value();
                return result.begin();
            });
    }
};

TEST(CustomTypeTest, CharWrapperWithDefaultFormatString)
{
    auto result = scn::scan<char_wrapper>("c", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(*result->begin(), '\0');
    EXPECT_EQ(result->value().value, 'c');
}

TEST(CustomTypeTest, CharWrapperWithCustomFormatString)
{
    auto result = scn::scan<char_wrapper>("c", scn::runtime_format("{:c}"));
    ASSERT_FALSE(result);
}

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
    constexpr typename ParseContext::iterator parse(ParseContext& pctx)
    {
        if (pctx.begin() == pctx.end() || *pctx.begin() == '}') {
            throw scn::scan_format_string_error(
                "Invalid format string: format specifier required");
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
                pctx.on_error(
                    "Invalid format string: invalid format specifier");
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

                SCN_CLANG_PUSH
                SCN_CLANG_IGNORE("-Wcovered-switch-default")
            default:
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
                SCN_CLANG_POP
        }
    }

private:
    enum {
        format_int,
        format_char,
        format_double,
        format_string,
    } format{};
};

#if !SCN_HAS_CONSTEVAL
TEST(CustomTypeTest, VariantWrapperWithDefaultFormatString)
{
    auto result = scn::scan<variant_wrapper>("123", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), scn::scan_error::invalid_format_string);
}
#endif

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

struct type_without_default_constructor {
    type_without_default_constructor() = delete;
    explicit type_without_default_constructor(int v) : val(v) {}

    int val;
};

template <>
struct scn::scanner<type_without_default_constructor> : scn::scanner<int> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx)
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(type_without_default_constructor& val, Context& ctx) const
        -> scan_expected<typename Context::iterator>
    {
        return scn::scanner<int>::scan(val.val, ctx);
    }
};

#if 0
TEST(CustomTypeTest, TypeWithoutDefaultConstructor)
{
    auto result = scn::scan<type_without_default_constructor>("123", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().val, 123);
}
#endif

struct non_copyable_type {
    non_copyable_type() = default;
    explicit non_copyable_type(int v) : val(v) {}

    non_copyable_type(const non_copyable_type&) = delete;
    non_copyable_type& operator=(const non_copyable_type&) = delete;

    non_copyable_type(non_copyable_type&&) = default;
    non_copyable_type& operator=(non_copyable_type&&) = default;

    int val;
};

template <>
struct scn::scanner<non_copyable_type> : scn::scanner<int> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx)
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(non_copyable_type& val, Context& ctx) const
        -> scan_expected<typename Context::iterator>
    {
        return scn::scanner<int>::scan(val.val, ctx);
    }
};

TEST(CustomTypeTest, NonCopyableType)
{
    auto result = scn::scan<non_copyable_type>("123", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().val, 123);
}

struct non_movable_type {
    non_movable_type() = default;
    explicit non_movable_type(int v) : val(v) {}

    non_movable_type(const non_movable_type&) = delete;
    non_movable_type& operator=(const non_movable_type&) = delete;

    non_movable_type(non_movable_type&&) = delete;
    non_movable_type& operator=(non_movable_type&&) = delete;

    int val;
};

#if 0
template <>
struct scn::scanner<non_movable_type> : scn::scanner<int> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx)
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(non_movable_type& val, Context& ctx) const
        -> scan_expected<typename Context::iterator>
    {
        return scn::scanner<int>::scan(val.val, ctx);
    }
};

TEST(CustomTypeTest, NonMovableType)
{
    auto result = scn::scan<non_movable_type>("123", "{}");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value().val, 123);
}
#endif
