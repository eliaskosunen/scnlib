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

#include <cfloat>
#include <cstring>
#include <iomanip>

#include "test_common.h"

#include <scn/scan.h>

#if !defined(LDBL_MANT_DIG)
#error "LDBL_MANT_DIG not defined, even though <cfloat> is included"
#endif

#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
inline constexpr bool finite_math_only = true;
#else
inline constexpr bool finite_math_only = false;
#endif

template <typename T>
std::string get_bytes_str(T val)
{
    alignas(T) std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &val, sizeof(T));

    std::ostringstream os;
    for (unsigned char b : bytes) {
        os << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<unsigned>(b) << ' ';
    }
    return os.str().substr(0, os.str().size() - 1);
}

template <typename T>
SCN_NODISCARD testing::AssertionResult
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

    if constexpr (sizeof(T) <= sizeof(double)) {
        return testing::AssertionFailure()
               << "Floats not equal: " << a << " and " << b
               << " (bytes: " << get_bytes_str(a) << ", " << get_bytes_str(b)
               << ")";
    }
    else {
        return testing::AssertionFailure()
               << "Floats not equal: " << static_cast<long double>(a) << " and "
               << static_cast<long double>(b) << " (bytes: " << get_bytes_str(a)
               << ", " << get_bytes_str(b) << ")";
    }
}

template <typename T>
SCN_NODISCARD testing::AssertionResult check_nan_eq(T lhs, T rhs)
{
    if (!std::isnan(lhs)) {
        return testing::AssertionFailure() << "lhs not nan";
    }
    if (!std::isnan(rhs)) {
        return testing::AssertionFailure() << "rhs not nan";
    }

    if constexpr (sizeof(T) <= sizeof(std::uint64_t)) {
        std::uint64_t lhs_bits{}, rhs_bits{};
        std::memcpy(&lhs_bits, &lhs, sizeof(T));
        std::memcpy(&rhs_bits, &rhs, sizeof(T));
        if (lhs_bits != rhs_bits) {
            return testing::AssertionFailure()
                   << "lhs bits: " << get_bytes_str(lhs_bits)
                   << " != rhs_bits: " << get_bytes_str(rhs_bits);
        }
        return testing::AssertionSuccess()
               << "lhs bits: " << get_bytes_str(lhs_bits)
               << " != rhs_bits: " << get_bytes_str(rhs_bits);
    }
    else {
        // Discard last six bytes (assuming 80-bit long double)
        // TODO: check better against other long double formats
        std::array<unsigned char, sizeof(T) - 6> lhs_bits{}, rhs_bits{};
        std::memcpy(lhs_bits.data(), &lhs, sizeof(T) - 6);
        std::memcpy(rhs_bits.data(), &rhs, sizeof(T) - 6);
        if (lhs_bits != rhs_bits) {
            return testing::AssertionFailure()
                   << "lhs bits: " << get_bytes_str(lhs_bits)
                   << " != rhs_bits: " << get_bytes_str(rhs_bits);
        }
        return testing::AssertionSuccess()
               << "lhs bits: " << get_bytes_str(lhs_bits)
               << " == rhs_bits: " << get_bytes_str(rhs_bits);
    }
}

enum class float_kind {
    f16,
    f32,
    f64,
    f80,    // x87 long double
    f128,   // ieee binary128
    bf16,   // bfloat16
    f2x64,  // double-double
};

template <typename T, typename Enable = void>
inline constexpr auto float_kind_for = std::monostate{};

template <typename T>
inline constexpr auto float_kind_for<T, std::enable_if_t<sizeof(T) == 4>> =
    float_kind::f32;
template <typename T>
inline constexpr auto float_kind_for<T, std::enable_if_t<sizeof(T) == 8>> =
    float_kind::f64;

template <typename T>
inline constexpr auto
    float_kind_for<T, std::enable_if_t<std::numeric_limits<T>::digits == 64>> =
        float_kind::f80;
template <typename T>
inline constexpr auto
    float_kind_for<T, std::enable_if_t<std::numeric_limits<T>::digits == 113>> =
        float_kind::f128;
template <typename T>
inline constexpr auto
    float_kind_for<T, std::enable_if_t<std::numeric_limits<T>::digits == 11>> =
        float_kind::f16;
