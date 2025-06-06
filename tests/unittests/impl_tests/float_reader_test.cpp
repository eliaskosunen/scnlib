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
#include <iomanip>

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
    return os.str();
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

    if constexpr (sizeof(T) <= sizeof(double)) {
        return testing::AssertionFailure()
               << "Floats not equal: " << a << " and " << b;
    }
    else {
        return testing::AssertionFailure()
               << "Floats not equal: " << static_cast<long double>(a) << " and "
               << static_cast<long double>(b);
    }
}

template <typename T>
[[nodiscard]] static testing::AssertionResult check_nan_eq(T lhs, T rhs)
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

using namespace std::string_view_literals;

template <bool Localized, typename CharT, typename ValueT>
using float_reader_wrapper =
    reader_wrapper<Localized, CharT, ValueT, scn::impl::reader_impl_for_float>;

using scn::detail::mp_bool;
using scn::detail::mp_cond;
using scn::detail::mp_value;

#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
inline constexpr bool finite_math_only = true;
#else
inline constexpr bool finite_math_only = false;
#endif

SCN_GCC_COMPAT_PUSH
SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")

template <typename T>
class FloatValueReaderTest : public testing::Test {
protected:
    using char_type = typename T::char_type;
    using float_type = typename T::value_type;
    using string_type = std::basic_string<char_type>;
    using string_view_type = std::basic_string_view<char_type>;

    static constexpr bool is_localized = T::is_localized;

    static constexpr bool is_char = std::is_same_v<char, char_type>;
    static constexpr bool is_wchar = std::is_same_v<wchar_t, char_type>;

    template <typename FloatT>
    static constexpr bool is_type = std::is_same_v<FloatT, float_type>;

    static constexpr bool is_extended =
#if SCN_HAS_STD_F16
        is_type<std::float16_t> ||
#endif
#if SCN_HAS_STD_F32
        is_type<std::float32_t> ||
#endif
#if SCN_HAS_STD_F64
        is_type<std::float64_t> ||
#endif
#if SCN_HAS_STD_F128
        is_type<std::float128_t> ||
#endif
#if SCN_HAS_STD_BF16
        is_type<std::bfloat16_t> ||
#endif
        false;

    enum class float_kind {
        f16,
        f32,
        f64,
        f80,    // x87
        f128,   // 128-bit ieee float
        bf16,   // bfloat16
        f2x64,  // double-double
    };

    static constexpr float_kind kind =
        mp_cond<mp_bool<sizeof(float_type) == 4>,
                mp_value<float_kind::f32>,
                mp_bool<sizeof(float_type) == 8>,
                mp_value<float_kind::f64>,
                mp_bool<std::numeric_limits<float_type>::digits == 64>,
                mp_value<float_kind::f80>,
                mp_bool<std::numeric_limits<float_type>::digits == 113>,
                mp_value<float_kind::f128>,
                mp_bool<std::numeric_limits<float_type>::digits == 11>,
                mp_value<float_kind::f16>,
                mp_bool<std::numeric_limits<float_type>::digits == 8>,
                mp_value<float_kind::bf16>,
                mp_bool<std::numeric_limits<float_type>::digits == 106>,
                mp_value<float_kind::f2x64>>::value;

    static_assert(std::numeric_limits<float_type>::is_iec559);

    void SetUp() override
    {
        // Check whether this type is supported.
        // If not, skip the test (don't fail).
        // This is only a runtime-checkable property for now,
        // so we can't disable the tests beforehand.
        // This can only fail for extended float types,
        // so if we have none of them, we don't need to run this check.

#if SCN_HAS_STD_F16 || SCN_HAS_STD_F32 || SCN_HAS_STD_F64 || \
    SCN_HAS_STD_F128 || SCN_HAS_STD_BF16
        T tmp_reader{};
        float_type value{};

        auto result = [&]() {
            if constexpr (is_char) {
                return tmp_reader.read_default("42.0"sv, value);
            }
            else {
                return tmp_reader.read_default(L"42.0"sv, value);
            }
        }();

        if (!result) {
            if (result.error().code() == scn::scan_error::type_not_supported) {
                GTEST_SKIP() << "Type not supported";
            }
        }
        ASSERT_TRUE(result);
        ASSERT_TRUE(check_floating_eq(value, static_cast<float_type>(42.0)));
#endif
    }

#if SCN_HAS_STD_F128
#define F_C(x) static_cast<float_type>(x##F128)
#else
#define F_C(x) static_cast<float_type>(x##L)
#endif

