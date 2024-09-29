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

#pragma once

#include "reader_test_common.h"

#include <scn/impl.h>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wheader-hygiene")

using namespace std::string_view_literals;

SCN_CLANG_POP

template <bool Localized, typename CharT, typename ValueT>
using int_reader_wrapper =
    reader_wrapper<Localized, CharT, ValueT, scn::impl::reader_impl_for_int>;

template <typename T>
class IntValueReaderTest : public testing::Test {
protected:
    using char_type = typename T::char_type;
    using int_type = typename T::value_type;
    using string_type = std::basic_string<char_type>;
    using string_view_type = std::basic_string_view<char_type>;

    static constexpr bool is_wide = std::is_same_v<char_type, wchar_t>;
    static constexpr bool is_localized = T::is_localized;

    template <typename Source>
    void set_source(Source&& s)
    {
        if constexpr (!is_wide) {
            widened_source = std::string{SCN_FWD(s)};
        }
        else if constexpr (std::is_same_v<
                               scn::detail::remove_cvref_t<
                                   scn::ranges::range_value_t<Source>>,
                               wchar_t>) {
            widened_source = std::wstring{SCN_FWD(s)};
        }
        else {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wconversion")
            auto sv = std::string_view{s};
            widened_source = std::wstring(sv.size(), L'\0');
            std::copy(sv.begin(), sv.end(), widened_source->begin());
            SCN_GCC_POP
        }
    }

    static auto get_zero()
    {
        return std::make_pair(int_type{0}, "0"sv);
    }
    static auto get_basic()
    {
        return std::make_pair(int_type{123}, "123"sv);
    }

    static constexpr bool has_neg()
    {
        return std::is_signed_v<int_type>;
    }
    static auto get_neg()
    {
        if constexpr (std::is_signed_v<int_type>) {
            return std::make_pair(int_type{-123}, "-123"sv);
        }
        else {
            return std::make_pair(int_type{}, "-123"sv);
        }
    }

    static auto get_hex()
    {
        return std::make_pair(int_type{0x7f}, "7f"sv);
    }
    static auto get_hex_prefixed()
    {
        return std::make_pair(int_type{0x7f}, "0x7f"sv);
    }

    static auto get_oct()
    {
        return std::make_pair(int_type{077}, "77"sv);
    }
    static auto get_oct_prefixed()
    {
        return std::make_pair(int_type{077}, "077"sv);
    }
    static auto get_oct_prefixed_alt()
    {
        return std::make_pair(int_type{077}, "0o77"sv);
    }
    static auto get_oct_followed_by_dec()
    {
        return std::make_pair(int_type{8}, "08"sv);
    }

    static auto get_bin()
    {
        return std::make_pair(int_type{5}, "101"sv);
    }
    static auto get_bin_prefixed()
    {
        return std::make_pair(int_type{5}, "0b101"sv);
    }

    static auto get_ternary()
    {
        return std::make_pair(int_type{5}, "12"sv);
    }

    static std::string format_int(int_type val)
    {
        std::ostringstream oss;
        if constexpr (sizeof(int_type) < sizeof(int)) {
            oss << static_cast<int>(val);
        }
        else {
            oss << val;
        }
        return oss.str();
    }

    static constexpr auto get_max_value()
    {
        return (std::numeric_limits<int_type>::max)();
    }

    static auto get_max()
    {
        constexpr auto val = (std::numeric_limits<int_type>::max)();
        return std::make_pair(val, format_int(val));
    }
    static auto get_min()
    {
        constexpr auto val = (std::numeric_limits<int_type>::min)();
        return std::make_pair(val, format_int(val));
    }

    static std::string_view get_overflow()
    {
        if constexpr (std::is_signed_v<int_type>) {
            if constexpr (sizeof(int_type) == 1) {
                return "128";
            }
            else if constexpr (sizeof(int_type) == 2) {
                return "32768";
            }
            else if constexpr (sizeof(int_type) == 4) {
                return "2147483648";
            }
            else if constexpr (sizeof(int_type) == 8) {
                return "9223372036854775808";
            }
        }
        else {
            if constexpr (sizeof(int_type) == 1) {
                return "256";
            }
            else if constexpr (sizeof(int_type) == 2) {
                return "65536";
            }
            else if constexpr (sizeof(int_type) == 4) {
                return "4294967296";
            }
            else if constexpr (sizeof(int_type) == 8) {
                return "18446744073709551616";
            }
        }
    }