template <typename T>
inline constexpr auto
    float_kind_for<T, std::enable_if_t<std::numeric_limits<T>::digits == 8>> =
        float_kind::bf16;
template <typename T>
inline constexpr auto
    float_kind_for<T, std::enable_if_t<std::numeric_limits<T>::digits == 106>> =
        float_kind::f2x64;

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wheader-hygiene")

using namespace std::string_literals;
using namespace std::string_view_literals;

SCN_CLANG_POP

#if SCN_HAS_STD_F128
#define SCN_FLOAT_CONSTANT(x) x##F128
#else
#define SCN_FLOAT_CONSTANT(x) x##L
#endif

template <typename ResultFloatT, typename InputFloatT>
constexpr std::pair<ResultFloatT, std::string_view> make_float_pair(
    InputFloatT value,
    std::string_view str)
{
    static_assert(sizeof(ResultFloatT) == sizeof(InputFloatT));
    static_assert(std::numeric_limits<ResultFloatT>::digits ==
                  std::numeric_limits<InputFloatT>::digits);
    static_assert(std::numeric_limits<ResultFloatT>::max_exponent ==
                  std::numeric_limits<InputFloatT>::max_exponent);
    return {static_cast<ResultFloatT>(value), str};
}

#define SCN_MAKE_FLOAT_PAIR(Value, LiteralSuffix) \
    make_float_pair<FloatT>(Value##LiteralSuffix, #Value)

template <typename FloatT, float_kind Kind>
struct float_test_suite_value_set;

template <typename FloatT>
struct float_test_suite_value_set<FloatT, float_kind::f32> {
    static constexpr auto subnormal = SCN_MAKE_FLOAT_PAIR(2e-40, f);
    static constexpr auto subnormal_hex = SCN_MAKE_FLOAT_PAIR(0x1.2p-130, f);

    static constexpr auto subnormal_max =
        SCN_MAKE_FLOAT_PAIR(1.1754942106924410754870294448492873e-38, f);
    static constexpr auto subnormal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.fffffcp-127, f);

    static constexpr auto subnormal_min =
        SCN_MAKE_FLOAT_PAIR(1.40129846432481707092372958328991613e-45, f);
    static constexpr auto subnormal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-149, f);

    static constexpr auto normal_max =
        SCN_MAKE_FLOAT_PAIR(3.40282346638528859811704183484516925e+38, f);
    static constexpr auto normal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.fffffep+127, f);

    static constexpr auto normal_min =
        SCN_MAKE_FLOAT_PAIR(1.17549435082228750796873653722224568e-38, f);
    static constexpr auto normal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-126, f);

    static constexpr auto underflow_str = "1.0e-90"sv;
    static constexpr auto underflow_hex_str = "0x1p-192"sv;

    static constexpr auto overflow_str = "4.0e38"sv;
    static constexpr auto overflow_hex_str = "0x1p128"sv;
};

template <typename FloatT>
struct float_test_suite_value_set<FloatT, float_kind::f64> {
    static constexpr auto subnormal = SCN_MAKE_FLOAT_PAIR(5e-320, );
    static constexpr auto subnormal_hex = SCN_MAKE_FLOAT_PAIR(0x1.2p-1050, );

    static constexpr auto subnormal_max =
        SCN_MAKE_FLOAT_PAIR(2.2250738585072008890245868760858599e-308, );
    static constexpr auto subnormal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.ffffffffffffep-1023, );

    static constexpr auto subnormal_min =
        SCN_MAKE_FLOAT_PAIR(4.94065645841246544176568792868221372e-324, );
    static constexpr auto subnormal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-1074, );

    static constexpr auto normal_max =
        SCN_MAKE_FLOAT_PAIR(1.79769313486231570814527423731704357e+308, );
    static constexpr auto normal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.fffffffffffffp+1023, );

    static constexpr auto normal_min =
        SCN_MAKE_FLOAT_PAIR(2.22507385850720138309023271733240406e-308, );
    static constexpr auto normal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-1022, );

    static constexpr auto underflow_str = "5.0e-400"sv;
    static constexpr auto underflow_hex_str = "0x1p-1200"sv;

    static constexpr auto overflow_str = "4.0e380"sv;
    static constexpr auto overflow_hex_str = "0x1p1024"sv;
};

