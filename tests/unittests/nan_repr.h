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

#include <iomanip>
#include <limits>
#include <sstream>

#include <scn/scan.h>
#include "test_common.h"

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

namespace nan_detail {

template <float_kind Kind>
struct nan_format;

template <>
struct nan_format<float_kind::f16> {
    using uint_type = std::uint16_t;
    static constexpr uint_type exp_mask = 0x7C00;
    static constexpr uint_type frac_mask = 0x03FF;
    static constexpr uint_type sign_mask = 0x8000;
    static constexpr uint_type payload_mask = 0x01FF;
};
template <>
struct nan_format<float_kind::bf16> {
    using uint_type = std::uint16_t;
    static constexpr uint_type exp_mask = 0x7F80;
    static constexpr uint_type frac_mask = 0x007F;
    static constexpr uint_type sign_mask = 0x8000;
    static constexpr uint_type payload_mask = 0x003F;
};
template <>
struct nan_format<float_kind::f32> {
    using uint_type = std::uint32_t;
    static constexpr uint_type exp_mask = 0x7F800000;
    static constexpr uint_type frac_mask = 0x007FFFFF;
    static constexpr uint_type sign_mask = 0x80000000;
    static constexpr uint_type payload_mask = 0x003FFFFF;
};
template <>
struct nan_format<float_kind::f64> {
    using uint_type = std::uint64_t;
    static constexpr uint_type exp_mask = 0x7FF0000000000000ULL;
    static constexpr uint_type frac_mask = 0x000FFFFFFFFFFFFFULL;
    static constexpr uint_type sign_mask = 0x8000000000000000ULL;
    static constexpr uint_type payload_mask = 0x0007FFFFFFFFFFFFULL;
};

template <>
struct nan_format<float_kind::f80> {
    static constexpr std::uint16_t exp_mask = 0x7FFF;
    static constexpr std::uint16_t sign_mask = 0x8000;
    static constexpr std::uint64_t frac_mask = 0x7FFFFFFFFFFFFFFFULL;
    static constexpr std::uint64_t one_bit = 0x8000000000000000ULL;
    static constexpr std::uint64_t payload_mask = 0x3FFFFFFFFFFFFFFFULL;
};

template <>
struct nan_format<float_kind::f128> {
    static constexpr std::uint64_t exp_mask = 0x7FFF000000000000ULL;
    static constexpr std::uint64_t frac_hi_mask = 0x0000FFFFFFFFFFFFULL;
    static constexpr std::uint64_t sign_mask = 0x8000000000000000ULL;
    static constexpr std::uint64_t payload_hi_mask = 0x00007FFFFFFFFFFFULL;
};

}  // namespace nan_detail

struct payload_type {
    std::uint64_t lo{0};
    std::uint64_t hi{0};  // only used for f128 (111-bit payload)

    bool operator==(const payload_type& other) const
    {
        return lo == other.lo && hi == other.hi;
    }
    bool operator!=(const payload_type& other) const
    {
        return !(*this == other);
    }
};

template <float_kind Kind>
struct nan_repr {
    using format_type = nan_detail::nan_format<Kind>;

    template <typename T,
              typename = typename std::enable_if<float_kind_for<T> == Kind>::type>
    explicit nan_repr(T value);

    template <typename T,
              typename = typename std::enable_if<float_kind_for<T> == Kind>::type>
    T to_float() const;

    payload_type payload{};
    std::uint64_t double_double_low_bits{0};  // raw low-double bits for f2x64
};