    static constexpr bool has_underflow()
    {
        return std::is_signed_v<int_type>;
    }
    static std::string_view get_underflow()
    {
        if constexpr (std::is_signed_v<int_type>) {
            if constexpr (sizeof(int_type) == 1) {
                return "-129";
            }
            else if constexpr (sizeof(int_type) == 2) {
                return "-32769";
            }
            else if constexpr (sizeof(int_type) == 4) {
                return "-2147483649";
            }
            else if constexpr (sizeof(int_type) == 8) {
                return "-9223372036854775809";
            }
        }
        else {
            return "";
        }
    }

    static constexpr bool has_four_digits()
    {
        return static_cast<uint64_t>(std::numeric_limits<int_type>::max()) >=
               1234ull;
    }
    static auto get_four_digits()
    {
        if constexpr (has_four_digits()) {
            return std::make_pair(int_type{1234}, "1234"sv);
        }
        else {
            return std::make_pair(get_max_value(), "1234"sv);
        }
    }

    static constexpr bool has_eight_digits()
    {
        return static_cast<uint64_t>(std::numeric_limits<int_type>::max()) >=
               12345678ull;
    }
    static auto get_eight_digits()
    {
        if constexpr (has_eight_digits()) {
            return std::make_pair(int_type{12345678}, "12345678"sv);
        }
        else {
            return std::make_pair(get_max_value(), "12345678"sv);
        }
    }

    static constexpr bool has_nine_digits()
    {
        return static_cast<uint64_t>(std::numeric_limits<int_type>::max()) >=
               123456789ull;
    }
    static auto get_nine_digits()
    {
        if constexpr (has_nine_digits()) {
            return std::make_pair(int_type{123456789}, "123456789"sv);
        }
        else {
            return std::make_pair(get_max_value(), "123456789"sv);
        }
    }

    static constexpr bool has_sixteen_digits()
    {
        return static_cast<uint64_t>(std::numeric_limits<int_type>::max()) >=
               1122334455667788ull;
    }
    static auto get_sixteen_digits()
    {
        auto s = "1122334455667788"sv;
        if constexpr (has_sixteen_digits()) {
            return std::make_pair(int_type{1122334455667788}, s);
        }
        else {
            return std::make_pair(get_max_value(), s);
        }
    }

    static constexpr bool has_seventeen_digits()
    {
        return static_cast<uint64_t>(std::numeric_limits<int_type>::max()) >=
               11223344556677889ull;
    }
    static auto get_seventeen_digits()
    {
        auto s = "11223344556677889"sv;
        if constexpr (has_seventeen_digits()) {
            return std::make_pair(int_type{11223344556677889}, s);
        }
        else {
            return std::make_pair(get_max_value(), s);
        }
    }

    template <bool IsAllowed, typename Digits>
    testing::AssertionResult digits_test(const Digits& digits)
    {
        if constexpr (IsAllowed) {
            const auto [val, src] = digits;
            return simple_default_test(src, val);
        }
        else {
            const auto [of_value, src] = digits;
            const auto [result, val] = simple_test(src);
            return check_failure_with_code(
                result, val, scn::scan_error::value_positive_overflow);
        }
    }