#if LDBL_MANT_DIG == 64
// long double is f80
template <typename FloatT>
struct float_test_suite_value_set<FloatT, float_kind::f80> {
    static constexpr auto subnormal = SCN_MAKE_FLOAT_PAIR(3e-4940, L);
    static constexpr auto subnormal_hex = SCN_MAKE_FLOAT_PAIR(0x1.2p-16400, L);

    static constexpr auto subnormal_max =
        SCN_MAKE_FLOAT_PAIR(3.3621031431120935058981578641335051e-4932, L);
    static constexpr auto subnormal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x7.fffffffffffffffp-16385, L);

    static constexpr auto subnormal_min =
        SCN_MAKE_FLOAT_PAIR(3.64519953188247460252840593361941982e-4951, L);
    static constexpr auto subnormal_min_hex =
        SCN_MAKE_FLOAT_PAIR(0x1p-16445, L);

    static constexpr auto normal_max =
        SCN_MAKE_FLOAT_PAIR(1.18973149535723176502126385303097021e+4932, L);
    static constexpr auto normal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0xf.fffffffffffffffp+16380, L);

    static constexpr auto normal_min =
        SCN_MAKE_FLOAT_PAIR(3.36210314311209350626267781732175260e-4932, L);
    static constexpr auto normal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-16382, L);

    static constexpr auto underflow_str = "4.0e-5500"sv;
    static constexpr auto underflow_hex_str = "0x1p-18000"sv;

    static constexpr auto overflow_str = "2.0e4932"sv;
    static constexpr auto overflow_hex_str = "0x1p16384"sv;
};
#endif

#if SCN_HAS_STD_F128 || LDBL_MANT_DIG == 113
// We either have std::float128_t, or long double is f128

#if SCN_HAS_STD_F128
#define SCN_MAKE_F128_PAIR(Value) SCN_MAKE_FLOAT_PAIR(Value, F128)
#else
#define SCN_MAKE_F128_PAIR(Value) SCN_MAKE_FLOAT_PAIR(Value, L)
#endif

template <typename FloatT>
struct float_test_suite_value_set<FloatT, float_kind::f128> {
    static constexpr auto subnormal = SCN_MAKE_F128_PAIR(5e-4960);
    static constexpr auto subnormal_hex = SCN_MAKE_F128_PAIR(0x1.2p-16450);

    static constexpr auto subnormal_max =
        SCN_MAKE_F128_PAIR(3.3621031431120935062626778173217519551e-4932);
    static constexpr auto subnormal_max_hex =
        SCN_MAKE_F128_PAIR(0x1.fffffffffffffffffffffffffffep-16383);

    static constexpr auto subnormal_min =
        SCN_MAKE_F128_PAIR(6.47517511943802511092443895822764655e-4966);
    static constexpr auto subnormal_min_hex = SCN_MAKE_F128_PAIR(0x1p-16494);

    static constexpr auto normal_max =
        SCN_MAKE_F128_PAIR(1.18973149535723176508575932662800702e+4932);
    static constexpr auto normal_max_hex =
        SCN_MAKE_F128_PAIR(0x1.ffffffffffffffffffffffffffffp+16383);

    static constexpr auto normal_min =
        SCN_MAKE_F128_PAIR(3.36210314311209350626267781732175260e-4932);
    static constexpr auto normal_min_hex = SCN_MAKE_F128_PAIR(0x1p-16382);

    static constexpr auto underflow_str = "4.0e-5500"sv;
    static constexpr auto underflow_hex_str = "0x1p-18000"sv;

    static constexpr auto overflow_str = "2.0e4932"sv;
    static constexpr auto overflow_hex_str = "0x1p16384"sv;
};
#endif

#if LDBL_MANT_DIG == 106
// long double is f2x64
template <typename FloatT>
struct float_test_suite_value_set<FloatT, float_kind::f2x64> {
    static constexpr auto subnormal = SCN_MAKE_FLOAT_PAIR(5e-320, L);
    static constexpr auto subnormal_hex = SCN_MAKE_FLOAT_PAIR(0x1.2p-1050, L);

