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

#include <scn/chrono.h>
#include <scn/impl.h>

#if !SCN_DISABLE_LOCALE
#include <locale>
#include <sstream>
#endif

#ifndef SCN_DISABLE_FAST_FLOAT
#define SCN_DISABLE_FAST_FLOAT 0
#endif

#if !SCN_DISABLE_FAST_FLOAT

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wold-style-cast")
SCN_GCC_IGNORE("-Wnoexcept")
SCN_GCC_IGNORE("-Wundef")
SCN_GCC_IGNORE("-Wsign-conversion")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wold-style-cast")
SCN_CLANG_IGNORE("-Wdeprecated")
SCN_CLANG_IGNORE("-Wcomma")
SCN_CLANG_IGNORE("-Wundef")
SCN_CLANG_IGNORE("-Wdocumentation-unknown-command")

#if SCN_CLANG >= SCN_COMPILER(8, 0, 0)
SCN_CLANG_IGNORE("-Wextra-semi-stmt")
#endif

#if SCN_CLANG >= SCN_COMPILER(16, 0, 0)
SCN_CLANG_IGNORE("-Wunsafe-buffer-usage")
#endif

#if SCN_CLANG >= SCN_COMPILER(13, 0, 0)
SCN_CLANG_IGNORE("-Wreserved-identifier")
#endif

#include <fast_float/fast_float.h>

SCN_CLANG_POP
SCN_GCC_POP

#endif  // !SCN_DISABLE_FAST_FLOAT

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

#define SCN_XLOCALE_POSIX 0
#define SCN_XLOCALE_MSVC  1
#define SCN_XLOCALE_OTHER 2

#if SCN_DISABLE_LOCALE
#define SCN_XLOCALE SCN_XLOCALE_OTHER
#elif (!defined(__ANDROID_API__) || __ANDROID_API__ >= 28) && \
    !defined(__EMSCRIPTEN__) && defined(__has_include) && __has_include(<xlocale.h>)
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX

#elif defined(_MSC_VER)
#define SCN_XLOCALE SCN_XLOCALE_MSVC

#elif defined(__GLIBC__) && !defined(__ANDROID_API__) && \
    !defined(__EMSCRIPTEN__)
// glibc

#include <features.h>

#if !((__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ > 25)))
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX
#endif  // __GLIBC__ <= 2.25

#elif defined(__FreeBSD_version) && __FreeBSD_version >= 1000010

// FreeBSD
#include <xlocale.h>
#define SCN_XLOCALE SCN_XLOCALE_POSIX

#endif  // SCN_DISABLE_LOCALE, others

#ifndef SCN_XLOCALE
#define SCN_XLOCALE SCN_XLOCALE_OTHER
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////
// Whitespace finders
/////////////////////////////////////////////////////////////////

namespace impl {
namespace {
template <typename R>
bool has_nonascii_char_64(R source)
{
    static_assert(sizeof(*source.data()) == 1);
    SCN_EXPECT(source.size() <= 8);
    uint64_t word{};
    std::memcpy(&word, source.data(), source.size());

    return has_byte_greater(word, 127) != 0;
}

template <typename CuCb, typename CpCb>
std::string_view::iterator find_classic_impl(std::string_view source,
                                             CuCb cu_cb,
                                             CpCb cp_cb)
{
    auto it = source.begin();

    while (it != source.end()) {
        auto sv =
            detail::make_string_view_from_iterators<char>(it, source.end())
                .substr(0, 8);

        if (!has_nonascii_char_64(sv)) {
            auto tmp_it = std::find_if(sv.begin(), sv.end(), cu_cb);
            it = detail::make_string_view_iterator(source, tmp_it);
            if (tmp_it != sv.end()) {
                break;
            }
            continue;
        }

        for (std::size_t i = 0; i < sv.size(); ++i) {
            auto tmp =
                detail::make_string_view_from_iterators<char>(it, source.end());
            auto res = get_next_code_point(tmp);
            if (cp_cb(res.value)) {
                return it;
            }
            i += static_cast<std::size_t>(
                ranges::distance(tmp.data(), detail::to_address(res.iterator)));
            it = detail::make_string_view_iterator(source, res.iterator);
            SCN_ENSURE(it <= source.end());
        }
    }

    return detail::make_string_view_iterator(source, it);
}

bool is_decimal_digit(char ch) noexcept
{
    static constexpr std::array<bool, 256> lookup = {
        {false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, true,  true,
         true,  true,  true,  true,  true,  true,  true,  true,  false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false}};

    return lookup[static_cast<size_t>(static_cast<unsigned char>(ch))];
}

std::string_view::iterator find_nondecimal_digit_simple_impl(
    std::string_view source)
{
    return std::find_if(source.begin(), source.end(),
                        [](char ch) noexcept { return !is_decimal_digit(ch); });
}
}  // namespace

std::string_view::iterator find_classic_space_narrow_fast(
    std::string_view source)
{
    return find_classic_impl(
        source, [](char ch) { return is_ascii_space(ch); },
        [](char32_t cp) { return detail::is_cp_space(cp); });
}

std::string_view::iterator find_classic_nonspace_narrow_fast(
    std::string_view source)
{
    return find_classic_impl(
        source, [](char ch) { return !is_ascii_space(ch); },
        [](char32_t cp) { return !detail::is_cp_space(cp); });
}

std::string_view::iterator find_nondecimal_digit_narrow_fast(
    std::string_view source)
{
    return find_nondecimal_digit_simple_impl(source);
}
}  // namespace impl

/////////////////////////////////////////////////////////////////
// Scanner implementations
/////////////////////////////////////////////////////////////////

namespace detail {
template <typename T, typename Context>
scan_expected<typename Context::iterator>
scanner_scan_for_builtin_type(T& val, Context& ctx, const format_specs& specs)
{
    if constexpr (!detail::is_type_disabled<T>) {
        return impl::arg_reader<Context>{ctx.range(), specs, {}}(val);
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
}

template <typename Range>
scan_expected<ranges::iterator_t<Range>> internal_skip_classic_whitespace(
    Range r,
    bool allow_exhaustion)
{
    return impl::skip_classic_whitespace(r, allow_exhaustion)
        .transform_error(impl::make_eof_scan_error);
}

#define SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(T, Context)                         \
    template scan_expected<Context::iterator> scanner_scan_for_builtin_type( \
        T&, Context&, const format_specs&);

#define SCN_DEFINE_SCANNER_SCAN_FOR_CTX(Context)                    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(Context::char_type, Context)   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(signed char, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(short, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(int, Context)                  \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long, Context)                 \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long long, Context)            \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned char, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned short, Context)       \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned int, Context)         \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(unsigned long long, Context)   \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(float, Context)                \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(double, Context)               \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(long double, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string, Context)          \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring, Context)         \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::string_view, Context)     \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(std::wstring_view, Context)    \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(regex_matches, Context)        \
    SCN_DEFINE_SCANNER_SCAN_FOR_TYPE(wregex_matches, Context)       \
    template scan_expected<ranges::iterator_t<Context::range_type>> \
    internal_skip_classic_whitespace(Context::range_type, bool);

SCN_DEFINE_SCANNER_SCAN_FOR_CTX(scan_context)
SCN_DEFINE_SCANNER_SCAN_FOR_CTX(wscan_context)

/////////////////////////////////////////////////////////////////
// scan_buffer implementations
/////////////////////////////////////////////////////////////////

template basic_scan_file_buffer<stdio_file_interface>::basic_scan_file_buffer(
    stdio_file_interface);
template basic_scan_file_buffer<
    stdio_file_interface>::~basic_scan_file_buffer();
template bool basic_scan_file_buffer<stdio_file_interface>::fill();
template bool basic_scan_file_buffer<stdio_file_interface>::sync(
    std::ptrdiff_t);

}  // namespace detail

/////////////////////////////////////////////////////////////////
// locale implementations
/////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_LOCALE

namespace detail {
template <typename Locale>
locale_ref::locale_ref(const Locale& loc) : m_locale(&loc)
{
    static_assert(std::is_same_v<Locale, std::locale>);
}

template <typename Locale>
Locale locale_ref::get() const
{
    static_assert(std::is_same_v<Locale, std::locale>);
    return m_locale ? *static_cast<const std::locale*>(m_locale)
                    : std::locale{};
}

template locale_ref::locale_ref(const std::locale&);
template auto locale_ref::get() const -> std::locale;
}  // namespace detail

#endif

namespace detail {
scan_error handle_error(scan_error e)
{
    SCN_UNLIKELY_ATTR
    return e;
}
}  // namespace detail

/////////////////////////////////////////////////////////////////
// Floating-point reader implementation
/////////////////////////////////////////////////////////////////

namespace impl {

namespace {

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wconversion")

template <typename F>
struct float_traits;

template <>
struct float_traits<float> {
    using type = float;

    struct value_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 8;
        unsigned mantissa : 23;
#else
        unsigned mantissa : 23;
        unsigned exponent : 8;
        unsigned negative : 1;
#endif
    };

    struct nan_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 8;
        unsigned quiet_nan : 1;
        unsigned mantissa : 22;
#else
        unsigned mantissa : 22;
        unsigned quiet_nan : 1;
        unsigned exponent : 8;
        unsigned negative : 1;
#endif
    };

    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r,
                                                   std::uint64_t payload)
    {
        SCN_EXPECT(r.quiet_nan == 1);
        SCN_EXPECT(r.exponent == 0xff);
        r.mantissa = static_cast<unsigned>(payload);
    }
};

template <>
struct float_traits<double> {
    using type = double;

    struct value_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 11;
        unsigned mantissa0 : 20;
        unsigned mantissa1 : 32;
#elif SCN_IS_FLOAT_BIG_ENDIAN
        unsigned mantissa0 : 20;
        unsigned exponent : 11;
        unsigned negative : 1;
        unsigned mantissa1 : 32;
#else
        unsigned mantissa1 : 32;
        unsigned mantissa0 : 20;
        unsigned exponent : 11;
        unsigned negative : 1;
#endif
    };

    struct nan_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 11;
        unsigned quiet_nan : 1;
        unsigned mantissa0 : 19;
        unsigned mantissa1 : 32;
#elif SCN_IS_FLOAT_BIG_ENDIAN
        unsigned mantissa0 : 19;
        unsigned quiet_nan : 1;
        unsigned exponent : 11;
        unsigned negative : 1;
        unsigned mantissa1 : 32;
#else
        unsigned mantissa1 : 32;
        unsigned mantissa0 : 19;
        unsigned quiet_nan : 1;
        unsigned exponent : 11;
        unsigned negative : 1;
#endif
    };

    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r,
                                                   std::uint64_t payload)
    {
        SCN_EXPECT(r.quiet_nan == 1);
        SCN_EXPECT(r.exponent == (1u << 11u) - 1u);
        r.mantissa0 = static_cast<unsigned>(payload >> 32);
        r.mantissa1 = static_cast<unsigned>(payload);
    }
};

struct float_traits_x87 {
    using type = long double;

    struct value_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned padding : 16;
        unsigned negative : 1;
        unsigned exponent : 15;
        unsigned one : 1;
        unsigned mantissa0 : 31;
        unsigned mantissa1 : 32;
#elif SCN_IS_FLOAT_BIG_ENDIAN
        unsigned exponent : 15;
        unsigned negative : 1;
        unsigned padding : 16;
        unsigned mantissa0 : 31;
        unsigned one : 1;
        unsigned mantissa1 : 32;
#else
        unsigned mantissa1 : 32;
        unsigned mantissa0 : 31;
        unsigned one : 1;
        unsigned exponent : 15;
        unsigned negative : 1;
        unsigned padding : 16;
#endif
    };

    struct nan_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned padding : 16;
        unsigned negative : 1;
        unsigned exponent : 15;
        unsigned one : 1;
        unsigned quiet_nan : 1;
        unsigned mantissa0 : 30;
        unsigned mantissa1 : 32;
#elif SCN_IS_FLOAT_BIG_ENDIAN
        unsigned exponent : 15;
        unsigned negative : 1;
        unsigned padding : 16;
        unsigned mantissa0 : 30;
        unsigned quiet_nan : 1;
        unsigned one : 1;
        unsigned mantissa1 : 32;
#else
        unsigned mantissa1 : 32;
        unsigned mantissa0 : 30;
        unsigned quiet_nan : 1;
        unsigned one : 1;
        unsigned exponent : 15;
        unsigned negative : 1;
        unsigned padding : 16;
#endif
    };

    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r,
                                                   std::uint64_t payload)
    {
        SCN_EXPECT(r.quiet_nan == 1);
        SCN_EXPECT(r.exponent == (1u << 15u) - 1u);
        r.mantissa0 = static_cast<unsigned>(payload >> 32);
        r.mantissa1 = static_cast<unsigned>(payload);
    }
};

struct float_traits_binary128 {
    using type = long double;

    struct value_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 15;
        unsigned mantissa0 : 16;
        unsigned mantissa1 : 32;
        unsigned mantissa2 : 32;
        unsigned mantissa3 : 32;
#elif SCN_IS_FLOAT_BIG_ENDIAN
        unsigned mantissa0 : 16;
        unsigned exponent : 15;
        unsigned negative : 1;
        unsigned mantissa1 : 32;
        unsigned mantissa2 : 32;
        unsigned mantissa3 : 32;
#else
        unsigned mantissa3 : 32;
        unsigned mantissa2 : 32;
        unsigned mantissa1 : 32;
        unsigned mantissa0 : 16;
        unsigned exponent : 15;
        unsigned negative : 1;
#endif
    };

    struct nan_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 15;
        unsigned quiet_nan : 1;
        unsigned mantissa0 : 15;
        unsigned mantissa1 : 32;
        unsigned mantissa2 : 32;
        unsigned mantissa3 : 32;
#elif SCN_IS_FLOAT_BIG_ENDIAN
        unsigned mantissa0 : 15;
        unsigned quiet_nan : 1;
        unsigned exponent : 15;
        unsigned negative : 1;
        unsigned mantissa1 : 32;
        unsigned mantissa2 : 32;
        unsigned mantissa3 : 32;
#else
        unsigned mantissa3 : 32;
        unsigned mantissa2 : 32;
        unsigned mantissa1 : 32;
        unsigned mantissa0 : 15;
        unsigned quiet_nan : 1;
        unsigned exponent : 15;
        unsigned negative : 1;
#endif
    };

    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r,
                                                   std::uint64_t payload)
    {
        SCN_EXPECT(r.quiet_nan == 1);
        SCN_EXPECT(r.exponent == (1u << 15u) - 1u);
        r.mantissa0 = 0;
        r.mantissa1 = 0;
        r.mantissa2 = static_cast<unsigned>(payload >> 32);
        r.mantissa3 = static_cast<unsigned>(payload);
    }

#if SCN_HAS_INT128
    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r, uint128 payload)
    {
        SCN_EXPECT(r.quiet_nan == 1);
        SCN_EXPECT(r.exponent == (1u << 15u) - 1u);
        r.mantissa0 = static_cast<unsigned>(payload >> 96);
        r.mantissa1 = static_cast<unsigned>(payload >> 64);
        r.mantissa2 = static_cast<unsigned>(payload >> 32);
        r.mantissa3 = static_cast<unsigned>(payload);
    }
#endif
};

struct float_traits_doubledouble {
    using type = long double;

    struct value_repr {
        float_traits<double>::nan_repr high;
        float_traits<double>::nan_repr low;
    };

    using nan_repr = value_repr;

    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r,
                                                   std::uint64_t payload)
    {
        // -2, because of the implicit +1,
        // and the bit for signaling/quiet NaN
        static constexpr auto high_bits =
            std::numeric_limits<double>::digits - 2;
        float_traits<double>::apply_nan_payload(r.high, payload);
        float_traits<double>::apply_nan_payload(r.low, payload >> high_bits);
    }