    static constexpr bool has_thsep_value()
    {
        return static_cast<uint64_t>(std::numeric_limits<int_type>::max()) >=
               123456ull;
    }
    static auto get_thsep_value()
    {
        if constexpr (has_thsep_value()) {
            return int_type{123456};
        }
        else {
            return get_max_value();
        }
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_generic_success(
        const Result& result) const
    {
        if (!result) {
            return testing::AssertionFailure()
                   << "Result not good: code " << result.error().code();
        }
        SCN_EXPECT(this->widened_source.has_value());
        if (scn::detail::to_address(result.value()) !=
            scn::detail::to_address(this->widened_source->end())) {
            return testing::AssertionFailure()
                   << "Result range not correct: diff "
                   << std::distance(
                          scn::detail::to_address(result.value()),
                          scn::detail::to_address(this->widened_source->end()));
        }
        return testing::AssertionSuccess();
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_value_success(
        const Result& result,
        int_type val,
        int_type expected) const
    {
        if (auto a = check_generic_success(result); !a) {
            return a;
        }
        if (val != expected) {
            return testing::AssertionFailure() << "Ints not equal: Got " << val
                                               << ", expected " << expected;
        }
        return testing::AssertionSuccess();
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_failure_with_code(
        const Result& result,
        int_type val,
        enum scn::scan_error::code c) const
    {
        if (result) {
            return testing::AssertionFailure()
                   << "Result good, expected failure";
        }
        if (result.error().code() != c) {
            return testing::AssertionFailure()
                   << "Result failed with wrong error code: "
                   << result.error().code() << ", expected " << c;
        }
        if (val != 0) {
            return testing::AssertionFailure()
                   << "Ints not equal: Got " << val << ", expected 0";
        }
        return testing::AssertionSuccess();
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_failure_with_code_and_value(
        const Result& result,
        int_type val,
        enum scn::scan_error::code c,
        int_type expected_value) const
    {
        if (result) {
            return testing::AssertionFailure()
                   << "Result good, expected failure";
        }
        if (result.error().code() != c) {
            return testing::AssertionFailure()
                   << "Result failed with wrong error code: "
                   << result.error().code() << ", expected " << c;
        }
        if (val != expected_value) {
            return testing::AssertionFailure()
                   << "Ints not equal: Got " << val << ", expected "
                   << expected_value;
        }
        return testing::AssertionSuccess();
    }

    template <typename Source>
    auto simple_test(Source&& source)
    {
        this->set_source(SCN_FWD(source));

        int_type val{};
        auto result =
            this->wrapped_reader.read_default(widened_source.value(), val);
        return std::make_pair(result, val);
    }
    template <typename Source>
    auto simple_specs_test(Source&& source,
                           const scn::detail::format_specs& specs)
    {
        return simple_specs_and_locale_test(SCN_FWD(source), specs, {});
    }
    template <typename Source>
    auto simple_specs_and_locale_test(Source&& source,
                                      const scn::detail::format_specs& specs,
                                      scn::detail::locale_ref loc)
    {
        this->set_source(SCN_FWD(source));

        int_type val{};
        auto result = this->wrapped_reader.read_specs_with_locale(
            widened_source.value(), specs, val, loc);
        return std::make_pair(result, val);
    }

    template <typename Source>
    auto simple_success_test(Source&& source)
    {
        this->set_source(SCN_FWD(source));

        int_type val{};
        auto result =
            this->wrapped_reader.read_default(widened_source.value(), val);
        return std::make_tuple(this->check_generic_success(result), result,
                               val);
    }
    template <typename Source>
    auto simple_success_specs_test(Source&& source,
                                   const scn::detail::format_specs& specs)
    {
        return simple_success_specs_and_locale_test(SCN_FWD(source), specs, {});
    }
    template <typename Source>
    auto simple_success_specs_and_locale_test(
        Source&& source,
        const scn::detail::format_specs& specs,
        scn::detail::locale_ref loc)
    {
        this->set_source(SCN_FWD(source));

        int_type val{};
        auto result = this->wrapped_reader.read_specs_with_locale(
            widened_source.value(), specs, val, loc);
        return std::make_tuple(this->check_generic_success(result), result,
                               val);
    }

    template <typename Source>
    [[nodiscard]] testing::AssertionResult simple_default_test(
        Source&& source,
        int_type expected_output)
    {
        auto [result, val] = simple_test(SCN_FWD(source));
        return check_value_success(result, val, expected_output);
    }

    scn::detail::format_specs make_format_specs_with_presentation_and_base(
        scn::detail::presentation_type type,
        uint8_t arb_base = 0,
        bool thsep = false) const
    {
        scn::detail::format_specs specs{};
        specs.type = type;
        SCN_GCC_PUSH
        SCN_GCC_IGNORE("-Wconversion")
        specs.arbitrary_base = arb_base;
        specs.localized = thsep;
        SCN_GCC_POP
        return specs;
    }

    T wrapped_reader{};
    std::optional<string_type> widened_source;
};

TYPED_TEST_SUITE_P(IntValueReaderTest);

TYPED_TEST_P(IntValueReaderTest, Zero)
{
    const auto [val, src] = this->get_zero();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

TYPED_TEST_P(IntValueReaderTest, Basic)
{
    const auto [val, src] = this->get_basic();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

TYPED_TEST_P(IntValueReaderTest, Negative)
{
    if constexpr (TestFixture::has_neg()) {
        const auto [val, src] = this->get_neg();
        EXPECT_TRUE(this->simple_default_test(src, val));
    }
    else {
        const auto [_, src] = this->get_neg();
        const auto [result, val] = this->simple_test(src);
        EXPECT_TRUE(this->check_failure_with_code(
            result, val, scn::scan_error::invalid_scanned_value));
        SCN_UNUSED(_);
    }
}

TYPED_TEST_P(IntValueReaderTest, Hex)
{
    const auto [orig_val, src] = this->get_hex();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_hex));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}
TYPED_TEST_P(IntValueReaderTest, HexDetect)
{
    const auto [orig_val, src] = this->get_hex_prefixed();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_generic));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}

TYPED_TEST_P(IntValueReaderTest, Oct)
{
    const auto [orig_val, src] = this->get_oct();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_octal));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}
