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

#include "reader_test_common.h"

#include <scn/impl.h>

#include <cmath>

// Detect architecture
#if defined(__x86_64__) || defined(_M_AMD64)
#define SCN_IS_X86_64 1
#define SCN_IS_32BIT  0
#elif defined(__i386__) || defined(_M_IX86)
#define SCN_IS_X86_32 1
#define SCN_IS_32BIT  1

#elif defined(__aarch64__) || defined(_M_ARM64)
#define SCN_IS_ARM64 1
#define SCN_IS_32BIT 0
#elif defined(__arm__) || defined(_M_ARM)
#define SCN_IS_ARM32 1
#define SCN_IS_32BIT 1

#elif defined(__PPC64__) || defined(_M_PPC64)
#define SCN_IS_PPC64 1
#define SCN_IS_32BIT 0
#elif defined(__PPC__) || defined(_M_PPC)
#define SCN_IS_PPC32 1
#define SCN_IS_32BIT 1

#elif defined(__s390__)
#define SCN_IS_S390  1
#define SCN_IS_32BIT 1

#endif  // defined __x86_64__ || defined _M_AMD64

#ifndef SCN_IS_X86_64
#define SCN_IS_X86_64 0
#endif
#ifndef SCN_IS_X86_32
#define SCN_IS_X86_32 0
#endif
#ifndef SCN_IS_ARM64
#define SCN_IS_ARM64 0
#endif
#ifndef SCN_IS_ARM32
#define SCN_IS_ARM32 0
#endif
#ifndef SCN_IS_PPC64
#define SCN_IS_PPC64 0
#endif
#ifndef SCN_IS_PPC32
#define SCN_IS_PPC32 0
#endif
#ifndef SCN_IS_S390
#define SCN_IS_S390 0
#endif

#ifndef SCN_IS_32BIT
#define SCN_IS_32BIT 0
#endif

#if SCN_IS_X86_64 || SCN_IS_X86_32
#define SCN_IS_X86 1
#else
#define SCN_IS_X86 0
#endif

#if SCN_IS_ARM64 || SCN_IS_ARM32
#define SCN_IS_ARM 1
#else
#define SCN_IS_ARM 0
#endif

#if SCN_IS_PPC64 || SCN_IS_PPC32
#define SCN_IS_PPC 1
#else
#define SCN_IS_PPC 0
#endif

// long double width
#if (SCN_WINDOWS && !SCN_GCC_COMPAT) || SCN_IS_ARM32 || \
    (SCN_IS_ARM64 && SCN_APPLE)
#define SCN_LONG_DOUBLE_WIDTH 64
#elif SCN_IS_ARM64 && !SCN_APPLE && !SCN_WINDOWS
#define SCN_LONG_DOUBLE_WIDTH 128
#elif SCN_IS_X86
#define SCN_LONG_DOUBLE_WIDTH 80
#elif SCN_IS_PPC
// PPC long double is wonky
#define SCN_LONG_DOUBLE_WIDTH 0
#else
// don't know enough
#define SCN_LONG_DOUBLE_WIDTH 0
#endif

template <typename T>
void dump_bytes(T val)
{
    alignas(T) std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &val, sizeof(T));

    for (unsigned char b : bytes) {
        std::printf("%02x ", static_cast<unsigned>(b));
    }
    std::puts("");
}

template <typename T>
constexpr T float_zero()
{
    if constexpr (std::is_same_v<T, float>) {
        return 0.0f;
    }
    else if constexpr (std::is_same_v<T, double>) {
        return 0.0;
    }
    else {
        return 0.0l;
    }
}

template <typename T>
[[nodiscard]] testing::AssertionResult
check_floating_eq(T a, T b, bool allow_approx = false)
{
    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")
    if (a == b) {
        return testing::AssertionSuccess();
    }
    SCN_GCC_COMPAT_POP

    if (allow_approx && std::abs(a - b) < std::numeric_limits<T>::epsilon()) {
        return testing::AssertionSuccess();
    }

    return testing::AssertionFailure()
           << "Floats not equal: " << a << " and " << b;
}

using namespace std::string_view_literals;

template <bool Localized, typename CharT, typename ValueT>
using float_reader_wrapper =
    reader_wrapper<Localized, CharT, ValueT, scn::impl::reader_impl_for_float>;