#if SCN_HAS_INT128
    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r, uint128 payload)
    {
        // -2, because of the implicit +1,
        // and the bit for signaling/quiet NaN
        static constexpr auto high_bits =
            std::numeric_limits<double>::digits - 2;
        float_traits<double>::apply_nan_payload(
            r.high, static_cast<std::uint64_t>(payload));
        r.low.quiet_nan = 1;
        r.low.exponent = (1u << 11u) - 1u;
        float_traits<double>::apply_nan_payload(
            r.low, static_cast<std::uint64_t>(payload >> high_bits));
    }
#endif
};

struct nil_float_traits {};

using detail::mp_bool;
using detail::mp_cond;
using float_traits_for_long_double = mp_cond<
    // long double is equivalent to a double,
    // true on (non-mingw) Windows, Arm32, and Apple Arm64
    mp_bool<sizeof(double) == sizeof(long double)>,
    float_traits<double>,
    // x87 long double, on non-Windows x86
    mp_bool<std::numeric_limits<long double>::digits == 64>,
    float_traits_x87,
    // binary128 long double,
    // true on non-Apple non-Windows Arm64
    mp_bool<std::numeric_limits<long double>::digits == 113>,
    float_traits_binary128,
    // double-double (two regular doubles next to each other),
    // true on PowerPC
    mp_bool<std::numeric_limits<long double>::digits == 106>,
    float_traits_doubledouble,
    // Exotic long double, no payload setting support
    mp_bool<true>,
    nil_float_traits>;

template <>
struct float_traits<long double> : float_traits_for_long_double {
    using type = long double;
};

#if SCN_HAS_STD_F16
template <>
struct float_traits<std::float16_t> {
    using type = std::float16_t;

    struct value_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 5;
        unsigned mantissa : 10;
#else
        unsigned mantissa : 10;
        unsigned exponent : 5;
        unsigned negative : 1;
#endif
    };

    struct nan_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 5;
        unsigned quiet_nan : 1;
        unsigned mantissa : 9;
#else
        unsigned mantissa : 9;
        unsigned quiet_nan : 1;
        unsigned exponent : 5;
        unsigned negative : 1;
#endif
    };

    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r,
                                                   std::uint64_t payload)
    {
        SCN_EXPECT(r.quiet_nan == 1);
        SCN_EXPECT(r.exponent == (1u << 5u) - 1u);
        r.mantissa = static_cast<unsigned>(payload);
    }
};
#endif

#if SCN_HAS_STD_F32
template <>
struct float_traits<std::float32_t> : float_traits<float> {
    using type = std::float32_t;
};
#endif

#if SCN_HAS_STD_F64
template <>
struct float_traits<std::float64_t> : float_traits<double> {
    using type = std::float64_t;
};
#endif

#if SCN_HAS_STD_F128
template <>
struct float_traits<std::float128_t> : float_traits_binary128 {
    using type = std::float128_t;
};
#endif

#if SCN_HAS_STD_BF16
template <>
struct float_traits<std::bfloat16_t> {
    using type = std::bfloat16_t;

    struct value_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 8;
        unsigned mantissa : 7;
#else
        unsigned mantissa : 7;
        unsigned exponent : 8;
        unsigned negative : 1;
#endif
    };

    struct nan_repr {
#if SCN_IS_BIG_ENDIAN
        unsigned negative : 1;
        unsigned exponent : 8;
        unsigned quiet_nan : 1;
        unsigned mantissa : 6;
#else
        unsigned mantissa : 6;
        unsigned quiet_nan : 1;
        unsigned exponent : 8;
        unsigned negative : 1;
#endif
    };

    SCN_MAYBE_UNUSED static void apply_nan_payload(nan_repr& r,
                                                   std::uint64_t payload)
    {
        SCN_EXPECT(r.quiet_nan == 1);
        SCN_EXPECT(r.exponent == (1u << 8u) - 1u);
        r.mantissa = static_cast<unsigned>(payload);
    }
};
#endif

SCN_GCC_POP  // -Wconversion

    namespace
{
}

// Detect +0.0, -0.0, +inf, and -inf, despite stuff like -ffast-math

SCN_GCC_COMPAT_PUSH
SCN_GCC_COMPAT_IGNORE("-Wfloat-equal")

template <typename T>
[[nodiscard]]
bool is_float_any_zero(T value)
{
    return value == static_cast<T>(0.0) || value == static_cast<T>(-0.0);
}

template <typename T>
[[nodiscard]]
bool is_float_positive_zero(T value)
{
#if defined(__NO_SIGNED_ZEROS__) && __NO_SIGNED_ZEROS__
    using repr = typename float_traits<T>::value_repr;
    repr expected{};
    repr received{};
    std::memcpy(&received, &value, sizeof(repr));
    if constexpr (std::is_base_of_v<float_traits_x87, float_traits<T>>) {
        received.padding = 0;
    }
    return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
    return value == static_cast<T>(0.0);
#endif
}
template <typename T>
[[nodiscard]]
bool is_float_negative_zero(T value)
{
#if defined(__NO_SIGNED_ZEROS__) && __NO_SIGNED_ZEROS__
    using repr = typename float_traits<T>::value_repr;
    repr expected{};
    expected.negative = 1;
    repr received{};
    std::memcpy(&received, &value, sizeof(repr));
    if constexpr (std::is_base_of_v<float_traits_x87, float_traits<T>>) {
        received.padding = 0;
    }
    return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
    return value == static_cast<T>(-0.0);
#endif
}

template <typename T>
[[nodiscard]]
bool is_float_positive_infinity(T value)
{
    if constexpr (std::numeric_limits<T>::has_infinity) {
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
        using repr = typename float_traits<T>::value_repr;
        repr expected{};
        SCN_GCC_PUSH
        SCN_GCC_IGNORE("-Woverflow")
        expected.exponent = std::numeric_limits<unsigned>::max();
        SCN_GCC_POP
        repr received{};
        std::memcpy(&received, &value, sizeof(repr));
        if constexpr (std::is_base_of_v<float_traits_x87, float_traits<T>>) {
            expected.one = 1;
            received.padding = 0;
        }
        return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
        return value == std::numeric_limits<T>::infinity();
#endif
    }
    else {
        return false;
    }
}
template <typename T>
[[nodiscard]]
bool is_float_negative_infinity(T value)
{
    if constexpr (std::numeric_limits<T>::has_infinity) {
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
        using repr = typename float_traits<T>::value_repr;
        repr expected{};
        SCN_GCC_PUSH
        SCN_GCC_IGNORE("-Woverflow")
        expected.exponent = std::numeric_limits<unsigned>::max();
        SCN_GCC_POP
        expected.negative = 1;
        repr received{};
        std::memcpy(&received, &value, sizeof(repr));
        if constexpr (std::is_base_of_v<float_traits_x87, float_traits<T>>) {
            expected.one = 1;
            received.padding = 0;
        }
        return std::memcmp(&received, &expected, sizeof(repr)) == 0;
#else
        return value == -std::numeric_limits<T>::infinity();
#endif
    }
    else {
        return false;
    }
}

SCN_GCC_COMPAT_POP  // -Wfloat-equal

    namespace
{
}

template <typename CharT>
struct impl_init_data {
    contiguous_range_factory<CharT>& input;
    float_reader_base::float_kind kind;
    unsigned options;
};

////////////////////////////////////////////////////////////////////
// strtod-based implementation
// Fallback for all CharT and standard FloatT
// (plus possibly float16 on glibc)
////////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_STRTOD
template <typename T>
struct strtod_impl_base {
    template <typename CharT, typename Strtod>
    scan_expected<std::ptrdiff_t> parse(T& value,
                                        const CharT* src,
                                        Strtod strtod_cb)
    {
        CharT* end{};
        errno = 0;
        value = strtod_cb(src, &end);
        const auto saved_errno = errno;
        auto chars_read = end - src;

        SCN_TRY_DISCARD(this->check_error(chars_read, saved_errno, value));

        if (m_kind == float_reader_base::float_kind::hex_without_prefix &&
            chars_read >= 2) {
            chars_read -= 2;
        }

        return chars_read;
    }

    template <typename CharT>
    const CharT* get_null_terminated_source(
        contiguous_range_factory<CharT>& input)
    {
        if (!input.stores_allocated_string()) {
            // TODO: call float_reader::read_source?
            auto first_space = read_until_classic_space(input.view());
            input.assign(
                std::basic_string<CharT>{input.view().begin(), first_space});
        }

        if (this->m_kind == float_reader_base::float_kind::hex_without_prefix) {
            if constexpr (std::is_same_v<CharT, char>) {
                input.get_allocated_string().insert(0, "0x");
            }
            else {
                input.get_allocated_string().insert(0, L"0x");
            }
        }

        return input.get_allocated_string().c_str();
    }

    SCN_NODISCARD scan_expected<void> check_error(std::ptrdiff_t chars_read,
                                                  int c_errno,
                                                  T value) const
    {
        if (is_float_any_zero(value) && chars_read == 0) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "strtod failed: No conversion");
        }

        if (m_kind == float_reader_base::float_kind::hex_with_prefix &&
            (m_options & float_reader_base::allow_hex) == 0) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "strtod failed: Hexfloats parsed, "
                "but they're disallowed by the format string");
        }

        if (c_errno == ERANGE && is_float_positive_zero(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_positive_underflow,
                "strtod failed: Value too small");
        }
        if (c_errno == ERANGE && is_float_negative_zero(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_negative_underflow,
                "strtod failed: Value too small");
        }

        // This doesn't set ERANGE on all C standard library implementations,
        // so we need to check whether we were actually expecting infinity
        if (m_kind != float_reader_base::float_kind::inf_short &&
            m_kind != float_reader_base::float_kind::inf_long &&
            is_float_positive_infinity(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_positive_overflow,
                "strtod failed: Value too large");
        }
        if (m_kind != float_reader_base::float_kind::inf_short &&
            m_kind != float_reader_base::float_kind::inf_long &&
            is_float_negative_infinity(value)) {
            SCN_UNLIKELY_ATTR
            return detail::unexpected_scan_error(
                scan_error::value_negative_overflow,
                "strtod failed: Value too large");
        }

        return {};
    }

    static T generic_narrow_strtod(const char* str, char** str_end)
    {
#if SCN_HAS_STD_F16 && defined(__HAVE_FLOAT16) && __HAVE_FLOAT16
        if constexpr (std::is_same_v<T, std::float16_t>) {
            set_clocale_classic_guard clocale_guard{LC_NUMERIC};
            return static_cast<std::float16_t>(::strtof16(str, str_end));
        }
#endif

#if SCN_XLOCALE == SCN_XLOCALE_POSIX
        static locale_t cloc = ::newlocale(LC_ALL_MASK, "C", NULL);
        if constexpr (std::is_same_v<T, float>) {
            return ::strtof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::strtod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::strtold_l(str, str_end, cloc);
        }
#elif SCN_XLOCALE == SCN_XLOCALE_MSVC
        static _locale_t cloc = ::_create_locale(LC_ALL, "C");
        if constexpr (std::is_same_v<T, float>) {
            return ::_strtof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::_strtod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::_strtold_l(str, str_end, cloc);
        }
#else
        set_clocale_classic_guard clocale_guard{LC_NUMERIC};
        if constexpr (std::is_same_v<T, float>) {
            return std::strtof(str, str_end);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return std::strtod(str, str_end);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return std::strtold(str, str_end);
        }
#endif

        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    static T generic_wide_strtod(const wchar_t* str, wchar_t** str_end)
    {
#if SCN_HAS_STD_F16 && defined(__HAVE_FLOAT16) && __HAVE_FLOAT16
        if constexpr (std::is_same_v<T, std::float16_t>) {
            set_clocale_classic_guard clocale_guard{LC_NUMERIC};
            return static_cast<std::float16_t>(::wcstof16(str, str_end));
        }
#endif

#if SCN_XLOCALE == SCN_XLOCALE_POSIX
        static locale_t cloc = ::newlocale(LC_ALL_MASK, "C", NULL);
        if constexpr (std::is_same_v<T, float>) {
            return ::wcstof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::wcstod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::wcstold_l(str, str_end, cloc);
        }
#elif SCN_XLOCALE == SCN_XLOCALE_MSVC
        static _locale_t cloc = ::_create_locale(LC_ALL, "C");
        if constexpr (std::is_same_v<T, float>) {
            return ::_wcstof_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return ::_wcstod_l(str, str_end, cloc);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return ::_wcstold_l(str, str_end, cloc);
        }
#else
        set_clocale_classic_guard clocale_guard{LC_NUMERIC};
        if constexpr (std::is_same_v<T, float>) {
            return std::wcstof(str, str_end);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return std::wcstod(str, str_end);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return std::wcstold(str, str_end);
        }
#endif

        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    float_reader_base::float_kind m_kind;
    unsigned m_options;
};

template <typename CharT, typename T>
class strtod_impl : public strtod_impl_base<T> {
public:
    explicit strtod_impl(impl_init_data<CharT>& data)
        : strtod_impl_base<T>{data.kind, data.options}, m_input(data.input)
    {
    }

    template <typename F>
    scan_expected<std::ptrdiff_t> operator()(T& value, F&&)
    {
        return this->parse(value, this->get_null_terminated_source(m_input),
                           generic_strtod);
    }

private:
    static T generic_strtod(const CharT* str, CharT** str_end)
    {
        if constexpr (std::is_same_v<CharT, char>) {
            return strtod_impl_base<T>::generic_narrow_strtod(str, str_end);
        }
        else {
            return strtod_impl_base<T>::generic_wide_strtod(str, str_end);
        }
    }

    contiguous_range_factory<CharT>& m_input;
};

struct strtod_impl_traits {
    template <typename CharT, typename FloatT>
    static constexpr bool enabled =
        (std::is_same_v<CharT, char> || std::is_same_v<CharT, wchar_t>) &&
        (std::is_same_v<FloatT, float> || std::is_same_v<FloatT, double> ||
         std::is_same_v<FloatT, long double>
#if SCN_HAS_STD_F16 && defined(__HAVE_FLOAT16) && __HAVE_FLOAT16
         || std::is_same_v<FloatT, std::float16_t>
#endif
        );

    template <typename CharT, typename FloatT>
    using type = strtod_impl<CharT, FloatT>;
};

#else

struct strtod_impl_traits {
    template <typename, typename>
    static constexpr bool enabled = false;

    template <typename, typename>
    using type = void;
};

#endif

////////////////////////////////////////////////////////////////////
// std::from_chars-based implementation
// Only for CharT=char, if available
////////////////////////////////////////////////////////////////////

#if SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS
template <typename Float, typename = void>
inline constexpr bool has_charconv_for = false;

template <typename Float>
inline constexpr bool has_charconv_for<
    Float,
    std::void_t<decltype(std::from_chars(SCN_DECLVAL(const char*),
                                         SCN_DECLVAL(const char*),
                                         SCN_DECLVAL(Float&)))>> = true;

#if SCN_STDLIB_GLIBCXX
// libstdc++ has buggy std::from_chars for long double
template <>
inline constexpr bool has_charconv_for<long double, void> = false;

// libstdc++ delegates float16_t and bfloat16_t to float,
// which will lead to erroneous over-/underflow detection
#if SCN_HAS_STD_F16
template <>
inline constexpr bool has_charconv_for<std::float16_t, void> = false;
#endif
#if SCN_HAS_STD_BF16
template <>
inline constexpr bool has_charconv_for<std::bfloat16_t, void> = false;
#endif
#endif

struct SCN_MAYBE_UNUSED from_chars_impl_base {
    SCN_MAYBE_UNUSED from_chars_impl_base(impl_init_data<char>& data)
        : m_input(data.input), m_kind(data.kind), m_options(data.options)
    {
    }

protected:
    SCN_MAYBE_UNUSED scan_expected<std::chars_format> get_flags(
        std::string_view& input) const
    {
        auto flags = map_options_to_flags();

        if ((flags & std::chars_format::hex) != std::chars_format{}) {
            if (m_kind == float_reader_base::float_kind::hex_without_prefix) {
                return std::chars_format::hex;
            }
            else if (m_kind == float_reader_base::float_kind::hex_with_prefix) {
                input = input.substr(2);
                return std::chars_format::hex;
            }

            flags &= ~std::chars_format::hex;
            if (flags == std::chars_format{}) {
                return detail::unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "std::from_chars: Expected a hexfloat");
            }
        }

        return flags;
    }

    contiguous_range_factory<char>& m_input;
    float_reader_base::float_kind m_kind;
    unsigned m_options;

private:
    std::chars_format map_options_to_flags() const
    {
        std::chars_format flags{};

        if (m_options & float_reader_base::allow_fixed) {
            flags |= std::chars_format::fixed;
        }
        if (m_options & float_reader_base::allow_scientific) {
            flags |= std::chars_format::scientific;
        }
        if (m_options & float_reader_base::allow_hex) {
            flags |= std::chars_format::hex;
        }

        return flags;
    }
};

