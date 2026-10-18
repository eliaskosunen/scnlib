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

#include <scn/istream.h>
#include <scn/scan.h>

#include <array>
#include <cstdio>
#include <deque>
#include <locale>
#include <sstream>

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
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');
    EXPECT_EQ(result->value().value, 'c');
}

TEST(CustomTypeTest, CharWrapperWithDefaultFormatStringFromNonContiguousSource)
{
    auto src = std::deque<char>{'c'};
    auto result = scn::scan<char_wrapper>(src, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), src.end());
    EXPECT_EQ(result->value().value, 'c');
}

// Scanner whose `parse` returns an iterator outside of the format string
struct bad_parse_type {};

template <>
struct scn::scanner<bad_parse_type, char> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx&) -> typename ParseCtx::iterator
    {
        return typename ParseCtx::iterator{};
    }

    template <typename Context>
    auto scan(bad_parse_type&, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        return ctx.begin();
    }
};

TEST(CustomTypeTest, ParseReturningIteratorOutsideOfFormatString)
{
    auto result =
        scn::scan<bad_parse_type, int>("abc 1", scn::runtime_format("{:} {}"));
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}

TEST(CustomTypeTest, CharWrapperWithCustomFormatString)
{
    auto result = scn::scan<char_wrapper>("c", scn::runtime_format("{:c}"));
    ASSERT_THAT(result, Failed());
}

#if !SCN_DISABLE_LOCALE
// Records whether it was scanned with a locale, without consuming anything
struct locale_probe {
    bool has_locale{};
};

template <>
struct scn::scanner<locale_probe, char> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(locale_probe& val, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        val.has_locale = static_cast<bool>(ctx.locale());
        return ctx.begin();
    }
};

TEST(CustomTypeTest, LocaleWithSingleArgument)
{
    auto result = scn::scan<locale_probe>(std::locale::classic(), "abc", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(result->value().has_locale);
}

TEST(CustomTypeTest, LocaleWithSingleArgumentFromNonContiguousSource)
{
    auto src = std::deque<char>{'a', 'b', 'c'};
    auto result = scn::scan<locale_probe>(std::locale::classic(), src, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_TRUE(result->value().has_locale);
}

TEST(CustomTypeTest, NoLocaleWithSingleArgument)
{
    auto result = scn::scan<locale_probe>("abc", "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_FALSE(result->value().has_locale);
}
#endif

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
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');

    const auto val = result->value().value;
    EXPECT_EQ(val, 123);
}

TEST(CustomTypeTest,
     IntegerWrapperWithDefaultFormatStringFromNonContiguousSource)
{
    auto src = std::deque<char>{'1', '2', '3'};
    auto result = scn::scan<integer_wrapper>(src, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), src.end());

    const auto val = result->value().value;
    EXPECT_EQ(val, 123);
}

TEST(CustomTypeTest, IntegerWrapperWithCustomFormatString)
{
    auto result = scn::scan<integer_wrapper>("123", "{:x}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');

    const auto val = result->value().value;
    EXPECT_EQ(val, 0x123);
}

TEST(CustomTypeTest,
     IntegerWrapperWithCustomFormatStringFromNonContiguousSource)
{
    auto src = std::deque<char>{'1', '2', '3'};
    auto result = scn::scan<integer_wrapper>(src, "{:x}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), src.end());

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
#if SCN_HAS_EXCEPTIONS
            throw scn::scan_format_string_error(
                "Invalid format string: format specifier required");
#else
            pctx.on_error("Invalid format string: format specifier required");
            return pctx.begin();
#endif
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
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
}
#endif

TEST(CustomTypeTest, VariantWrapperWithIntegerFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<int>(val));
    EXPECT_EQ(std::get<int>(val), 123);
}

TEST(CustomTypeTest, VariantWrapperWithCharFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:c}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_STREQ(result->begin(), "23");

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<char>(val));
    EXPECT_EQ(std::get<char>(val), '1');
}

TEST(CustomTypeTest, VariantWrapperWithDoubleFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:f}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<double>(val));
    EXPECT_DOUBLE_EQ(std::get<double>(val), 123.0);
}

TEST(CustomTypeTest, VariantWrapperWithStringFormat)
{
    auto result = scn::scan<variant_wrapper>("123", "{:s}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(*result->begin(), '\0');

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<std::string>(val));
    EXPECT_EQ(std::get<std::string>(val), "123");
}

TEST(CustomTypeTest, VariantWrapperInvalidFormat)
{
    auto result =
        scn::scan<variant_wrapper>("123", scn::runtime_format("{:d}"));
    ASSERT_THAT(result, Failed());

    result = scn::scan<variant_wrapper>("123", scn::runtime_format("{}"));
    ASSERT_THAT(result, Failed());
}

TEST(CustomTypeTest, VariantWrapperFromNonContiguousSource)
{
    auto src = std::deque<char>{'1', '2', '3'};
    auto result = scn::scan<variant_wrapper>(src, "{:i}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->begin(), src.end());

    const auto& val = result->value().value;
    ASSERT_TRUE(std::holds_alternative<int>(val));
    EXPECT_EQ(std::get<int>(val), 123);
}

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
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().val, 123);
}

TEST(CustomTypeTest, NonCopyableTypeFromNonContiguousSource)
{
    auto result =
        scn::scan<non_copyable_type>(std::deque<char>{'1', '2', '3'}, "{}");
    ASSERT_THAT(result, Succeeded());
    EXPECT_EQ(result->value().val, 123);
}

struct two_errors_type {};