    constexpr static auto get_pi()
    {
        return std::pair(F_C(3.14), "3.14"sv);
    }
    constexpr static auto get_neg()
    {
        return std::pair(F_C(-123.456), "-123.456"sv);
    }
    constexpr static auto get_leading_plus()
    {
        return std::pair(F_C(3.14), "+3.14"sv);
    }

#define MAKE_PAIR_RETURN(Value, LiteralSuffix) \
    std::pair{Value##LiteralSuffix, #Value}

#if SCN_HAS_STD_F128
#define MAKE_PAIR_RETURN_F128(Value) std::pair{Value##F128, #Value}
#else
#define MAKE_PAIR_RETURN_F128(Value) std::pair{Value##L, #Value}
#endif

#define MAKE_CHECKED_PAIR_RETURN(Value, LiteralSuffix, Check) \
    []() {                                                    \
        static_assert(Value##LiteralSuffix == Check);         \
        return MAKE_PAIR_RETURN(Value, LiteralSuffix);        \
    }()

#if SCN_HAS_STD_F128
#define MAKE_CHECKED_PAIR_RETURN_F128(Value, Check) \
    []() {                                          \
        static_assert(Value##F128 == Check);        \
        return MAKE_PAIR_RETURN_F128(Value);        \
    }()
#else
#define MAKE_CHECKED_PAIR_RETURN_F128(Value, Check) \
    []() {                                          \
        static_assert(Value##L == Check);           \
        return MAKE_PAIR_RETURN_F128(Value);        \
    }()
#endif

    constexpr static auto get_subnormal_()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_PAIR_RETURN(2e-40, f);
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_PAIR_RETURN(5e-320, );
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_PAIR_RETURN(3e-4940, L);
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_PAIR_RETURN_F128(5e-4960);
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_PAIR_RETURN(5e-320, L);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_PAIR_RETURN(5e-6, F16);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_PAIR_RETURN(2e-40, BF16);
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static auto get_subnormal_hex_()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_PAIR_RETURN(0x1.2p-130, f);
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_PAIR_RETURN(0x1.2p-1050, );
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_PAIR_RETURN(0x1.2p-16400, L);
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_PAIR_RETURN_F128(0x1.2p-16450);
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_PAIR_RETURN(0x1.2p-1050, L);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_PAIR_RETURN(0x1.2p-16, F16);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_PAIR_RETURN(0x1.2p-130, BF16);
        }
#endif
        SCN_EXPECT(false);
    }

    static auto get_subnormal() -> std::pair<float_type, std::string_view>
    {
        auto r = get_subnormal_();
        EXPECT_FALSE(std::isnormal(r.first));
        return r;
    }
    static auto get_subnormal_hex() -> std::pair<float_type, std::string_view>
    {
        auto r = get_subnormal_hex_();
        EXPECT_FALSE(std::isnormal(r.first));
        return r;
    }

    constexpr static auto get_subnormal_max_()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_PAIR_RETURN(1.1754942106924410754870294448492873e-38,
                                    f);
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_PAIR_RETURN(
                2.2250738585072008890245868760858599e-308, );
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_PAIR_RETURN(3.3621031431120935058981578641335051e-4932,
                                    L);
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_PAIR_RETURN_F128(
                3.3621031431120935062626778173217519551e-4932);
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_PAIR_RETURN(2.004168360008972777996108051350113e-292,
                                    L);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_PAIR_RETURN(1.219511032104492e-4, F16);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_PAIR_RETURN(1.166310801206488e-38, BF16);
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static auto get_subnormal_max_hex_()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_PAIR_RETURN(0x1.fffffcp-127, f);
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_PAIR_RETURN(0x1.ffffffffffffep-1023, );
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_PAIR_RETURN(0x7.fffffffffffffffp-16385, L);
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_PAIR_RETURN_F128(
                0x1.fffffffffffffffffffffffffffep-16383);
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_PAIR_RETURN(0x1.ffffffffffffffffffffffffffp-970, L);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_PAIR_RETURN(0x1.ff8p-15, F16);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_PAIR_RETURN(0x1.7cp-127, BF16);
        }
#endif
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    static auto get_subnormal_max() -> std::pair<float_type, std::string_view>
    {
        auto r = get_subnormal_max_();
        EXPECT_FALSE(std::isnormal(r.first));
        EXPECT_TRUE(check_floating_eq(
            std::nextafter(r.first, std::numeric_limits<float_type>::max()),
            std::numeric_limits<float_type>::min()));
        return r;
    }
    static auto get_subnormal_max_hex()
        -> std::pair<float_type, std::string_view>
    {
        auto r = get_subnormal_max_hex_();
        EXPECT_FALSE(std::isnormal(r.first));
        EXPECT_TRUE(check_floating_eq(
            std::nextafter(r.first, std::numeric_limits<float_type>::max()),
            std::numeric_limits<float_type>::min()));
        return r;
    }

    static auto get_normal_min() -> std::pair<float_type, std::string>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_CHECKED_PAIR_RETURN(
                1.17549435082228750796873653722224568e-38, f,
                std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_CHECKED_PAIR_RETURN(
                2.22507385850720138309023271733240406e-308, ,
                std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_CHECKED_PAIR_RETURN(
                3.36210314311209350626267781732175260e-4932, L,
                std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_CHECKED_PAIR_RETURN_F128(
                3.36210314311209350626267781732175260e-4932,
                std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_CHECKED_PAIR_RETURN(
                2.00416836000897277799610805135016205e-292, L,
                std::numeric_limits<float_type>::min());
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_CHECKED_PAIR_RETURN(
                6.103515625e-5, F16, std::numeric_limits<float_type>::min());
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_CHECKED_PAIR_RETURN(
                1.17549435082228750796873653722224568e-38, BF16,
                std::numeric_limits<float_type>::min());
        }
#endif
        SCN_EXPECT(false);
    }
    static auto get_normal_min_hex() -> std::pair<float_type, std::string>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-126, f, std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-1022, , std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-16382, L, std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_CHECKED_PAIR_RETURN_F128(
                0x1p-16382, std::numeric_limits<float_type>::min());
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-969, L, std::numeric_limits<float_type>::min());
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-14, F16, std::numeric_limits<float_type>::min());
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-126, BF16, std::numeric_limits<float_type>::min());
        }