    static constexpr auto subnormal_max =
        SCN_MAKE_FLOAT_PAIR(2.004168360008972777996108051350113e-292, L);
    static constexpr auto subnormal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.ffffffffffffffffffffffffffp-970, L);

    static constexpr auto subnormal_min =
        SCN_MAKE_FLOAT_PAIR(4.940656458412465441765687928682214e-324, L);
    static constexpr auto subnormal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-1074, L);

    static constexpr auto normal_max =
        SCN_MAKE_FLOAT_PAIR(1.797693134862315807937289714053012e+308, L);
    static constexpr auto normal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.fffffffffffff7ffffffffffff8p+1023, L);

    static constexpr auto normal_min =
        SCN_MAKE_FLOAT_PAIR(2.00416836000897277799610805135016205e-292, L);
    static constexpr auto normal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-969, L);

    static constexpr auto underflow_str = "5.0e-400"sv;
    static constexpr auto underflow_hex_str = "0x1p-1200"sv;

    static constexpr auto overflow_str = "2.0e308"sv;
    static constexpr auto overflow_hex_str = "0x1p308"sv;
};
#endif

#if SCN_HAS_STD_F16
template <typename FloatT>
struct float_test_suite_value_set<FloatT, float_kind::f16> {
    static constexpr auto subnormal = SCN_MAKE_FLOAT_PAIR(5e-6, F16);
    static constexpr auto subnormal_hex = SCN_MAKE_FLOAT_PAIR(0x1.2p-16, F16);

    static constexpr auto subnormal_max =
        SCN_MAKE_FLOAT_PAIR(6.097555160522461e-5, F16);
    static constexpr auto subnormal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.ff8p-15, F16);

    static constexpr auto subnormal_min =
        SCN_MAKE_FLOAT_PAIR(5.9604644775390625e-8, F16);
    static constexpr auto subnormal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-24, F16);

    static constexpr auto normal_max = SCN_MAKE_FLOAT_PAIR(6.5504e+4, F16);
    static constexpr auto normal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.ffcp+15, F16);

    static constexpr auto normal_min = SCN_MAKE_FLOAT_PAIR(6.103515625e-5, F16);
    static constexpr auto normal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-14, F16);

    static constexpr auto underflow_str = "5.0e-16"sv;
    static constexpr auto underflow_hex_str = "0x1p-40"sv;

    static constexpr auto overflow_str = "7.0e4"sv;
    static constexpr auto overflow_hex_str = "0x1p16"sv;
};
#endif

#if SCN_HAS_STD_BF16
template <typename FloatT>
struct float_test_suite_value_set<FloatT, float_kind::bf16> {
    static constexpr auto subnormal = SCN_MAKE_FLOAT_PAIR(2e-40, BF16);
    static constexpr auto subnormal_hex = SCN_MAKE_FLOAT_PAIR(0x1.2p-130, BF16);

    static constexpr auto subnormal_max =
        SCN_MAKE_FLOAT_PAIR(1.166310801206488e-38, BF16);
    static constexpr auto subnormal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.fcp-127, BF16);

    static constexpr auto subnormal_min =
        SCN_MAKE_FLOAT_PAIR(9.18354961579912115600575419704879436e-41, BF16);
    static constexpr auto subnormal_min_hex =
        SCN_MAKE_FLOAT_PAIR(0x1p-133, BF16);

    static constexpr auto normal_max =
        SCN_MAKE_FLOAT_PAIR(3.38953138925153547590470800371487867e+38, BF16);
    static constexpr auto normal_max_hex =
        SCN_MAKE_FLOAT_PAIR(0x1.fep+127, BF16);

    static constexpr auto normal_min =
        SCN_MAKE_FLOAT_PAIR(1.17549435082228750796873653722224568e-38, BF16);
    static constexpr auto normal_min_hex = SCN_MAKE_FLOAT_PAIR(0x1p-126, BF16);

    static constexpr auto underflow_str = "1.0e-90"sv;
    static constexpr auto underflow_hex_str = "0x1p-192"sv;

    static constexpr auto overflow_str = "4.0e38"sv;
    static constexpr auto overflow_hex_str = "0x1p128"sv;
};
#endif

template <typename T>
using float_test_suite_values =
    float_test_suite_value_set<T, float_kind_for<T>>;

template <typename CharT, typename FloatT>
class FloatTestSuiteBase : public testing::Test {
protected:
    using char_type = CharT;
    using float_type = FloatT;

    using string_type = std::basic_string<char_type>;
    using string_view_type = std::basic_string_view<char_type>;

    using pair_type = std::pair<float_type, std::string_view>;