template <typename T>
class from_chars_impl : public from_chars_impl_base {
public:
    using from_chars_impl_base::from_chars_impl_base;

    template <typename F>
    scan_expected<std::ptrdiff_t> operator()(T& value, F&& fallback) const
    {
        auto input_view = m_input.view();
        const auto flags = get_flags(input_view);
        if (SCN_UNLIKELY(!flags)) {
            return unexpected(flags.error());
        }

        const auto result = std::from_chars(
            input_view.data(), input_view.data() + input_view.size(), value,
            *flags);

        if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "std::from_chars: invalid_argument");
        }
        if (result.ec == std::errc::result_out_of_range) {
            // std::from_chars doesn't give us a way to distinguish between
            // different kinds of over-/underflow:
            // per the standard, `value` is unmodified.
            // Do some parsing manually to try to determine what's up.

            // Get the exponent of the parsed float.
            // This is used to grok the order of magnitude
            // to determine whether we overflowed or underflowed.

            const auto [exponent, exponent_base] =
                [kind = m_kind,
                 input = detail::make_string_view_from_pointers<char>(
                     input_view.data(),
                     result.ptr)]() mutable -> std::pair<int, int> {
                if (kind == float_reader_base::float_kind::generic) {
                    kind =
                        read_until_code_unit(input,
                                             [](char ch) {
                                                 return ch == 'e' || ch == 'E';
                                             }) == input.end()
                            ? float_reader_base::float_kind::fixed
                            : float_reader_base::float_kind::scientific;
                }

                if (kind == float_reader_base::float_kind::fixed) {
                    // xxx.yyy

                    auto decimal_point_it = read_until_code_unit(input, '.');
                    auto whole_part =
                        detail::make_string_view_from_iterators<char>(
                            read_while_code_unit(input, '0'), decimal_point_it);

                    if (!whole_part.empty()) {
                        // xxx.yyy
                        // Non-zero digits before the decimal point,
                        // that determines the base-10 exponent
                        return {static_cast<int>(whole_part.size()), 10};
                    }

                    auto decimal_part =
                        detail::make_string_view_from_iterators<char>(
                            decimal_point_it == input.end()
                                ? decimal_point_it
                                : decimal_point_it + 1,
                            input.end());
                    auto decimal_initial_zeroes_part =
                        detail::make_string_view_from_iterators<char>(
                            decimal_part.begin(),
                            read_while_code_unit(decimal_part, '0'));

                    // 0.000yyy
                    // Base-10 exponent determined by number of zeroes
                    // after the decimal point, plus one (0.y -> exponent is -1)
                    return {-(1 + static_cast<int>(
                                      decimal_initial_zeroes_part.size())),
                            10};
                }

                if (kind == float_reader_base::float_kind::scientific) {
                    // xxx.yyyEzzz

                    auto exponent_it = read_until_code_unit(
                        input, [](char ch) { return ch == 'e' || ch == 'E'; });
                    auto significand_part =
                        detail::make_string_view_from_iterators<char>(
                            input.begin(), exponent_it);

                    auto decimal_point_it =
                        read_until_code_unit(significand_part, '.');
                    auto whole_part =
                        detail::make_string_view_from_iterators<char>(
                            read_while_code_unit(significand_part, '0'),
                            decimal_point_it);
                    auto decimal_part =
                        detail::make_string_view_from_iterators<char>(
                            detail::to_address(decimal_point_it) ==
                                    detail::to_address(exponent_it)
                                ? decimal_point_it
                                : decimal_point_it + 1,
                            exponent_it);

                    SCN_EXPECT(exponent_it != input.end());
                    auto exponent_part =
                        detail::make_string_view_from_iterators<char>(
                            exponent_it + 1, input.end());
                    int exponent_value{};
                    if (auto r = reader_impl_for_int<char>{}.read_default(
                            exponent_part, exponent_value, {});
                        !r) {
                        if (r.error() == scan_error::value_positive_overflow) {
                            return {std::numeric_limits<int>::max(), 0};
                        }
                        if (r.error() == scan_error::value_negative_overflow) {
                            return {std::numeric_limits<int>::min(), 0};
                        }
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                    }

                    if (!whole_part.empty()) {
                        return {static_cast<int>(whole_part.size()) +
                                    exponent_value,
                                10};
                    }

                    auto decimal_initial_zeroes_part =
                        detail::make_string_view_from_iterators<char>(
                            decimal_part.begin(),
                            read_while_code_unit(decimal_part, '0'));
                    return {-(1 + static_cast<int>(
                                      decimal_initial_zeroes_part.size())) +
                                exponent_value,
                            10};
                }

                if (kind == float_reader_base::float_kind::hex_with_prefix ||
                    kind == float_reader_base::float_kind::hex_without_prefix) {
                    // xxx.yyyPzzz
                    // Hex prefix ("0x") has been stripped previously
                    auto exponent_it = read_until_code_unit(
                        input, [](char ch) { return ch == 'p' || ch == 'P'; });
                    auto significand_part =
                        detail::make_string_view_from_iterators<char>(
                            input.begin(), exponent_it);

                    auto radix_point_it =
                        read_until_code_unit(significand_part, '.');
                    auto whole_part =
                        detail::make_string_view_from_iterators<char>(
                            read_while_code_unit(significand_part, '0'),
                            radix_point_it);
                    auto fractional_part =
                        detail::make_string_view_from_iterators<char>(
                            detail::to_address(radix_point_it) ==
                                    detail::to_address(exponent_it)
                                ? radix_point_it
                                : radix_point_it + 1,
                            exponent_it);

                    SCN_EXPECT(exponent_it != input.end());
                    auto exponent_part =
                        detail::make_string_view_from_iterators<char>(
                            exponent_it + 1, input.end());
                    int exponent_value{};
                    if (auto r = reader_impl_for_int<char>{}.read_default(
                            exponent_part, exponent_value, {});
                        !r) {
                        if (r.error() == scan_error::value_positive_overflow) {
                            return {std::numeric_limits<int>::max(), 0};
                        }
                        if (r.error() == scan_error::value_negative_overflow) {
                            return {std::numeric_limits<int>::min(), 0};
                        }
                        SCN_EXPECT(false);
                        SCN_UNREACHABLE;
                    }

                    if (!whole_part.empty()) {
                        return {static_cast<int>(whole_part.size()) +
                                    exponent_value / 8,
                                16};
                    }

                    auto fractional_initial_zeroes_part =
                        detail::make_string_view_from_iterators<char>(
                            fractional_part.begin(),
                            read_while_code_unit(fractional_part, '0'));
                    return {-(1 + static_cast<int>(
                                      fractional_initial_zeroes_part.size())) +
                                exponent_value / 8,
                            16};
                }

                SCN_EXPECT(false);
                SCN_UNREACHABLE;
            }();

            if (exponent > 0) {
                return detail::unexpected_scan_error(
                    scan_error::value_positive_overflow,
                    "std::from_chars: result_out_of_range, value too large");
            }

            // Some implementations treat subnormals as underflow.
            // We don't want that, we consider them to be valid values.
            // Check if the exponent we got could be a subnormal,
            // and fall back if that's the case.

            if (exponent_base == 10) {
                if (exponent >= static_cast<int>(std::log10(
                                    std::numeric_limits<T>::denorm_min()))) {
                    return fallback(detail::unexpected_scan_error(
                        scan_error::value_positive_underflow,
                        "std::from_chars: result_out_of_range, "
                        "value too small, possibly subnormal"));
                }
            }
            if (exponent_base == 16) {
                if (exponent >=
                    static_cast<int>(
                        std::log2(std::numeric_limits<T>::denorm_min()) /
                        std::log2(static_cast<T>(16.0)))) {
                    return fallback(detail::unexpected_scan_error(
                        scan_error::value_positive_underflow,
                        "std::from_chars: result_out_of_range, "
                        "value too small, possibly subnormal"));
                }
            }

            // Definitely smaller than the smallest subnormal,
            // just error out.
            return detail::unexpected_scan_error(
                scan_error::value_positive_underflow,
                "std::from_chars: result_out_of_range, value too small");
        }

        return result.ptr - m_input.view().data();
    }
};

struct from_chars_impl_traits {
    template <typename CharT, typename FloatT>
    static constexpr bool enabled =
        std::is_same_v<CharT, char> && has_charconv_for<FloatT>;

    template <typename, typename FloatT>
    using type = from_chars_impl<FloatT>;
};

#else

struct from_chars_impl_traits {
    template <typename, typename>
    static constexpr bool enabled = false;

    template <typename, typename>
    using type = void;
};

#endif  // SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS

////////////////////////////////////////////////////////////////////
// fast_float-based implementation
// Only for FloatT=(float OR double OR extended-float)
////////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_FAST_FLOAT

struct fast_float_impl_base {
    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wunneeded-member-function")
    // false positive

    SCN_NODISCARD fast_float::chars_format get_flags() const
    {
        unsigned format_flags{};
        if ((m_options & float_reader_base::allow_fixed) != 0) {
            format_flags |=
                static_cast<unsigned>(fast_float::chars_format::fixed);
        }
        if ((m_options & float_reader_base::allow_scientific) != 0) {
            format_flags |=
                static_cast<unsigned>(fast_float::chars_format::scientific);
        }

        return static_cast<fast_float::chars_format>(format_flags);
    }

    SCN_CLANG_POP

    float_reader_base::float_kind m_kind;
    unsigned m_options;
};

template <typename CharT, typename T>
struct fast_float_impl : fast_float_impl_base {
    fast_float_impl(impl_init_data<CharT>& data)
        : fast_float_impl_base{data.m_kind, data.m_options}, m_input(data.input)
    {
    }

    template <typename F>
    scan_expected<std::ptrdiff_t> operator()(T& value, F&& fallback) const
    {
        if (m_kind == float_reader_base::float_kind::hex_without_prefix ||
            m_kind == float_reader_base::float_kind::hex_with_prefix) {
            // fast_float doesn't support hexfloats
            return fallback({});
        }

        const auto flags = get_flags();
        const auto view = get_view();
        const auto result = fast_float::from_chars(
            view.data(), view.data() + view.size(), value, flags);

        if (SCN_UNLIKELY(result.ec == std::errc::invalid_argument)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "fast_float: invalid_argument");
        }
        if (SCN_UNLIKELY(result.ec == std::errc::result_out_of_range)) {
            // No need to handle -inf and -0.0,
            // the sign is stripped from the input,
            // so the result is always positive.
            // The sign is applied outside this call,
            // and these errors are turned into their negative counterparts,
            // if necessary.

            if (is_float_positive_infinity(value)) {
                return detail::unexpected_scan_error(
                    scan_error::value_positive_overflow,
                    "fast_float: result_out_of_range, value too large");
            }
            if (is_float_any_zero(value)) {
                return detail::unexpected_scan_error(
                    scan_error::value_positive_underflow,
                    "fast_float: result_out_of_range, value too small");
            }

            return fallback(detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "fast_float: Unknown result_out_of_range error"));
        }

        return result.ptr - view.data();
    }

private:
    auto get_view() const
    {
        if constexpr (sizeof(CharT) == 1) {
            return m_input.view();
        }
        else if constexpr (sizeof(CharT) == 2) {
            return std::u16string_view{
                reinterpret_cast<const char16_t*>(m_input.view().data()),
                m_input.view().size()};
        }
        else {
            return std::u32string_view{
                reinterpret_cast<const char32_t*>(m_input.view().data()),
                m_input.view().size()};
        }
    }

    contiguous_range_factory<CharT>& m_input;
};

struct fast_float_impl_traits {
    template <typename CharT, typename FloatT>
    static constexpr bool enabled =
        fast_float::is_supported_char_type<CharT>() &&
        fast_float::is_supported_float_type<CharT>();

    template <typename CharT, typename FloatT>
    using type = fast_float_impl<CharT, FloatT>;
};

#else

struct fast_float_impl_traits {
    template <typename, typename>
    static constexpr bool enabled = false;

    template <typename, typename>
    using type = void;
};

#endif  // !SCN_DISABLE_FAST_FLOAT

////////////////////////////////////////////////////////////////////
// Dispatch implementation
////////////////////////////////////////////////////////////////////

template <typename F, typename Payload>
void apply_nan_payload(F& value, Payload payload)
{
    if constexpr (!std::is_same_v<F, long double> ||
                  !std::is_same_v<float_traits_for_long_double,
                                  nil_float_traits>) {
        using traits = float_traits<F>;
        typename traits::nan_repr bits{};
        std::memcpy(&bits, &value, sizeof(bits));
        traits::apply_nan_payload(bits, payload);
        std::memcpy(&value, &bits, sizeof(bits));
    }
    else {
        static_assert(detail::dependent_false<F, float_traits_for_long_double,
                                              Payload>::value,
                      "");
    }
}

template <typename Traits, typename CharT, typename FloatT>
struct float_impl {
    using char_type = CharT;
    using float_type = FloatT;
    using traits = Traits;
    using impl_type = typename Traits::template type<CharT, FloatT>;
};

struct float_null_impl {};

template <typename Traits, typename CharT, typename FloatT>
using get_float_impl_for = detail::mp_cond<
    // Default to FloatT, if available
    detail::mp_bool<Traits::template enabled<CharT, FloatT>>,
    float_impl<Traits, CharT, FloatT>,
    // If FloatT is long double, but it's an alias to double, check that
    detail::mp_bool<std::is_same_v<FloatT, long double> &&
                    sizeof(double) == sizeof(long double) &&
                    std::numeric_limits<double>::digits ==
                        std::numeric_limits<long double>::digits &&
                    Traits::template enabled<CharT, double>>,
    float_impl<Traits, CharT, double>,
#if SCN_HAS_STD_F32
    // If FloatT is std::float32_t, but it's compatible with float, check that
    detail::mp_bool<std::is_same_v<FloatT, std::float32_t> &&
                    std::numeric_limits<float>::is_iec559 &&
                    sizeof(float) == sizeof(std::float32_t) &&
                    std::numeric_limits<float>::digits ==
                        std::numeric_limits<std::float32_t>::digits &&
                    Traits::template enabled<CharT, float>>,
    float_impl<Traits, CharT, float>,
#endif
#if SCN_HAS_STD_F64
    // If FloatT is std::float64_t, but it's compatible with double, check that
    detail::mp_bool<std::is_same_v<FloatT, std::float64_t> &&
                    std::numeric_limits<double>::is_iec559 &&
                    sizeof(double) == sizeof(std::float64_t) &&
                    std::numeric_limits<double>::digits ==
                        std::numeric_limits<std::float64_t>::digits &&
                    Traits::template enabled<CharT, double>>,
    float_impl<Traits, CharT, double>,
#endif
#if SCN_HAS_STD_F128
    // If FloatT is std::float128_t,
    // but it's compatible with long double, check that
    detail::mp_bool<std::is_same_v<FloatT, std::float128_t> &&
                    std::numeric_limits<long double>::is_iec559 &&
                    sizeof(long double) == sizeof(std::float128_t) &&
                    std::numeric_limits<long double>::digits ==
                        std::numeric_limits<std::float128_t>::digits &&
                    Traits::template enabled<CharT, long double>>,
    float_impl<Traits, CharT, long double>,
#endif
    // Nothing found
    std::true_type,
    float_null_impl>;

template <typename CharT, typename T, typename Impl, typename Fallback>
scan_expected<std::ptrdiff_t> parse_float_value_using_impl(
    impl_init_data<CharT>& data,
    T& value,
    Fallback&& fallback)
{
    auto impl = typename Impl::impl_type{data};

    if constexpr (std::is_same_v<T, typename Impl::float_type>) {
        return impl(value, fallback);
    }
    else {
        return impl(*reinterpret_cast<typename Impl::float_type*>(&value),
                    fallback);
    }
}