template <typename T>
class FloatValueReaderTest : public testing::Test {
protected:
    using char_type = typename T::char_type;
    using float_type = typename T::value_type;
    using string_type = std::basic_string<char_type>;
    using string_view_type = std::basic_string_view<char_type>;

    static constexpr bool is_localized = T::is_localized;

    static constexpr bool is_wide()
    {
        return std::is_same_v<char_type, wchar_t>;
    }
    static constexpr bool is_f32()
    {
        return sizeof(float_type) == sizeof(float);
    }
    static constexpr bool is_f64()
    {
        // double on Linux, double and long double on Windows
        return sizeof(float_type) == sizeof(double);
    }
    static constexpr bool is_double()
    {
        return std::is_same_v<float_type, double>;
    }
    static constexpr bool is_long_double()
    {
        return std::is_same_v<float_type, long double>;
    }
    static constexpr bool is_double_64()
    {
        return is_f64() && is_double();
    }
    static constexpr bool is_long_double_64()
    {
        return is_f64() && is_long_double();
    }
    static constexpr bool is_f80()
    {
        // long double on Linux
        return std::numeric_limits<float_type>::digits == 64;
    }
    static constexpr bool is_f128()
    {
        return !is_f80() && sizeof(float_type) == 16;
    }

    static_assert(std::numeric_limits<float_type>::is_iec559);
    static_assert(is_f32() || is_double() || is_long_double());
    static_assert(is_f32() || is_f64() || is_f80() || is_f128());
    static_assert(is_f32() || is_double_64() || is_long_double_64() ||
                  is_f80() || is_f128());

#if SCN_LONG_DOUBLE_WIDTH == 64
    static_assert(!is_long_double() || is_long_double_64());
#elif SCN_LONG_DOUBLE_WIDTH == 80
    static_assert(!is_long_double() || is_f80());
#elif SCN_LONG_DOUBLE_WIDTH == 128
    static_assert(!is_long_double() || is_f128());
#endif

    static constexpr const char* get_length_flag()
    {
        if constexpr (std::is_same_v<float_type, long double>) {
            return "L";
        }
        else {
            return "";
        }
    }

    static std::string format_float(float_type val,
                                    const std::string& flags_before_len,
                                    const std::string& flags_after_len)
    {
        std::string buf(256, '\0');
        auto casted_val = [&]() {
            if constexpr (std::is_same_v<float_type, float>) {
                return static_cast<double>(val);
            }
            else {
                return val;
            }
        };
        SCN_GCC_COMPAT_PUSH
        SCN_GCC_COMPAT_IGNORE("-Wformat-nonliteral")
        const auto fmt = std::string{"%"} + flags_before_len +
                         get_length_flag() + flags_after_len;
        const auto ret =
            std::snprintf(buf.data(), buf.size(), fmt.c_str(), casted_val());
        SCN_GCC_COMPAT_POP
        SCN_EXPECT(ret > 0);
        buf = buf.substr(0, buf.find('\0'));
        return buf;
    }

    static auto get_pi()
    {
        if constexpr (is_f32()) {
            return std::make_pair(3.14f, "3.14"sv);
        }
        else if constexpr (is_double()) {
            return std::make_pair(3.14, "3.14"sv);
        }
        else if constexpr (is_long_double()) {
            return std::make_pair(3.14l, "3.14"sv);
        }
    }
    static auto get_neg()
    {
        if constexpr (is_f32()) {
            return std::make_pair(-123.456f, "-123.456"sv);
        }
        else if constexpr (is_double()) {
            return std::make_pair(-123.456, "-123.456"sv);
        }
        else if constexpr (is_long_double()) {
            return std::make_pair(-123.456l, "-123.456"sv);
        }
    }
    static auto get_leading_plus()
    {
        if constexpr (is_f32()) {
            return std::make_pair(3.14f, "+3.14"sv);
        }
        else if constexpr (is_double()) {
            return std::make_pair(3.14, "+3.14"sv);
        }
        else if constexpr (is_long_double()) {
            return std::make_pair(3.14l, "+3.14"sv);
        }
    }

