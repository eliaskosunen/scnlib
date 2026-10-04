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

#include <array>
#include <cstring>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <utility>
#include <variant>

#include <scn/scan.h>
#include "test_common.h"

// Legacy (pre-IEEE 754-2008) NaN encoding, where the quiet bit is 0 in a quiet
// NaN: MIPS without NaN2008, and PA-RISC
#if ((defined(__mips__) || defined(__mips)) && !defined(__mips_nan2008)) || \
    defined(__hppa__) || defined(__hppa)
inline constexpr bool legacy_nan_encoding = true;
#else
inline constexpr bool legacy_nan_encoding = false;
#endif

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
struct float_kind_for_impl {
    static constexpr auto value = std::monostate{};
};

template <typename T>
struct float_kind_for_impl<T, std::enable_if_t<sizeof(T) == 4>> {
    static constexpr auto value = float_kind::f32;
};

template <typename T>
struct float_kind_for_impl<T, std::enable_if_t<sizeof(T) == 8>> {
    static constexpr auto value = float_kind::f64;
};

template <typename T>
struct float_kind_for_impl<
    T,
    std::enable_if_t<std::numeric_limits<T>::digits == 64>> {
    static constexpr auto value = float_kind::f80;
};

template <typename T>
struct float_kind_for_impl<
    T,
    std::enable_if_t<std::numeric_limits<T>::digits == 113>> {
    static constexpr auto value = float_kind::f128;
};

template <typename T>
struct float_kind_for_impl<
    T,
    std::enable_if_t<std::numeric_limits<T>::digits == 11>> {
    static constexpr auto value = float_kind::f16;
};

template <typename T>
struct float_kind_for_impl<
    T,
    std::enable_if_t<std::numeric_limits<T>::digits == 8>> {
    static constexpr auto value = float_kind::bf16;
};

template <typename T>
struct float_kind_for_impl<
    T,
    std::enable_if_t<std::numeric_limits<T>::digits == 106>> {
    static constexpr auto value = float_kind::f2x64;
};

template <typename T>
inline constexpr auto float_kind_for = float_kind_for_impl<T>::value;

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

namespace float_detail {

template <float_kind Kind>
struct float_format;

template <>
struct float_format<float_kind::f16> {
    using uint_type = std::uint16_t;
    static constexpr uint_type exp_mask = 0x7C00;
    static constexpr uint_type frac_mask = 0x03FF;
    static constexpr uint_type sign_mask = 0x8000;
    static constexpr uint_type quiet_mask = 0x0200;
};
template <>
struct float_format<float_kind::bf16> {
    using uint_type = std::uint16_t;
    static constexpr uint_type exp_mask = 0x7F80;
    static constexpr uint_type frac_mask = 0x007F;
    static constexpr uint_type sign_mask = 0x8000;
    static constexpr uint_type quiet_mask = 0x0040;
};
template <>
struct float_format<float_kind::f32> {
    using uint_type = std::uint32_t;
    static constexpr uint_type exp_mask = 0x7F800000;
    static constexpr uint_type frac_mask = 0x007FFFFF;
    static constexpr uint_type sign_mask = 0x80000000;
    static constexpr uint_type quiet_mask = 0x00400000;
};
template <>
struct float_format<float_kind::f64> {
    using uint_type = std::uint64_t;
    static constexpr uint_type exp_mask = 0x7FF0000000000000ULL;
    static constexpr uint_type frac_mask = 0x000FFFFFFFFFFFFFULL;
    static constexpr uint_type sign_mask = 0x8000000000000000ULL;
    static constexpr uint_type quiet_mask = 0x0008000000000000ULL;
};

template <>
struct float_format<float_kind::f80> {
    static constexpr std::uint16_t exp_mask = 0x7FFF;
    static constexpr std::uint16_t sign_mask = 0x8000;
    // Excludes the explicit integer bit
    static constexpr std::uint64_t frac_mask = 0x7FFFFFFFFFFFFFFFULL;
    static constexpr std::uint64_t quiet_mask = 0x4000000000000000ULL;
};

template <>
struct float_format<float_kind::f128> {
    static constexpr std::uint64_t exp_mask = 0x7FFF000000000000ULL;
    static constexpr std::uint64_t frac_hi_mask = 0x0000FFFFFFFFFFFFULL;
    static constexpr std::uint64_t sign_mask = 0x8000000000000000ULL;
    static constexpr std::uint64_t quiet_hi_mask = 0x0000800000000000ULL;
};

inline std::uint64_t maybe_swap_float_words(std::uint64_t bits)
{
#if SCN_IS_BIG_ENDIAN != SCN_IS_FLOAT_BIG_ENDIAN
    return (bits >> 32) | (bits << 32);
#else
    return bits;
#endif
}

struct f80_fields {
    std::uint64_t significand{};
    std::uint16_t sign_exp{};
};

template <typename T>
f80_fields read_f80(T value)
{
    alignas(T) std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(T));