    static std::pair<float_type, std::string> make_negative(pair_type input)
    {
        SCN_EXPECT(!input.second.empty() && input.second.front() != '-' &&
                   input.second.front() != '+');
        SCN_EXPECT(!std::signbit(input.first));
        return {std::copysign(input.first, static_cast<float_type>(-1.0)),
                "-" + std::string{input.second}};
    }

    static std::string add_leading_plus(std::string_view input)
    {
        SCN_EXPECT(!input.empty() && input.front() != '-' &&
                   input.front() != '+');
        return "+" + std::string{input};
    }
};

template <typename T>
class FloatTestSuite
    : public FloatTestSuiteBase<typename T::char_type, typename T::float_type> {
    using base =
        FloatTestSuiteBase<typename T::char_type, typename T::float_type>;

protected:
    using interface_type = T;
    using values = float_test_suite_values<typename T::float_type>;

    void SetUp() override
    {
        if constexpr (!T::enabled) {
            GTEST_SKIP() << "Test suite disabled for this type";
        }
    }

    static auto run_test(interface_type& interface,
                         std::string_view source,
                         typename base::float_type& parsed)
    {
        if constexpr (std::is_same_v<typename base::char_type, char>) {
            return interface.test(source, parsed);
        }
        else {
            std::wstring wide_source{};
            std::copy(source.begin(), source.end(),
                      std::back_inserter(wide_source));
            return interface.test(std::wstring_view{wide_source}, parsed);
        }
    }

    static testing::AssertionResult test_failure(
        std::string_view input,
        enum scn::scan_error::code expected)
    {
        interface_type i{};
        typename base::float_type parsed{};

        const auto skipped_count_before =
            testing::UnitTest::GetInstance()->skipped_test_count();

        if (auto result = run_test(i, input, parsed); result) {
            if (skipped_count_before <
                testing::UnitTest::GetInstance()->skipped_test_count()) {
                return testing::AssertionSuccess() << "Test skipped";
            }
            return testing::AssertionFailure() << "Expected failure";
        }
        else if (result.error().code() != expected) {
            return testing::AssertionFailure()
                   << "Failed with wrong code, expected: " << expected
                   << " got " << result.error().code();
        }

        return testing::AssertionSuccess();
    }

    template <typename F,
              std::enable_if_t<
                  std::is_invocable_r_v<testing::AssertionResult,
                                        F&&,
                                        typename base::float_type>>* = nullptr>
    static testing::AssertionResult test(std::string_view input, F&& check)
    {
        interface_type i{};
        typename base::float_type parsed{};

        const auto skipped_count_before =
            testing::UnitTest::GetInstance()->skipped_test_count();

        if (auto result = run_test(i, input, parsed); !result) {
            return testing::AssertionFailure()
                   << "Failed with " << result.error().code() << " ("
                   << result.error().msg() << ")";
        }
        if (skipped_count_before <
            testing::UnitTest::GetInstance()->skipped_test_count()) {
            return testing::AssertionSuccess() << "Test skipped";
        }

        return std::forward<F>(check)(parsed);
    }

    static testing::AssertionResult test_success(typename base::pair_type input)
    {
        return test(input.second,
                    [expected = input.first](typename base::float_type parsed) {
                        return check_floating_eq(expected, parsed);
                    });
    }
};

TYPED_TEST_SUITE_P(FloatTestSuite);

TYPED_TEST_P(FloatTestSuite, Zero)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0."}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), ".0"}));
}

TYPED_TEST_P(FloatTestSuite, ZeroWithZeroExponent)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.0e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), ".0e0"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.0e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), ".0e+0"}));
}

TYPED_TEST_P(FloatTestSuite, ZeroWithOneExponent)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.0e1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0e1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.e1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), ".0e1"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.0e+1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0e+1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "0.e+1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), ".0e+1"}));
}

TYPED_TEST_P(FloatTestSuite, NegativeZero)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-0.0), "-0.0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-0.0), "-0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-0.0), "-0."}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-0.0), "-.0"}));
}

TYPED_TEST_P(FloatTestSuite, PositiveZero)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "+0.0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "+0."}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(0.0), "+.0"}));
}

TYPED_TEST_P(FloatTestSuite, One)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1.0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1."}));
}

TYPED_TEST_P(FloatTestSuite, OneWithExponent)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1.0e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1.e0"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1.0e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "1.e+0"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "10.0e-1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "0.1e+1"}));
}