    static auto get_subnormal()
    {
        if constexpr (is_f32()) {
            return std::make_pair(2e-40f, "2e-40"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(5e-320, "5e-320"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(5e-320l, "5e-320"sv);
        }
#if SCN_LONG_DOUBLE_WIDTH > 64
        else if constexpr (is_f80()) {
            return std::make_pair(3e-4940l, "3e-4940"sv);
        }
#endif
#if SCN_LONG_DOUBLE_WIDTH > 80
        else if constexpr (is_f128()) {
            return std::make_pair(5e-4960l, "5e-4960"sv);
        }
#endif
    }
    static auto get_subnormal_hex()
    {
        if constexpr (is_f32()) {
            return std::make_pair(0x1.2p-130f, "0x1.2p-130"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(0x1.2p-1050, "0x1.2p-1050"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(0x1.2p-1050l, "0x1.2p-1050"sv);
        }
#if SCN_LONG_DOUBLE_WIDTH > 64
        else if constexpr (is_f80()) {
            return std::make_pair(0x1.2p-16400l, "0x1.2p-16400"sv);
        }
#endif
#if SCN_LONG_DOUBLE_WIDTH > 80
        else if constexpr (is_f128()) {
            return std::make_pair(0x1.2p-16450l, "0x1.2p-16450"sv);
        }
#endif
    }

    static auto get_subnormal_max()
    {
        if constexpr (is_f32()) {
            return std::make_pair(1e-38f, "1e-38"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(2e-308, "2e-308"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(2e-308l, "2e-308"sv);
        }
#if SCN_LONG_DOUBLE_WIDTH > 64
        else if constexpr (is_f80() || is_f128()) {
            return std::make_pair(3.2e-4932l, "3.2e-4932"sv);
        }
#endif
    }
    static auto get_subnormal_max_hex()
    {
        if constexpr (is_f32()) {
            return std::make_pair(0x1.fp-127f, "0x1.fp-127"sv);
        }
        else if constexpr (is_double_64()) {
            return std::make_pair(0x1.fp-1023, "0x1.fp-1023"sv);
        }
        else if constexpr (is_long_double_64()) {
            return std::make_pair(0x1.fp-1023l, "0x1.fp-1023"sv);
        }
#if SCN_LONG_DOUBLE_WIDTH > 64
        else if constexpr (is_f80() || is_f128()) {
            return std::make_pair(0x1.fp-16383l, "0x1.fp-16383"sv);
        }
#endif
    }

    static auto get_normal_min()
    {
        auto val = std::numeric_limits<float_type>::min();
        return std::make_pair(val, format_float(val, ".48", "e"));
    }
    static auto get_normal_min_hex()
    {
        auto val = std::numeric_limits<float_type>::min();
        return std::make_pair(val, format_float(val, ".32", "a"));
    }

    static auto get_subnormal_min()
    {
        auto val = std::numeric_limits<float_type>::denorm_min();
        return std::make_pair(val, format_float(val, ".48", "e"));
    }
    static auto get_subnormal_min_hex()
    {
        auto val = std::numeric_limits<float_type>::denorm_min();
        return std::make_pair(val, format_float(val, ".32", "a"));
    }

    static auto get_underflow()
    {
        if constexpr (is_f32()) {
            return "1.0e-90"sv;
        }
        else if constexpr (is_f64()) {
            return "5.0e-400"sv;
        }
        else if constexpr (is_f80()) {
            return "4.0e-5500"sv;
        }
        else if constexpr (is_f128()) {
            return "6.0e-5500"sv;
        }
    }
    static auto get_underflow_hex()
    {
        if constexpr (is_f32()) {
            return "0x1p-192"sv;
        }
        else if constexpr (is_f64()) {
            return "0x1p-1200"sv;
        }
        else if constexpr (is_f80()) {
            return "0x1p-18000"sv;
        }
        else if constexpr (is_f128()) {
            return "0x1p-18000"sv;
        }
    }
    static auto get_underflow_neg()
    {
        if constexpr (is_f32()) {
            return "-1.0e-90"sv;
        }
        else if constexpr (is_f64()) {
            return "-5.0e-400"sv;
        }
        else if constexpr (is_f80()) {
            return "-4.0e-5500"sv;
        }
        else if constexpr (is_f128()) {
            return "-6.0e-5500"sv;
        }
    }

    static auto get_maximum()
    {
        auto val = std::numeric_limits<float_type>::max();
        return std::make_pair(val, format_float(val, ".48", "e"));
    }
    static auto get_maximum_hex()
    {
        auto val = std::numeric_limits<float_type>::max();
        return std::make_pair(val, format_float(val, ".32", "a"));
    }

    static auto get_overflow()
    {
        if constexpr (is_f32()) {
            return "4.0e38"sv;
        }
        else if constexpr (is_f64()) {
            return "2.0e308"sv;
        }
        else if constexpr (is_f80() || is_f128()) {
            return "2.0e4932"sv;
        }
    }
    static auto get_overflow_hex()
    {
        if constexpr (is_f32()) {
            return "0x1p+128"sv;
        }
        else if constexpr (is_f64()) {
            return "0x1p+1024"sv;
        }
        else if constexpr (is_f80() || is_f128()) {
            return "0x1p+16384"sv;
        }
    }

    static auto get_overflow_neg()
    {
        if constexpr (is_f32()) {
            return "-4.0e38"sv;
        }
        else if constexpr (is_f64()) {
            return "-2.0e308"sv;
        }
        else if constexpr (is_f80() || is_f128()) {
            return "-2.0e4932"sv;
        }
    }
    static auto get_overflow_neg_hex()
    {
        if constexpr (is_f32()) {
            return "-0x1p+128"sv;
        }
        else if constexpr (is_f64()) {
            return "-0x1p+1024"sv;
        }
        else if constexpr (is_f80() || is_f128()) {
            return "-0x1p+16384"sv;
        }
    }

    static auto get_thsep_number()
    {
        if constexpr (is_f32()) {
            return 123456.789f;
        }
        else if constexpr (is_double()) {
            return 123456.789;
        }
        else if constexpr (is_long_double()) {
            return 123456.789L;
        }
    }

    template <typename Source>
    void set_source(Source&& s)
    {
        if constexpr (!is_wide()) {
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

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_generic_success(
        const Result& result) const
    {
        if (!result) {
            return testing::AssertionFailure()
                   << "Result not good: code " << result.error().code();
        }
        SCN_EXPECT(this->widened_source);
        if (scn::detail::to_address(result.value()) !=
            scn::detail::to_address(this->widened_source->end())) {
            return testing::AssertionFailure()
                   << "Result range not correct: diff "
                   << std::distance(
                          scn::detail::to_address(result.value()),
                          scn::detail::to_address(this->widened_source->end()))
                   << ", result points to "
                   << static_cast<char>(*(result.value()));
        }
        return testing::AssertionSuccess();
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_value_success(
        const Result& result,
        float_type val,
        float_type expected) const
    {
        if (auto a = check_generic_success(result); !a) {
            return a;
        }
        if (auto a = check_floating_eq(val, expected); !a) {
            return a;
        }
        return testing::AssertionSuccess();
    }

    template <typename Result>
    [[nodiscard]] testing::AssertionResult check_failure_with_code(
        const Result& result,
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
        return testing::AssertionSuccess();
    }

    template <typename Source>
    auto simple_test(Source&& source)
    {
        this->set_source(SCN_FWD(source));

        float_type val{};
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

        float_type val{};
        auto result = this->wrapped_reader.read_specs_with_locale(
            widened_source.value(), specs, val, loc);
        return std::make_pair(result, val);
    }

    template <typename Source>
    auto simple_success_test(Source&& source)
    {
        this->set_source(SCN_FWD(source));

        float_type val{};
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

        float_type val{};
        auto result = this->wrapped_reader.read_specs_with_locale(
            widened_source.value(), specs, val, loc);
        return std::make_tuple(this->check_generic_success(result), result,
                               val);
    }

    template <typename Source>
    [[nodiscard]] testing::AssertionResult simple_default_test(
        Source&& source,
        float_type expected_output)
    {
        auto [result, val] = simple_test(SCN_FWD(source));
        return check_value_success(result, val, expected_output);
    }

    scn::detail::format_specs make_format_specs_with_presentation(
        scn::detail::presentation_type type) const
    {
        scn::detail::format_specs specs{};
        specs.type = type;
        return specs;
    }

    T wrapped_reader{};
    std::optional<string_type> widened_source;
};

using type_list =
    ::testing::Types<float_reader_wrapper<false, char, float>,
                     float_reader_wrapper<false, char, double>,
                     float_reader_wrapper<false, wchar_t, float>,
                     float_reader_wrapper<false, wchar_t, double>
#if !SCN_DISABLE_LOCALE
                     ,
                     float_reader_wrapper<true, char, float>,
                     float_reader_wrapper<true, char, double>,
                     float_reader_wrapper<true, wchar_t, float>,
                     float_reader_wrapper<true, wchar_t, double>
#endif
#if SCN_LONG_DOUBLE_WIDTH != 0
                     ,
                     float_reader_wrapper<false, char, long double>,
                     float_reader_wrapper<false, wchar_t, long double>
#if !SCN_DISABLE_LOCALE
                     ,
                     float_reader_wrapper<true, char, long double>,
                     float_reader_wrapper<true, wchar_t, long double>
#endif
#endif  // has long double
                     >;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

TYPED_TEST_SUITE(FloatValueReaderTest, type_list);

SCN_CLANG_POP

TYPED_TEST(FloatValueReaderTest, Basic)
{
    const auto [val, src] = this->get_pi();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

TYPED_TEST(FloatValueReaderTest, Negative)
{
    const auto [val, src] = this->get_neg();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

TYPED_TEST(FloatValueReaderTest, LeadingPlus)
{
    const auto [val, src] = this->get_leading_plus();
    EXPECT_TRUE(this->simple_default_test(src, val));
}

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wdouble-promotion")

TYPED_TEST(FloatValueReaderTest, Scientific)
{
    EXPECT_TRUE(this->simple_default_test("4.20e1", 42.0));
}

TYPED_TEST(FloatValueReaderTest, Hex)
{
    EXPECT_TRUE(this->simple_default_test("0x1.2ap3", 0x1.2ap3));
}
TYPED_TEST(FloatValueReaderTest, NegativeHex)
{
    EXPECT_TRUE(this->simple_default_test("-0x1.2ap3", -0x1.2ap3));
}

SCN_CLANG_POP  // -Wdouble-promotion

TYPED_TEST(FloatValueReaderTest, InfinityWithInf)
{
    auto [a, _, val] = this->simple_success_test("inf");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isinf(val));
    EXPECT_FALSE(std::signbit(val));
}
TYPED_TEST(FloatValueReaderTest, InfinityWithNegInfinity)
{
    auto [a, _, val] = this->simple_success_test("-infinity");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isinf(val));
    EXPECT_TRUE(std::signbit(val));
}

TYPED_TEST(FloatValueReaderTest, NaN)
{
    auto [a, _, val] = this->simple_success_test("nan");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnan(val));
    EXPECT_FALSE(std::signbit(val));
}
TYPED_TEST(FloatValueReaderTest, NaNWithPayload)
{
    auto [a, _, val] = this->simple_success_test("nan(123_abc)");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnan(val));
    EXPECT_FALSE(std::signbit(val));
}

TYPED_TEST(FloatValueReaderTest, Overflow)
{
    auto [result, _] = this->simple_test("9999999999999.9999e999999999999999");
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_positive_overflow));
}

TYPED_TEST(FloatValueReaderTest, Subnormal)
{
    const auto [orig_val, source] = this->get_subnormal();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, SubnormalFromHex)
{
    const auto [orig_val, source] = this->get_subnormal();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, LargeSubnormal)
{
    const auto [orig_val, source] = this->get_subnormal_max();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, LargeSubnormalFromHex)
{
    const auto [orig_val, source] = this->get_subnormal_max_hex();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, MinimumNormal)
{
    const auto [orig_val, source] = this->get_normal_min();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, MinimumNormalFromHex)
{
    const auto [orig_val, source] = this->get_normal_min_hex();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, MinimumSubnormal)
{
    const auto [orig_val, source] = this->get_subnormal_min();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, MinimumSubnrmalFromHex)
{
    const auto [orig_val, source] = this->get_subnormal_min_hex();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, Underflow)
{
    auto [result, _] = this->simple_test(this->get_underflow());
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_positive_underflow));
}
TYPED_TEST(FloatValueReaderTest, UnderflowFromHex)
{
    auto [result, _] = this->simple_test(this->get_underflow_hex());
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_positive_underflow));
}
TYPED_TEST(FloatValueReaderTest, UnderflowNeg)
{
    auto [result, _] = this->simple_test(this->get_underflow_neg());
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_negative_underflow));
}