    f80_fields f{};
#if SCN_IS_BIG_ENDIAN
    // m68k: sign and exponent first, then padding, then the significand
    std::memcpy(&f.sign_exp, bytes.data(), 2);
    std::memcpy(&f.significand, bytes.data() + sizeof(T) - 8, 8);
#else
    std::memcpy(&f.significand, bytes.data(), 8);
    std::memcpy(&f.sign_exp, bytes.data() + 8, 2);
#if SCN_IS_FLOAT_BIG_ENDIAN
    f.significand = (f.significand >> 32) | (f.significand << 32);
#endif
#endif
    return f;
}

struct f128_halves {
    std::uint64_t low{};
    std::uint64_t high{};
};

template <typename T>
f128_halves read_f128(T value)
{
    std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &value, 16);

    f128_halves h{};
#if SCN_IS_BIG_ENDIAN
    std::memcpy(&h.high, bytes.data(), 8);
    std::memcpy(&h.low, bytes.data() + 8, 8);
#else
    std::memcpy(&h.low, bytes.data(), 8);
    std::memcpy(&h.high, bytes.data() + 8, 8);
#endif

    h.low = maybe_swap_float_words(h.low);
    h.high = maybe_swap_float_words(h.high);

    return h;
}

}  // namespace float_detail

// Float kind detection that works even with -ffast-math
struct float_classification {
    bool sign{};
    bool exponent_all_ones{};
    bool exponent_zero{};
    bool fraction_msb{};  // quiet/signaling bit, if a NaN
    // The fraction field, without the explicit integer bit of an x87 long
    // double. `fraction_high` is only used for binary128.
    std::uint64_t fraction_low{};
    std::uint64_t fraction_high{};

    template <typename T>
    static float_classification classify(T value)
    {
        constexpr auto kind = float_kind_for<T>;

        if constexpr (kind == float_kind::f16 || kind == float_kind::f32 ||
                      kind == float_kind::f64 || kind == float_kind::bf16) {
            using format_type = float_detail::float_format<kind>;
            using uint_type = typename format_type::uint_type;

            uint_type bits{};
            std::memcpy(&bits, &value, sizeof(bits));
            if constexpr (sizeof(uint_type) == 8) {
                bits = static_cast<uint_type>(
                    float_detail::maybe_swap_float_words(bits));
            }

            return {(bits & format_type::sign_mask) != 0,
                    (bits & format_type::exp_mask) == format_type::exp_mask,
                    (bits & format_type::exp_mask) == 0,
                    (bits & format_type::quiet_mask) != 0,
                    static_cast<std::uint64_t>(bits & format_type::frac_mask),
                    0};
        }
        else if constexpr (kind == float_kind::f80) {
            using format_type = float_detail::float_format<kind>;
            const auto f = float_detail::read_f80(value);
            const auto exp = f.sign_exp & format_type::exp_mask;

            return {(f.sign_exp & format_type::sign_mask) != 0,
                    exp == format_type::exp_mask,
                    exp == 0,
                    (f.significand & format_type::quiet_mask) != 0,
                    f.significand & format_type::frac_mask,
                    0};
        }
        else if constexpr (kind == float_kind::f128) {
            using format_type = float_detail::float_format<kind>;
            const auto h = float_detail::read_f128(value);
            const auto exp = h.high & format_type::exp_mask;

            return {(h.high & format_type::sign_mask) != 0,
                    exp == format_type::exp_mask,
                    exp == 0,
                    (h.high & format_type::quiet_hi_mask) != 0,
                    h.low,
                    h.high & format_type::frac_hi_mask};
        }
        else if constexpr (kind == float_kind::f2x64) {
            // The class of a double-double is the class of its high double
            double high{};
            std::memcpy(&high, &value, sizeof(double));
            return classify(high);
        }
        else {
            static_assert(scn::detail::dependent_false<T>::value,
                          "Unsupported float type");
        }
    }