TYPED_TEST_P(FloatTestSuite, NegativeOne)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1.0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1."}));
}

TYPED_TEST_P(FloatTestSuite, NegativeOneWithExponent)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1.0e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1.e0"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1.0e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-1.e+0"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-10.0e-1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(-1.0), "-0.1e+1"}));
}

TYPED_TEST_P(FloatTestSuite, PositiveOne)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1.0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1."}));
}

TYPED_TEST_P(FloatTestSuite, PositiveOneWithExponent)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1.0e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1e0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1.e0"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1.0e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1e+0"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+1.e+0"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+10.0e-1"}));
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(1.0), "+0.1e+1"}));
}

TYPED_TEST_P(FloatTestSuite, Pi)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(3.14)),
         "3.14"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(3.141592653589793238462643383279502884)),
         "3.141592653589793238462643383279502884"}));
}

TYPED_TEST_P(FloatTestSuite, OneThousandAndOne)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(1'001.0)),
         "1001.0"}));
}

TYPED_TEST_P(FloatTestSuite, OneThousandPlusOneThousandth)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(1'000.001)),
         "1000.001"}));
}

TYPED_TEST_P(FloatTestSuite, LeadingZeroes)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(123.456)),
         "000123.456"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(123.000456)),
         "123.000456"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(1234.56)),
         "123.456e0001"}));
}

TYPED_TEST_P(FloatTestSuite, TrailingZeroes)
{
    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(1230.456)),
         "1230.456"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(123.456)),
         "123.456000"}));

    if constexpr (std::numeric_limits<
                      typename TestFixture::float_type>::max_exponent10 > 10) {
        EXPECT_TRUE(TestFixture::test_success(
            {static_cast<typename TestFixture::float_type>(
                 SCN_FLOAT_CONSTANT(1234560000000.0)),
             "123.456e10"}));
    }
}

TYPED_TEST_P(FloatTestSuite, Hex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(0x1.ap3)),
         "0x1.ap3"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(0x1.bp2)),
         "0x1.bp2"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(0x1.cp0)),
         "0x1.c"}));

    EXPECT_TRUE(TestFixture::test_success(
        {static_cast<typename TestFixture::float_type>(
             SCN_FLOAT_CONSTANT(0x0.dp1)),
         "0x0.dp1"}));
}

TYPED_TEST_P(FloatTestSuite, Infinity)
{
    if (!TestFixture::interface_type::supports_inf) {
        GTEST_SKIP() << "Infinities not supported by the reader";
    }

    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "inf"}));
    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "infinity"}));

    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "+inf"}));
    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "+infinity"}));

    EXPECT_TRUE(TestFixture::test_success(
        {-std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "-inf"}));
    EXPECT_TRUE(TestFixture::test_success(
        {-std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "-infinity"}));

    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "InF"}));
    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "INF"}));
    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "InFiNiTy"}));
    EXPECT_TRUE(TestFixture::test_success(
        {std::numeric_limits<typename TestFixture::float_type>::infinity(),
         "INFINITY"}));
}

TYPED_TEST_P(FloatTestSuite, Nan)
{
    if (!std::numeric_limits<typename TestFixture::float_type>::has_quiet_NaN ||
        finite_math_only) {
        GTEST_SKIP() << "NaNs not supported by the float type";
    }
    if (!TestFixture::interface_type::supports_nan) {
        GTEST_SKIP() << "NaNs not supported by the reader";
    }

    const auto check = [](typename TestFixture::float_type parsed) {
        return check_nan_eq(
            parsed,
            std::numeric_limits<typename TestFixture::float_type>::quiet_NaN());
    };

    EXPECT_TRUE(TestFixture::test("nan", check));
    EXPECT_TRUE(TestFixture::test("NaN", check));
    EXPECT_TRUE(TestFixture::test("NAN", check));
}

template <typename T>
T make_nan_with_payload(const char* payload);

template <>
inline float make_nan_with_payload(const char* payload)
{
#if SCN_HAS_BUILTIN(__builtin_nanf)
    return __builtin_nanf(payload);
#else
    return std::nanf(payload);
#endif
}

template <>
inline double make_nan_with_payload(const char* payload)
{
#if SCN_HAS_BUILTIN(__builtin_nan)
    return __builtin_nan(payload);
#else
    return std::nan(payload);
#endif
}