#endif
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    static auto get_subnormal_min() -> std::pair<float_type, std::string>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_CHECKED_PAIR_RETURN(
                1.40129846432481707092372958328991613e-45, f,
                std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_CHECKED_PAIR_RETURN(
                4.94065645841246544176568792868221372e-324, ,
                std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_CHECKED_PAIR_RETURN(
                3.64519953188247460252840593361941982e-4951, L,
                std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_CHECKED_PAIR_RETURN_F128(
                6.47517511943802511092443895822764655e-4966,
                std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_CHECKED_PAIR_RETURN(
                4.940656458412465441765687928682214e-324, L,
                std::numeric_limits<float_type>::denorm_min());
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_CHECKED_PAIR_RETURN(
                5.9604644775390625e-8, F16,
                std::numeric_limits<float_type>::denorm_min());
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_CHECKED_PAIR_RETURN(
                9.18354961579912115600575419704879436e-41, BF16,
                std::numeric_limits<float_type>::denorm_min());
        }
#endif
        SCN_EXPECT(false);
    }
    static auto get_subnormal_min_hex() -> std::pair<float_type, std::string>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-149, f, std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-1074, , std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f80) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-16445, L, std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_CHECKED_PAIR_RETURN_F128(
                0x1p-16494, std::numeric_limits<float_type>::denorm_min());
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-1074, L, std::numeric_limits<float_type>::denorm_min());
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-24, F16, std::numeric_limits<float_type>::denorm_min());
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1p-133, BF16, std::numeric_limits<float_type>::denorm_min());
        }
#endif
        SCN_EXPECT(false);
    }

    constexpr static std::string_view get_underflow()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "1.0e-90"sv;
        }
        else if constexpr (kind == float_kind::f64 ||
                           kind == float_kind::f2x64) {
            return "5.0e-400"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "4.0e-5500"sv;
        }
        else if constexpr (kind == float_kind::f16) {
            return "5.0e-16"sv;
        }
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_underflow_hex()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "0x1p-192"sv;
        }
        else if constexpr (kind == float_kind::f64 ||
                           kind == float_kind::f2x64) {
            return "0x1p-1200"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "0x1p-18000"sv;
        }
        else if constexpr (kind == float_kind::f16) {
            return "0x1p-40"sv;
        }
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_underflow_neg()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "-1.0e-90"sv;
        }
        else if constexpr (kind == float_kind::f64 ||
                           kind == float_kind::f2x64) {
            return "-5.0e-400"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "-4.0e-5500"sv;
        }
        else if constexpr (kind == float_kind::f16) {
            return "-5.0e-16"sv;
        }
        SCN_EXPECT(false);
    }

    static auto get_maximum() -> std::pair<float_type, std::string>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_CHECKED_PAIR_RETURN(
                3.40282346638528859811704183484516925e+38, f,
                std::numeric_limits<float_type>::max());
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_CHECKED_PAIR_RETURN(
                1.79769313486231570814527423731704357e+308, ,
                std::numeric_limits<float_type>::max());
        }
        // MSVC hard-errors with these float constants (C2177),
        // even though we're never hitting this code there