template <typename CharT, typename T>
scan_expected<std::ptrdiff_t> dispatch_parse_float_value(impl_init_data<CharT>&,
                                                         T&)
{
    return detail::unexpected_scan_error(
        scan_error::type_not_supported,
        "No valid floating-point parser available for this type");
}

template <typename CharT, typename T, typename Impl, typename... Impls>
scan_expected<std::ptrdiff_t> dispatch_parse_float_value(
    impl_init_data<CharT>& data,
    T& value)
{
    if constexpr (std::is_same_v<Impl, float_null_impl>) {
        return dispatch_parse_float_value<CharT, T, Impls...>(data, value);
    }
    else {
        auto next =
            [&](scan_expected<void> err) -> scan_expected<std::ptrdiff_t> {
            if constexpr ((std::is_same_v<Impls, float_null_impl> && ...)) {
                // If this is the last valid impl we have,
                // propagate the error we got
                if (!err.has_value()) {
                    return unexpected(err.error());
                }
            }
            // We still have valid impls to go, try those out
            return dispatch_parse_float_value<CharT, T, Impls...>(data, value);
        };
        return parse_float_value_using_impl<CharT, T, Impl>(data, value, next);
    }
}

template <typename CharT, typename T>
scan_expected<std::ptrdiff_t> parse_float_value(
    impl_init_data<CharT> data,
    contiguous_range_factory<CharT>& nan_payload,
    T& value)
{
    if (data.kind == float_reader_base::float_kind::inf_short) {
        if constexpr (std::numeric_limits<T>::has_infinity) {
            value = std::numeric_limits<T>::infinity();
            return static_cast<std::ptrdiff_t>(std::strlen("inf"));
        }
        else {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Type doesn't support infinities");
        }
    }
    if (data.kind == float_reader_base::float_kind::inf_long) {
        if constexpr (std::numeric_limits<T>::has_infinity) {
            value = std::numeric_limits<T>::infinity();
            return static_cast<std::ptrdiff_t>(std::strlen("infinity"));
        }
        else {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Type doesn't support infinities");
        }
    }
    if (data.kind == float_reader_base::float_kind::nan_simple) {
        if constexpr (std::numeric_limits<T>::has_quiet_NaN) {
            value = std::numeric_limits<T>::quiet_NaN();
            return static_cast<std::ptrdiff_t>(std::strlen("nan"));
        }
        else {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Type doesn't support quiet NaNs");
        }
    }
    if (data.kind == float_reader_base::float_kind::nan_with_payload) {
        if constexpr (std::numeric_limits<T>::has_quiet_NaN) {
            value = std::numeric_limits<T>::quiet_NaN();

            if constexpr (std::numeric_limits<T>::is_iec559) {
                // Use uint64, if the mantissa of T has 64 (or less) bits.
#if SCN_HAS_INT128
                using payload_type =
                    std::conditional_t<std::numeric_limits<T>::digits <= 64,
                                       std::uint64_t, uint128>;
#else
                using payload_type = std::uint64_t;
#endif
                payload_type payload{};
                if (auto result = reader_impl_for_int<CharT>{}.read_default(
                        nan_payload.view(), payload, {})) {
                    apply_nan_payload(value, payload);
                }
                else if (result.error().code() ==
                         scan_error::value_positive_overflow) {
                    apply_nan_payload(value,
                                      std::numeric_limits<payload_type>::max());
                }
            }

            return static_cast<std::ptrdiff_t>(std::strlen("nan()") +
                                               nan_payload.view().size());
        }
        else {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Type doesn't support quiet NaNs");
        }
    }

    SCN_EXPECT(!data.input.view().empty());
    if (data.kind == float_reader_base::float_kind::hex_without_prefix) {
        if (SCN_UNLIKELY(char_to_int(data.input.view().front()) >= 16)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Invalid floating-point digit");
        }
    }
    if (SCN_UNLIKELY(char_to_int(data.input.view().front()) >= 10)) {
        return detail::unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Invalid floating-point digit");
    }

    return dispatch_parse_float_value<
        CharT, T, get_float_impl_for<fast_float_impl_traits, CharT, T>,
        get_float_impl_for<from_chars_impl_traits, CharT, T>,
        get_float_impl_for<strtod_impl_traits, CharT, T>>(data, value);
}
}  // namespace

template <typename CharT>
template <typename T>
scan_expected<std::ptrdiff_t> float_reader<CharT>::parse_value_impl(T& value)
{
    auto n = parse_float_value<CharT>({this->m_buffer, m_kind, m_options},
                                      m_nan_payload_buffer, value);
    if (SCN_LIKELY(n)) {
        value = this->setsign(value);
        return n;
    }

    if (n.error().code() == scan_error::value_positive_overflow &&
        m_sign == sign_type::minus_sign) {
        return detail::unexpected_scan_error(
            scan_error::value_negative_overflow, n.error().msg());
    }
    if (n.error().code() == scan_error::value_positive_underflow &&
        m_sign == sign_type::minus_sign) {
        return detail::unexpected_scan_error(
            scan_error::value_negative_underflow, n.error().msg());
    }
    return n;
}

#define SCN_DEFINE_FLOAT_READER_TEMPLATE(CharT, FloatT)          \
    template auto float_reader<CharT>::parse_value_impl(FloatT&) \
        -> scan_expected<std::ptrdiff_t>;

#if !SCN_DISABLE_TYPE_FLOAT
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, float)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, float)
#endif
#if !SCN_DISABLE_TYPE_DOUBLE
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, double)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, double)
#endif
#if !SCN_DISABLE_TYPE_LONG_DOUBLE
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, long double)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, long double)
#endif

// C++23 extended float types:

#if SCN_HAS_STD_F16 && !SCN_DISABLE_TYPE_FLOAT16
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, std::float16_t)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, std::float16_t)
#endif
#if SCN_HAS_STD_F32 && !SCN_DISABLE_TYPE_FLOAT32
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, std::float32_t)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, std::float32_t)
#endif
#if SCN_HAS_STD_F64 && !SCN_DISABLE_TYPE_FLOAT64
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, std::float64_t)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, std::float64_t)
#endif
#if SCN_HAS_STD_F128 && !SCN_DISABLE_TYPE_FLOAT128
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, std::float128_t)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, std::float128_t)
#endif
#if SCN_HAS_STD_BF16 && !SCN_DISABLE_TYPE_BFLOAT16
SCN_DEFINE_FLOAT_READER_TEMPLATE(char, std::bfloat16_t)
SCN_DEFINE_FLOAT_READER_TEMPLATE(wchar_t, std::bfloat16_t)
#endif

#undef SCN_DEFINE_FLOAT_READER_TEMPLATE

/////////////////////////////////////////////////////////////////
// Integer reader implementation
/////////////////////////////////////////////////////////////////

namespace {
uint64_t get_eight_digits_word(const char* input)
{
    uint64_t val{};
    std::memcpy(&val, input, sizeof(uint64_t));
    if constexpr (SCN_IS_BIG_ENDIAN) {
        val = byteswap(val);
    }
    return val;
}

constexpr uint32_t parse_eight_decimal_digits_unrolled_fast(uint64_t word)
{
    constexpr uint64_t mask = 0x000000FF000000FF;
    constexpr uint64_t mul1 = 0x000F424000000064;  // 100 + (1000000ULL << 32)
    constexpr uint64_t mul2 = 0x0000271000000001;  // 1 + (10000ULL << 32)
    word -= 0x3030303030303030;
    word = (word * 10) + (word >> 8);  // val = (val * 2561) >> 8;
    word = (((word & mask) * mul1) + (((word >> 16) & mask) * mul2)) >> 32;
    return static_cast<uint32_t>(word);
}

constexpr bool is_word_made_of_eight_decimal_digits_fast(uint64_t word)
{
    return !((((word + 0x4646464646464646) | (word - 0x3030303030303030)) &
              0x8080808080808080));
}

void loop_parse_if_eight_decimal_digits(const char*& p,
                                        const char* const end,
                                        uint64_t& val)
{
    while (
        std::distance(p, end) >= 8 &&
        is_word_made_of_eight_decimal_digits_fast(get_eight_digits_word(p))) {
        val = val * 100'000'000 + parse_eight_decimal_digits_unrolled_fast(
                                      get_eight_digits_word(p));
        p += 8;
    }
}

const char* parse_decimal_integer_fast_impl(const char* begin,
                                            const char* const end,
                                            uint64_t& val)
{
    loop_parse_if_eight_decimal_digits(begin, end, val);

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (digit >= 10) {
            break;
        }
        val = 10ull * val + static_cast<uint64_t>(digit);
        ++begin;
    }

    return begin;
}

constexpr size_t maxdigits_u64_table[] = {
    0,  0,  64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16, 16, 16,
    16, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13};

SCN_FORCE_INLINE constexpr size_t maxdigits_u64(int base)
{
    SCN_EXPECT(base >= 2 && base <= 36);
    return maxdigits_u64_table[static_cast<size_t>(base)];
}

constexpr uint64_t min_safe_u64_table[] = {0,
                                           0,
                                           9223372036854775808ull,
                                           12157665459056928801ull,
                                           4611686018427387904,
                                           7450580596923828125,
                                           4738381338321616896,
                                           3909821048582988049,
                                           9223372036854775808ull,
                                           12157665459056928801ull,
                                           10000000000000000000ull,
                                           5559917313492231481,
                                           2218611106740436992,
                                           8650415919381337933,
                                           2177953337809371136,
                                           6568408355712890625,
                                           1152921504606846976,
                                           2862423051509815793,
                                           6746640616477458432,
                                           15181127029874798299ull,
                                           1638400000000000000,
                                           3243919932521508681,
                                           6221821273427820544,
                                           11592836324538749809ull,
                                           876488338465357824,
                                           1490116119384765625,
                                           2481152873203736576,
                                           4052555153018976267,
                                           6502111422497947648,
                                           10260628712958602189ull,
                                           15943230000000000000ull,
                                           787662783788549761,
                                           1152921504606846976,
                                           1667889514952984961,
                                           2386420683693101056,
                                           3379220508056640625,
                                           4738381338321616896};

SCN_FORCE_INLINE constexpr uint64_t min_safe_u64(int base)
{
    SCN_EXPECT(base >= 2 && base <= 36);
    return min_safe_u64_table[static_cast<size_t>(base)];
}

template <typename T>
constexpr bool check_integer_overflow(uint64_t val,
                                      size_t digits_count,
                                      int base,
                                      bool is_negative)
{
    SCN_UNUSED(is_negative);  // not really

    auto max_digits = maxdigits_u64(base);
    if (digits_count > max_digits) {
        return true;
    }
    if (digits_count == max_digits && val < min_safe_u64(base)) {
        return true;
    }
    if constexpr (!std::is_same_v<T, uint64_t>) {
        if (val > static_cast<uint64_t>(std::numeric_limits<T>::max()) +
                      static_cast<uint64_t>(is_negative)) {
            SCN_UNLIKELY_ATTR
            return true;
        }
    }

    return false;
}

template <typename T, typename Acc>
constexpr T store_result(Acc acc, bool is_negative)
{
    if (is_negative) {
        SCN_MSVC_PUSH
        SCN_MSVC_IGNORE(4146)
        return static_cast<T>(
            -std::numeric_limits<T>::max() -
            static_cast<T>(acc - std::numeric_limits<T>::max()));
        SCN_MSVC_POP
    }

    return static_cast<T>(acc);
}

template <typename T>
auto parse_decimal_integer_fast(std::string_view input,
                                T& val,
                                bool is_negative) -> scan_expected<const char*>
{
    static_assert(sizeof(T) <= sizeof(std::uint64_t));

    uint64_t u64val{};
    auto ptr = parse_decimal_integer_fast_impl(
        input.data(), input.data() + input.size(), u64val);

    auto digits_count = static_cast<size_t>(ptr - input.data());
    if (SCN_UNLIKELY(
            check_integer_overflow<T>(u64val, digits_count, 10, is_negative))) {
        return detail::unexpected_scan_error(
            is_negative ? scan_error::value_negative_overflow
                        : scan_error::value_positive_overflow,
            "Integer overflow");
    }

    val = store_result<T>(u64val, is_negative);
    return ptr;
}

template <typename CharT, typename T>
auto parse_regular_integer(std::basic_string_view<CharT> input,
                           T& val,
                           int base,
                           bool is_negative) -> scan_expected<const CharT*>
{
    uint64_t u64val{};
    const CharT* begin = input.data();
    const CharT* const end = input.data() + input.size();

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (digit >= base) {
            break;
        }
        u64val =
            static_cast<uint64_t>(base) * u64val + static_cast<uint64_t>(digit);
        ++begin;
    }

    auto digits_count = static_cast<size_t>(begin - input.data());
    if (SCN_UNLIKELY(check_integer_overflow<T>(u64val, digits_count, base,
                                               is_negative))) {
        return detail::unexpected_scan_error(
            is_negative ? scan_error::value_negative_overflow
                        : scan_error::value_positive_overflow,
            "Integer overflow");
    }

    val = store_result<T>(u64val, is_negative);
    return begin;
}

#if SCN_HAS_INT128
// int128 is parsed with a different algorithm,
// going over the input char-by-char and checking for overflow on each step.
// It's slower, but simpler,
// and we don't have to build separate lookup tables for it.
template <typename CharT, typename T>
auto parse_int128(std::basic_string_view<CharT> input,
                  T& val,
                  int base,
                  bool is_negative) -> scan_expected<const CharT*>
{
    constexpr uint128 uint_max = std::numeric_limits<uint128>::max();
    constexpr uint128 int_max = uint_max >> 1;
    constexpr uint128 abs_int_min = int_max + 1;

    SCN_GCC_COMPAT_PUSH
    SCN_GCC_COMPAT_IGNORE("-Wsign-conversion")
    auto const [limit_val, max_digit] = [&]() -> std::pair<uint128, uint128> {
        if constexpr (std::is_same_v<T, int128>) {
            if (is_negative) {
                return {abs_int_min / base, abs_int_min % static_cast<T>(base)};
            }
            return {int_max / base, int_max % static_cast<T>(base)};
        }
        else {
            return {uint_max / base, uint_max % static_cast<T>(base)};
        }
    }();

    const CharT* begin = input.data();
    const CharT* const end = input.data() + input.size();
    uint128 acc{};

    while (begin != end) {
        const auto digit = char_to_int(*begin);
        if (SCN_UNLIKELY(digit >= base)) {
            break;
        }
        if (acc < limit_val || (acc == limit_val && digit <= max_digit)) {
            SCN_LIKELY_ATTR
            acc = acc * static_cast<T>(base) + static_cast<T>(digit);
        }
        else {
            return detail::unexpected_scan_error(
                is_negative ? scan_error::value_negative_overflow
                            : scan_error::value_positive_overflow,
                "Integer overflow");
        }
        ++begin;
    }
    SCN_GCC_COMPAT_POP

    val = store_result<T>(acc, is_negative);
    return begin;
}

template <typename CharT>
auto parse_regular_integer(std::basic_string_view<CharT> input,
                           int128& val,
                           int base,
                           bool is_negative) -> scan_expected<const CharT*>
{
    return parse_int128(input, val, base, is_negative);
}

template <typename CharT>
auto parse_regular_integer(std::basic_string_view<CharT> input,
                           uint128& val,
                           int base,
                           bool is_negative) -> scan_expected<const CharT*>
{
    SCN_EXPECT(!is_negative);
    return parse_int128(input, val, base, is_negative);
}
#endif
}  // namespace