    [[nodiscard]]
    bool fraction_zero() const
    {
        return fraction_low == 0 && fraction_high == 0;
    }

    [[nodiscard]]
    bool is_nan() const
    {
        return exponent_all_ones && !fraction_zero();
    }

    [[nodiscard]]
    bool is_quiet_nan() const
    {
        return is_nan() && fraction_msb == !legacy_nan_encoding;
    }

    [[nodiscard]]
    bool is_inf() const
    {
        return exponent_all_ones && fraction_zero();
    }

    [[nodiscard]]
    bool is_zero() const
    {
        return exponent_zero && fraction_zero();
    }

    [[nodiscard]]
    bool is_subnormal() const
    {
        return exponent_zero && !fraction_zero();
    }

    [[nodiscard]]
    bool is_normal() const
    {
        return !exponent_zero && !exponent_all_ones;
    }

    [[nodiscard]]
    bool signbit() const
    {
        return sign;
    }
};

template <typename T>
float_classification classify_float(T value)
{
    return float_classification::classify(value);
}

// The payload of a NaN: its fraction without the quiet bit (and without the
// explicit integer bit of an x87 long double), as {low 64 bits, high bits}.
// For a double-double, the payload of the high double.
template <typename T>
std::pair<std::uint64_t, std::uint64_t> get_nan_payload(T value)
{
    constexpr auto kind = float_kind_for<T>;
    if constexpr (kind == float_kind::f2x64) {
        double high{};
        std::memcpy(&high, &value, sizeof(double));
        return get_nan_payload(high);
    }
    else {
        using format_type = float_detail::float_format<kind>;
        const auto c = classify_float(value);
        if constexpr (kind == float_kind::f128) {
            return {c.fraction_low,
                    c.fraction_high & ~format_type::quiet_hi_mask};
        }
        else {
            return {c.fraction_low &
                        ~static_cast<std::uint64_t>(format_type::quiet_mask),
                    0};
        }
    }
}

// Checks that `value` is a quiet NaN with the given sign, and `payload`, if
// given. Without one, checks that `value` has the payload of `quiet_NaN()`.
template <typename T>
SCN_NODISCARD testing::AssertionResult
check_nan(T value, bool negative, std::optional<std::uint64_t> payload)
{
    const auto c = classify_float(value);
    if (!c.is_quiet_nan()) {
        return testing::AssertionFailure()
               << "not a quiet NaN (bytes: " << get_bytes_str(value) << ")";
    }
    if (c.signbit() != negative) {
        return testing::AssertionFailure()
               << "NaN sign: expected " << (negative ? "negative" : "positive")
               << " (bytes: " << get_bytes_str(value) << ")";
    }

    const auto expected_payload =
        payload ? std::pair<std::uint64_t, std::uint64_t>{*payload, 0}
                : get_nan_payload(std::numeric_limits<T>::quiet_NaN());
    if (const auto actual_payload = get_nan_payload(value);
        actual_payload != expected_payload) {
        // testing::AssertionResult doesn't keep std::hex between values
        std::ostringstream os;
        os << std::hex << "NaN payload (high, low): expected (0x"
           << expected_payload.second << ", 0x" << expected_payload.first
           << "), got (0x" << actual_payload.second << ", 0x"
           << actual_payload.first << ")";
        return testing::AssertionFailure()
               << os.str() << " (bytes: " << get_bytes_str(value)
               << ", quiet_NaN() bytes: "
               << get_bytes_str(std::numeric_limits<T>::quiet_NaN()) << ")";
    }

    if constexpr (float_kind_for<T> == float_kind::f2x64) {
        // The low double of a NaN is zero (eknan zeroes it when setting a
        // payload), with the sign of the high double
        double parts[2]{};
        std::memcpy(parts, &value, sizeof(parts));
        const auto low = classify_float(parts[1]);
        if (!low.is_zero() || low.signbit() != negative) {
            return testing::AssertionFailure()
                   << "double-double NaN: expected the low double to be "
                   << (negative ? "-0.0" : "+0.0")
                   << " (bytes: " << get_bytes_str(value) << ")";
        }
    }

    return testing::AssertionSuccess();
}
