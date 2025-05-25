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
constexpr T float_zero()
{
    return static_cast<T>(0.0L);
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
    }

    return testing::AssertionSuccess();
}

using namespace std::string_view_literals;

template <bool Localized, typename CharT, typename ValueT>
using float_reader_wrapper =
    reader_wrapper<Localized, CharT, ValueT, scn::impl::reader_impl_for_float>;

using scn::detail::mp_bool;
using scn::detail::mp_cond;
using scn::detail::mp_value;

template <typename T>
class FloatValueReaderTest : public testing::Test {
protected:
    using char_type = typename T::char_type;
    using float_type = typename T::value_type;
    using string_type = std::basic_string<char_type>;
    using string_view_type = std::basic_string_view<char_type>;

    static constexpr bool is_localized = T::is_localized;

    template <typename CharT>
    static constexpr bool is_char = std::is_same_v<CharT, char_type>;

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
        f80,  // x87
        f128,
        bf16,
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
                mp_value<float_kind::bf16>>::value;

    static_assert(std::numeric_limits<float_type>::is_iec559);

    static constexpr const char* get_length_flag()
    {
        if constexpr (is_type<long double> ||
                      sizeof(float_type) > sizeof(double)) {
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
            if constexpr (is_type<double> || is_type<long double>) {
                return val;
            }
            else if constexpr (sizeof(float_type) > sizeof(double)) {
                return static_cast<long double>(val);
            }
            else {
                return static_cast<double>(val);
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

#if SCN_HAS_STD_F128
#define SCN_F_C(x) static_cast<float_type>(x##F128)
#else
#define SCN_F_C(x) static_cast<float_type>(x##L)
#endif

    constexpr static auto get_pi()
    {
        return std::pair(SCN_F_C(3.14), "3.14"sv);
    }
    constexpr static auto get_neg()
    {
        return std::pair(SCN_F_C(-123.456), "-123.456"sv);
    }
    constexpr static auto get_leading_plus()
    {
        return std::pair(SCN_F_C(3.14), "+3.14"sv);
    }

    SCN_GCC_COMPAT_PUSH
    // 128-bit literals will overflow if long double is not 128-bit,
    // but that's not a problem since we won't be hitting that code in that case
    SCN_GCC_COMPAT_IGNORE("-Woverflow")

    constexpr static auto get_subnormal()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return std::pair(2e-40f, "2e-40"sv);
        }
        else if constexpr (kind == float_kind::f64) {
            return std::pair(5e-320, "5e-320"sv);
        }
        else if constexpr (kind == float_kind::f80) {
            return std::pair(3e-4940l, "3e-4940"sv);
        }
        else if constexpr (kind == float_kind::f128) {
            return std::pair(SCN_F_C(5e-4960), "5e-4960"sv);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return std::pair(5e-6F16, "5e-6"sv);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return std::pair(2e-40BF16, "2e-40"sv);
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static auto get_subnormal_hex()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return std::pair(0x1.2p-130f, "0x1.2p-130"sv);
        }
        else if constexpr (kind == float_kind::f64) {
            return std::pair(0x1.2p-1050, "0x1.2p-1050"sv);
        }
        else if constexpr (kind == float_kind::f80) {
            return std::pair(0x1.2p-16400l, "0x1.2p-16400"sv);
        }
        else if constexpr (kind == float_kind::f128) {
            return std::pair(SCN_F_C(0x1.2p-16450), "0x1.2p-16450"sv);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return std::pair(0x1.2p-16F16, "0x1.2p-16"sv);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return std::pair(0x1.2p-130BF16, "0x1.2p-130"sv);
        }
#endif
        SCN_EXPECT(false);
    }

    constexpr static auto get_subnormal_max()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return std::pair(1e-38f, "1e-38"sv);
        }
        else if constexpr (kind == float_kind::f64) {
            return std::pair(2e-308, "2e-308"sv);
        }
        else if constexpr (kind == float_kind::f80) {
            return std::pair(3.2e-4932l, "3.2e-4932"sv);
        }
        else if constexpr (kind == float_kind::f128) {
            return std::pair(SCN_F_C(3.2e-4932), "3.2e-4932"sv);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return std::pair(6e-5F16, "6e-5"sv);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return std::pair(1e-38BF16, "1e-38"sv);
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static auto get_subnormal_max_hex()
        -> std::pair<float_type, std::string_view>
    {
        if constexpr (kind == float_kind::f32) {
            return std::pair(0x1.fp-127f, "0x1.fp-127"sv);
        }
        else if constexpr (kind == float_kind::f64) {
            return std::pair(0x1.fp-1023, "0x1.fp-1023"sv);
        }
        else if constexpr (kind == float_kind::f80) {
            return std::pair(0x1.fp-16383l, "0x1.fp-16383"sv);
        }
        else if constexpr (kind == float_kind::f128) {
            return std::pair(SCN_F_C(0x1.fp-16383), "0x1.fp-16383"sv);
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return std::pair(0x1.fp-15F16, "0x1.fp-15"sv);
        }
#endif
#if SCN_HAS_STD_BF16
        if constexpr (kind == float_kind::bf16) {
            return std::pair(0x1.fp-127BF16, "0x1.fp-127"sv);
        }
#endif
        SCN_EXPECT(false);
    }

    SCN_GCC_COMPAT_POP

    static auto get_normal_min() -> std::pair<float_type, std::string>
    {
        auto val = std::numeric_limits<float_type>::min();
        return std::pair(val, format_float(val, ".48", "e"));
    }
    static auto get_normal_min_hex() -> std::pair<float_type, std::string>
    {
        auto val = std::numeric_limits<float_type>::min();
        return std::pair(val, format_float(val, ".32", "a"));
    }

    static auto get_subnormal_min() -> std::pair<float_type, std::string>
    {
        auto val = std::numeric_limits<float_type>::denorm_min();
        return std::pair(val, format_float(val, ".48", "e"));
    }
    static auto get_subnormal_min_hex() -> std::pair<float_type, std::string>
    {
        auto val = std::numeric_limits<float_type>::denorm_min();
        return std::pair(val, format_float(val, ".32", "a"));
    }

    constexpr static std::string_view get_underflow()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "1.0e-90"sv;
        }
        else if constexpr (kind == float_kind::f64) {
            return "5.0e-400"sv;
        }
        else if constexpr (kind == float_kind::f80) {
            return "4.0e-5500"sv;
        }
        else if constexpr (kind == float_kind::f128) {
            return "6.0e-5500"sv;
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return "5.0e-16"sv;
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_underflow_hex()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "0x1p-192"sv;
        }
        else if constexpr (kind == float_kind::f64) {
            return "0x1p-1200"sv;
        }
        else if constexpr (kind == float_kind::f80) {
            return "0x1p-18000"sv;
        }
        else if constexpr (kind == float_kind::f128) {
            return "0x1p-18000"sv;
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return "0x1p-40"sv;
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_underflow_neg()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "-1.0e-90"sv;
        }
        else if constexpr (kind == float_kind::f64) {
            return "-5.0e-400"sv;
        }
        else if constexpr (kind == float_kind::f80) {
            return "-4.0e-5500"sv;
        }
        else if constexpr (kind == float_kind::f128) {
            return "-6.0e-5500"sv;
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return "-5.0e-16"sv;
        }
#endif
        SCN_EXPECT(false);
    }

    static auto get_maximum() -> std::pair<float_type, std::string>
    {
        auto val = std::numeric_limits<float_type>::max();
        return std::pair(val, format_float(val, ".48", "e"));
    }
    static auto get_maximum_hex() -> std::pair<float_type, std::string>
    {
        auto val = std::numeric_limits<float_type>::max();
        return std::pair(val, format_float(val, ".32", "a"));
    }

    constexpr static std::string_view get_overflow()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "4.0e38"sv;
        }
        else if constexpr (kind == float_kind::f64) {
            return "2.0e308"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "2.0e4932"sv;
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return "7.0e4"sv;
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_overflow_hex()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "0x1p+128"sv;
        }
        else if constexpr (kind == float_kind::f64) {
            return "0x1p+1024"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "0x1p+16384"sv;
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return "0x1p+16"sv;
        }