template <>
inline long double make_nan_with_payload(const char* payload)
{
#if SCN_HAS_BUILTIN(__builtin_nanl)
    return __builtin_nanl(payload);
#else
    return std::nanl(payload);
#endif
}

template <typename T>
bool can_make_nan_with_payload()
{
    return !check_nan_eq(make_nan_with_payload<T>("0"),
                         make_nan_with_payload<T>("1234"));
}

TYPED_TEST_P(FloatTestSuite, NanWithPayload)
{
    if (!std::numeric_limits<typename TestFixture::float_type>::has_quiet_NaN ||
        finite_math_only) {
        GTEST_SKIP() << "NaNs not supported by the float type";
    }
    if (!TestFixture::interface_type::supports_nan) {
        GTEST_SKIP() << "NaNs not supported by the reader";
    }

    const auto make_check = [](const char* payload) {
        SCN_EXPECT(payload);
        return [payload](typename TestFixture::float_type parsed) {
            if constexpr (std::is_same_v<typename TestFixture::float_type,
                                         float> ||
                          std::is_same_v<typename TestFixture::float_type,
                                         double> ||
                          std::is_same_v<typename TestFixture::float_type,
                                         long double>) {
                if (!can_make_nan_with_payload<
                        typename TestFixture::float_type>()) {
                    static bool warned = false;
                    if (!warned) {
                        warned = true;
                        std::cerr << "The input of std::nan is ignored "
                                     "on this platform. "
                                     "Contents of NaN payloads are not checked "
                                     "in this test.\n";
                        // TODO: maybe still check them somehow, perhaps by
                        // constructing the NaN here by hand
                    }

                    return testing::AssertionResult(std::isnan(parsed));
                }

                return check_nan_eq(
                    parsed,
                    make_nan_with_payload<typename TestFixture::float_type>(
                        payload));
            }
            else {
                // TODO: check payloads for other float types
                SCN_UNUSED(payload);
                return testing::AssertionResult(std::isnan(parsed));
            }
        };
    };

    EXPECT_TRUE(TestFixture::test("nan(0)", make_check("0")));
    EXPECT_TRUE(TestFixture::test("nan(1234)", make_check("1234")));
    EXPECT_TRUE(TestFixture::test("nan(01234)", make_check("01234")));
    EXPECT_TRUE(TestFixture::test("nan(0x1234)", make_check("0x1234")));
    EXPECT_TRUE(TestFixture::test("nan()", make_check("")));

    EXPECT_TRUE(TestFixture::test("nan(Foo_Bar)", make_check("Foo_Bar")));
    EXPECT_TRUE(TestFixture::test("nan(Foo_Bar)", make_check("")));
}

TYPED_TEST_P(FloatTestSuite, Overflow)
{
    EXPECT_TRUE(
        TestFixture::test_failure(TestFixture::values::overflow_str,
                                  scn::scan_error::value_positive_overflow));
}

TYPED_TEST_P(FloatTestSuite, OverflowHex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    EXPECT_TRUE(
        TestFixture::test_failure(TestFixture::values::overflow_hex_str,
                                  scn::scan_error::value_positive_overflow));
}

TYPED_TEST_P(FloatTestSuite, Underflow)
{
    EXPECT_TRUE(
        TestFixture::test_failure(TestFixture::values::underflow_str,
                                  scn::scan_error::value_positive_underflow));
}

TYPED_TEST_P(FloatTestSuite, UnderflowHex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    EXPECT_TRUE(
        TestFixture::test_failure(TestFixture::values::underflow_hex_str,
                                  scn::scan_error::value_positive_underflow));
}

TYPED_TEST_P(FloatTestSuite, Subnormal)
{
    ASSERT_FALSE(std::isnormal(TestFixture::values::subnormal.first));
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::subnormal));
}

TYPED_TEST_P(FloatTestSuite, SubnormalHex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    ASSERT_FALSE(std::isnormal(TestFixture::values::subnormal_hex.first));
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::subnormal_hex));
}

TYPED_TEST_P(FloatTestSuite, SubnormalMax)
{
    ASSERT_EQ(
        std::nextafter(
            TestFixture::values::subnormal_max.first,
            std::numeric_limits<typename TestFixture::float_type>::infinity()),
        std::numeric_limits<typename TestFixture::float_type>::min());
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::subnormal_max));
}