TYPED_TEST_P(IntValueReaderTest, OctDetect)
{
    const auto [orig_val, src] = this->get_oct_prefixed();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_generic));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}
TYPED_TEST_P(IntValueReaderTest, OctAltDefault)
{
    const auto [_, src] = this->get_oct_prefixed_alt();
    const auto [result, val] = this->simple_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::none));
    EXPECT_TRUE(result);
    EXPECT_EQ(val, 0);
}
TYPED_TEST_P(IntValueReaderTest, OctAltDetected)
{
    const auto [orig_val, src] = this->get_oct_prefixed_alt();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_generic));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}
TYPED_TEST_P(IntValueReaderTest, OctFollowedByDec)
{
    const auto [_, src] = this->get_oct_followed_by_dec();
    const auto [result, val] = this->simple_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_octal));
    ASSERT_TRUE(result);
    EXPECT_NE(scn::detail::to_address(result.value()),
              scn::detail::to_address(this->widened_source->end()));
    EXPECT_EQ(val, 0);
}
TYPED_TEST_P(IntValueReaderTest, OctFollowedByDecDefault)
{
    const auto [orig_val, src] = this->get_oct_followed_by_dec();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::none));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}
TYPED_TEST_P(IntValueReaderTest, OctFollowedByDecDetected)
{
    const auto [_, src] = this->get_oct_followed_by_dec();
    const auto [result, val] = this->simple_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_generic));
    ASSERT_TRUE(result);
    EXPECT_NE(scn::detail::to_address(result.value()),
              scn::detail::to_address(this->widened_source->end()));
    EXPECT_EQ(val, 0);
}

TYPED_TEST_P(IntValueReaderTest, Bin)
{
    const auto [orig_val, src] = this->get_bin();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_binary));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}
TYPED_TEST_P(IntValueReaderTest, BinDetect)
{
    const auto [orig_val, src] = this->get_bin_prefixed();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_generic));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}

TYPED_TEST_P(IntValueReaderTest, Ternary)
{
    const auto [orig_val, src] = this->get_ternary();
    const auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation_and_base(
                 scn::detail::presentation_type::int_arbitrary_base, 3));
    EXPECT_TRUE(a);
    EXPECT_EQ(val, orig_val);
}

TYPED_TEST_P(IntValueReaderTest, Min)
{
    const auto [val, src] = this->get_min();
    EXPECT_TRUE(this->simple_default_test(src, val));
}
TYPED_TEST_P(IntValueReaderTest, Max)
{
    const auto [val, src] = this->get_max();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

TYPED_TEST_P(IntValueReaderTest, Overflow)
{
    const auto src = this->get_overflow();
    const auto [result, val] = this->simple_test(src);
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::value_positive_overflow));
}
TYPED_TEST_P(IntValueReaderTest, Underflow)
{
    if (!this->has_underflow()) {
        return SUCCEED() << "No Underflow-test for unsigned types";
    }

    const auto src = this->get_underflow();
    const auto [result, val] = this->simple_test(src);
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::value_negative_overflow));
}