#if !SCN_MSVC
        else if constexpr (kind == float_kind::f80) {
            return MAKE_CHECKED_PAIR_RETURN(
                1.18973149535723176502126385303097021e+4932, L,
                std::numeric_limits<float_type>::max());
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_CHECKED_PAIR_RETURN_F128(
                1.18973149535723176508575932662800702e+4932,
                std::numeric_limits<float_type>::max());
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_CHECKED_PAIR_RETURN(
                1.797693134862315807937289714053012e+308, L,
                std::numeric_limits<float_type>::max());
        }
#endif
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_CHECKED_PAIR_RETURN(
                6.5504e+4, F16, std::numeric_limits<float_type>::max());
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_CHECKED_PAIR_RETURN(
                3.38953138925153547590470800371487867e+38, BF16,
                std::numeric_limits<float_type>::max());
        }
#endif
        SCN_EXPECT(false);
    }
    static auto get_maximum_hex() -> std::pair<float_type, std::string>
    {
        if constexpr (kind == float_kind::f32) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1.fffffep+127, f, std::numeric_limits<float_type>::max());
        }
        else if constexpr (kind == float_kind::f64) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1.fffffffffffffp+1023, ,
                std::numeric_limits<float_type>::max());
        }
#if !SCN_MSVC
        else if constexpr (kind == float_kind::f80) {
            return MAKE_CHECKED_PAIR_RETURN(
                0xf.fffffffffffffffp+16380, L,
                std::numeric_limits<float_type>::max());
        }
        else if constexpr (kind == float_kind::f128) {
            return MAKE_CHECKED_PAIR_RETURN_F128(
                0x1.ffffffffffffffffffffffffffffp+16383,
                std::numeric_limits<float_type>::max());
        }
        else if constexpr (kind == float_kind::f2x64) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1.fffffffffffff7ffffffffffff8p+1023, L,
                std::numeric_limits<float_type>::max());
        }
#endif
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1.ffcp+15, F16, std::numeric_limits<float_type>::max());
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return MAKE_CHECKED_PAIR_RETURN(
                0x1.fep+127, BF16, std::numeric_limits<float_type>::max());
        }