TYPED_TEST_P(FloatTestSuite, SubnormalMaxHex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    ASSERT_EQ(
        std::nextafter(
            TestFixture::values::subnormal_max_hex.first,
            std::numeric_limits<typename TestFixture::float_type>::infinity()),
        std::numeric_limits<typename TestFixture::float_type>::min());
    EXPECT_TRUE(
        TestFixture::test_success(TestFixture::values::subnormal_max_hex));
}

TYPED_TEST_P(FloatTestSuite, SubnormalMin)
{
    ASSERT_EQ(
        TestFixture::values::subnormal_min.first,
        std::numeric_limits<typename TestFixture::float_type>::denorm_min());
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::subnormal_min));
}

TYPED_TEST_P(FloatTestSuite, SubnormalMinHex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    ASSERT_EQ(
        TestFixture::values::subnormal_min_hex.first,
        std::numeric_limits<typename TestFixture::float_type>::denorm_min());
    EXPECT_TRUE(
        TestFixture::test_success(TestFixture::values::subnormal_min_hex));
}

TYPED_TEST_P(FloatTestSuite, Max)
{
    ASSERT_EQ(TestFixture::values::normal_max.first,
              std::numeric_limits<typename TestFixture::float_type>::max());
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::normal_max));
}

TYPED_TEST_P(FloatTestSuite, MaxHex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    ASSERT_EQ(TestFixture::values::normal_max_hex.first,
              std::numeric_limits<typename TestFixture::float_type>::max());
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::normal_max_hex));
}

TYPED_TEST_P(FloatTestSuite, NormalMin)
{
    ASSERT_EQ(TestFixture::values::normal_min.first,
              std::numeric_limits<typename TestFixture::float_type>::min());
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::normal_min));
}

TYPED_TEST_P(FloatTestSuite, NormalMinHex)
{
    if (!TestFixture::interface_type::supports_hex) {
        GTEST_SKIP() << "Hexfloats not supported by the reader";
    }

    ASSERT_EQ(TestFixture::values::normal_min_hex.first,
              std::numeric_limits<typename TestFixture::float_type>::min());
    EXPECT_TRUE(TestFixture::test_success(TestFixture::values::normal_min_hex));
}

REGISTER_TYPED_TEST_SUITE_P(FloatTestSuite,
                            Zero,
                            ZeroWithZeroExponent,
                            ZeroWithOneExponent,
                            NegativeZero,
                            PositiveZero,
                            One,
                            OneWithExponent,
                            NegativeOne,
                            NegativeOneWithExponent,
                            PositiveOne,
                            PositiveOneWithExponent,
                            Pi,
                            OneThousandAndOne,
                            OneThousandPlusOneThousandth,
                            LeadingZeroes,
                            TrailingZeroes,
                            Hex,
                            Infinity,
                            Nan,
                            NanWithPayload,
                            Overflow,
                            OverflowHex,
                            Underflow,
                            UnderflowHex,
                            Subnormal,
                            SubnormalHex,
                            SubnormalMax,
                            SubnormalMaxHex,
                            SubnormalMin,
                            SubnormalMinHex,
                            Max,
                            MaxHex,
                            NormalMin,
                            NormalMinHex);

template <template <typename, typename> class T>
using float_test_suite_types = ::testing::Types<T<char, float>,
                                                T<char, double>,
                                                T<char, long double>,
                                                T<wchar_t, float>,
                                                T<wchar_t, double>,
                                                T<wchar_t, long double>

#if SCN_HAS_STD_F16
                                                ,
                                                T<char, std::float16_t>,
                                                T<wchar_t, std::float16_t>
#endif

#if SCN_HAS_STD_F32
                                                ,
                                                T<char, std::float32_t>,
                                                T<wchar_t, std::float32_t>
#endif

#if SCN_HAS_STD_F64
                                                ,
                                                T<char, std::float64_t>,
                                                T<wchar_t, std::float64_t>
#endif

#if SCN_HAS_STD_F128
                                                ,
                                                T<char, std::float128_t>,
                                                T<wchar_t, std::float128_t>
#endif

#if SCN_HAS_STD_BF16
                                                ,
                                                T<char, std::bfloat16_t>,
                                                T<wchar_t, std::bfloat16_t>
#endif
                                                >;