TYPED_TEST_P(IntValueReaderTest, FourDigits)
{
    EXPECT_TRUE(this->template digits_test<TestFixture::has_four_digits()>(
        this->get_four_digits()));
}
TYPED_TEST_P(IntValueReaderTest, EightDigits)
{
    EXPECT_TRUE(this->template digits_test<TestFixture::has_eight_digits()>(
        this->get_eight_digits()));
}
TYPED_TEST_P(IntValueReaderTest, NineDigits)
{
    EXPECT_TRUE(this->template digits_test<TestFixture::has_nine_digits()>(
        this->get_nine_digits()));
}
TYPED_TEST_P(IntValueReaderTest, SixteenDigits)
{
    EXPECT_TRUE(this->template digits_test<TestFixture::has_sixteen_digits()>(
        this->get_sixteen_digits()));
}
TYPED_TEST_P(IntValueReaderTest, SeventeenDigits)
{
    EXPECT_TRUE(this->template digits_test<TestFixture::has_seventeen_digits()>(
        this->get_seventeen_digits()));
}

TYPED_TEST_P(IntValueReaderTest, StartsAsDecimalNumber)
{
    auto [result, val] = this->simple_test("123abc");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 123);
    EXPECT_EQ(scn::detail::to_address(*result),
              scn::detail::to_address(this->widened_source->begin() + 3));
}

TYPED_TEST_P(IntValueReaderTest, Nonsense)
{
    auto [result, val] = this->simple_test("helloworld");
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::invalid_scanned_value));
}
TYPED_TEST_P(IntValueReaderTest, NonsenseStartingWithZero)
{
    auto [result, val] = this->simple_test("0helloworld");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0);
    EXPECT_EQ(**result, 'h');
}
TYPED_TEST_P(IntValueReaderTest, NonsenseStartingWithHexPrefix)
{
    auto [result, val] = this->simple_test("0xhelloworld");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0);
    EXPECT_EQ(**result, 'x');
}
TYPED_TEST_P(IntValueReaderTest, HexFollowedByNonsenseWithDefault)
{
    auto [result, val] = this->simple_test("0xehelloworld");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0);
    EXPECT_EQ(**result, 'x');
}

TYPED_TEST_P(IntValueReaderTest, OnlyPlusSign)
{
    auto [result, val] = this->simple_test("+");
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::invalid_scanned_value));
}
TYPED_TEST_P(IntValueReaderTest, OnlyMinusSign)
{
    auto [result, val] = this->simple_test("-");
    EXPECT_TRUE(this->check_failure_with_code(
        result, val, scn::scan_error::invalid_scanned_value));
}

TYPED_TEST_P(IntValueReaderTest, OnlyHexPrefix)
{
    auto [result, val] = this->simple_test("0x");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0);
    EXPECT_EQ(scn::detail::to_address(*result),
              scn::detail::to_address(this->widened_source->begin() + 1));
}
TYPED_TEST_P(IntValueReaderTest, OnlyLongOctPrefix)
{
    auto [result, val] = this->simple_test("0o");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0);
    EXPECT_EQ(scn::detail::to_address(*result),
              scn::detail::to_address(this->widened_source->begin() + 1));
}
TYPED_TEST_P(IntValueReaderTest, OnlyBinPrefix)
{
    auto [result, val] = this->simple_test("0b");
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 0);
    EXPECT_EQ(scn::detail::to_address(*result),
              scn::detail::to_address(this->widened_source->begin() + 1));
}

TYPED_TEST_P(IntValueReaderTest, InputWithNullBytes)
{
    std::string src(5, '\0');
    src[0] = '1';
    EXPECT_EQ(src.size(), 5);
    EXPECT_EQ(src[0], '1');
    EXPECT_EQ(std::strlen(src.data()), 1);

    auto [result, val] = this->simple_test(std::move(src));
    ASSERT_TRUE(result);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(scn::detail::to_address(*result),
              scn::detail::to_address(this->widened_source->begin() + 1));
}