TYPED_TEST(FloatValueReaderTest, Maximum)
{
    const auto [orig_val, source] = this->get_maximum();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isinf(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, MaximumFromHex)
{
    const auto [orig_val, source] = this->get_maximum_hex();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isinf(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}

TYPED_TEST(FloatValueReaderTest, BarelyOverflow)
{
    auto [result, _] = this->simple_test(this->get_overflow());
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_positive_overflow));
}
TYPED_TEST(FloatValueReaderTest, BarelyOverflowFromHex)
{
    auto [result, _] = this->simple_test(this->get_overflow_hex());
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_positive_overflow));
}

TYPED_TEST(FloatValueReaderTest, BarelyOverflowNeg)
{
    auto [result, _] = this->simple_test(this->get_overflow_neg());
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_negative_overflow));
}
TYPED_TEST(FloatValueReaderTest, BarelyOverflowNegFromHex)
{
    auto [result, _] = this->simple_test(this->get_overflow_neg_hex());
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::value_negative_overflow));
}

TYPED_TEST(FloatValueReaderTest, PresentationScientificValueScientific)
{
    auto [a, _, val] = this->simple_success_specs_test(
        "12.3e4", this->make_format_specs_with_presentation(
                      scn::detail::presentation_type::float_scientific));
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(12.3e4)));
}
TYPED_TEST(FloatValueReaderTest, PresentationScientificValueFixed)
{
    auto [result, _] = this->simple_specs_test(
        "12.3", this->make_format_specs_with_presentation(
                    scn::detail::presentation_type::float_scientific));
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::invalid_scanned_value));
}
TYPED_TEST(FloatValueReaderTest, PresentationScientificValueHexWithPrefix)
{
    auto [result, _] = this->simple_specs_test(
        "0x1.fp3", this->make_format_specs_with_presentation(
                       scn::detail::presentation_type::float_scientific));
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::invalid_scanned_value));
}
TYPED_TEST(FloatValueReaderTest, PresentationScientificValueHexWithoutPrefix)
{
    auto [result, _] = this->simple_specs_test(
        "1.fp3", this->make_format_specs_with_presentation(
                     scn::detail::presentation_type::float_scientific));
    EXPECT_TRUE(this->check_failure_with_code(
        result, scn::scan_error::invalid_scanned_value));
}