template <typename CharT, typename T>
auto parse_integer_value(std::basic_string_view<CharT> source,
                         T& value,
                         sign_type sign,
                         int base)
    -> scan_expected<typename std::basic_string_view<CharT>::iterator>
{
    SCN_EXPECT(!source.empty());
    SCN_EXPECT(std::is_signed_v<T> || sign == sign_type::plus_sign);
    SCN_EXPECT(sign != sign_type::default_sign);
    SCN_EXPECT(base > 0);

    if (char_to_int(source[0]) >= base) {
        SCN_UNLIKELY_ATTR
        return detail::unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Invalid integer value");
    }

    // Skip leading zeroes
    auto start = source.data();
    const auto end = source.data() + source.size();
    {
        for (; start != end; ++start) {
            if (*start != CharT{'0'}) {
                break;
            }
        }
        if (SCN_UNLIKELY(start == end || char_to_int(*start) >= base)) {
            value = 0;
            return ranges::next(source.begin(),
                                ranges::distance(source.data(), start));
        }
    }

    if constexpr (std::is_same_v<CharT, char> &&
                  sizeof(T) <= sizeof(std::uint64_t)) {
        if (base == 10) {
            SCN_TRY(ptr, parse_decimal_integer_fast(
                             detail::make_string_view_from_pointers(start, end),
                             value, sign == sign_type::minus_sign));
            return ranges::next(source.begin(),
                                ranges::distance(source.data(), ptr));
        }
    }

    SCN_TRY(ptr, parse_regular_integer(
                     detail::make_string_view_from_pointers(start, end), value,
                     base, sign == sign_type::minus_sign));
    return ranges::next(source.begin(), ranges::distance(source.data(), ptr));
}

template <typename T>
void parse_integer_value_exhaustive_valid(std::string_view source, T& value)
{
    SCN_EXPECT(!source.empty());

    bool negative_sign = false;
    if constexpr (std::is_signed_v<T>) {
        if (source.front() == '-') {
            source = source.substr(1);
            negative_sign = true;
        }
    }
    SCN_EXPECT(!source.empty());
    SCN_EXPECT(char_to_int(source.front()) < 10);

    const char* p = source.data();
    const char* const end = source.data() + source.size();

    uint64_t u64val{};
    while (std::distance(p, end) >= 8) {
        SCN_EXPECT(is_word_made_of_eight_decimal_digits_fast(
            get_eight_digits_word(p)));
        u64val =
            u64val * 100'000'000 +
            parse_eight_decimal_digits_unrolled_fast(get_eight_digits_word(p));
        p += 8;
    }

    while (p != end) {
        const auto digit = char_to_int(*p);
        SCN_EXPECT(digit < 10);
        u64val = 10ull * u64val + static_cast<uint64_t>(digit);
        ++p;
    }
    SCN_EXPECT(p == end);

    {
        auto digits_count = static_cast<size_t>(p - source.data());
        SCN_UNUSED(digits_count);
        SCN_EXPECT(check_integer_overflow<T>(u64val, digits_count, 10,
                                             negative_sign) == false);
    }

    value = store_result<T>(u64val, negative_sign);
}

#define SCN_DEFINE_INTEGER_READER_TEMPLATE(CharT, IntT)                      \
    template auto parse_integer_value(std::basic_string_view<CharT> source,  \
                                      IntT& value, sign_type sign, int base) \
        -> scan_expected<typename std::basic_string_view<CharT>::iterator>;

#if !SCN_DISABLE_TYPE_SCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, signed char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, signed char)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   signed char&);
#endif
#if !SCN_DISABLE_TYPE_SHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, short)
template void parse_integer_value_exhaustive_valid(std::string_view, short&);
#endif
#if !SCN_DISABLE_TYPE_INT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, int)
template void parse_integer_value_exhaustive_valid(std::string_view, int&);
#endif
#if !SCN_DISABLE_TYPE_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long)
template void parse_integer_value_exhaustive_valid(std::string_view, long&);
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, long long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   long long&);
#endif
#if !SCN_DISABLE_TYPE_UCHAR
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned char)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned char)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned char&);
#endif
#if !SCN_DISABLE_TYPE_USHORT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned short)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned short)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned short&);
#endif
#if !SCN_DISABLE_TYPE_UINT
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned int)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned int)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned int&);
#endif
#if !SCN_DISABLE_TYPE_ULONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned long&);
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, unsigned long long)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long long)
template void parse_integer_value_exhaustive_valid(std::string_view,
                                                   unsigned long long&);
#endif

#if SCN_HAS_INT128

#if !SCN_DISABLE_TYPE_INT128
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, int128)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, int128)
// no parse_integer_value_exhaustive_valid
#endif

#if !SCN_DISABLE_TYPE_UINT128
SCN_DEFINE_INTEGER_READER_TEMPLATE(char, uint128)
SCN_DEFINE_INTEGER_READER_TEMPLATE(wchar_t, uint128)
// no parse_integer_value_exhaustive_valid
#endif

#endif  // SCN_HAS_INT128

#undef SCN_DEFINE_INTEGER_READER_TEMPLATE
}  // namespace impl

/////////////////////////////////////////////////////////////////
// vscan implementation
/////////////////////////////////////////////////////////////////

namespace {
template <typename CharT>
constexpr bool is_simple_single_argument_format_string(
    std::basic_string_view<CharT> format)
{
    if (format.size() != 2) {
        return false;
    }
    return format[0] == CharT{'{'} && format[1] == CharT{'}'};
}

template <typename CharT>
scan_expected<std::ptrdiff_t> scan_simple_single_argument(
    std::basic_string_view<CharT> source,
    basic_scan_args<detail::default_context<CharT>> args,
    basic_scan_arg<detail::default_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Argument #0 not found");
    }

    auto reader =
        impl::default_arg_reader<impl::basic_contiguous_scan_context<CharT>>{
            ranges::subrange<const CharT*>{source.data(),
                                           source.data() + source.size()},
            SCN_MOVE(args), loc};
    SCN_TRY(it, arg.visit(SCN_MOVE(reader)));
    return ranges::distance(source.data(), it);
}
template <typename CharT>
scan_expected<std::ptrdiff_t> scan_simple_single_argument(
    detail::basic_scan_buffer<CharT>& source,
    basic_scan_args<detail::default_context<CharT>> args,
    basic_scan_arg<detail::default_context<CharT>> arg,
    detail::locale_ref loc = {})
{
    if (SCN_UNLIKELY(!arg)) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Argument #0 not found");
    }

    if (SCN_LIKELY(source.is_contiguous())) {
        auto reader = impl::default_arg_reader<
            impl::basic_contiguous_scan_context<CharT>>{source.get_contiguous(),
                                                        SCN_MOVE(args), loc};
        SCN_TRY(it, arg.visit(SCN_MOVE(reader)));
        return ranges::distance(source.get_contiguous().begin(), it);
    }

    auto reader = impl::default_arg_reader<detail::default_context<CharT>>{
        source.get(), SCN_MOVE(args), loc};
    SCN_TRY(it, arg.visit(SCN_MOVE(reader)));
    return it.position();
}

template <typename Context, typename ID, typename Handler>
auto get_arg(Context& ctx, ID id, Handler& handler)
{
    auto arg = ctx.arg(id);
    if (SCN_UNLIKELY(!arg)) {
        handler.on_error("Failed to find argument with ID");
    }
    return arg;
}

struct auto_id {};

template <typename Context>
class specs_handler : public detail::specs_setter {
public:
    using char_type = typename Context::char_type;

    constexpr specs_handler(detail::format_specs& specs,
                            basic_scan_parse_context<char_type>& parse_ctx,
                            Context& ctx)
        : detail::specs_setter(specs), m_parse_ctx(parse_ctx), m_ctx(ctx)
    {
    }

private:
    constexpr auto get_arg(auto_id)
    {
        return get_arg(m_ctx, m_parse_ctx.next_arg_id(), *this);
    }

    constexpr auto get_arg(std::size_t arg_id)
    {
        m_parse_ctx.check_arg_id(arg_id);
        return get_arg(m_ctx, arg_id, *this);
    }

    basic_scan_parse_context<char_type>& m_parse_ctx;
    Context& m_ctx;
};

struct format_handler_base {
    format_handler_base(size_t argcount) : args_count(argcount)
    {
        if (SCN_UNLIKELY(args_count >= 64)) {
            visited_args_upper.resize((args_count - 64) / 8);
        }
    }

    void check_args_exhausted()
    {
        {
            const auto args_count_lower64 = args_count >= 64 ? 64 : args_count;
            const uint64_t mask = args_count_lower64 == 64
                                      ? std::numeric_limits<uint64_t>::max()
                                      : (1ull << args_count_lower64) - 1;

            if (visited_args_lower64 != mask) {
                return on_error({scan_error::invalid_format_string,
                                 "Argument list not exhausted"});
            }
        }

        if (args_count < 64) {
            return;
        }

        auto last_args_count = args_count - 64;
        for (auto it = visited_args_upper.begin();
             it != visited_args_upper.end() - 1; ++it) {
            if (*it != std::numeric_limits<uint8_t>::max()) {
                return on_error({scan_error::invalid_format_string,
                                 "Argument list not exhausted"});
            }
            last_args_count -= 8;
        }

        const auto mask = static_cast<uint8_t>(1u << last_args_count) - 1;
        if (visited_args_upper.back() != mask) {
            return on_error({scan_error::invalid_format_string,
                             "Argument list not exhausted"});
        }
    }

    void on_error(const char* msg)
    {
        SCN_UNLIKELY_ATTR
        error = detail::unexpected_scan_error(scan_error::invalid_format_string,
                                              msg);
    }
    void on_error(scan_error err)
    {
        error = unexpected(err);
    }

    SCN_NODISCARD scan_expected<void> get_error() const
    {
        return error;
    }

    SCN_NODISCARD bool has_arg_been_visited(size_t id)
    {
        if (SCN_UNLIKELY(id >= args_count)) {
            on_error({scan_error::invalid_format_string,
                      "Argument ID out-of-range"});
            return false;
        }

        if (SCN_LIKELY(id < 64)) {
            return (visited_args_lower64 >> id) & 1ull;
        }

        id -= 64;
        return (visited_args_upper[id / 8] >> (id % 8)) & 1ull;
    }

    void set_arg_as_visited(size_t id)
    {
        if (SCN_UNLIKELY(id >= args_count)) {
            on_error({scan_error::invalid_format_string,
                      "Argument ID out-of-range"});
            return;
        }

        if (SCN_UNLIKELY(has_arg_been_visited(id))) {
            on_error({scan_error::invalid_format_string,
                      "Argument with this ID has already been scanned"});
        }

        if (SCN_LIKELY(id < 64u)) {
            visited_args_lower64 |= (1ull << id);
            return;
        }

        id -= 64u;
        visited_args_upper[id / 8u] |= static_cast<uint8_t>(1u << (id % 8u));
    }

    std::size_t args_count;
    scan_expected<void> error{};
    uint64_t visited_args_lower64{0};
    std::vector<uint8_t> visited_args_upper{};
};

template <typename CharT>
struct simple_context_wrapper {
    using context_type = detail::default_context<CharT>;

    simple_context_wrapper(detail::basic_scan_buffer<CharT>& source,
                           basic_scan_args<detail::default_context<CharT>> args,
                           detail::locale_ref loc)
        : ctx(source.get().begin(), SCN_MOVE(args), loc)
    {
    }

    detail::default_context<CharT>& get()
    {
        return ctx;
    }
    detail::default_context<CharT>& get_custom()
    {
        return ctx;
    }

    detail::default_context<CharT> ctx;
};

template <typename CharT>
struct contiguous_context_wrapper {
    using context_type = impl::basic_contiguous_scan_context<CharT>;

    contiguous_context_wrapper(
        ranges::subrange<const CharT*> source,
        basic_scan_args<detail::default_context<CharT>> args,
        detail::locale_ref loc)
        : contiguous_ctx(source, args, loc)
    {
    }

    impl::basic_contiguous_scan_context<CharT>& get()
    {
        return contiguous_ctx;
    }
    detail::default_context<CharT>& get_custom()
    {
        if (!buffer) {
            buffer.emplace(detail::make_string_view_from_pointers(
                ranges::data(contiguous_ctx.underlying_range()),
                ranges::data(contiguous_ctx.underlying_range()) +
                    ranges::size(contiguous_ctx.underlying_range())));
        }
        auto it = buffer->get().begin();
        it.batch_advance_to(contiguous_ctx.begin_position());
        custom_ctx.emplace(it, contiguous_ctx.args(), contiguous_ctx.locale());
        return *custom_ctx;
    }

    impl::basic_contiguous_scan_context<CharT> contiguous_ctx;
    std::optional<detail::basic_scan_string_buffer<CharT>> buffer{std::nullopt};
    std::optional<detail::default_context<CharT>> custom_ctx{std::nullopt};
};

template <bool Contiguous, typename CharT>
using context_wrapper_t = std::conditional_t<Contiguous,
                                             contiguous_context_wrapper<CharT>,
                                             simple_context_wrapper<CharT>>;

template <bool Contiguous, typename CharT>
struct format_handler : format_handler_base {
    using context_wrapper_type = context_wrapper_t<Contiguous, CharT>;
    using context_type = typename context_wrapper_type::context_type;
    using char_type = typename context_type::char_type;
    using format_type = std::basic_string_view<char_type>;

    using parse_context_type = typename context_type::parse_context_type;
    using args_type = basic_scan_args<detail::default_context<char_type>>;
    using arg_type = basic_scan_arg<detail::default_context<char_type>>;

    template <typename Source>
    format_handler(Source&& source,
                   format_type format,
                   args_type args,
                   detail::locale_ref loc,
                   std::size_t argcount)
        : format_handler_base{argcount},
          parse_ctx{source_tag<Source&&>, format},
          ctx{SCN_FWD(source), SCN_MOVE(args), SCN_MOVE(loc)}
    {
    }

    void on_literal_text(const char_type* begin, const char_type* end)
    {
        for (; begin != end; ++begin) {
            auto it = get_ctx().begin();
            if (impl::is_range_eof(it, get_ctx().end())) {
                SCN_UNLIKELY_ATTR
                return on_error(
                    {scan_error::invalid_literal, "Unexpected end of source"});
            }

            if (auto [after_space_it, cp, is_space] = impl::is_first_char_space(
                    detail::make_string_view_from_pointers(begin, end));
                cp == detail::invalid_code_point) {
                SCN_UNLIKELY_ATTR
                return on_error({scan_error::invalid_format_string,
                                 "Invalid encoding in format string"});
            }
            else if (is_space) {
                // Skip all whitespace in input
                get_ctx().advance_to(
                    impl::read_while_classic_space(get_ctx().range()));
                // And, skip all whitespace in the format string
                auto begin_it = impl::read_while_classic_space(
                    detail::make_string_view_from_pointers(
                        detail::to_address(after_space_it),
                        detail::to_address(end)));
                // (-1 because of the for loop ++begin)
                begin = detail::to_address(begin_it) - 1;
                continue;
            }

            if (*it != *begin) {
                SCN_UNLIKELY_ATTR
                return on_error({scan_error::invalid_literal,
                                 "Unexpected literal character in source"});
            }
            get_ctx().advance_to(ranges::next(it));
        }
    }

    constexpr std::size_t on_arg_id()
    {
        return parse_ctx.next_arg_id();
    }
    constexpr std::size_t on_arg_id(std::size_t id)
    {
        parse_ctx.check_arg_id(id);
        return id;
    }

    template <typename Visitor>
    void on_visit_scan_arg(Visitor&& visitor, arg_type arg)
    {
        if (!get_error() || !arg) {
            SCN_UNLIKELY_ATTR
            return;
        }

        auto r = arg.visit(SCN_FWD(visitor));
        if (SCN_UNLIKELY(!r)) {
            on_error(r.error());
        }
        else {
            get_ctx().advance_to(*r);
        }
    }

    void on_replacement_field(std::size_t arg_id, const char_type*)
    {
        auto arg = get_arg(get_ctx(), arg_id, *this);
        set_arg_as_visited(arg_id);

        on_visit_scan_arg(
            impl::default_arg_reader<context_type>{
                get_ctx().range(), get_ctx().args(), get_ctx().locale()},
            arg);
    }