#if !SCN_DISABLE_LOCALE
template <typename CharT>
struct numpunct_with_comma_thsep : std::numpunct<CharT> {
    numpunct_with_comma_thsep(std::string s)
        : std::numpunct<CharT>{}, g(std::move(s))
    {
    }

    CharT do_thousands_sep() const override
    {
        return CharT{','};
    }
    std::string do_grouping() const override
    {
        return g;
    }

    std::string g;
};

template <typename CharT>
struct thsep_test_state {
    thsep_test_state(std::string grouping)
        : stdloc(std::locale::classic(),
                 new numpunct_with_comma_thsep<CharT>{std::move(grouping)}),
          locref(stdloc)
    {
    }

    scn::detail::format_specs specs{};
    std::locale stdloc;
    scn::detail::locale_ref locref;
};

TYPED_TEST_P(IntValueReaderTest, ThousandsSeparators)
{
    if constexpr (!TestFixture::has_thsep_value()) {
        return SUCCEED() << "Type too small to hold '123,456'";
    }
    if constexpr (!TestFixture::is_localized) {
        return SUCCEED() << "This test requires a localized reader";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\3"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "123,456", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_EQ(val, this->get_thsep_value());
}

TYPED_TEST_P(IntValueReaderTest, ThousandsSeparatorsWithInvalidGrouping)
{
    if constexpr (!TestFixture::has_thsep_value()) {
        return SUCCEED() << "Type too small to hold '123,456'";
    }
    if constexpr (!TestFixture::is_localized) {
        return SUCCEED() << "This test requires a localized reader";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\3"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "12,34,56", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_EQ(val, this->get_thsep_value());
}

TYPED_TEST_P(IntValueReaderTest, ExoticThousandsSeparators)
{
    if constexpr (!TestFixture::has_thsep_value()) {
        return SUCCEED() << "Type too small to hold '123,456'";
    }

    if (!TestFixture::is_localized) {
        return SUCCEED() << "This test only works with localized_interface";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\1\2"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "1,23,45,6", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_EQ(val, this->get_thsep_value());
}

TYPED_TEST_P(IntValueReaderTest, ExoticThousandsSeparatorsWithInvalidGrouping)
{
    if constexpr (!TestFixture::has_thsep_value()) {
        return SUCCEED() << "Type too small to hold '123,456'";
    }

    if (!TestFixture::is_localized) {
        return SUCCEED() << "This test only works with localized_interface";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\1\2"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "123,456", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_EQ(val, this->get_thsep_value());
}
#else
TYPED_TEST_P(IntValueReaderTest, ThousandsSeparators) {}
TYPED_TEST_P(IntValueReaderTest, ThousandsSeparatorsWithInvalidGrouping) {}
TYPED_TEST_P(IntValueReaderTest, ExoticThousandsSeparators) {}
TYPED_TEST_P(IntValueReaderTest, ExoticThousandsSeparatorsWithInvalidGrouping)
{
}

#endif

REGISTER_TYPED_TEST_SUITE_P(IntValueReaderTest,
                            Zero,
                            Basic,
                            Negative,
                            Hex,
                            HexDetect,
                            Oct,
                            OctDetect,
                            OctAltDetected,
                            OctAltDefault,
                            OctFollowedByDec,
                            OctFollowedByDecDetected,
                            OctFollowedByDecDefault,
                            Bin,
                            BinDetect,
                            Ternary,
                            Min,
                            Max,
                            Overflow,
                            Underflow,
                            FourDigits,
                            EightDigits,
                            NineDigits,
                            SixteenDigits,
                            SeventeenDigits,
                            StartsAsDecimalNumber,
                            Nonsense,
                            NonsenseStartingWithZero,
                            NonsenseStartingWithHexPrefix,
                            HexFollowedByNonsenseWithDefault,
                            OnlyPlusSign,
                            OnlyMinusSign,
                            OnlyHexPrefix,
                            OnlyLongOctPrefix,
                            OnlyBinPrefix,
                            InputWithNullBytes,
                            ThousandsSeparators,
                            ThousandsSeparatorsWithInvalidGrouping,
                            ExoticThousandsSeparators,
                            ExoticThousandsSeparatorsWithInvalidGrouping);