#endif
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    constexpr static std::string_view get_overflow()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "4.0e38"sv;
        }
        else if constexpr (kind == float_kind::f64 ||
                           kind == float_kind::f2x64) {
            return "2.0e308"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "2.0e4932"sv;
        }
        else if constexpr (kind == float_kind::f16) {
            return "7.0e4"sv;
        }
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_overflow_hex()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "0x1p+128"sv;
        }
        else if constexpr (kind == float_kind::f64 ||
                           kind == float_kind::f2x64) {
            return "0x1p+1024"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "0x1p+16384"sv;
        }
        else if constexpr (kind == float_kind::f16) {
            return "0x1p+16"sv;
        }
        SCN_EXPECT(false);
    }

    constexpr static std::string_view get_overflow_neg()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "-4.0e38"sv;
        }
        else if constexpr (kind == float_kind::f64 ||
                           kind == float_kind::f2x64) {
            return "-2.0e308"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "-2.0e4932"sv;
        }
        else if constexpr (kind == float_kind::f16) {
            return "-7.0e4"sv;
        }
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_overflow_neg_hex()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "-0x1p+128"sv;
        }
        else if constexpr (kind == float_kind::f64 ||
                           kind == float_kind::f2x64) {
            return "-0x1p+1024"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "-0x1p+16384"sv;
        }
        else if constexpr (kind == float_kind::f16) {
            return "-0x1p+16"sv;
        }
        SCN_EXPECT(false);
    }

    static auto get_thsep_number()
    {
        return static_cast<float_type>(123456.789L);
    }

    template <typename Source>
    void set_source(Source&& s)
    {
        if constexpr (is_char) {
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
        return std::pair(result, val);
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
        return std::pair(result, val);
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
                     float_reader_wrapper<false, wchar_t, double>,
                     float_reader_wrapper<false, char, long double>,
                     float_reader_wrapper<false, wchar_t, long double>
#if !SCN_DISABLE_LOCALE
                     ,
                     float_reader_wrapper<true, char, float>,
                     float_reader_wrapper<true, char, double>,
                     float_reader_wrapper<true, wchar_t, float>,
                     float_reader_wrapper<true, wchar_t, double>,
                     float_reader_wrapper<true, char, long double>,
                     float_reader_wrapper<true, wchar_t, long double>
#endif

#if SCN_HAS_STD_F16
                     ,
                     float_reader_wrapper<false, char, std::float16_t>,
                     float_reader_wrapper<false, wchar_t, std::float16_t>
#endif

#if SCN_HAS_STD_F32
                     ,
                     float_reader_wrapper<false, char, std::float32_t>,
                     float_reader_wrapper<false, wchar_t, std::float32_t>
#endif

#if SCN_HAS_STD_F64
                     ,
                     float_reader_wrapper<false, char, std::float64_t>,
                     float_reader_wrapper<false, wchar_t, std::float64_t>
#endif

#if SCN_HAS_STD_F128
                     ,
                     float_reader_wrapper<false, char, std::float128_t>,
                     float_reader_wrapper<false, wchar_t, std::float128_t>
#endif

#if SCN_HAS_STD_BF16
                     ,
                     float_reader_wrapper<false, char, std::bfloat16_t>,
                     float_reader_wrapper<false, wchar_t, std::bfloat16_t>
#endif
                     >;

SCN_GCC_COMPAT_POP  // -Wfloat-equal

#undef F_C
#if SCN_HAS_STD_F128
#define F_C(x) static_cast<typename TestFixture::float_type>(x##F128)
#else
#define F_C(x) static_cast<typename TestFixture::float_type>(x##L)
#endif

    SCN_CLANG_PUSH SCN_CLANG_IGNORE("-Wgnu-zero-variadic-macro-arguments")

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
    EXPECT_TRUE(this->simple_default_test(
        "4.20e1", static_cast<typename TestFixture::float_type>(42.0)));
}

TYPED_TEST(FloatValueReaderTest, Hex)
{
    EXPECT_TRUE(this->simple_default_test(
        "0x1.2ap3", static_cast<typename TestFixture::float_type>(0x1.2ap3)));
}
TYPED_TEST(FloatValueReaderTest, NegativeHex)
{
    EXPECT_TRUE(this->simple_default_test(
        "-0x1.2ap3", static_cast<typename TestFixture::float_type>(-0x1.2ap3)));
}

SCN_CLANG_POP  // -Wdouble-promotion

TYPED_TEST(FloatValueReaderTest, InfinityWithInf)
{
    auto [a, _, val] = this->simple_success_test("inf");
    EXPECT_TRUE(a);
    if constexpr (std::numeric_limits<
                      typename TestFixture::float_type>::has_infinity &&
                  !finite_math_only) {
        EXPECT_TRUE(std::isinf(val));
    }
    EXPECT_FALSE(std::signbit(val));
}
TYPED_TEST(FloatValueReaderTest, InfinityWithNegInfinity)
{
    auto [a, _, val] = this->simple_success_test("-infinity");
    EXPECT_TRUE(a);
    if constexpr (std::numeric_limits<
                      typename TestFixture::float_type>::has_infinity &&
                  !finite_math_only) {
        EXPECT_TRUE(std::isinf(val));
    }
    EXPECT_TRUE(std::signbit(val));
}

TYPED_TEST(FloatValueReaderTest, NaN)
{
    auto [a, _, val] = this->simple_success_test("nan");
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::signbit(val));
    if constexpr (std::numeric_limits<
                      typename TestFixture::float_type>::has_quiet_NaN &&
                  !finite_math_only) {
        EXPECT_TRUE(std::isnan(val));
        EXPECT_TRUE(
            check_nan_eq(val, std::numeric_limits<decltype(val)>::quiet_NaN()));
    }
}
TYPED_TEST(FloatValueReaderTest, NaNWithPayload)
{
    auto [a, _, val] = this->simple_success_test("nan(1234)");
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::signbit(val));
    if constexpr (std::numeric_limits<
                      typename TestFixture::float_type>::has_quiet_NaN &&
                  !finite_math_only) {
        EXPECT_TRUE(std::isnan(val));
#if !SCN_S390  // s390x discards NaN payloads on function return
        EXPECT_FALSE(
            check_nan_eq(val, std::numeric_limits<decltype(val)>::quiet_NaN()));
#endif
#if SCN_POSIX
        if constexpr (std::is_same_v<decltype(val), float>) {
            EXPECT_TRUE(check_nan_eq(val, std::nanf("1234")));
        }
        else if constexpr (std::is_same_v<decltype(val), double>) {
            EXPECT_TRUE(check_nan_eq(val, std::nan("1234")));
        }
        else if constexpr (std::is_same_v<decltype(val), long double>) {
            EXPECT_TRUE(check_nan_eq(val, std::nanl("1234")));
        }
#endif
    }
}
TYPED_TEST(FloatValueReaderTest, NaNWithEmptyPayload)
{
    auto [a, _, val] = this->simple_success_test("nan()");
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::signbit(val));
    if constexpr (std::numeric_limits<
                      typename TestFixture::float_type>::has_quiet_NaN &&
                  !finite_math_only) {
        EXPECT_TRUE(std::isnan(val));
        EXPECT_TRUE(
            check_nan_eq(val, std::numeric_limits<decltype(val)>::quiet_NaN()));
#if SCN_POSIX
        if constexpr (std::is_same_v<decltype(val), float>) {
            EXPECT_TRUE(check_nan_eq(val, std::nanf("")));
        }
        else if constexpr (std::is_same_v<decltype(val), double>) {
            EXPECT_TRUE(check_nan_eq(val, std::nan("")));
        }
        else if constexpr (std::is_same_v<decltype(val), long double>) {
            EXPECT_TRUE(check_nan_eq(val, std::nanl("")));
        }
#endif
    }
}
TYPED_TEST(FloatValueReaderTest, NanWithNonNumericPayload)
{
    auto [a, _, val] = this->simple_success_test("nan(HelloWorld)");
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::signbit(val));
    if constexpr (std::numeric_limits<
                      typename TestFixture::float_type>::has_quiet_NaN &&
                  !finite_math_only) {
        EXPECT_TRUE(std::isnan(val));
        EXPECT_TRUE(
            check_nan_eq(val, std::numeric_limits<decltype(val)>::quiet_NaN()));
#if SCN_POSIX
        if constexpr (std::is_same_v<decltype(val), float>) {
            EXPECT_TRUE(check_nan_eq(val, std::nanf("")));
            EXPECT_TRUE(check_nan_eq(val, std::nanf("HelloWorld")));
        }
        else if constexpr (std::is_same_v<decltype(val), double>) {
            EXPECT_TRUE(check_nan_eq(val, std::nan("")));
            EXPECT_TRUE(check_nan_eq(val, std::nan("HelloWorld")));
        }
        else if constexpr (std::is_same_v<decltype(val), long double>) {
            EXPECT_TRUE(check_nan_eq(val, std::nanl("")));
            EXPECT_TRUE(check_nan_eq(val, std::nanl("HelloWorld")));
        }
#endif
    }
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