    const char_type* on_format_specs(std::size_t arg_id,
                                     const char_type* begin,
                                     const char_type* end)
    {
        auto arg = get_arg(get_ctx(), arg_id, *this);
        set_arg_as_visited(arg_id);

        if (detail::get_arg_type(arg) == detail::arg_type::custom_type) {
            parse_ctx.advance_to(begin);
            on_visit_scan_arg(
                impl::custom_reader<detail::default_context<char_type>>{
                    parse_ctx, get_custom_ctx()},
                arg);
            return parse_ctx.begin();
        }

        auto specs = detail::format_specs{};
        detail::specs_checker<specs_handler<context_type>> handler{
            specs_handler<context_type>{specs, parse_ctx, get_ctx()},
            detail::get_arg_type(arg)};

        begin = detail::parse_format_specs(begin, end, handler);
        if (begin == end || *begin != char_type{'}'}) {
            SCN_UNLIKELY_ATTR
            on_error({scan_error::invalid_format_string,
                      "Missing '}' in format string"});
            return parse_ctx.begin();
        }
        if (SCN_UNLIKELY(!handler.get_error())) {
            return parse_ctx.begin();
        }
        parse_ctx.advance_to(begin);

        on_visit_scan_arg(
            impl::arg_reader<context_type>{get_ctx().range(), specs,
                                           get_ctx().locale()},
            arg);
        return parse_ctx.begin();
    }

    context_type& get_ctx()
    {
        return ctx.get();
    }
    auto& get_custom_ctx()
    {
        return ctx.get_custom();
    }

    parse_context_type parse_ctx;
    context_wrapper_type ctx;
};

template <typename CharT, typename Handler>
scan_expected<std::ptrdiff_t> vscan_parse_format_string(
    std::basic_string_view<CharT> format,
    Handler& handler)
{
    const auto beg = handler.get_ctx().begin();
    detail::parse_format_string<false>(format, handler);
    if (auto err = handler.get_error(); SCN_UNLIKELY(!err)) {
        return unexpected(err.error());
    }
    return ranges::distance(beg, handler.get_ctx().begin());
}

template <typename CharT>
scan_expected<std::ptrdiff_t> vscan_internal(
    std::basic_string_view<CharT> source,
    std::basic_string_view<CharT> format,
    basic_scan_args<detail::default_context<CharT>> args,
    detail::locale_ref loc = {})
{
    const auto argcount = args.size();
    if (is_simple_single_argument_format_string(format) && argcount == 1) {
        auto arg = args.get(0);
        return scan_simple_single_argument(source, SCN_MOVE(args), arg);
    }

    auto handler = format_handler<true, CharT>{
        ranges::subrange<const CharT*>{source.data(),
                                       source.data() + source.size()},
        format, SCN_MOVE(args), SCN_MOVE(loc), argcount};
    return vscan_parse_format_string(format, handler);
}

template <typename CharT>
scan_expected<std::ptrdiff_t> vscan_internal(
    detail::basic_scan_buffer<CharT>& buffer,
    std::basic_string_view<CharT> format,
    basic_scan_args<detail::default_context<CharT>> args,
    detail::locale_ref loc = {})
{
    const auto argcount = args.size();
    if (is_simple_single_argument_format_string(format) && argcount == 1) {
        auto arg = args.get(0);
        return scan_simple_single_argument(buffer, SCN_MOVE(args), arg);
    }

    if (buffer.is_contiguous()) {
        auto handler = format_handler<true, CharT>{buffer.get_contiguous(),
                                                   format, SCN_MOVE(args),
                                                   SCN_MOVE(loc), argcount};
        return vscan_parse_format_string(format, handler);
    }

    SCN_UNLIKELY_ATTR
    {
        auto handler = format_handler<false, CharT>{
            buffer, format, SCN_MOVE(args), SCN_MOVE(loc), argcount};
        return vscan_parse_format_string(format, handler);
    }
}

template <typename Source, typename CharT>
scan_expected<std::ptrdiff_t> vscan_value_internal(
    Source&& source,
    basic_scan_arg<detail::default_context<CharT>> arg)
{
    return scan_simple_single_argument(SCN_FWD(source), {}, arg);
}
}  // namespace

namespace detail {
template <typename T>
auto scan_int_impl(std::string_view source, T& value, int base)
    -> scan_expected<std::string_view::iterator>
{
    SCN_TRY(beg, impl::skip_classic_whitespace(source).transform_error(
                     impl::make_eof_scan_error));
    auto reader = impl::reader_impl_for_int<char>{};
    return reader.read_default_with_base(ranges::subrange{beg, source.end()},
                                         value, base);
}

template <typename T>
auto scan_int_exhaustive_valid_impl(std::string_view source) -> T
{
    T value{};
    impl::parse_integer_value_exhaustive_valid(source, value);
    return value;
}
}  // namespace detail

scan_expected<void> vinput(std::string_view format, scan_args args)
{
    auto buffer = detail::make_file_scan_buffer(stdin);
    auto n = vscan_internal(buffer, format, args);
    if (n) {
        if (SCN_UNLIKELY(!buffer.sync(*n))) {
            return detail::unexpected_scan_error(
                scan_error::invalid_source_state,
                "Failed to sync with underlying FILE");
        }
        return {};
    }
    if (SCN_UNLIKELY(!buffer.sync_all())) {
        return detail::unexpected_scan_error(
            scan_error::invalid_source_state,
            "Failed to sync with underlying FILE");
    }
    return unexpected(n.error());
}

namespace detail {

namespace {

template <typename Source>
scan_expected<std::ptrdiff_t> sync_after_vscan(
    Source& source,
    scan_expected<std::ptrdiff_t> result)
{
    if (SCN_LIKELY(result)) {
        if (SCN_UNLIKELY(!source.sync(*result))) {
            return detail::unexpected_scan_error(
                scan_error::invalid_source_state,
                "Failed to sync with underlying source");
        }
    }
    else {
        if (SCN_UNLIKELY(!source.sync_all())) {
            return detail::unexpected_scan_error(
                scan_error::invalid_source_state,
                "Failed to sync with underlying source");
        }
    }
    return result;
}

}  // namespace

scan_expected<std::ptrdiff_t> vscan_impl(std::string_view source,
                                         std::string_view format,
                                         scan_args args)
{
    return vscan_internal(source, format, args);
}
scan_expected<std::ptrdiff_t> vscan_impl(scan_buffer& source,
                                         std::string_view format,
                                         scan_args args)
{
    auto n = vscan_internal(source, format, args);
    return sync_after_vscan(source, n);
}

scan_expected<std::ptrdiff_t> vscan_impl(std::wstring_view source,
                                         std::wstring_view format,
                                         wscan_args args)
{
    return vscan_internal(source, format, args);
}
scan_expected<std::ptrdiff_t> vscan_impl(wscan_buffer& source,
                                         std::wstring_view format,
                                         wscan_args args)
{
    auto n = vscan_internal(source, format, args);
    return sync_after_vscan(source, n);
}

#if !SCN_DISABLE_LOCALE
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   std::string_view source,
                                                   std::string_view format,
                                                   scan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   scan_buffer& source,
                                                   std::string_view format,
                                                   scan_args args)
{
    auto n = vscan_internal(source, format, args, detail::locale_ref{loc});
    return sync_after_vscan(source, n);
}

template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   std::wstring_view source,
                                                   std::wstring_view format,
                                                   wscan_args args)
{
    return vscan_internal(source, format, args, detail::locale_ref{loc});
}
template <typename Locale>
scan_expected<std::ptrdiff_t> vscan_localized_impl(const Locale& loc,
                                                   wscan_buffer& source,
                                                   std::wstring_view format,
                                                   wscan_args args)
{
    auto n = vscan_internal(source, format, args, detail::locale_ref{loc});
    return sync_after_vscan(source, n);
}

template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                std::string_view,
                                                std::string_view,
                                                scan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                scan_buffer&,
                                                std::string_view,
                                                scan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                std::wstring_view,
                                                std::wstring_view,
                                                wscan_args)
    -> scan_expected<std::ptrdiff_t>;
template auto vscan_localized_impl<std::locale>(const std::locale&,
                                                wscan_buffer&,
                                                std::wstring_view,
                                                wscan_args)
    -> scan_expected<std::ptrdiff_t>;
#endif

scan_expected<std::ptrdiff_t> vscan_value_impl(std::string_view source,
                                               basic_scan_arg<scan_context> arg)
{
    return vscan_value_internal(source, arg);
}
scan_expected<std::ptrdiff_t> vscan_value_impl(scan_buffer& source,
                                               basic_scan_arg<scan_context> arg)
{
    auto n = vscan_value_internal(source, arg);
    return sync_after_vscan(source, n);
}

scan_expected<std::ptrdiff_t> vscan_value_impl(
    std::wstring_view source,
    basic_scan_arg<wscan_context> arg)
{
    return vscan_value_internal(source, arg);
}
scan_expected<std::ptrdiff_t> vscan_value_impl(
    wscan_buffer& source,
    basic_scan_arg<wscan_context> arg)
{
    auto n = vscan_value_internal(source, arg);
    return sync_after_vscan(source, n);
}