#endif
        SCN_EXPECT(false);
    }

    constexpr static std::string_view get_overflow_neg()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "-4.0e38"sv;
        }
        else if constexpr (kind == float_kind::f64) {
            return "-2.0e308"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "-2.0e4932"sv;
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return "-7.0e4"sv;
        }
#endif
        SCN_EXPECT(false);
    }
    constexpr static std::string_view get_overflow_neg_hex()
    {
        if constexpr (kind == float_kind::f32 || kind == float_kind::bf16) {
            return "-0x1p+128"sv;
        }
        else if constexpr (kind == float_kind::f64) {
            return "-0x1p+1024"sv;
        }
        else if constexpr (kind == float_kind::f80 ||
                           kind == float_kind::f128) {
            return "-0x1p+16384"sv;
        }
#if SCN_HAS_STD_F16
        if constexpr (kind == float_kind::f16) {
            return "-0x1p+16"sv;
        }
#endif
        SCN_EXPECT(false);
    }

    static auto get_thsep_number()
    {
        return static_cast<float_type>(123456.789L);
    }

    template <typename Source>
    void set_source(Source&& s)
    {
        if constexpr (is_char<char>) {
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
    EXPECT_TRUE(
        check_nan_eq(val, std::numeric_limits<decltype(val)>::quiet_NaN()));
}
TYPED_TEST(FloatValueReaderTest, NaNWithPayload)
{
    auto [a, _, val] = this->simple_success_test("nan(1234)");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnan(val));
    EXPECT_FALSE(std::signbit(val));
    EXPECT_FALSE(
        check_nan_eq(val, std::numeric_limits<decltype(val)>::quiet_NaN()));
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
TYPED_TEST(FloatValueReaderTest, NaNWithEmptyPayload)
{
    auto [a, _, val] = this->simple_success_test("nan()");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnan(val));
    EXPECT_FALSE(std::signbit(val));
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
TYPED_TEST(FloatValueReaderTest, NanWithNonNumericPayload)
{
    auto [a, _, val] = this->simple_success_test("nan(HelloWorld)");
    EXPECT_TRUE(a);
    EXPECT_TRUE(std::isnan(val));
    EXPECT_FALSE(std::signbit(val));
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