template <>
struct scn::scanner<two_errors_type> {
    template <typename ParseContext>
    constexpr typename ParseContext::iterator parse(ParseContext& pctx)
    {
        pctx.on_error("First error");
        pctx.on_error("Second error");
        return pctx.begin();
    }

    template <typename Context>
    auto scan(two_errors_type&, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        return ctx.begin();
    }
};

TEST(CustomTypeTest, ParseContextKeepsFirstError)
{
    auto result = scn::scan<two_errors_type>("", scn::runtime_format("{}"));
    ASSERT_THAT(result, FailedWith(scn::scan_error::invalid_format_string));
    EXPECT_STREQ(result.error().msg(), "First error");
}

// Scanner that advances the iterator by `Advance` characters
// without checking the source
template <int Advance>
struct advancing_type {
    int unused{};
};

template <int Advance>
struct scn::scanner<advancing_type<Advance>, char> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(advancing_type<Advance>&, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        auto it = ctx.begin();
        for (int i = 0; i < Advance; ++i) {
            ++it;
        }
        return it;
    }
};

using overrunning_type = advancing_type<100>;
using exact_type = advancing_type<3>;

TEST(CustomTypeTest, OverrunningIteratorContiguous)
{
    constexpr std::array<const char*, 2> formats{"{}", "{:}"};
    for (auto fmt : formats) {
        auto r = scn::scan<overrunning_type>("abc", scn::runtime_format(fmt));
        ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state))
            << fmt;
    }
    auto r = scn::scan<overrunning_type, int>("abc 1", "{} {}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state));
}

TEST(CustomTypeTest, OverrunningIteratorNonContiguous)
{
    constexpr std::array<const char*, 2> formats{"{}", "{:}"};
    for (auto fmt : formats) {
        auto src = std::deque<char>{'a', 'b', 'c'};
        auto r = scn::scan<overrunning_type>(src, scn::runtime_format(fmt));
        ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state))
            << fmt;
    }
    auto src = std::deque<char>{'a', 'b', 'c', ' ', '1'};
    auto r = scn::scan<overrunning_type, int>(src, "{} {}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state));
}

#if !SCN_DISABLE_IOSTREAM
TEST(CustomTypeTest, OverrunningIteratorIstream)
{
    constexpr std::array<const char*, 2> formats{"{}", "{:}"};
    for (auto fmt : formats) {
        std::istringstream ss{"abc"};
        auto r = scn::scan<overrunning_type>(ss, scn::runtime_format(fmt));
        ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state))
            << fmt;
    }
    std::istringstream ss{"abc 1"};
    auto r = scn::scan<overrunning_type, int>(ss, "{} {}");
    ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state));
}
#endif

TEST(CustomTypeTest, OverrunningIteratorScanFile)
{
    constexpr std::array<const char*, 2> formats{"{}", "{:}"};
    for (auto fmt : formats) {
        auto f = std::tmpfile();
        ASSERT_NE(f, nullptr);
        std::fputs("abc def", f);
        std::rewind(f);
        {
            scn::scan_file file{f};
            auto r =
                scn::scan<overrunning_type>(file, scn::runtime_format(fmt));
            ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state))
                << fmt;
        }
        std::fclose(f);
    }
}

TEST(CustomTypeTest, IteratorAtEndOfSourceIsAcceptedContiguous)
{
    constexpr std::array<const char*, 2> formats{"{}", "{:}"};
    for (auto fmt : formats) {
        auto r = scn::scan<exact_type>("abc", scn::runtime_format(fmt));
        ASSERT_THAT(r, Succeeded()) << fmt;
        EXPECT_EQ(r->begin(), r->end());
    }
}

TEST(CustomTypeTest, IteratorAtEndOfSourceIsAcceptedNonContiguous)
{
    constexpr std::array<const char*, 2> formats{"{}", "{:}"};
    for (auto fmt : formats) {
        auto src = std::deque<char>{'a', 'b', 'c'};
        auto r = scn::scan<exact_type>(src, scn::runtime_format(fmt));
        ASSERT_THAT(r, Succeeded()) << fmt;
        EXPECT_EQ(r->begin(), r->end());
    }
}

#if !SCN_DISABLE_IOSTREAM
TEST(CustomTypeTest, IteratorAtEndOfSourceIsAcceptedIstream)
{
    std::istringstream ss{"abc"};
    auto r = scn::scan<exact_type>(ss, "{}");
    ASSERT_THAT(r, Succeeded());
}
#endif

// Scanner that steps the iterator back before the start of its field
struct stepping_back_type {};

template <>
struct scn::scanner<stepping_back_type, char> {
    template <typename ParseCtx>
    constexpr auto parse(ParseCtx& pctx) -> typename ParseCtx::iterator
    {
        return pctx.begin();
    }

    template <typename Context>
    auto scan(stepping_back_type&, Context& ctx) const
        -> scn::scan_expected<typename Context::iterator>
    {
        auto it = ctx.begin();
        --it;
        --it;
        return it;
    }
};

TEST(CustomTypeTest, IteratorBeforeStartOfFieldNonContiguous)
{
    constexpr std::array<const char*, 2> formats{"{}{}{}", "{}{:}{}"};
    for (auto fmt : formats) {
        auto src = std::deque<char>{'1', '2', '3', ' ', '4', '5', '6'};
        auto r = scn::scan<int, stepping_back_type, int>(
            src, scn::runtime_format(fmt));
        ASSERT_THAT(r, FailedWith(scn::scan_error::invalid_source_state))
            << fmt;
    }
}