namespace nan_detail {

inline std::uint64_t maybe_swap_float_words(std::uint64_t bits)
{
#if SCN_IS_BIG_ENDIAN != SCN_IS_FLOAT_BIG_ENDIAN
    return (bits >> 32) | (bits << 32);
#else
    return bits;
#endif
}

template <typename T>
std::uint64_t extract_nan_single(T value)
{
    using format_type = nan_format<float_kind_for<T>>;
    using uint_type = typename format_type::uint_type;

    uint_type bits{};
    std::memcpy(&bits, &value, sizeof(bits));

    if constexpr (sizeof(uint_type) == 8) {
        bits = static_cast<uint_type>(maybe_swap_float_words(bits));
    }
    return bits & format_type::payload_mask;
}

template <typename T>
T construct_nan_single(T base_nan, std::uint64_t payload_low)
{
    using format_type = nan_format<float_kind_for<T>>;
    using uint_type = typename format_type::uint_type;

    uint_type bits{};
    std::memcpy(&bits, &base_nan, sizeof(bits));
    if constexpr (sizeof(uint_type) == 8) {
        bits = static_cast<uint_type>(maybe_swap_float_words(bits));
    }
    bits = static_cast<uint_type>(
        (bits & ~format_type::payload_mask) |
        static_cast<uint_type>(payload_low & format_type::payload_mask));

    if constexpr (sizeof(uint_type) == 8) {
        bits = static_cast<uint_type>(maybe_swap_float_words(bits));
    }

    T result{};
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

}  // namespace nan_detail

#define SCN_NAN_REPR_SINGLE(Kind)                              \
    template <>                                                \
    template <typename T, typename>                       \
    nan_repr<Kind>::nan_repr(T value)                          \
    {                                                          \
        payload.lo = nan_detail::extract_nan_single<T>(value); \
    }                                                          \
    template <>                                                \
    template <typename T, typename>                       \
    T nan_repr<Kind>::to_float() const                         \
    {                                                          \
        return nan_detail::construct_nan_single<T>(            \
            std::numeric_limits<T>::quiet_NaN(), payload.lo);  \
    }

SCN_NAN_REPR_SINGLE(float_kind::f16)
SCN_NAN_REPR_SINGLE(float_kind::f32)
SCN_NAN_REPR_SINGLE(float_kind::f64)
SCN_NAN_REPR_SINGLE(float_kind::bf16)

#undef SCN_NAN_REPR_SINGLE

// f80

namespace nan_detail {

struct f80_fields {
    std::uint64_t significand{};
    std::uint16_t sign_exp{};
};

template <typename T,
          std::enable_if_t<float_kind_for<T> == float_kind::f80>* = nullptr>
f80_fields read_f80(T value)
{
    alignas(T) std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(T));

    f80_fields f{};
#if SCN_IS_BIG_ENDIAN
    std::memcpy(&f.sign_exp, bytes.data() + sizeof(T) - 10, 2);
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

template <typename T,
          std::enable_if_t<float_kind_for<T> == float_kind::f80>* = nullptr>
T write_f80(T base, f80_fields f)
{
    alignas(T) std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &base, sizeof(T));

#if SCN_IS_BIG_ENDIAN
    std::memcpy(bytes.data() + sizeof(T) - 10, &f.sign_exp, 2);
    std::memcpy(bytes.data() + sizeof(T) - 8, &f.significand, 8);
#else
#if SCN_IS_FLOAT_BIG_ENDIAN
    f.significand = (f.significand >> 32) | (f.significand << 32);
#endif
    std::memcpy(bytes.data(), &f.significand, 8);
    std::memcpy(bytes.data() + 8, &f.sign_exp, 2);
#endif

    T result{};
    std::memcpy(&result, bytes.data(), sizeof(T));
    return result;
}

}  // namespace nan_detail

template <>
template <typename T, typename>
nan_repr<float_kind::f80>::nan_repr(T value)
{
    auto f = nan_detail::read_f80(value);
    payload.lo = f.significand & format_type::payload_mask;
}

template <>
template <typename T, typename>
T nan_repr<float_kind::f80>::to_float() const
{
    auto base = std::numeric_limits<T>::quiet_NaN();
    auto f = nan_detail::read_f80(base);

    f.significand = (f.significand & ~format_type::payload_mask) |
                    (payload.lo & format_type::payload_mask);
    f.significand |= format_type::one_bit;

    return nan_detail::write_f80(base, f);
}

// f128

namespace nan_detail {

struct f128_halves {
    std::uint64_t low{};
    std::uint64_t high{};
};

template <typename T,
          std::enable_if_t<float_kind_for<T> == float_kind::f128>* = nullptr>
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

template <typename T,
          std::enable_if_t<float_kind_for<T> == float_kind::f128>* = nullptr>
T write_f128(f128_halves h)
{
    h.low = maybe_swap_float_words(h.low);
    h.high = maybe_swap_float_words(h.high);

    std::array<unsigned char, sizeof(T)> bytes{};
#if SCN_IS_BIG_ENDIAN
    std::memcpy(bytes.data(), &h.high, 8);
    std::memcpy(bytes.data() + 8, &h.low, 8);
#else
    std::memcpy(bytes.data(), &h.low, 8);
    std::memcpy(bytes.data() + 8, &h.high, 8);
#endif

    T result{};
    std::memcpy(&result, bytes.data(), 16);
    return result;
}

}  // namespace nan_detail