TYPED_TEST(FloatValueReaderTest, PresentationFixedValueScientific)
{
    auto [result, val] = this->simple_specs_test(
        "12.3e4", this->make_format_specs_with_presentation(
                      scn::detail::presentation_type::float_fixed));
    ASSERT_TRUE(result);
    EXPECT_EQ(scn::detail::to_address(result.value()),
              this->widened_source->data() + 4);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(12.3l)));
}
TYPED_TEST(FloatValueReaderTest, PresentationFixedValueFixed)
{
    const auto [orig_val, src] = this->get_pi();
    auto [a, _, val] = this->simple_success_specs_test(
        src, this->make_format_specs_with_presentation(
                 scn::detail::presentation_type::float_fixed));

    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, PresentationFixedValueHexWithPrefix)
{
    auto [result, val] = this->simple_specs_test(
        "0x1.fp3", this->make_format_specs_with_presentation(
                       scn::detail::presentation_type::float_fixed));
    ASSERT_TRUE(result);
    EXPECT_EQ(scn::detail::to_address(result.value()),
              this->widened_source->data() + 1);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(0.0)));
}
TYPED_TEST(FloatValueReaderTest, PresentationFixedValueHexWithoutPrefix)
{
    auto [result, val] = this->simple_specs_test(
        "1.fp3", this->make_format_specs_with_presentation(
                     scn::detail::presentation_type::float_fixed));
    ASSERT_TRUE(result);
    EXPECT_EQ(scn::detail::to_address(result.value()),
              this->widened_source->data() + 2);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(1.0)));
}