#if !SCN_DISABLE_TYPE_SCHAR
template auto scan_int_impl(std::string_view, signed char&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> signed char;
#endif
#if !SCN_DISABLE_TYPE_SHORT
template auto scan_int_impl(std::string_view, short&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> short;
#endif
#if !SCN_DISABLE_TYPE_INT
template auto scan_int_impl(std::string_view, int&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> int;
#endif
#if !SCN_DISABLE_TYPE_LONG
template auto scan_int_impl(std::string_view, long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> long;
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
template auto scan_int_impl(std::string_view, long long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> long long;
#endif
#if !SCN_DISABLE_TYPE_UCHAR
template auto scan_int_impl(std::string_view, unsigned char&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned char;
#endif
#if !SCN_DISABLE_TYPE_USHORT
template auto scan_int_impl(std::string_view, unsigned short&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned short;
#endif
#if !SCN_DISABLE_TYPE_UINT
template auto scan_int_impl(std::string_view, unsigned int&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned int;
#endif
#if !SCN_DISABLE_TYPE_ULONG
template auto scan_int_impl(std::string_view, unsigned long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view) -> unsigned long;
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
template auto scan_int_impl(std::string_view, unsigned long long&, int)
    -> scan_expected<std::string_view::iterator>;
template auto scan_int_exhaustive_valid_impl(std::string_view)
    -> unsigned long long;
#endif

#if SCN_HAS_INT128

#if !SCN_DISABLE_TYPE_INT128
template auto scan_int_impl(std::string_view, int128&, int)
    -> scan_expected<std::string_view::iterator>;
#endif

#if !SCN_DISABLE_TYPE_UINT128
template auto scan_int_impl(std::string_view, uint128&, int)
    -> scan_expected<std::string_view::iterator>;
#endif

#endif

///////////////////////////////////////////////////////////////////////////////
// <chrono> scanning
///////////////////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_CHRONO

template <typename T>
struct datetime_setter;

template <typename T>
struct unreachable_datetime_setter {
    template <typename Handler>
    static void set_subsec(Handler&, T&, setter_state&, double)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Handler>
    static void set_sec(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_min(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_hour24(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_hour12(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_mday(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_mon(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_full_year(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_century(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_short_year(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_wday(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    template <typename Handler>
    static void set_yday(Handler&, T&, setter_state&, int)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Handler>
    static void set_tz_offset(Handler&, T&, setter_state&, std::chrono::minutes)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Handler>
    static void set_tz_name(Handler&, T&, setter_state&, const std::string&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    static void handle_am_pm(T&, setter_state&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    static void handle_short_year_and_century(T&, setter_state&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
};

template <>
struct datetime_setter<std::tm> {
    template <typename Handler>
    static void set_subsec(Handler& h, std::tm&, setter_state&, double)
    {
        h.set_error({scan_error::invalid_format_string,
                     "Subsecond precision not supported with std::tm"});
    }

    template <typename Handler>
    static void set_sec(Handler& h, std::tm& t, setter_state& st, int s)
    {
        if (SCN_UNLIKELY(s < 0 || s > 60)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_sec"});
        }
        t.tm_sec = s;
        st.set_sec(h);
    }
    template <typename Handler>
    static void set_min(Handler& h, std::tm& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 0 || m > 59)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_min"});
        }
        t.tm_min = m;
        st.set_min(h);
    }
    template <typename Handler>
    static void set_hour24(Handler& hdl, std::tm& t, setter_state& st, int h)
    {
        if (SCN_UNLIKELY(h < 0 || h > 23)) {
            return hdl.set_error({scan_error::invalid_scanned_value,
                                  "Invalid value for tm_hour"});
        }
        t.tm_hour = h;
        st.set_hour24(hdl);
    }
    template <typename Handler>
    static void set_hour12(Handler& hdl, std::tm& t, setter_state& st, int h)
    {
        if (SCN_UNLIKELY(h < 1 || h > 12)) {
            return hdl.set_error({scan_error::invalid_scanned_value,
                                  "Invalid value for 12-hour tm_hour"});
        }
        t.tm_hour = h;
        st.set_hour12(hdl);
    }
    template <typename Handler>
    static void set_mday(Handler& h, std::tm& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_mday"});
        }
        t.tm_mday = d;
        st.set_mday(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h, std::tm& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 12)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_mon"});
        }
        t.tm_mon = m - 1;
        st.set_mon(h);
    }
    template <typename Handler>
    static void set_full_year(Handler& h, std::tm& t, setter_state& st, int y)
    {
        if (SCN_UNLIKELY(y < std::numeric_limits<int>::min() + 1900)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_year"});
        }
        t.tm_year = y - 1900;
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h, std::tm&, setter_state& st, int c)
    {
        // TODO: range check
        st.century_value = static_cast<unsigned char>(c);
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h, std::tm&, setter_state& st, int y)
    {
        if (SCN_UNLIKELY(y < 0 || y > 99)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_year"});
        }
        st.short_year_value = static_cast<unsigned char>(y);
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_wday(Handler& h, std::tm& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_wday"});
        }
        t.tm_wday = d;
        st.set_wday(h);
    }
    template <typename Handler>
    static void set_yday(Handler& h, std::tm& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 365)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for tm_yday"});
        }
        t.tm_yday = d;
        st.set_yday(h);
    }

    template <typename Handler>
    static void set_tz_offset(Handler& h,
                              std::tm& t,
                              setter_state&,
                              std::chrono::minutes o)
    {
        if constexpr (mp_valid<has_tm_gmtoff_predicate, std::tm>::value) {
            assign_gmtoff(t,
                          std::chrono::duration_cast<std::chrono::seconds>(o));
        }
        else {
            return h.set_error(
                {scan_error::invalid_format_string, "tm_gmtoff not supported"});
        }
    }

    template <typename Handler>
    static void set_tz_name(Handler& h,
                            std::tm&,
                            setter_state&,
                            const std::string&)
    {
        return h.set_error(
            {scan_error::invalid_format_string, "tm_zone not supported"});
    }

    static void handle_am_pm(std::tm& t, setter_state& st)
    {
        return st.handle_am_pm(t.tm_hour);
    }

    static void handle_short_year_and_century(std::tm& t, setter_state& st)
    {
        st.handle_short_year_and_century(t.tm_year, 1900);
    }
};

template <>
struct datetime_setter<datetime_components> {
    template <typename Handler>
    static void set_subsec(Handler& h,
                           datetime_components& t,
                           setter_state& st,
                           double s)
    {
        assert(s >= 0.0 && s < 1.0);
        t.subsec = s;
        st.set_subsec(h);
    }
    template <typename Handler>
    static void set_sec(Handler& h,
                        datetime_components& t,
                        setter_state& st,
                        int s)
    {
        if (SCN_UNLIKELY(s < 0 || s > 60)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for seconds"});
        }
        t.sec = s;
        st.set_sec(h);
    }
    template <typename Handler>
    static void set_min(Handler& h,
                        datetime_components& t,
                        setter_state& st,
                        int m)
    {
        if (SCN_UNLIKELY(m < 0 || m > 59)) {
            return h.set_error({scan_error::invalid_scanned_value,
                                "Invalid value for minutes"});
        }
        t.min = m;
        st.set_min(h);
    }
    template <typename Handler>
    static void set_hour24(Handler& hdl,
                           datetime_components& t,
                           setter_state& st,
                           int h)
    {
        if (SCN_UNLIKELY(h < 0 || h > 23)) {
            return hdl.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for hours"});
        }
        t.hour = h;
        st.set_hour24(hdl);
    }
    template <typename Handler>
    static void set_hour12(Handler& hdl,
                           datetime_components& t,
                           setter_state& st,
                           int h)
    {
        if (SCN_UNLIKELY(h < 1 || h > 12)) {
            return hdl.set_error({scan_error::invalid_scanned_value,
                                  "Invalid value for hours (12-hour clock)"});
        }
        t.hour = h;
        st.set_hour12(hdl);
    }
    template <typename Handler>
    static void set_mday(Handler& h,
                         datetime_components& t,
                         setter_state& st,
                         int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t.mday = d;
        st.set_mday(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h,
                        datetime_components& t,
                        setter_state& st,
                        int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 12)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mon"});
        }
        t.mon = month{static_cast<unsigned>(m)};
        st.set_mon(h);
    }
    template <typename Handler>
    static void set_full_year(Handler& h,
                              datetime_components& t,
                              setter_state& st,
                              int y)
    {
        t.year = y;
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h,
                            datetime_components& t,
                            setter_state& st,
                            int c)
    {
        if (!t.year) {
            t.year = c * 100;
        }
        else {
            t.year = *t.year + c * 100;
        }
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h,
                               datetime_components& t,
                               setter_state& st,
                               int y)
    {
        if (!t.year) {
            t.year = y;
        }
        else {
            t.year = *t.year + y;
        }
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_wday(Handler& h,
                         datetime_components& t,
                         setter_state& st,
                         int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for wday"});
        }
        t.wday = weekday{static_cast<unsigned>(d)};
        st.set_wday(h);
    }
    template <typename Handler>
    static void set_yday(Handler& h,
                         datetime_components& t,
                         setter_state& st,
                         int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for yday"});
        }
        t.yday = d;
        st.set_yday(h);
    }

    template <typename Handler>
    static void set_tz_offset(Handler& h,
                              datetime_components& t,
                              setter_state& st,
                              std::chrono::minutes o)
    {
        t.tz_offset = o;
        return st.set_tzoff(h);
    }

    template <typename Handler>
    static void set_tz_name(Handler& h,
                            datetime_components& t,
                            setter_state& st,
                            std::string n)
    {
        t.tz_name = std::move(n);
        return st.set_tzname(h);
    }

    static void handle_am_pm(datetime_components& t, setter_state& st)
    {
        assert(t.hour);
        st.handle_am_pm(*t.hour);
    }

    static void handle_short_year_and_century(datetime_components& t,
                                              setter_state& st)
    {
        assert(t.year);
        st.handle_short_year_and_century(*t.year, 0);
    }
};

template <>
struct datetime_setter<tm_with_tz> : datetime_setter<std::tm> {
    template <typename Handler>
    static void set_tz_offset(Handler& h,
                              tm_with_tz& t,
                              setter_state& st,
                              std::chrono::minutes o)
    {
        if constexpr (mp_valid<has_tm_gmtoff_predicate, std::tm>::value) {
            t.tz_offset = o;
            return datetime_setter<std::tm>::set_tz_offset(h, t, st, o);
        }
        else {
            t.tz_offset = o;
            return st.set_tzoff(h);
        }
    }

    template <typename Handler>
    static void set_tz_name(Handler& h,
                            tm_with_tz& t,
                            setter_state& st,
                            std::string n)
    {
        t.tz_name = std::move(n);
        return st.set_tzname(h);
    }
};

template <>
struct datetime_setter<weekday> : unreachable_datetime_setter<weekday> {
    template <typename Handler>
    static void set_wday(Handler& h, weekday& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 0 || d > 6)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for wday"});
        }
        t = weekday{static_cast<unsigned>(d)};
        st.set_wday(h);
    }
};

template <>
struct datetime_setter<day> : unreachable_datetime_setter<day> {
    template <typename Handler>
    static void set_mday(Handler& h, day& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t = day{static_cast<unsigned>(d)};
        st.set_mday(h);
    }
};

template <>
struct datetime_setter<month> : unreachable_datetime_setter<month> {
    template <typename Handler>
    static void set_mon(Handler& h, month& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = month{static_cast<unsigned>(m)};
        st.set_mon(h);
    }
};

template <>
struct datetime_setter<year> : unreachable_datetime_setter<year> {
    template <typename Handler>
    static void set_full_year(Handler& h, year& t, setter_state& st, int y)
    {
        t = year{static_cast<int>(y)};
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h, year& t, setter_state& st, int c)
    {
        t = year{static_cast<int>(t) + c * 100};
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h, year& t, setter_state& st, int y)
    {
        t = year{static_cast<int>(t) + y};
        st.set_short_year(h);
    }
};

template <>
struct datetime_setter<month_day> : unreachable_datetime_setter<month_day> {
    template <typename Handler>
    static void set_mon(Handler& h, month_day& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = month_day{month{static_cast<unsigned>(m)}, t.day()};
        st.set_mon(h);
    }

    template <typename Handler>
    static void set_mday(Handler& h, month_day& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t = month_day{t.month(), day{static_cast<unsigned>(d)}};
        st.set_mday(h);
    }
};

template <>
struct datetime_setter<year_month> : unreachable_datetime_setter<year_month> {
    template <typename Handler>
    static void set_full_year(Handler& h,
                              year_month& t,
                              setter_state& st,
                              int y)
    {
        t = year_month{year{static_cast<int>(y)}, t.month()};
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h, year_month& t, setter_state& st, int c)
    {
        t = year_month{year{static_cast<int>(t.year()) + c * 100}, t.month()};
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h,
                               year_month& t,
                               setter_state& st,
                               int y)
    {
        t = year_month{year{static_cast<int>(t.year()) + y}, t.month()};
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h, year_month& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = year_month{t.year(), month{static_cast<unsigned>(m)}};
        st.set_mon(h);
    }
};

template <>
struct datetime_setter<year_month_day>
    : unreachable_datetime_setter<year_month_day> {
    template <typename Handler>
    static void set_full_year(Handler& h,
                              year_month_day& t,
                              setter_state& st,
                              int y)
    {
        t = year_month_day{year{static_cast<int>(y)}, t.month(), t.day()};
        st.set_full_year(h);
    }
    template <typename Handler>
    static void set_century(Handler& h,
                            year_month_day& t,
                            setter_state& st,
                            int c)
    {
        t = year_month_day{year{static_cast<int>(t.year()) + c * 100},
                           t.month(), t.day()};
        st.set_century(h);
    }
    template <typename Handler>
    static void set_short_year(Handler& h,
                               year_month_day& t,
                               setter_state& st,
                               int y)
    {
        t = year_month_day{year{static_cast<int>(t.year()) + y}, t.month(),
                           t.day()};
        st.set_short_year(h);
    }
    template <typename Handler>
    static void set_mon(Handler& h, year_month_day& t, setter_state& st, int m)
    {
        if (SCN_UNLIKELY(m < 1 || m > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for month"});
        }
        t = year_month_day{t.year(), month{static_cast<unsigned>(m)}, t.day()};
        st.set_mon(h);
    }
    template <typename Handler>
    static void set_mday(Handler& h, year_month_day& t, setter_state& st, int d)
    {
        if (SCN_UNLIKELY(d < 1 || d > 31)) {
            return h.set_error(
                {scan_error::invalid_scanned_value, "Invalid value for mday"});
        }
        t = year_month_day{t.year(), t.month(), day{static_cast<unsigned>(d)}};
        st.set_mday(h);
    }
};

template <typename T, typename Range, typename CharT>
class tm_reader {
public:
    using type = T;
    using setter = datetime_setter<T>;
    using iterator = ranges::iterator_t<Range>;

    tm_reader(Range r, T& t, locale_ref loc)
        : m_range(SCN_MOVE(r)),
          m_begin(ranges::begin(m_range)),
          m_tm(t),
          m_loc(loc)
    {
    }

    void on_text(const CharT* beg, const CharT* end)
    {
        while (beg != end) {
            if (m_begin == ranges::end(m_range)) {
                return set_error({scan_error::end_of_input, "EOF"});
            }
            if (*beg != *m_begin) {
                return on_error("Invalid literal character");
            }
            ++beg;
            ++m_begin;
        }
    }
    void on_whitespace()
    {
        if (auto res = internal_skip_classic_whitespace(
                ranges::subrange{m_begin, ranges::end(m_range)}, true);
            res) {
            m_begin = SCN_MOVE(*res);
        }
        else {
            set_error(res.error());
        }
    }

    void on_localized()
    {
        m_st.localized = true;
    }

    void on_full_year(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%EY", L"%EY")) {
                setter::set_full_year(*this, m_tm, m_st, t->tm_year + 1900);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int yr = read_classic_unsigned_integer(4, 4);
        setter::set_full_year(*this, m_tm, m_st, yr);
    }
    void on_short_year(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Ey", L"%Ey")) {
                setter::set_short_year(*this, m_tm, m_st,
                                       (t->tm_year + 1900) % 100);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int yr = read_classic_unsigned_integer(2, 2);
        setter::set_short_year(*this, m_tm, m_st, yr);
    }
    void on_century(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%EC", L"%EC")) {
                setter::set_century(*this, m_tm, m_st,
                                    (t->tm_year + 1900) / 100);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int c = read_classic_unsigned_integer(2, 2);
        setter::set_century(*this, m_tm, m_st, c);
    }
    void on_iso_week_based_year()
    {
        unimplemented();
    }
    void on_iso_week_based_short_year()
    {
        unimplemented();
    }
    void on_loc_offset_year()
    {
        unimplemented();
    }

    void on_month_name()
    {
#if !SCN_DISABLE_FAST_FLOAT
        if (m_st.localized) {
            if (auto t = read_localized("%b", L"%b")) {
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
            }
            return;
        }
#endif

        std::array<std::pair<std::string_view, int>, 12> long_mapping = {{
            {"January", 1},
            {"February", 2},
            {"March", 3},
            {"April", 4},
            {"May", 5},
            {"June", 6},
            {"July", 7},
            {"August", 8},
            {"September", 9},
            {"October", 10},
            {"November", 11},
            {"December", 12},
        }};
        if (auto m = try_one_of_str_nocase(long_mapping)) {
            return setter::set_mon(*this, m_tm, m_st, *m);
        }
        std::array<std::pair<std::string_view, int>, 11> short_mapping = {{
            {"Jan", 1},
            {"Feb", 2},
            {"Mar", 3},
            {"Apr", 4},
            {"Jun", 6},
            {"Jul", 7},
            {"Aug", 8},
            {"Sep", 9},
            {"Oct", 10},
            {"Nov", 11},
            {"Dec", 12},
        }};
        if (auto m = try_one_of_str_nocase(short_mapping)) {
            return setter::set_mon(*this, m_tm, m_st, *m);
        }
        set_error({scan_error::invalid_scanned_value, "Invalid month name"});
    }
    void on_dec_month(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Om", L"%Om")) {
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int mon = read_classic_unsigned_integer(1, 2);
        setter::set_mon(*this, m_tm, m_st, mon);
    }

    void on_dec0_week_of_year(numeric_system = numeric_system::standard)
    {
        unimplemented();
    }
    void on_dec1_week_of_year()
    {
        unimplemented();
    }
    void on_iso_week_of_year()
    {
        unimplemented();
    }
    void on_day_of_year()
    {
        int yday = read_classic_unsigned_integer(1, 3);
        setter::set_yday(*this, m_tm, m_st, yday - 1);
    }
    void on_day_of_month(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Od", L"%Od")) {
                setter::set_mday(*this, m_tm, m_st, t->tm_mday);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int mday = read_classic_unsigned_integer(1, 2);
        setter::set_mday(*this, m_tm, m_st, mday);
    }

    void on_weekday_name()
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            if (auto t = read_localized("%a", L"%a")) {
                setter::set_wday(*this, m_tm, m_st, t->tm_wday);
            }
            return;
        }
#endif

        std::array<std::pair<std::string_view, int>, 7> long_mapping = {{
            {"Sunday", 0},
            {"Monday", 1},
            {"Tuesday", 2},
            {"Wednesday", 3},
            {"Thursday", 4},
            {"Friday", 5},
            {"Saturday", 6},
        }};
        if (auto d = try_one_of_str_nocase(long_mapping)) {
            return setter::set_wday(*this, m_tm, m_st, *d);
        }
        std::array<std::pair<std::string_view, int>, 7> short_mapping = {{
            {"Sun", 0},
            {"Mon", 1},
            {"Tue", 2},
            {"Wed", 3},
            {"Thu", 4},
            {"Fri", 5},
            {"Sat", 6},
        }};
        if (auto d = try_one_of_str_nocase(short_mapping)) {
            return setter::set_wday(*this, m_tm, m_st, *d);
        }
        return set_error(
            {scan_error::invalid_scanned_value, "Invalid weekday name"});
    }
    void on_dec0_weekday(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Ow", L"%Ow")) {
                setter::set_wday(*this, m_tm, m_st, t->tm_wday);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int wday = read_classic_unsigned_integer(1, 1);
        setter::set_wday(*this, m_tm, m_st, wday);
    }
    void on_dec1_weekday(numeric_system sys = numeric_system::standard)
    {
        auto adjust_wday = [](int d) {
            if (d == 0) {
                return 6;
            }
            return d - 1;
        };

#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%Ow", L"%Ow")) {
                setter::set_wday(*this, m_tm, m_st, adjust_wday(t->tm_wday));
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int wday = read_classic_unsigned_integer(1, 1);
        setter::set_wday(*this, m_tm, m_st, adjust_wday(wday));
    }

    void on_24_hour(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OH", L"%OH")) {
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int hr = read_classic_unsigned_integer(1, 2);
        setter::set_hour24(*this, m_tm, m_st, hr);
    }
    void on_12_hour(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OI", L"%OI")) {
                setter::set_hour12(*this, m_tm, m_st, t->tm_hour);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int hr = read_classic_unsigned_integer(1, 2);
        setter::set_hour12(*this, m_tm, m_st, hr);
    }
    void on_minute(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OM", L"%OM")) {
                setter::set_min(*this, m_tm, m_st, t->tm_min);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int min = read_classic_unsigned_integer(1, 2);
        setter::set_min(*this, m_tm, m_st, min);
    }
    void on_second(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized && sys != numeric_system::standard) {
            if (auto t = read_localized("%OS", L"%OS")) {
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        int sec = read_classic_unsigned_integer(1, 2);
        setter::set_sec(*this, m_tm, m_st, sec);
    }
    void on_subsecond(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_TYPE_STRING && !SCN_DISABLE_TYPE_DOUBLE
        int whole = read_classic_unsigned_integer(1, 2);
        setter::set_sec(*this, m_tm, m_st, whole);

        if (!m_st.localized || sys == numeric_system::standard) {
            if (!consume_ch('.')) {
                return set_error({scan_error::invalid_scanned_value,
                                  "Expected `.` in subsecond value"});
            }
        }
#if !SCN_DISABLE_LOCALE
        else {
            auto& state = get_localized_read_state();
            CharT sep = state.numpunct_facet->decimal_point();
            if (!consume_code_unit(sep)) {
                return set_error(
                    {scan_error::invalid_scanned_value,
                     "Expected decimal separator in subsecond value"});
            }
        }
#endif  // !SCN_DISABLE_LOCALE

        auto str_res = scan<std::string>(
            ranges::subrange{m_begin, m_range.end()}, []() -> decltype(auto) {
                if constexpr (std::is_same_v<CharT, char>) {
                    return "{:[0-9]}";
                }
                else {
                    return L"{:[0-9]}";
                }
            }());
        if (!str_res) {
            return set_error(str_res.error());
        }
        if (str_res->begin() == m_begin) {
            return set_error({scan_error::invalid_scanned_value,
                              "Expected digits after `.` in subsecond value"});
        }
        m_begin = str_res->begin();

        auto subsecond_str = std::move(str_res->value());
        subsecond_str.insert(0, "0.");
        auto dbl_res = scan<double>(subsecond_str, "{}");
        if (!dbl_res) {
            return set_error(dbl_res.error());
        }

        setter::set_subsec(*this, m_tm, m_st, dbl_res->value());

#else  // !SCN_DISABLE_TYPE_STRING && !SCN_DISABLE_TYPE_DOUBLE
        SCN_UNUSED(sys);
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
#endif
    }

    void on_tz_offset(numeric_system sys = numeric_system::standard)
    {
        // [+|-]
        if (m_begin == ranges::end(m_range)) {
            return set_error({scan_error::end_of_input, "EOF"});
        }
        bool is_minus = false;
        {
            const auto sign_ch = *m_begin;
            if (sign_ch == CharT{'+'}) {
                ++m_begin;
            }
            else if (sign_ch == CharT{'-'}) {
                is_minus = true;
                ++m_begin;
            }
        }
        if (m_begin == ranges::end(m_range)) {
            return set_error({scan_error::end_of_input, "EOF"});
        }

        int hour = 0;
        int minute = 0;
        if (sys == numeric_system::standard) {
            // hh[[:]mm]
            hour = read_classic_unsigned_integer(2, 2);
            if (m_begin != ranges::end(m_range)) {
                auto it_before_colon = m_begin;
                if (*m_begin == CharT{':'}) {
                    ++m_begin;
                }
                if (m_begin == ranges::end(m_range) || *m_begin < CharT{'0'} ||
                    *m_begin > CharT{'9'}) {
                    m_begin = it_before_colon;
                }
                else {
                    minute = read_classic_unsigned_integer(2, 2);
                }
            }
        }
        else {
            // h[h][:mm]
            hour = read_classic_unsigned_integer(1, 2);
            if (m_begin != ranges::end(m_range)) {
                auto it_before_colon = m_begin;
                if (*m_begin == CharT{':'}) {
                    ++m_begin;
                    if (m_begin == ranges::end(m_range) ||
                        *m_begin < CharT{'0'} || *m_begin > CharT{'9'}) {
                        m_begin = it_before_colon;
                    }
                    else {
                        minute = read_classic_unsigned_integer(2, 2);
                    }
                }
            }
        }

        setter::set_tz_offset(
            *this, m_tm, m_st,
            std::chrono::minutes{(is_minus ? -1 : 1) * (hour * 60 + minute)});
    }
    void on_tz_name()
    {
        auto res = scan<std::string>(
            ranges::subrange{m_begin, m_range.end()}, []() -> decltype(auto) {
                if constexpr (std::is_same_v<CharT, char>) {
                    return "{:[a-zA-Z0-9-+_/]}";
                }
                else {
                    return L"{:[a-zA-Z0-9-+_/]}";
                }
            }());
        if (!res) {
            set_error(res.error());
        }
        else {
            setter::set_tz_name(*this, m_tm, m_st, std::move(res->value()));
        }
        m_begin = res->begin();
    }

    void on_loc_datetime(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            const auto t = [&]() {
                if (sys != numeric_system::standard) {
                    return read_localized("%Ec", L"%Ec");
                }
                return read_localized("%c", L"%c");
            }();

            if (t) {
                setter::set_full_year(*this, m_tm, m_st, t->tm_year + 1900);
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
                setter::set_mday(*this, m_tm, m_st, t->tm_mday);
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
                setter::set_min(*this, m_tm, m_st, t->tm_min);
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif
        // %c == %a %b %d %H:%M:%S %Y
        constexpr CharT colon = ':';
        on_weekday_name();
        on_whitespace();
        on_month_name();
        on_whitespace();
        on_day_of_month();
        on_whitespace();
        on_24_hour();
        on_text(&colon, &colon + 1);
        on_minute();
        on_text(&colon, &colon + 1);
        on_second();
        on_whitespace();
        on_full_year();
    }
    void on_loc_date(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            const auto t = [&]() {
                if (sys != numeric_system::standard) {
                    return read_localized("%Ex", L"%Ex");
                }
                return read_localized("%x", L"%x");
            }();

            if (t) {
                setter::set_full_year(*this, m_tm, m_st, t->tm_year + 1900);
                setter::set_mon(*this, m_tm, m_st, t->tm_mon + 1);
                setter::set_mday(*this, m_tm, m_st, t->tm_mday);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif

        // %x == %m/%d/%Y
        constexpr CharT slash = '/';
        on_dec_month();
        on_text(&slash, &slash + 1);
        on_day_of_month();
        on_text(&slash, &slash + 1);
        on_full_year();
    }
    void on_loc_time(numeric_system sys = numeric_system::standard)
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            const auto t = [&]() {
                if (sys != numeric_system::standard) {
                    return read_localized("%EX", L"%EX");
                }
                return read_localized("%X", L"%X");
            }();

            if (t) {
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
                setter::set_min(*this, m_tm, m_st, t->tm_min);
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#else
        SCN_UNUSED(sys);
#endif
        // %X == %H:%M:%S
        on_iso_time();
    }
    void on_us_date()
    {
        // %m/%d/%y
        constexpr CharT slash = '/';
        on_dec_month();
        on_text(&slash, &slash + 1);
        on_day_of_month();
        on_text(&slash, &slash + 1);
        on_short_year();
    }
    void on_iso_date()
    {
        // %Y-%m-%d
        constexpr CharT dash = '-';
        on_full_year();
        on_text(&dash, &dash + 1);
        on_dec_month();
        on_text(&dash, &dash + 1);
        on_day_of_month();
    }
    void on_loc_12_hour_time()
    {
#if !SCN_DISABLE_LOCALE
        if (m_st.localized) {
            if (auto t = read_localized("%r", L"%r")) {
                setter::set_hour24(*this, m_tm, m_st, t->tm_hour);
                setter::set_min(*this, m_tm, m_st, t->tm_min);
                setter::set_sec(*this, m_tm, m_st, t->tm_sec);
            }
            return;
        }
#endif
        // %r == %I:%M:%S %p
        constexpr CharT colon = ':';
        on_12_hour();
        on_text(&colon, &colon + 1);
        on_minute();
        on_text(&colon, &colon + 1);
        on_second();
        on_whitespace();
        on_am_pm();
    }
    void on_24_hour_time()
    {
        // %H:%M
        constexpr CharT colon = ':';
        on_24_hour();
        on_text(&colon, &colon + 1);
        on_minute();
    }
    void on_iso_time()
    {
        // %H:%M:%S
        constexpr CharT colon = ':';
        on_24_hour();
        on_text(&colon, &colon + 1);
        on_minute();
        on_text(&colon, &colon + 1);
        on_second();
    }
    void on_am_pm()
    {
        std::array<std::pair<std::string_view, bool>, 4> mapping = {{
            {"am", false},
            {"a.m.", false},
            {"pm", true},
            {"p.m.", true},
        }};
        if (auto b = try_one_of_str_nocase(mapping)) {
            m_st.is_pm = *b;
            return m_st.set_am_pm(*this);
        }
        set_error(
            {scan_error::invalid_scanned_value, "Invalid am/pm specifier"});
    }

    void on_epoch_offset()
    {
        unimplemented();
    }
    void on_duration_tick_count()
    {
        unimplemented();
    }
    void on_duration_suffix()
    {
        unimplemented();
    }

    void verify()
    {
        m_st.verify(*this);
        if (m_st.am_pm_set && m_st.hour12_set) {
            setter::handle_am_pm(m_tm, m_st);
        }
        if (!m_st.full_year_set && (m_st.short_year_set || m_st.century_set)) {
            setter::handle_short_year_and_century(m_tm, m_st);
        }
    }

    scan_expected<void> get_error() const
    {
        return m_error;
    }

    void on_error(const char* msg)
    {
        set_error({scan_error::invalid_format_string, msg});
    }

    void set_error(scan_error e)
    {
        if (m_error.has_value()) {
            m_error = unexpected(e);
        }
    }

    iterator get_iterator() const
    {
        return m_begin;
    }

private:
    void unimplemented()
    {
        on_error("Unimplemented");
    }

    int read_classic_unsigned_integer(int min_digits, int max_digits)
    {
        int digits_read = 0;
        int accumulator = 0;
        while (m_begin != m_range.end()) {
            const auto ch = *m_begin;
            if (ch < CharT{'0'} || ch > CharT{'9'}) {
                break;
            }
            ++m_begin;
            ++digits_read;
            accumulator = accumulator * 10 + static_cast<int>(ch - CharT{'0'});
            if (digits_read >= max_digits) {
                break;
            }
        }
        if (digits_read < min_digits) {
            set_error(scan_error{scan_error::invalid_scanned_value,
                                 "Too few integer digits"});
            return -1;
        }
        return accumulator;
    }

    bool consume_ch(char ch)
    {
        if (m_begin == m_range.end()) {
            return false;
        }
        if (*m_begin == static_cast<CharT>(ch)) {
            ++m_begin;
            return true;
        }
        return false;
    }

    bool consume_code_unit(CharT ch)
    {
        if (m_begin == m_range.end()) {
            return false;
        }
        if (*m_begin == ch) {
            ++m_begin;
            return true;
        }
        return false;
    }

    template <typename OptT, std::size_t N>
    std::optional<OptT> try_one_of_str_nocase(
        std::array<std::pair<std::string_view, OptT>, N>& options)
    {
        auto start_it = m_begin;
        std::size_t options_available = N;
        std::size_t chars_consumed = 0;
        while (options_available >= 1 &&
               options.front().first.size() > chars_consumed) {
            std::size_t i = 0;
            if (m_begin == m_range.end()) {
                options_available = 0;
                break;
            }
            const auto ch = *m_begin;
            ++m_begin;
            while (i < options_available) {
                const auto cmp = static_cast<unsigned>(
                    ch ^ options[i].first[chars_consumed]);
                if (options[i].first.size() <= chars_consumed ||
                    (cmp != 0 && cmp != 32)) {
                    std::rotate(
                        options.begin() + static_cast<std::ptrdiff_t>(i),
                        options.begin() + static_cast<std::ptrdiff_t>(i) + 1,
                        options.end());
                    --options_available;
                    continue;
                }
                ++i;
            }
            ++chars_consumed;
        }
        if (options_available != 1) {
            m_begin = start_it;
            return std::nullopt;
        }
        return options.front().second;
    }

#if !SCN_DISABLE_LOCALE
    struct localized_read_state {
        using time_facet_type = std::time_get<CharT, iterator>;
        using numpunct_facet_type = std::numpunct<CharT>;

        std::locale locale;
        const time_facet_type* time_facet;
        const numpunct_facet_type* numpunct_facet;
        std::basic_stringstream<CharT> dummy_stream;
    };

    localized_read_state& get_localized_read_state()
    {
        if (!m_loc_state) {
            auto loc = [&]() {
                if (m_st.localized) {
                    return m_loc.get<std::locale>();
                }
                return std::locale::classic();
            }();
            if (!std::has_facet<typename localized_read_state::time_facet_type>(
                    loc)) {
                loc = std::locale(
                    loc, new typename localized_read_state::time_facet_type{});
            }
            if (!std::has_facet<
                    typename localized_read_state::numpunct_facet_type>(loc)) {
                loc = std::locale(
                    loc,
                    new typename localized_read_state::numpunct_facet_type{});
            }

            m_loc_state = localized_read_state{
                SCN_MOVE(loc),
                &std::use_facet<typename localized_read_state::time_facet_type>(
                    loc),
                &std::use_facet<
                    typename localized_read_state::numpunct_facet_type>(loc),
                std::basic_stringstream<CharT>{}};

            m_loc_state->dummy_stream.imbue(m_loc_state->locale);
        }

        return *m_loc_state;
    }

    std::optional<std::tm> do_read_localized(std::basic_string_view<CharT> fmt)
    {
        const auto& facet = *get_localized_read_state().time_facet;
        std::ios_base::iostate err{std::ios_base::goodbit};
        std::tm tm{};
        auto [begin, end] = [&]() {
            if constexpr (std::is_same_v<
                              iterator,
                              remove_cvref_t<decltype(m_range.end())>>) {
                return std::pair{m_range.begin(), m_range.end()};
            }
            else {
                using common_iterator_type =
                    typename basic_scan_buffer<CharT>::common_forward_iterator;
                return std::pair{common_iterator_type{m_range.begin()},
                                 common_iterator_type{m_range.end()}};
            }
        }();
        auto iter = facet.get(begin, end, m_loc_state->dummy_stream, err, &tm,
                              fmt.data(), fmt.data() + fmt.size());
        if ((err & std::ios_base::failbit) != 0) {
            set_error({scan_error::invalid_scanned_value,
                       "Failed to scan localized datetime"});
            return std::nullopt;
        }
        m_begin = SCN_MOVE(iter);
        return tm;
    }

    std::optional<std::tm> read_localized(std::string_view fmt,
                                          std::wstring_view wfmt)
    {
        SCN_UNUSED(fmt);
        SCN_UNUSED(wfmt);
        if constexpr (std::is_same_v<CharT, char>) {
            return do_read_localized(fmt);
        }
        else {
            return do_read_localized(wfmt);
        }
    }

    std::optional<localized_read_state> m_loc_state;
#else
    std::optional<std::tm> read_localized(std::string_view, std::wstring_view)
    {
        set_error(
            {scan_error::invalid_format_string,
             "Failed to scan localized datetime with SCN_DISABLE_LOCALE on"});
        return std::nullopt;
    }
#endif  // !SCN_DISABLE_LOCALE

    Range m_range;
    iterator m_begin;
    T& m_tm;
    setter_state m_st{};
    locale_ref m_loc{};
    scan_expected<void> m_error{};
};

template <typename CharT, typename T, typename Context>
auto chrono_scan_inner_impl(std::basic_string_view<CharT> fmt,
                            T& t,
                            Context& ctx)
    -> scan_expected<typename Context::iterator>
{
    {
        SCN_TRY(it,
                detail::internal_skip_classic_whitespace(ctx.range(), false));
        ctx.advance_to(SCN_MOVE(it));
    }

    auto r = detail::tm_reader<T, typename Context::range_type, CharT>(
        ctx.range(), t, ctx.locale());
    detail::parse_chrono_format_specs(fmt.data(), fmt.data() + fmt.size(), r);
    if (auto e = r.get_error(); SCN_UNLIKELY(!e)) {
        return unexpected(e.error());
    }
    return r.get_iterator();
}

template <typename CharT, typename T, typename Context>
auto chrono_scan_impl(std::basic_string_view<CharT> fmt_str, T& t, Context& ctx)
    -> scan_expected<typename Context::iterator>
{
    if (ctx.begin().stores_parent()) {
        // ctx.begin() stores parent (buffer) -> not contiguous
        return chrono_scan_inner_impl(fmt_str, t, ctx);
    }

    auto contiguous_ctx = impl::basic_contiguous_scan_context<CharT>(
        ctx.begin().contiguous_segment(), ctx.args(), ctx.locale());
    auto begin = contiguous_ctx.begin();
    SCN_TRY(it, chrono_scan_inner_impl(fmt_str, t, contiguous_ctx));
    return ctx.begin().batch_advance(std::distance(begin, it));
}

template auto chrono_scan_impl(std::string_view, std::tm&, scan_context&)
    -> scan_expected<scan_context::iterator>;
template auto chrono_scan_impl(std::string_view, tm_with_tz&, scan_context&)
    -> scan_expected<scan_context::iterator>;
template auto chrono_scan_impl(std::string_view,
                               datetime_components&,
                               scan_context&)
    -> scan_expected<scan_context::iterator>;

template auto chrono_scan_impl(std::wstring_view, std::tm&, wscan_context&)
    -> scan_expected<wscan_context::iterator>;
template auto chrono_scan_impl(std::wstring_view, tm_with_tz&, wscan_context&)
    -> scan_expected<wscan_context::iterator>;
template auto chrono_scan_impl(std::wstring_view,
                               datetime_components&,
                               wscan_context&)
    -> scan_expected<wscan_context::iterator>;

}  // namespace detail

#endif  // !SCN_DISABLE_CHRONO

SCN_END_NAMESPACE
}  // namespace scn