template <>
template <typename T, typename>
nan_repr<float_kind::f128>::nan_repr(T value)
{
    auto h = nan_detail::read_f128(value);

    payload.lo = h.low;
    payload.hi = h.high & format_type::payload_hi_mask;
}

template <>
template <typename T, typename>
T nan_repr<float_kind::f128>::to_float() const
{
    auto base = std::numeric_limits<T>::quiet_NaN();
    auto h = nan_detail::read_f128(base);

    h.low = payload.lo;
    h.high = (h.high & ~format_type::payload_hi_mask) |
             (payload.hi & format_type::payload_hi_mask);

    return nan_detail::write_f128<T>(h);
}

// double-double

template <>
template <typename T, typename>
nan_repr<float_kind::f2x64>::nan_repr(T value)
{
    alignas(T) std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(T));

    double high{}, low{};
    std::memcpy(&high, bytes.data(), 8);
    std::memcpy(&low, bytes.data() + 8, 8);

    nan_repr<float_kind::f64> high_repr(high);
    payload = high_repr.payload;

    std::memcpy(&double_double_low_bits, &low, 8);
}

template <>
template <typename T, typename>
T nan_repr<float_kind::f2x64>::to_float() const
{
    auto base = std::numeric_limits<T>::quiet_NaN();
    alignas(T) std::array<unsigned char, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &base, sizeof(T));

    double high{};
    std::memcpy(&high, bytes.data(), 8);

    nan_repr<float_kind::f64> high_repr(high);
    high_repr.payload = payload;
    high = high_repr.to_float<double>();

    std::memcpy(bytes.data(), &high, 8);

    long double result{};
    std::memcpy(&result, bytes.data(), 16);
    return result;
}

template <typename T>
T make_nan_with_payload(const char* payload_str)
{
    std::uint64_t parsed_payload = 0;
    if (payload_str && *payload_str) {
        auto result =
            scn::scan<std::uint64_t>(std::string_view(payload_str), "{:i}");
        if (result) {
            parsed_payload = result->value();
        }
    }

    nan_repr<float_kind_for<T>> repr(std::numeric_limits<T>::quiet_NaN());
    repr.payload.lo = parsed_payload;
    return repr.template to_float<T>();
}

template <typename T>
SCN_NODISCARD testing::AssertionResult check_nan_eq(T lhs, T rhs)
{
    if (!std::isnan(lhs)) {
        return testing::AssertionFailure()
               << "lhs not nan (bytes: " << get_bytes_str(lhs) << ")";
    }
    if (!std::isnan(rhs)) {
        return testing::AssertionFailure()
               << "rhs not nan (bytes: " << get_bytes_str(rhs) << ")";
    }

    if (std::signbit(lhs) != std::signbit(rhs)) {
        return testing::AssertionFailure()
               << "NaN signs differ: lhs "
               << (std::signbit(lhs) ? "negative" : "positive") << ", rhs "
               << (std::signbit(rhs) ? "negative" : "positive")
               << " (lhs bytes: " << get_bytes_str(lhs)
               << ", rhs bytes: " << get_bytes_str(rhs) << ")";
    }

    nan_repr<float_kind_for<T>> lhs_repr(lhs);
    nan_repr<float_kind_for<T>> rhs_repr(rhs);

    if (lhs_repr.payload != rhs_repr.payload) {
        return testing::AssertionFailure()
               << "NaN payloads differ: lhs.lo=" << std::hex
               << lhs_repr.payload.lo << " lhs.hi=" << lhs_repr.payload.hi
               << ", rhs.lo=" << rhs_repr.payload.lo
               << " rhs.hi=" << rhs_repr.payload.hi
               << " (lhs bytes: " << get_bytes_str(lhs)
               << ", rhs bytes: " << get_bytes_str(rhs) << ")";
    }

    if constexpr (float_kind_for<T> == float_kind::f2x64) {
        if (lhs_repr.double_double_low_bits !=
            rhs_repr.double_double_low_bits) {
            return testing::AssertionFailure()
                   << "Double-double low bits differ: lhs=" << std::hex
                   << lhs_repr.double_double_low_bits
                   << ", rhs=" << rhs_repr.double_double_low_bits
                   << " (lhs bytes: " << get_bytes_str(lhs)
                   << ", rhs bytes: " << get_bytes_str(rhs) << ")";
        }
    }

    return testing::AssertionSuccess()
           << "NaNs equal, payload.lo=" << std::hex << lhs_repr.payload.lo
           << ", payload.hi=" << lhs_repr.payload.hi;
}