template <typename T>
T get_hexfloat_interpreted_as_decimal(const std::string& input)
{
    if constexpr (std::is_same_v<T, float>) {
        return std::strtof(input.c_str(), nullptr);
    }
    else if constexpr (std::is_same_v<T, double>) {
        return std::strtod(input.c_str(), nullptr);
    }
    else {
        return std::strtold(input.c_str(), nullptr);
    }
}

TYPED_TEST(FloatValueReaderTest, PresentationHexValueScientific)
{
    auto [result, val] = this->simple_specs_test(
        "12.3e4", this->make_format_specs_with_presentation(
                      scn::detail::presentation_type::float_hex));
    ASSERT_TRUE(result);
    EXPECT_TRUE(check_floating_eq(
        val,
        get_hexfloat_interpreted_as_decimal<typename TestFixture::float_type>(
            "0x12.3e4")));
    EXPECT_EQ(scn::detail::to_address(*result),
              this->widened_source->data() + this->widened_source->size());
}
TYPED_TEST(FloatValueReaderTest, PresentationHexValueFixed)
{
    auto [result, val] = this->simple_specs_test(
        "12.3", this->make_format_specs_with_presentation(
                    scn::detail::presentation_type::float_hex));
    ASSERT_TRUE(result);
    EXPECT_TRUE(check_floating_eq(
        val,
        get_hexfloat_interpreted_as_decimal<typename TestFixture::float_type>(
            "0x12.3")));
    EXPECT_EQ(scn::detail::to_address(*result),
              this->widened_source->data() + this->widened_source->size());
}
TYPED_TEST(FloatValueReaderTest, PresentationHexValueHexWithPrefix)
{
    auto [a, _, val] = this->simple_success_specs_test(
        "0x1.fp3", this->make_format_specs_with_presentation(
                       scn::detail::presentation_type::float_hex));
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(0x1.fp3)));
}
TYPED_TEST(FloatValueReaderTest, PresentationHexValueHexWithoutPrefix)
{
    auto [a, _, val] = this->simple_success_specs_test(
        "1.fp3", this->make_format_specs_with_presentation(
                     scn::detail::presentation_type::float_hex));
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(0x1.fp3)));
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