TYPED_TEST(FloatValueReaderTest, MaximumSubnormal)
{
    const auto [orig_val, source] = this->get_subnormal_max();
    auto [a, _, val] = this->simple_success_test(source);
    EXPECT_TRUE(a);
    EXPECT_FALSE(std::isnormal(val));
    EXPECT_TRUE(check_floating_eq(val, orig_val));
}
TYPED_TEST(FloatValueReaderTest, MaximumSubnormalFromHex)
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
        val, static_cast<typename TestFixture::float_type>(F_C(12.3e4))));
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
        val, static_cast<typename TestFixture::float_type>(F_C(12.3))));
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
        val, static_cast<typename TestFixture::float_type>(F_C(1.0))));
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
    else if constexpr (std::is_same_v<T, long double>) {
        return std::strtold(input.c_str(), nullptr);
    }
    SCN_EXPECT(false);
}

TYPED_TEST(FloatValueReaderTest, PresentationHexValueScientific)
{
    if constexpr (TestFixture::is_extended) {
        return SUCCEED() << "This test requires a non-extended float type";
    }

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
    if constexpr (TestFixture::is_extended) {
        return SUCCEED() << "This test requires a non-extended float type";
    }

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
        val, static_cast<typename TestFixture::float_type>(F_C(0x1.fp3))));
}
TYPED_TEST(FloatValueReaderTest, PresentationHexValueHexWithoutPrefix)
{
    auto [a, _, val] = this->simple_success_specs_test(
        "1.fp3", this->make_format_specs_with_presentation(
                     scn::detail::presentation_type::float_hex));
    EXPECT_TRUE(a);
    EXPECT_TRUE(check_floating_eq(
        val, static_cast<typename TestFixture::float_type>(F_C(0x1.fp3))));
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