TYPED_TEST(FloatValueReaderTest, ThousandsSeparators)
{
    if constexpr (!TestFixture::is_localized) {
        return SUCCEED() << "This test requires a localized reader";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\3"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "123,456.789", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(val, this->get_thsep_number()));
}

TYPED_TEST(FloatValueReaderTest, ThousandsSeparatorsWithInvalidGrouping)
{
    if constexpr (!TestFixture::is_localized) {
        return SUCCEED() << "This test requires a localized reader";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\3"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "12,34,56.789", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(val, this->get_thsep_number()));
}

TYPED_TEST(FloatValueReaderTest, ExoticThousandsSeparators)
{
    if (!TestFixture::is_localized) {
        return SUCCEED() << "This test only works with localized_interface";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\1\2"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "1,23,45,6.789", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(val, this->get_thsep_number()));
}

TYPED_TEST(FloatValueReaderTest, ExoticThousandsSeparatorsWithInvalidGrouping)
{
    if (!TestFixture::is_localized) {
        return SUCCEED() << "This test only works with localized_interface";
    }

    auto state = thsep_test_state<typename TestFixture::char_type>{"\1\2"};

    auto [a, _, val] = this->simple_success_specs_and_locale_test(
        "123,456.789", state.specs, state.locref);
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(val, this->get_thsep_number()));
}

template <typename CharT>
struct numpunct_with_comma_decimal_separator : std::numpunct<CharT> {
    numpunct_with_comma_decimal_separator() = default;

    CharT do_decimal_point() const override
    {
        return CharT{','};
    }
};

template <typename CharT>
struct decimal_comma_test_state {
    decimal_comma_test_state()
        : stdloc(std::locale::classic(),
                 new numpunct_with_comma_decimal_separator<CharT>{}),
          locref(stdloc)
    {
    }

    std::locale stdloc;
    scn::detail::locale_ref locref;
};

TYPED_TEST(FloatValueReaderTest, LocalizedDecimalSeparator)
{
    if (!TestFixture::is_localized) {
        return SUCCEED() << "This test only works with localized_interface";
    }

    auto state = decimal_comma_test_state<typename TestFixture::char_type>{};

    auto [a, _, val] =
        this->simple_success_specs_and_locale_test("3,14", {}, state.locref);
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(val, this->get_pi().first));
}
#endif  // !SCN_DISABLE_LOCALE
