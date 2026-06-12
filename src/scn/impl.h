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

// Transitively includes <scn/scan.h>
#include <scn/regex.h>
#include <scn/xchar.h>

#include <algorithm>
#include <cfenv>
#include <clocale>
#include <cmath>
#include <cwchar>
#include <functional>
#include <memory>
#include <vector>

#if SCN_HAS_BITOPS
#include <bit>
#elif SCN_MSVC
#include <IntSafe.h>
#include <intrin.h>
#elif SCN_POSIX && !SCN_GCC_COMPAT

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wreserved-id-macro")
#define _XOPEN_SOURCE 700
SCN_CLANG_POP

#include <strings.h>
#endif

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
#include <regex>
#if SCN_REGEX_BOOST_USE_ICU
#error "Can't use the ICU with std::regex"
#endif
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
#include <boost/regex.hpp>
#if SCN_REGEX_BOOST_USE_ICU
#include <boost/regex/icu.hpp>
#endif
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
#include <re2/re2.h>
#endif

#if SCN_POSIX
#include <fcntl.h>
#endif

#if !SCN_DISABLE_FAST_FLOAT

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wold-style-cast")
SCN_GCC_IGNORE("-Wnoexcept")
SCN_GCC_IGNORE("-Wundef")
SCN_GCC_IGNORE("-Wsign-conversion")
SCN_GCC_IGNORE("-Wuseless-cast")

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wold-style-cast")
SCN_CLANG_IGNORE("-Wdeprecated")
SCN_CLANG_IGNORE("-Wcomma")
SCN_CLANG_IGNORE("-Wundef")
SCN_CLANG_IGNORE("-Wdocumentation-unknown-command")

#if SCN_CLANG >= SCN_COMPILER(8, 0, 0)
SCN_CLANG_IGNORE("-Wextra-semi-stmt")
#endif

#if SCN_CLANG >= SCN_COMPILER(13, 0, 0)
SCN_CLANG_IGNORE("-Wreserved-identifier")
#endif

#if SCN_CLANG >= SCN_COMPILER(16, 0, 0)
SCN_CLANG_IGNORE("-Wunsafe-buffer-usage")
#endif

#include <fast_float/fast_float.h>

SCN_CLANG_POP
SCN_GCC_POP

#endif  // !SCN_DISABLE_FAST_FLOAT

#if SCN_HAS_FLOAT_CHARCONV
#include <charconv>
#endif

namespace scn {
SCN_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////
// Private ranges stuff
/////////////////////////////////////////////////////////////////

namespace ranges {

template <typename R>
using const_iterator_t = iterator_t<std::add_const_t<R>>;

// Like std::ranges::distance, utilizing .position if available
namespace detail::distance_ {
struct fn {
private:
    template <typename I, typename S>
    static constexpr auto impl(I i, S s, priority_tag<1>)
        -> decltype(s.position() - i.position())
    {
        return s.position() - i.position();
    }

    template <typename I, typename S>
    static constexpr auto impl(I i, S s, priority_tag<0>)
        -> std::enable_if_t<sized_sentinel_for<S, I>, iter_difference_t<I>>
    {
        return s - i;
    }

    template <typename I, typename S>
    static constexpr auto impl(I i, S s, priority_tag<0>)
        -> std::enable_if_t<!sized_sentinel_for<S, I>, iter_difference_t<I>>
    {
        iter_difference_t<I> counter{0};
        while (i != s) {
            ++i;
            ++counter;
        }
        return counter;
    }

public:
    template <typename I, typename S>
    constexpr auto operator()(I first, S last) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            iter_difference_t<I>>
    {
        return fn::impl(std::move(first), std::move(last), priority_tag<0>{});
    }
};
}  // namespace detail::distance_

inline constexpr auto distance = detail::distance_::fn{};

namespace detail {
template <typename I, typename = void>
struct has_batch_advance : std::false_type {};
template <typename I>
struct has_batch_advance<I,
                         std::void_t<decltype(SCN_DECLVAL(I&).batch_advance(
                             SCN_DECLVAL(std::ptrdiff_t)))>> : std::true_type {
};
}  // namespace detail

// std::advance, utilizing .batch_advance if available
namespace detail::advance_ {
struct fn {
private:
    template <typename T>
    static constexpr T abs(T t)
    {
        if (t < T{0}) {
            return -t;
        }
        return t;
    }

    template <typename I>
    static constexpr auto impl(I& i, iter_difference_t<I> n, priority_tag<1>)
        -> std::enable_if_t<has_batch_advance<I>::value>
    {
        i.batch_advance(n);
    }

    template <typename I>
    static constexpr auto impl_i_n(I& i,
                                   iter_difference_t<I> n,
                                   priority_tag<0>)
        -> std::enable_if_t<random_access_iterator<I>>
    {
        i += n;
    }

    template <typename I>
    static constexpr auto impl_i_n(I& i,
                                   iter_difference_t<I> n,
                                   priority_tag<0>)
        -> std::enable_if_t<bidirectional_iterator<I> &&
                            !random_access_iterator<I>>
    {
        constexpr auto zero = iter_difference_t<I>{0};

        if (n > zero) {
            while (n-- > zero) {
                ++i;
            }
        }
        else {
            while (n++ < zero) {
                --i;
            }
        }
    }

    template <typename I>
    static constexpr auto impl_i_n(I& i,
                                   iter_difference_t<I> n,
                                   priority_tag<0>)
        -> std::enable_if_t<!bidirectional_iterator<I>>
    {
        while (n-- > iter_difference_t<I>{0}) {
            ++i;
        }
    }

    template <typename I, typename S>
    static constexpr auto impl_i_s(I& i, S bound, priority_tag<2>)
        -> std::enable_if_t<std::is_assignable_v<I&, S>>
    {
        i = std::move(bound);
    }

    template <typename I, typename S>
    static constexpr auto impl_i_s(I& i, S bound, priority_tag<1>)
        -> std::enable_if_t<sized_sentinel_for<S, I>>
    {
        fn::impl_i_n(i, bound - i);
    }

    template <typename I, typename S>
    static constexpr void impl_i_s(I& i, S bound, priority_tag<0>)
    {
        while (i != bound) {
            ++i;
        }
    }

    template <typename I, typename S>
    static constexpr auto impl_i_n_s(I& i, iter_difference_t<I> n, S bound)
        -> std::enable_if_t<sized_sentinel_for<S, I>, iter_difference_t<I>>
    {
        if (fn::abs(n) >= fn::abs(bound - i)) {
            auto dist = bound - i;
            fn::impl_i_s(i, bound, priority_tag<2>{});
            return dist;
        }
        fn::impl_i_n(i, n, priority_tag<1>{});
        return n;
    }

    template <typename I, typename S>
    static constexpr auto impl_i_n_s(I& i, iter_difference_t<I> n, S bound)
        -> std::enable_if_t<bidirectional_iterator<I> &&
                                !sized_sentinel_for<S, I>,
                            iter_difference_t<I>>
    {
        constexpr iter_difference_t<I> zero{0};
        iter_difference_t<I> counter{0};

        if (n < zero) {
            do {
                --i;
                --counter;  // Yes, really
            } while (++n < zero && i != bound);
        }
        else {
            while (n-- > zero && i != bound) {
                ++i;
                ++counter;
            }
        }

        return counter;
    }

    template <typename I, typename S>
    static constexpr auto impl_i_n_s(I& i, iter_difference_t<I> n, S bound)
        -> std::enable_if_t<!bidirectional_iterator<I> &&
                                !sized_sentinel_for<S, I>,
                            iter_difference_t<I>>
    {
        constexpr iter_difference_t<I> zero{0};
        iter_difference_t<I> counter{0};

        while (n-- > zero && i != bound) {
            ++i;
            ++counter;
        }

        return counter;
    }

public:
    template <typename I>
    constexpr auto operator()(I& i, iter_difference_t<I> n) const
        -> std::enable_if_t<input_or_output_iterator<I>>
    {
        fn::impl_i_n(i, n, detail::priority_tag<1>{});
    }

    template <typename I, typename S>
    constexpr auto operator()(I& i, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>>
    {
        fn::impl_i_s(i, bound, priority_tag<2>{});
    }

    template <typename I, typename S>
    constexpr auto operator()(I& i, iter_difference_t<I> n, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            iter_difference_t<I>>
    {
        return n - fn::impl_i_n_s(i, n, bound);
    }
};
}  // namespace detail::advance_

inline constexpr auto advance = detail::advance_::fn{};

namespace next_impl {
struct fn {
    template <typename I>
    constexpr auto operator()(I x) const
        -> std::enable_if_t<input_or_output_iterator<I>, I>
    {
        ++x;
        return x;
    }

    template <typename I>
    constexpr auto operator()(I x, iter_difference_t<I> n) const
        -> std::enable_if_t<input_or_output_iterator<I>, I>
    {
        ranges::advance(x, n);
        return x;
    }

    template <typename I, typename S>
    constexpr auto operator()(I x, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            I>
    {
        ranges::advance(x, bound);
        return x;
    }

    template <typename I, typename S>
    constexpr auto operator()(I x, iter_difference_t<I> n, S bound) const
        -> std::enable_if_t<input_or_output_iterator<I> && sentinel_for<S, I>,
                            I>
    {
        ranges::advance(x, n, bound);
        return x;
    }
};
}  // namespace next_impl

inline constexpr next_impl::fn next{};

// prev, for forward_iterators
namespace detail::prev_backtrack_ {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It it, It, priority_tag<2>)
        -> std::enable_if_t<bidirectional_iterator<It>, It>
    {
        --it;
        return it;
    }

    template <typename It>
    static constexpr auto impl(It it, It beg, priority_tag<1>)
        -> remove_cvref_t<decltype((void)beg.batch_advance(42), it)>
    {
        return beg.batch_advance(it.position() - 1);
    }

    template <typename It>
    static constexpr auto impl(It it, It beg, priority_tag<0>)
        -> std::enable_if_t<forward_iterator<It>, It>
    {
        SCN_EXPECT(it != beg);

        while (true) {
            auto tmp = beg;
            ++beg;
            if (beg == it) {
                return tmp;
            }
        }
    }

public:
    template <typename It>
    constexpr auto operator()(It it, It beg) const
        -> decltype(fn::impl(it, beg, priority_tag<2>{}))
    {
        return fn::impl(it, beg, priority_tag<2>{});
    }
};
}  // namespace detail::prev_backtrack_

inline constexpr auto prev_backtrack = detail::prev_backtrack_::fn{};

// operator<, for forward_iterators
namespace detail::less_backtrack_ {
struct fn {
private:
    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It, priority_tag<2>)
        -> decltype(static_cast<void>(lhs < rhs), true)
    {
        return lhs < rhs;
    }

    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It, priority_tag<1>)
        -> decltype(static_cast<void>(lhs.position() < rhs.position()), true)
    {
        return lhs.position() < rhs.position();
    }

    template <typename It>
    static constexpr auto impl(It lhs, It rhs, It beg, priority_tag<0>)
        -> std::enable_if_t<ranges::forward_iterator<It>, bool>
    {
        while (true) {
            if (beg == rhs) {
                return false;
            }
            if (beg == lhs) {
                return true;
            }
            ++beg;
        }
    }

public:
    template <typename It>
    constexpr auto operator()(It lhs, It rhs, It beg) const
        -> decltype(fn::impl(lhs, rhs, beg, priority_tag<2>{}))
    {
        return fn::impl(lhs, rhs, beg, priority_tag<2>{});
    }
};
}  // namespace detail::less_backtrack_

inline constexpr auto less_backtrack = detail::less_backtrack_::fn{};

}  // namespace ranges

/////////////////////////////////////////////////////////////////
// ASCII-only locale-free <cctype>
/////////////////////////////////////////////////////////////////

namespace impl {
inline constexpr std::array<bool, 256> is_ascii_space_lookup = {
    {false, false, false, false, false, false, false, false, false, true,
     true,  true,  true,  true,  false, false, false, false, false, false,
     false, false, false, false, false, false, false, false, false, false,
     false, false, true,  false, false, false, false, false, false, false,
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
     false, false, false, false, false, false, false, false, false, false,
     false, false, false, false, false, false, false, false, false, false,
     false, false, false, false, false, false}};

constexpr bool is_ascii_space(char ch) noexcept
{
    return is_ascii_space_lookup[static_cast<size_t>(
        static_cast<unsigned char>(ch))];
}

constexpr bool is_ascii_space(wchar_t ch) noexcept
{
    return ch == 0x20 || (ch >= 0x09 && ch <= 0x0d);
}

constexpr bool is_ascii_char(char ch) noexcept
{
    return static_cast<unsigned char>(ch) <= 127;
}

constexpr bool is_ascii_char(wchar_t ch) noexcept
{
#if WCHAR_MIN < 0
    return ch >= 0 && ch <= 127;
#else
    return ch <= 127;
#endif
}

constexpr bool is_ascii_char(char32_t cp) noexcept
{
    return cp <= 127;
}

/////////////////////////////////////////////////////////////////
// <bits>
/////////////////////////////////////////////////////////////////

template <typename T,
          std::enable_if_t<std::is_unsigned_v<T> &&
                           sizeof(T) <= sizeof(uint64_t)>* = nullptr>
int count_trailing_zeroes(T val)
{
    SCN_EXPECT(val != 0);

#if SCN_HAS_BITOPS
    // Simple case: has C++20 <bit>
    return std::countr_zero(val);

#elif SCN_GCC_COMPAT
    // Has GCC builtins for int, long, and long long
    if constexpr (std::is_same_v<T, long long>) {
        return __builtin_ctzll(val);
    }
    else if constexpr (std::is_same_v<T, long>) {
        return __builtin_ctzl(val);
    }
    else {
        return __builtin_ctz(static_cast<unsigned>(val));
    }

#elif SCN_MSVC && SCN_WINDOWS_64BIT
    // 64-bit MSVC, use BitScanForward(64)
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        DWORD ret{};
        _BitScanForward64(&ret, val);
        return static_cast<int>(ret);
    }
    else {
        DWORD ret{};
        _BitScanForward(&ret, static_cast<uint32_t>(val));
        return static_cast<int>(ret);
    }

#elif SCN_MSVC && !SCN_WINDOWS_64BIT
    // 32-bit MSVC, need custom behavior for 64-bit integers
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        DWORD ret{};
        if (_BitScanForward(&ret, static_cast<uint32_t>(val))) {
            return static_cast<int>(ret);
        }
        _BitScanForward(&ret, static_cast<uint32_t>(val >> 32));
        return static_cast<int>(ret);
    }
    else {
        DWORD ret{};
        _BitScanForward(&ret, static_cast<uint32_t>(val));
        return static_cast<int>(ret);
    }

#else
    // Polyfill with a custom loop
    int ret{};
    std::uint64_t mask{1};
    while ((val & mask) == 0u) {
        mask <<= 1ull;
        ++ret;
    }
    return ret;

#define SCN_HAS_NATIVE_BITS_IMPL 0

#endif
}

template <typename T>
auto count_trailing_zeroes(T val) -> decltype(val.high, val.low, int{})
{
    if (val.low == 0) {
        return count_trailing_zeroes(val.high) + sizeof(val.low) * 8;
    }
    return count_trailing_zeroes(val.low);
}

#if SCN_HAS_INT128
template <typename T, std::enable_if_t<std::is_same_v<T, uint128>>* = nullptr>
int count_trailing_zeroes(T val)
{
    const auto low = static_cast<uint64_t>(val);
    if (low == 0) {
        return count_trailing_zeroes(static_cast<uint64_t>(val >> 64u)) + 64;
    }
    return count_trailing_zeroes(low);
}
#endif

template <typename T,
          std::enable_if_t<std::is_unsigned_v<T> &&
                           sizeof(T) <= sizeof(uint64_t)>* = nullptr>
int count_leading_zeroes(T val)
{
    SCN_EXPECT(val != 0);

#if SCN_HAS_BITOPS
    // Simple case: has C++20 <bit>
    return std::countl_zero(val);

#elif SCN_GCC_COMPAT
    // Has GCC builtins for int, long, and long long
    if constexpr (std::is_same_v<T, long long>) {
        return __builtin_clzll(val);
    }
    else if constexpr (std::is_same_v<T, long>) {
        return __builtin_clzl(val);
    }
    else {
        return __builtin_clz(static_cast<unsigned>(val)) -
               static_cast<int>(sizeof(unsigned) - sizeof(T)) * 8;
    }

#elif SCN_MSVC && SCN_WINDOWS_64BIT
    // 64-bit MSVC, use BitScanReverse(64)
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        DWORD ret{};
        _BitScanReverse64(&ret, val);
        return static_cast<int>(ret);
    }
    else {
        DWORD ret{};
        _BitScanReverse(&ret, static_cast<uint32_t>(val));
        return static_cast<int>(ret) - (sizeof(unsigned) - sizeof(T)) * 8;
    }

#elif SCN_MSVC && !SCN_WINDOWS_64BIT
    // 32-bit MSVC, need custom behavior for 64-bit integers
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        DWORD ret{};
        if (_BitScanForward(&ret, static_cast<uint32_t>(val))) {
            return static_cast<int>(ret);
        }
        _BitScanForward(&ret, static_cast<uint32_t>(val >> 32));
        return static_cast<int>(ret);
    }
    else {
        DWORD ret{};
        _BitScanForward(&ret, static_cast<uint32_t>(val));
        return static_cast<int>(ret) - (sizeof(unsigned) - sizeof(T)) * 8;
    }

#else
    // Polyfill with a custom loop
    int ret{};
    std::uint64_t mask{1};
    while ((val & mask) == 0u) {
        mask <<= 1ull;
        ++ret;
    }
    return ret;

#endif
}

template <typename T>
auto count_leading_zeroes(T val) -> decltype(val.high, val.low, int{})
{
    if (val.high == 0) {
        return count_leading_zeroes(val.low) +
               static_cast<int>(sizeof(val.high)) * 8;
    }
    return count_leading_zeroes(val.high);
}

#if SCN_HAS_INT128
template <typename T, std::enable_if_t<std::is_same_v<T, uint128>>* = nullptr>
int count_leading_zeroes(T val)
{
    const auto high = static_cast<uint64_t>(val >> 64u);
    if (high == 0) {
        return count_leading_zeroes(static_cast<uint64_t>(val)) + 64;
    }
    return count_leading_zeroes(high);
}
#endif

#ifndef SCN_HAS_NATIVE_BITS_IMPL
#define SCN_HAS_NATIVE_BITS_IMPL 1
#endif

constexpr uint64_t has_zero_byte(uint64_t word)
{
    return (word - 0x0101010101010101ull) & ~word & 0x8080808080808080ull;
}

constexpr uint64_t has_byte_between(uint64_t word, uint8_t a, uint8_t b)
{
    const auto m = static_cast<uint64_t>(a) - 1,
               n = static_cast<uint64_t>(b) + 1;
    return (((~0ull / 255 * (127 + n) - (word & ~0ull / 255 * 127)) & ~word &
             ((word & ~0ull / 255 * 127) + ~0ull / 255 * (127 - m))) &
            (~0ull / 255 * 128));
}

constexpr uint64_t has_byte_greater(uint64_t word, uint8_t n)
{
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wsign-conversion")
    return ((word + ~0ull / 255 * (127 - n)) | word) & ~0ull / 255 * 128;
    SCN_GCC_POP
}

inline size_t get_index_of_first_nonmatching_byte(uint64_t word)
{
    word ^= 0x8080808080808080ull;
    if (word == 0) {
        return 8;
    }
    return static_cast<size_t>(count_trailing_zeroes(word)) / 8;
}

inline size_t get_index_of_first_matching_byte(uint64_t word, uint64_t pattern)
{
    constexpr auto mask = 0x7f7f7f7f7f7f7f7full;
    auto input = word ^ pattern;
    auto tmp = (input & mask) + mask;
    tmp = ~(tmp | input | mask);
    return static_cast<size_t>(count_trailing_zeroes(tmp)) / 8;
}

constexpr uint32_t log2_fast(uint32_t val)
{
    constexpr uint8_t lookup[] = {0,  9,  1,  10, 13, 21, 2,  29, 11, 14, 16,
                                  18, 22, 25, 3,  30, 8,  12, 20, 28, 15, 17,
                                  24, 7,  19, 27, 23, 6,  26, 5,  4,  31};

    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;

    return static_cast<uint32_t>(lookup[(val * 0x07c4acddu) >> 27]);
}

constexpr uint32_t log2_pow2_fast(uint32_t val)
{
    constexpr uint8_t lookup[] = {0,  1,  28, 2,  29, 14, 24, 3,  30, 22, 20,
                                  15, 25, 17, 4,  8,  31, 27, 13, 23, 21, 19,
                                  16, 7,  26, 12, 18, 6,  11, 5,  10, 9};

    return static_cast<uint32_t>(lookup[(val * 0x077cb531u) >> 27]);
}

constexpr uint64_t byteswap(uint64_t val)
{
    return (val & 0xFF00000000000000) >> 56 | (val & 0x00FF000000000000) >> 40 |
           (val & 0x0000FF0000000000) >> 24 | (val & 0x000000FF00000000) >> 8 |
           (val & 0x00000000FF000000) << 8 | (val & 0x0000000000FF0000) << 24 |
           (val & 0x000000000000FF00) << 40 | (val & 0x00000000000000FF) << 56;
}

/////////////////////////////////////////////////////////////////
// <function_ref>
/////////////////////////////////////////////////////////////////

namespace fnref_detail {
template <class T>
inline constexpr auto select_param_type = [] {
    if constexpr (std::is_trivially_copyable_v<T>) {
        return detail::type_identity<T>();
    }
    else {
        return std::add_rvalue_reference<T>();
    }
};

template <class T>
using param_t =
    typename std::invoke_result_t<decltype(select_param_type<T>)>::type;

template <typename Sig>
struct qual_fn_sig;

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...)> {
    using function = R(Args...);

    static constexpr bool is_noexcept = false;

    template <typename... T>
    static constexpr bool is_invocable_using =
        std::is_invocable_r_v<R, T..., Args...>;

    template <typename T>
    using cv = T;
};

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...) noexcept> {
    using function = R(Args...);

    static constexpr bool is_noexcept = true;

    template <typename... T>
    static constexpr bool is_invocable_using =
        std::is_nothrow_invocable_r_v<R, T..., Args...>;

    template <typename T>
    using cv = T;
};

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...) const> : qual_fn_sig<R(Args...)> {
    template <typename T>
    using cv = T const;
};

template <typename R, typename... Args>
struct qual_fn_sig<R(Args...) const noexcept>
    : qual_fn_sig<R(Args...) noexcept> {
    template <typename T>
    using cv = T const;
};

#if SCN_CLANG >= SCN_COMPILER(16, 0, 0)
SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wcast-function-type-strict")
#endif
struct base {
    union storage {
        constexpr storage() = default;

        template <typename T, std::enable_if_t<std::is_object_v<T>>* = nullptr>
        constexpr explicit storage(T* p) noexcept : m_p(p)
        {
        }

        template <typename T, std::enable_if_t<std::is_object_v<T>>* = nullptr>
        constexpr explicit storage(const T* p) noexcept : m_cp(p)
        {
        }

        template <typename F,
                  std::enable_if_t<std::is_function_v<F>>* = nullptr>
        constexpr explicit storage(F* f) noexcept
            : m_fp(reinterpret_cast<decltype(m_fp)>(f))
        {
        }

        void* m_p{nullptr};
        const void* m_cp;
        void (*m_fp)();
    };

    template <typename T>
    static constexpr auto get(storage s)
    {
        if constexpr (std::is_const_v<T>) {
            return static_cast<T*>(s.m_cp);
        }
        else if constexpr (std::is_object_v<T>) {
            return static_cast<T*>(s.m_p);
        }
        else {
            return reinterpret_cast<T*>(s.m_fp);
        }
    }
};
#if SCN_CLANG >= SCN_COMPILER(16, 0, 0)
SCN_CLANG_POP  // -Wcast-function-type-strict
#endif

}  // namespace fnref_detail

template <typename Sig,
          typename = typename fnref_detail::qual_fn_sig<Sig>::function>
class function_ref;

template <typename Sig, typename R, typename... Args>
class function_ref<Sig, R(Args...)> : fnref_detail::base {
    using signature = fnref_detail::qual_fn_sig<Sig>;

    template <typename T>
    using cv = typename signature::template cv<T>;
    template <typename T>
    using cvref = cv<T>&;
    static constexpr bool noex = signature::is_noexcept;

    template <typename... T>
    static constexpr bool is_invocable_using =
        signature::template is_invocable_using<T...>;

    using fwd_t = R(storage, fnref_detail::param_t<Args>...) noexcept(noex);

public:
    template <typename F,
              std::enable_if_t<std::is_function_v<F> &&
                               is_invocable_using<F>>* = nullptr>
    /*implicit*/ function_ref(F* f) noexcept
        : m_fptr([](storage fn,
                    fnref_detail::param_t<Args>... args) noexcept(noex) -> R {
              if constexpr (std::is_void_v<R>) {
                  get<F>(fn)(static_cast<decltype(args)>(args)...);
              }
              else {
                  return get<F>(fn)(static_cast<decltype(args)>(args)...);
              }
          }),
          m_storage(f)
    {
        SCN_EXPECT(f != nullptr);
    }

    template <typename F,
              typename T = std::remove_reference_t<F>,
              std::enable_if_t<detail::is_not_self<F, function_ref> &&
                               !std::is_member_pointer_v<T> &&
                               is_invocable_using<cvref<T>>>* = nullptr>
    /*implicit*/ constexpr function_ref(F&& f) noexcept
        : m_fptr([](storage fn,
                    fnref_detail::param_t<Args>... args) noexcept(noex) -> R {
              cvref<T> obj = *get<T>(fn);
              if constexpr (std::is_void_v<R>) {
                  obj(static_cast<decltype(args)>(args)...);
              }
              else {
                  return obj(static_cast<decltype(args)>(args)...);
              }
          }),
          m_storage(std::addressof(f))
    {
    }

    template <typename T,
              std::enable_if_t<detail::is_not_self<T, function_ref> &&
                               !std::is_pointer_v<T>>* = nullptr>
    function_ref& operator=(T) = delete;

    constexpr R operator()(Args... args) const noexcept(noex)
    {
        return m_fptr(m_storage, SCN_FWD(args)...);
    }

private:
    fwd_t* m_fptr{nullptr};
    storage m_storage;
};

template <typename F, std::enable_if_t<std::is_function_v<F>>* = nullptr>
function_ref(F*) -> function_ref<F>;
}  // namespace impl

/////////////////////////////////////////////////////////////////
// Internal error types
/////////////////////////////////////////////////////////////////

namespace impl {
enum class eof_error { good, eof };

inline constexpr bool operator!(eof_error e)
{
    return e != eof_error::good;
}

template <typename T>
struct eof_expected : public expected<T, eof_error> {
    using base = expected<T, eof_error>;
    using base::base;

    constexpr eof_expected(const base& other) : base(other) {}
    constexpr eof_expected(base&& other) : base(SCN_MOVE(other)) {}
};

inline constexpr auto make_eof_scan_error(eof_error err)
{
    SCN_EXPECT(err == eof_error::eof);
    return scan_error{scan_error::end_of_input, "EOF"};
}

struct SCN_TRIVIAL_ABI parse_error {
    enum code { good, eof, error };
    using code_t = code;

    constexpr parse_error() = default;
    constexpr parse_error(code c) : m_code(c)
    {
        SCN_UNLIKELY_ATTR SCN_UNUSED(m_code);
    }

    constexpr explicit operator bool() const
    {
        return m_code == good;
    }
    constexpr explicit operator code_t() const
    {
        return m_code;
    }

    friend constexpr bool operator==(parse_error a, parse_error b)
    {
        return a.m_code == b.m_code;
    }
    friend constexpr bool operator!=(parse_error a, parse_error b)
    {
        return !(a == b);
    }

private:
    code m_code{good};
};

template <typename T>
struct parse_expected : public expected<T, parse_error> {
    using base = expected<T, parse_error>;
    using base::base;

    constexpr parse_expected(const base& other) : base(other) {}
    constexpr parse_expected(base&& other) : base(SCN_MOVE(other)) {}
};

inline constexpr parse_error make_eof_parse_error(eof_error err)
{
    SCN_EXPECT(err == eof_error::eof);
    return parse_error::eof;
}

inline constexpr scan_expected<void> make_scan_error_from_parse_error(
    parse_error err,
    enum scan_error::code code,
    const char* msg)
{
    if (err == parse_error::good) {
        return {};
    }

    if (err == parse_error::eof) {
        return detail::unexpected_scan_error(scan_error::end_of_input, "EOF");
    }

    return detail::unexpected_scan_error(code, msg);
}

inline constexpr auto map_parse_error_to_scan_error(enum scan_error::code code,
                                                    const char* msg)
{
    return [code, msg](parse_error err) {
        assert(err != parse_error::good);
        return make_scan_error_from_parse_error(err, code, msg).error();
    };
}
}  // namespace impl

namespace detail {
template <typename T>
struct is_expected_impl<scn::impl::parse_expected<T>> : std::true_type {};
}  // namespace detail

/////////////////////////////////////////////////////////////////
// File buffers
/////////////////////////////////////////////////////////////////

namespace impl {

template <typename CharT, typename Putback>
SCN_NODISCARD std::ptrdiff_t buffer_sync_helper(
    std::ptrdiff_t position,
    std::basic_string_view<CharT>& current_view,
    std::basic_string<CharT>& putback_buffer,
    Putback&& putback)
{
    const auto total_chars_avail = static_cast<std::ptrdiff_t>(
        current_view.size() + putback_buffer.size());
    SCN_EXPECT(total_chars_avail >= position);
    if (total_chars_avail == position) {
        return position;
    }

    const auto n_to_put_back = total_chars_avail - position;
    std::ptrdiff_t n_put_back = 0;

    {
        const auto n_to_put_back_from_current_view =
            std::min(current_view.size(),
                     static_cast<std::size_t>(total_chars_avail - position));
        const auto current_view_segment = current_view.substr(
            current_view.size() -
            static_cast<std::size_t>(n_to_put_back_from_current_view));
        for (auto it = current_view_segment.rbegin();
             it != current_view_segment.rend(); ++it, (void)++n_put_back) {
            if (!putback(*it)) {
                current_view = current_view.substr(
                    0,
                    current_view.size() - static_cast<std::size_t>(n_put_back));
                return total_chars_avail - n_put_back;
            }
        }

        if (n_to_put_back == n_put_back) {
            current_view = current_view.substr(
                0, current_view.size() - static_cast<std::size_t>(n_put_back));
            return position;
        }
        current_view = {};
    }

    {
        const auto n_to_put_back_from_putback_buffer =
            n_to_put_back - n_put_back;
        const auto putback_buffer_segment =
            std::basic_string_view<CharT>{putback_buffer.data(),
                                          putback_buffer.size()}
                .substr(putback_buffer.size() -
                        static_cast<std::size_t>(
                            n_to_put_back_from_putback_buffer));
        for (auto it = putback_buffer_segment.rbegin();
             it != putback_buffer_segment.rend(); ++it, (void)++n_put_back) {
            if (!putback(*it)) {
                putback_buffer.resize(
                    static_cast<std::size_t>(total_chars_avail - n_put_back));
                return total_chars_avail - n_put_back;
            }
        }

        putback_buffer.resize(static_cast<std::size_t>(position));
        return position;
    }
}

struct default_file_tag {};
struct gnu_file_tag {};
struct bsd_file_tag {};
struct musl_file_tag {};
struct win32_file_tag {};

// Non-pretty workaround for MSVC silliness
template <typename F, typename = void>
inline constexpr bool is_gnu_file = false;
template <typename F>
inline constexpr bool
    is_gnu_file<F,
                std::void_t<decltype(SCN_DECLVAL(F)._IO_read_ptr),
                            decltype(SCN_DECLVAL(F)._IO_read_end)>> = true;

template <typename F, typename = void>
inline constexpr bool is_bsd_file = false;
template <typename F>
inline constexpr bool is_bsd_file<
    F,
    std::void_t<decltype(SCN_DECLVAL(F)._p), decltype(SCN_DECLVAL(F)._r)>> =
    true;

template <typename F, typename = void>
inline constexpr bool is_musl_file = false;
template <typename F>
inline constexpr bool is_musl_file<
    F,
    std::void_t<decltype(SCN_DECLVAL(F).rpos), decltype(SCN_DECLVAL(F).rend)>> =
    true;

template <typename F>
inline constexpr bool is_win32_file =
    std::is_same_v<F, std::FILE> && SCN_WINDOWS && !SCN_MINGW;

SCN_MAYBE_UNUSED constexpr auto get_file_tag()
{
    if constexpr (is_gnu_file<std::FILE>) {
        return detail::tag_type<gnu_file_tag>{};
    }
    else if constexpr (is_bsd_file<std::FILE>) {
        return detail::tag_type<bsd_file_tag>{};
    }
    else if constexpr (is_musl_file<std::FILE>) {
        return detail::tag_type<musl_file_tag>{};
    }
    else if constexpr (is_win32_file<std::FILE>) {
        return detail::tag_type<win32_file_tag>{};
    }
    else {
        return detail::tag_type<default_file_tag>{};
    }
}

template <typename File>
struct stdio_file_interface_base {
    explicit constexpr stdio_file_interface_base(File* f) noexcept : file(f) {}
    ~stdio_file_interface_base() = default;

    stdio_file_interface_base(const stdio_file_interface_base&) = delete;
    stdio_file_interface_base& operator=(const stdio_file_interface_base&) =
        delete;

    constexpr stdio_file_interface_base(
        stdio_file_interface_base&& other) noexcept = delete;
    constexpr stdio_file_interface_base& operator=(
        stdio_file_interface_base&& other) noexcept = delete;

    File* file;
};

template <typename File, typename Tag>
struct stdio_file_interface_impl;

enum class stdio_file_error {
    eof,
    error,
};

template <typename File>
struct stdio_file_interface_impl<File, default_file_tag>
    : stdio_file_interface_base<File> {
    using stdio_file_interface_base<File>::stdio_file_interface_base;

    static constexpr void lock() {}
    static constexpr void unlock() {}

    SCN_NODISCARD static constexpr bool is_never_readable()
    {
        return false;
    }

    SCN_NODISCARD static constexpr bool has_buffering()
    {
        return false;
    }

    SCN_NODISCARD static std::string_view buffer()
    {
        return {};
    }
    static bool is_buffer_readable()
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    static void unsafe_advance_n(std::ptrdiff_t)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    static bool fill_buffer()
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    SCN_NODISCARD expected<char, stdio_file_error> read_one()
    {
        SCN_EXPECT(!is_never_readable());
        std::clearerr(this->file);
        auto res = std::fgetc(this->file);
        if (SCN_UNLIKELY(res == EOF)) {
            if (std::ferror(this->file)) {
                return unexpected(stdio_file_error::error);
            }
            return unexpected(stdio_file_error::eof);
        }
        return static_cast<char>(res);
    }

    static void prepare_putback() {}
    static void finalize_putback() {}

    SCN_NODISCARD bool putback(char ch)
    {
        return std::ungetc(static_cast<unsigned char>(ch), this->file) != EOF;
    }
};

template <typename File>
struct posix_stdio_file_interface : stdio_file_interface_base<File> {
    using stdio_file_interface_base<File>::stdio_file_interface_base;

    void lock()
    {
        flockfile(this->file);
    }
    void unlock()
    {
        funlockfile(this->file);
    }

#if SCN_POSIX
    SCN_NODISCARD bool is_never_readable() const
    {
        errno = 0;
        int fd = fileno_unlocked(this->file);
        int accmode = fcntl(fd, F_GETFL) & O_ACCMODE;
        return accmode == O_WRONLY;
    }
#else
    SCN_NODISCARD bool is_never_readable()
    {
        return false;
    }
#endif

    SCN_NODISCARD expected<char, stdio_file_error> read_one()
    {
        clearerr_unlocked(this->file);
        auto res = getc_unlocked(this->file);
        if (res == EOF) {
            if (ferror_unlocked(this->file)) {
                return unexpected(stdio_file_error::error);
            }
            return unexpected(stdio_file_error::eof);
        }
        return static_cast<char>(res);
    }

    void prepare_putback()
    {
        unlock();
    }
    void finalize_putback()
    {
        lock();
    }

    SCN_NODISCARD bool putback(char ch)
    {
        return std::ungetc(static_cast<unsigned char>(ch), this->file) != EOF;
    }
};

template <typename File>
struct stdio_file_interface_impl<File, gnu_file_tag>
    : posix_stdio_file_interface<File> {
    using posix_stdio_file_interface<File>::posix_stdio_file_interface;

    SCN_NODISCARD static constexpr bool has_buffering()
    {
        return true;
    }

    SCN_NODISCARD std::string_view buffer() const
    {
        return detail::make_string_view_from_pointers(this->file->_IO_read_ptr,
                                                      this->file->_IO_read_end);
    }
    void unsafe_advance_n(std::ptrdiff_t n)
    {
        SCN_EXPECT(this->file->_IO_read_ptr != nullptr);
        SCN_EXPECT(n >= 0);
        SCN_EXPECT(this->file->_IO_read_end - this->file->_IO_read_ptr >= n);
        this->file->_IO_read_ptr += n;
        clearerr_unlocked(this->file);
    }
    SCN_NODISCARD bool fill_buffer()
    {
        if (__uflow(this->file) != EOF) {
            --this->file->_IO_read_ptr;
            return true;
        }
        return false;
    }
};

template <typename File>
struct stdio_file_interface_impl<File, bsd_file_tag>
    : posix_stdio_file_interface<File> {
    using posix_stdio_file_interface<File>::posix_stdio_file_interface;

    SCN_NODISCARD static constexpr bool has_buffering()
    {
        return true;
    }

    SCN_NODISCARD std::string_view buffer() const
    {
        return {reinterpret_cast<const char*>(this->file->_p),
                static_cast<std::size_t>(this->file->_r)};
    }
    void unsafe_advance_n(std::ptrdiff_t n)
    {
        SCN_GCC_PUSH
        SCN_GCC_IGNORE("-Wuseless-cast")

        SCN_EXPECT(this->file->_p != nullptr);
        SCN_EXPECT(n >= 0);
        SCN_EXPECT(this->file->_r >= n);
        this->file->_p += n;
        this->file->_r -= static_cast<int>(n);
        clearerr_unlocked(this->file);

        SCN_GCC_POP
    }
    SCN_NODISCARD bool fill_buffer()
    {
        if (__srget(this->file) != EOF) {
            --this->file->_p;
            ++this->file->_r;
            return true;
        }
        return false;
    }
};

template <typename File>
struct stdio_file_interface_impl<File, musl_file_tag>
    : posix_stdio_file_interface<File> {
    using posix_stdio_file_interface<File>::posix_stdio_file_interface;

    SCN_NODISCARD static constexpr bool has_buffering()
    {
        return true;
    }

    SCN_NODISCARD std::string_view buffer() const
    {
        return detail::make_string_view_from_pointers(
            reinterpret_cast<const char*>(this->file->rpos),
            reinterpret_cast<const char*>(this->file->rend));
    }
    void unsafe_advance_n(std::ptrdiff_t n)
    {
        SCN_EXPECT(this->file->rpos != nullptr);
        SCN_EXPECT(n >= 0);
        SCN_EXPECT(this->file->rend - this->file->rpos >= n);
        this->file->rpos += n;
        clearerr_unlocked(this->file);
    }
    SCN_NODISCARD bool fill_buffer()
    {
        if (__uflow(this->file) != EOF) {
            --this->file->rpos;
            return true;
        }
        return false;
    }
};

template <typename File>
struct stdio_file_interface_impl<File, win32_file_tag>
    : stdio_file_interface_base<File> {
    using stdio_file_interface_base<File>::stdio_file_interface_base;

    void lock()
    {
        _lock_file(this->file);
    }
    void unlock()
    {
        _unlock_file(this->file);
    }

    SCN_NODISCARD static constexpr bool is_never_readable()
    {
        return false;
    }
    SCN_NODISCARD static constexpr bool has_buffering()
    {
        return false;
    }

    SCN_NODISCARD static std::string_view buffer()
    {
        return {};
    }
    static void unsafe_advance_n(std::ptrdiff_t n)
    {
        SCN_UNUSED(n);
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
    SCN_NODISCARD static bool fill_buffer()
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    SCN_NODISCARD expected<char, stdio_file_error> read_one()
    {
        auto res = _fgetc_nolock(this->file);
        if (res == EOF) {
            unlock();
            const bool is_eof = feof(this->file);
            lock();
            return unexpected(is_eof ? stdio_file_error::eof
                                     : stdio_file_error::error);
        }
        return static_cast<char>(res);
    }

    static void prepare_putback() {}
    static void finalize_putback() {}

    SCN_NODISCARD bool putback(char ch)
    {
        return _ungetc_nolock(static_cast<unsigned char>(ch), this->file) !=
               EOF;
    }
};

using stdio_file_interface =
    stdio_file_interface_impl<std::FILE, decltype(get_file_tag())::type>;

template <typename CharT, typename FileInterface>
struct file_buffer_interface {
    using char_type = CharT;

    static void construct(FileInterface& file,
                          scan_expected<void>& source_error)
    {
        file.lock();
        if (file.is_never_readable()) {
            source_error = detail::unexpected_scan_error(
                scan_error::invalid_source_state,
                "Failed to read FILE, not opened for reading");
        }
    }

    static void destruct(FileInterface& file)
    {
        file.unlock();
    }

    SCN_NODISCARD static bool fill(
        FileInterface& file,
        std::basic_string_view<char_type>& current_view,
        std::basic_string<char_type>& putback_buffer,
        scan_expected<void>& source_error,
        std::optional<char_type>& latest)
    {
        std::basic_string<char_type> previous_view{current_view};

        if (file.has_buffering()) {
            if (!current_view.empty()) {
                file.unsafe_advance_n(
                    static_cast<std::ptrdiff_t>(current_view.size()));
            }

            if (!file.buffer().empty()) {
                current_view = file.buffer();
                return true;
            }
            if (file.fill_buffer()) {
                current_view = file.buffer();
                putback_buffer.append(previous_view.begin(),
                                      previous_view.end());
                return true;
            }
        }

        auto res = file.read_one();
        if (!res) {
            if (res.error() == stdio_file_error::error) {
                source_error = detail::unexpected_scan_error(
                    scan_error::invalid_source_state,
                    "Failed to read FILE, ferror true");
            }
            putback_buffer.append(previous_view.begin(), previous_view.end());
            current_view = {};
            return false;
        }

        latest = *res;
        putback_buffer.append(previous_view.begin(), previous_view.end());
        current_view = {&*latest, 1};
        return true;
    }

    SCN_NODISCARD static std::ptrdiff_t sync(
        FileInterface& file,
        std::ptrdiff_t position,
        [[maybe_unused]] const detail::basic_scan_buffer<char_type>& buffer,
        std::basic_string_view<char_type>& current_view,
        std::basic_string<char_type>& putback_buffer,
        [[maybe_unused]] bool is_prelude_empty)
    {
        struct putback_guard {
            explicit putback_guard(FileInterface& file) : m_file(file)
            {
                m_file.prepare_putback();
            }
            ~putback_guard()
            {
                m_file.finalize_putback();
            }

        private:
            FileInterface& m_file;
        };

        if (file.has_buffering()) {
            const auto pbb_size =
                static_cast<std::ptrdiff_t>(putback_buffer.size());
            if (position == pbb_size) {
                return position;
            }
            const auto pos_without_putback_buffer = position - pbb_size;
            if (pos_without_putback_buffer > 0 &&
                pos_without_putback_buffer <=
                    static_cast<std::ptrdiff_t>(file.buffer().size())) {
                file.unsafe_advance_n(pos_without_putback_buffer);
                return position;
            }
        }

        putback_guard guard{file};
        return impl::buffer_sync_helper(
            position, current_view, putback_buffer,
            [&](char ch) { return file.putback(ch); });
    }
};

}  // namespace impl

/////////////////////////////////////////////////////////////////
// Range reading support
/////////////////////////////////////////////////////////////////

namespace impl {
#if SCN_MSVC_DEBUG_ITERATORS
#define SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND 1
#else
#define SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND 0
#endif

template <typename T>
constexpr bool range_supports_nocopy() noexcept
{
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    return ranges::contiguous_range<T> ||
           (ranges::random_access_range<T> &&
            detail::can_make_address_from_iterator<ranges::iterator_t<T>>);
#else
    return ranges::contiguous_range<T>;
#endif
}

template <typename R>
constexpr auto range_nocopy_data(const R& r) noexcept
{
    static_assert(range_supports_nocopy<R>());
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    return detail::to_address(ranges::begin(r));
#else
    return ranges::data(r);
#endif
}

template <typename R>
constexpr auto range_nocopy_size(const R& r) noexcept
{
    static_assert(range_supports_nocopy<R>());
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    return static_cast<size_t>(ranges::distance(detail::to_address(r.begin()),
                                                detail::to_address(r.end())));
#else
    return r.size();
#endif
}

template <typename I, typename S>
SCN_NODISCARD constexpr bool is_range_eof(I begin, S end)
{
#if SCN_NEED_MS_DEBUG_ITERATOR_WORKAROUND
    if constexpr (ranges::contiguous_iterator<I> ||
                  (ranges::random_access_iterator<I> &&
                   detail::can_make_address_from_iterator<I>)) {
        return detail::to_address(begin) == detail::to_address(end);
    }
    else
#endif
    {
        return begin == end;
    }
}

template <typename Range>
SCN_NODISCARD constexpr bool is_range_eof(Range r)
{
    return is_range_eof(r.begin(), r.end());
}

template <typename Range>
SCN_NODISCARD constexpr eof_error eof_check(Range range)
{
    if (SCN_UNLIKELY(is_range_eof(range))) {
        return eof_error::eof;
    }
    return eof_error::good;
}

template <typename Range>
bool is_entire_source_contiguous(Range r)
{
    SCN_UNUSED(r);
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return true;
    }
    else if constexpr (std::is_same_v<ranges::const_iterator_t<Range>,
                                      typename detail::basic_scan_buffer<
                                          detail::char_t<Range>>::iterator>) {
        auto beg = r.begin();
        if (!beg.stores_parent()) {
            return true;
        }
        return beg.parent()->is_contiguous();
    }
    else {
        return false;
    }
}

template <typename Range>
std::size_t contiguous_beginning_size(Range r)
{
    SCN_UNUSED(r);

    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return r.size();
    }
    else if constexpr (std::is_same_v<ranges::const_iterator_t<Range>,
                                      typename detail::basic_scan_buffer<
                                          detail::char_t<Range>>::iterator>) {
        if constexpr (ranges::common_range<Range>) {
            auto seg = r.begin().contiguous_segment();
            auto dist =
                static_cast<size_t>(ranges::distance(r.begin(), r.end()));
            return std::min(seg.size(), dist);
        }
        else {
            return r.begin().contiguous_segment().size();
        }
    }
    else {
        return false;
    }
}

template <typename Range>
auto get_contiguous_beginning(Range r)
{
    SCN_UNUSED(r);

    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return r;
    }
    else if constexpr (std::is_same_v<ranges::const_iterator_t<Range>,
                                      typename detail::basic_scan_buffer<
                                          detail::char_t<Range>>::iterator>) {
        if constexpr (ranges::common_range<Range>) {
            auto seg = r.begin().contiguous_segment();
            auto dist =
                static_cast<size_t>(ranges::distance(r.begin(), r.end()));
            return seg.substr(0, std::min(seg.size(), dist));
        }
        else {
            return r.begin().contiguous_segment();
        }
    }
    else {
        return std::basic_string_view<detail::char_t<Range>>{};
    }
}

template <typename Range>
auto get_as_contiguous(Range r)
{
    SCN_EXPECT(is_entire_source_contiguous(r));

    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return r;
    }
    else if constexpr (std::is_same_v<ranges::const_iterator_t<Range>,
                                      typename detail::basic_scan_buffer<
                                          detail::char_t<Range>>::iterator>) {
        if constexpr (ranges::common_range<Range>) {
            return detail::make_string_view_from_pointers(
                r.begin().to_contiguous_segment_iterator(),
                r.end().to_contiguous_segment_iterator());
        }
        else {
            return r.begin().contiguous_segment();
        }
    }
    else {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
        SCN_UNUSED(r);
        // for return type deduction
        return std::basic_string_view<detail::char_t<Range>>{};
    }
}

template <typename Range>
std::size_t guaranteed_minimum_size(Range r)
{
    if constexpr (ranges::sized_range<Range>) {
        return r.size();
    }
    else if constexpr (std::is_same_v<ranges::const_iterator_t<Range>,
                                      typename detail::basic_scan_buffer<
                                          detail::char_t<Range>>::iterator>) {
        if constexpr (ranges::common_range<Range>) {
            return static_cast<size_t>(ranges::distance(r.begin(), r.end()));
        }
        else {
            if (r.begin().stores_parent()) {
                return static_cast<size_t>(
                    r.begin().parent()->chars_available() -
                    r.begin().position());
            }
            return r.begin().contiguous_segment().size();
        }
    }
    else {
        SCN_UNUSED(r);
        return 0;
    }
}

template <typename I, typename T>
struct iterator_value_result {
    SCN_NO_UNIQUE_ADDRESS I iterator;
    SCN_NO_UNIQUE_ADDRESS T value;
};

}  // namespace impl

/////////////////////////////////////////////////////////////////
// Unicode
/////////////////////////////////////////////////////////////////

namespace impl {

template <typename CharT>
constexpr bool validate_unicode(std::basic_string_view<CharT> src)
{
    auto it = src.begin();
    while (it != src.end()) {
        const auto len = static_cast<std::ptrdiff_t>(
            detail::code_point_length_by_starting_code_unit(*it));
        SCN_EXPECT(len >= 0);
        if (len == 0) {
            return false;
        }
        if (std::distance(it, src.end()) < len) {
            return false;
        }
        const auto cp = detail::decode_code_point_exhaustive(
            detail::make_string_view_from_iterators<CharT>(it, it + len));
        if (cp >= detail::invalid_code_point) {
            return false;
        }
        it += len;
    }
    return true;
}

template <typename Range>
constexpr auto get_start_for_next_code_point(Range input)
    -> ranges::const_iterator_t<Range>
{
    auto it = input.begin();
    for (; it != input.end(); ++it) {
        if (detail::code_point_length_by_starting_code_unit(*it) != 0) {
            break;
        }
    }
    return it;
}

template <typename CharT>
constexpr auto get_next_code_point(std::basic_string_view<CharT> input)
    -> iterator_value_result<typename std::basic_string_view<CharT>::iterator,
                             char32_t>
{
    SCN_EXPECT(!input.empty());

    const auto len = detail::code_point_length_by_starting_code_unit(input[0]);
    if (SCN_UNLIKELY(len == 0)) {
        return {get_start_for_next_code_point(input),
                detail::invalid_code_point};
    }
    if (SCN_UNLIKELY(len > input.size())) {
        return {input.end(), detail::invalid_code_point};
    }

    return {input.begin() + len,
            detail::decode_code_point_exhaustive(input.substr(0, len))};
}

template <typename CharT>
constexpr auto get_next_code_point_valid(std::basic_string_view<CharT> input)
    -> iterator_value_result<typename std::basic_string_view<CharT>::iterator,
                             char32_t>
{
    SCN_EXPECT(!input.empty());

    const auto len = detail::code_point_length_by_starting_code_unit(input[0]);
    SCN_EXPECT(len <= input.size());

    return {input.begin() + static_cast<std::ptrdiff_t>(len),
            detail::decode_code_point_exhaustive_valid(input.substr(0, len))};
}

template <typename CharT>
struct is_first_char_space_result {
    ranges::iterator_t<std::basic_string_view<CharT>> iterator;
    char32_t cp;
    bool is_space;
};

template <typename CharT>
inline constexpr auto is_first_char_space(std::basic_string_view<CharT> str)
    -> is_first_char_space_result<CharT>
{
    // TODO: optimize
    SCN_EXPECT(!str.empty());
    auto res = get_next_code_point(str);
    return {res.iterator, res.value, detail::is_cp_space(res.value)};
}

inline constexpr scan_expected<wchar_t> encode_code_point_as_wide_character(
    char32_t cp,
    bool error_on_overflow)
{
    SCN_EXPECT(cp < detail::invalid_code_point);
    if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
        SCN_UNUSED(error_on_overflow);
        return static_cast<wchar_t>(cp);
    }
    else {
        if (cp < 0x10000) {
            return static_cast<wchar_t>(cp);
        }
        if (error_on_overflow) {
            return detail::unexpected_scan_error(
                scan_error::value_positive_overflow,
                "Non-BMP code point can't be "
                "narrowed to a single 2-byte "
                "wchar_t code unit");
        }
        // Return the lead surrogate
        return static_cast<wchar_t>(
            (static_cast<uint32_t>(cp) - 0x10000) / 0x400 + 0xd800);
    }
}

template <typename SourceCharT, typename DestCharT>
void transcode_to_string_impl_to32(std::basic_string_view<SourceCharT> src,
                                   std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(DestCharT) == 4);

    auto it = src.begin();
    while (it != src.end()) {
        auto res = get_next_code_point(
            detail::make_string_view_from_iterators<SourceCharT>(it,
                                                                 src.end()));
        if (SCN_UNLIKELY(res.value == detail::invalid_code_point)) {
            dest.push_back(DestCharT{0xfffd});
        }
        else {
            dest.push_back(static_cast<DestCharT>(res.value));
        }
        it = detail::make_string_view_iterator(src, res.iterator);
    }
}
template <typename SourceCharT, typename DestCharT>
void transcode_valid_to_string_impl_to32(
    std::basic_string_view<SourceCharT> src,
    std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(DestCharT) == 4);

    auto it = src.begin();
    while (it != src.end()) {
        auto res = get_next_code_point_valid(
            detail::make_string_view_from_iterators<SourceCharT>(it,
                                                                 src.end()));
        SCN_EXPECT(res.value < detail::invalid_code_point);
        dest.push_back(static_cast<DestCharT>(res.value));
        it = detail::make_string_view_iterator(src, res.iterator);
    }
}

template <bool VerifiedValid, typename SourceCharT, typename DestCharT>
void transcode_to_string_impl_32to8(std::basic_string_view<SourceCharT> src,
                                    std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) == 4);
    static_assert(sizeof(DestCharT) == 1);

    for (auto cp : src) {
        const auto u32cp = static_cast<uint32_t>(cp);
        if (SCN_UNLIKELY(!VerifiedValid && static_cast<char32_t>(cp) >=
                                               detail::invalid_code_point)) {
            // Replacement character
            dest.push_back(static_cast<char>(0xef));
            dest.push_back(static_cast<char>(0xbf));
            dest.push_back(static_cast<char>(0xbd));
        }
        else if (cp < 128) {
            dest.push_back(static_cast<char>(cp));
        }
        else if (cp < 2048) {
            dest.push_back(
                static_cast<char>(0xc0 | (static_cast<char>(u32cp >> 6))));
            dest.push_back(
                static_cast<char>(0x80 | (static_cast<char>(u32cp) & 0x3f)));
        }
        else if (cp < 65536) {
            dest.push_back(
                static_cast<char>(0xe0 | (static_cast<char>(u32cp >> 12))));
            dest.push_back(static_cast<char>(
                0x80 | (static_cast<char>(u32cp >> 6) & 0x3f)));
            dest.push_back(
                static_cast<char>(0x80 | (static_cast<char>(u32cp) & 0x3f)));
        }
        else {
            dest.push_back(
                static_cast<char>(0xf0 | (static_cast<char>(u32cp >> 18))));
            dest.push_back(static_cast<char>(
                0x80 | (static_cast<char>(u32cp >> 12) & 0x3f)));
            dest.push_back(static_cast<char>(
                0x80 | (static_cast<char>(u32cp >> 6) & 0x3f)));
            dest.push_back(
                static_cast<char>(0x80 | (static_cast<char>(u32cp) & 0x3f)));
        }
    }
}

template <bool VerifiedValid, typename SourceCharT, typename DestCharT>
void transcode_to_string_impl_32to16(std::basic_string_view<SourceCharT> src,
                                     std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) == 4);
    static_assert(sizeof(DestCharT) == 2);

    for (auto cp : src) {
        const auto u32cp = static_cast<uint32_t>(cp);
        if (SCN_UNLIKELY(!VerifiedValid && cp >= detail::invalid_code_point)) {
            dest.push_back(char16_t{0xfffd});
        }
        else if (cp < 0x10000) {
            dest.push_back(static_cast<char16_t>(cp));
        }
        else {
            dest.push_back(
                static_cast<char16_t>((u32cp - 0x10000) / 0x400 + 0xd800));
            dest.push_back(
                static_cast<char16_t>((u32cp - 0x10000) % 0x400 + 0xd800));
        }
    }
}

template <typename SourceCharT, typename DestCharT>
void transcode_to_string(std::basic_string_view<SourceCharT> src,
                         std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) != sizeof(DestCharT));

    if constexpr (sizeof(SourceCharT) == 1) {
        if constexpr (sizeof(DestCharT) == 2) {
            std::u32string tmp;
            transcode_to_string_impl_to32(src, tmp);
            return transcode_to_string_impl_32to16<false>(
                std::u32string_view{tmp}, dest);
        }
        else if constexpr (sizeof(DestCharT) == 4) {
            return transcode_to_string_impl_to32(src, dest);
        }
    }
    else if constexpr (sizeof(SourceCharT) == 2) {
        if constexpr (sizeof(DestCharT) == 1) {
            std::u32string tmp;
            transcode_to_string_impl_to32(src, tmp);
            return transcode_to_string_impl_32to8<false>(
                std::u32string_view{tmp}, dest);
        }
        else if constexpr (sizeof(DestCharT) == 4) {
            return trasncode_to_string_impl_to32(src, dest);
        }
    }
    else if constexpr (sizeof(SourceCharT) == 4) {
        if constexpr (sizeof(DestCharT) == 1) {
            return transcode_to_string_impl_32to8<false>(src, dest);
        }
        else if constexpr (sizeof(DestCharT) == 2) {
            return transcode_to_string_impl_32to16<false>(src, dest);
        }
    }

    SCN_EXPECT(false);
    SCN_UNREACHABLE;
}
template <typename SourceCharT, typename DestCharT>
void transcode_valid_to_string(std::basic_string_view<SourceCharT> src,
                               std::basic_string<DestCharT>& dest)
{
    static_assert(sizeof(SourceCharT) != sizeof(DestCharT));

    SCN_EXPECT(validate_unicode(src));
    if constexpr (sizeof(SourceCharT) == 1) {
        if constexpr (sizeof(DestCharT) == 2) {
            // TODO: Optimize, remove utf32-step, go direct utf8->utf16
            std::u32string tmp;
            transcode_valid_to_string_impl_to32(src, tmp);
            return transcode_to_string_impl_32to16<true>(
                std::u32string_view{tmp}, dest);
        }
        else if constexpr (sizeof(DestCharT) == 4) {
            return transcode_valid_to_string_impl_to32(src, dest);
        }
    }
    else if constexpr (sizeof(SourceCharT) == 2) {
        if constexpr (sizeof(DestCharT) == 1) {
            std::u32string tmp;
            transcode_valid_to_string_impl_to32(src, tmp);
            return transcode_to_string_impl_32to8<true>(
                std::u32string_view{tmp}, dest);
        }
        else if constexpr (sizeof(DestCharT) == 4) {
            return trasncode_valid_to_string_impl_to32(src, dest);
        }
    }
    else if constexpr (sizeof(SourceCharT) == 4) {
        if constexpr (sizeof(DestCharT) == 1) {
            return transcode_to_string_impl_32to8<true>(src, dest);
        }
        else if constexpr (sizeof(DestCharT) == 2) {
            return transcode_to_string_impl_32to16<true>(src, dest);
        }
    }

    SCN_EXPECT(false);
    SCN_UNREACHABLE;
}

template <typename CharT>
constexpr void for_each_code_point(std::basic_string_view<CharT> input,
                                   function_ref<void(char32_t)> cb)
{
    // TODO: Could be optimized by being eager
    auto it = input.begin();
    while (it != input.end()) {
        auto res = get_next_code_point(
            detail::make_string_view_from_iterators<CharT>(it, input.end()));
        cb(res.value);
        it = detail::make_string_view_iterator(input, res.iterator);
    }
}

template <typename CharT>
constexpr void for_each_code_point_valid(std::basic_string_view<CharT> input,
                                         function_ref<void(char32_t)> cb)
{
    auto it = input.begin();
    while (it != input.end()) {
        auto res = get_next_code_point_valid(
            detail::make_string_view_from_iterators<CharT>(it, input.end()));
        cb(res.value);
        it = detail::make_string_view_iterator(input, res.iterator);
    }
}

/////////////////////////////////////////////////////////////////
// contiguous_range_factory
/////////////////////////////////////////////////////////////////

template <typename View>
class take_width_view;

template <typename CharT>
struct string_view_wrapper {
    using char_type = CharT;
    using string_type = std::basic_string<CharT>;
    using string_view_type = std::basic_string_view<CharT>;

    constexpr string_view_wrapper() = default;

    template <typename Range,
              std::enable_if_t<ranges::borrowed_range<Range> &&
                               ranges::contiguous_range<Range> &&
                               ranges::sized_range<Range>>* = nullptr>
    constexpr string_view_wrapper(Range&& r) : sv(ranges::data(r), r.size())
    {
    }

    template <typename Range,
              std::enable_if_t<ranges::borrowed_range<Range> &&
                               ranges::contiguous_range<Range> &&
                               ranges::sized_range<Range>>* = nullptr>
    void assign(Range&& r)
    {
        sv = string_view_type{ranges::data(r), r.size()};
    }

    constexpr auto view() const
    {
        return sv;
    }

    constexpr bool stores_allocated_string() const
    {
        return false;
    }

    [[noreturn]] string_type get_allocated_string() const
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    string_view_type sv;
};

template <typename Range>
string_view_wrapper(Range)
    -> string_view_wrapper<detail::char_t<detail::remove_cvref_t<Range>>>;

template <typename CharT>
class contiguous_range_factory {
public:
    using char_type = CharT;
    using string_type = std::basic_string<CharT>;
    using string_view_type = std::basic_string_view<CharT>;

    contiguous_range_factory() = default;

    template <typename Range,
              std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
    contiguous_range_factory(Range&& range)
    {
        emplace_range(SCN_FWD(range));
    }

    contiguous_range_factory(string_view_wrapper<CharT> svw)
        : m_storage(std::nullopt), m_view(svw.view())
    {
    }

    contiguous_range_factory(const contiguous_range_factory&) = delete;
    contiguous_range_factory& operator=(const contiguous_range_factory&) =
        delete;

    contiguous_range_factory(contiguous_range_factory&& other)
        : m_storage(SCN_MOVE(other.m_storage))
    {
        if (m_storage) {
            m_view = *m_storage;
        }
        else {
            m_view = other.m_view;
        }
    }
    contiguous_range_factory& operator=(contiguous_range_factory&& other)
    {
        m_storage = SCN_MOVE(other.m_storage);
        if (m_storage) {
            m_view = *m_storage;
        }
        else {
            m_view = other.m_view;
        }
        return *this;
    }

    ~contiguous_range_factory() = default;

    template <typename Range,
              std::enable_if_t<ranges::forward_range<Range>>* = nullptr>
    void assign(Range&& range)
    {
        emplace_range(SCN_FWD(range));
    }

    string_view_type view() const
    {
        return m_view;
    }

    constexpr bool stores_allocated_string() const
    {
        return m_storage.has_value();
    }

    string_type& get_allocated_string() &
    {
        SCN_EXPECT(stores_allocated_string());
        return *m_storage;
    }
    const string_type& get_allocated_string() const&
    {
        SCN_EXPECT(stores_allocated_string());
        return *m_storage;
    }
    string_type&& get_allocated_string() &&
    {
        SCN_EXPECT(stores_allocated_string());
        return *m_storage;
    }

    string_type& make_into_allocated_string()
    {
        if (stores_allocated_string()) {
            return get_allocated_string();
        }

        auto& str = m_storage.emplace(m_view.data(), m_view.size());
        m_view = string_view_type{str.data(), str.size()};
        return str;
    }

private:
    template <typename Range>
    void emplace_range(Range&& range)
    {
        using value_t = ranges::range_value_t<Range>;

        if constexpr (ranges::borrowed_range<Range> &&
                      ranges::contiguous_range<Range> &&
                      ranges::sized_range<Range>) {
            m_storage.reset();
            m_view = string_view_type{ranges::data(range), range.size()};
        }
        else if constexpr (std::is_same_v<detail::remove_cvref_t<Range>,
                                          std::basic_string<CharT>>) {
            m_storage.emplace(SCN_FWD(range));
            m_view = string_view_type{m_storage->data(), m_storage->size()};
        }
        else if constexpr (std::is_same_v<ranges::iterator_t<Range>,
                                          typename detail::basic_scan_buffer<
                                              value_t>::iterator> &&
                           ranges::common_range<Range>) {
            auto beg_seg = range.begin().contiguous_segment();
            auto end_seg = range.end().contiguous_segment();
            if (SCN_UNLIKELY(detail::to_address(beg_seg.end()) !=
                             detail::to_address(end_seg.end()))) {
                auto& str = m_storage.emplace();
                str.reserve(static_cast<std::size_t>(range.end().position() -
                                                     range.begin().position()));
                std::copy(range.begin(), range.end(), std::back_inserter(str));
                m_view = string_view_type{str.data(), str.size()};
                return;
            }

            m_view = detail::make_string_view_from_pointers(beg_seg.data(),
                                                            end_seg.data());
            m_storage.reset();
        }
        else {
            auto& str = m_storage.emplace();
            if constexpr (ranges::sized_range<Range>) {
                str.reserve(range.size());
            }
            if constexpr (ranges::common_range<Range>) {
                std::copy(ranges::begin(range), ranges::end(range),
                          std::back_inserter(str));
            }
            else {
                for (auto it = ranges::begin(range); it != ranges::end(range);
                     ++it) {
                    str.push_back(*it);
                }
            }
            m_view = string_view_type{str.data(), str.size()};
        }
    }

    std::optional<string_type> m_storage{std::nullopt};
    string_view_type m_view{};
};

template <typename Range>
contiguous_range_factory(Range)
    -> contiguous_range_factory<detail::char_t<detail::remove_cvref_t<Range>>>;

template <typename Range>
auto make_contiguous_buffer(Range&& range)
{
    if constexpr (ranges::borrowed_range<Range> &&
                  ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range>) {
        return string_view_wrapper{SCN_FWD(range)};
    }
    else {
        return contiguous_range_factory{SCN_FWD(range)};
    }
}
}  // namespace impl

/////////////////////////////////////////////////////////////////
// locale stuff
/////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_LOCALE

namespace detail {
extern template locale_ref::locale_ref(const std::locale&);
extern template auto locale_ref::get() const -> std::locale;
}  // namespace detail

namespace impl {
template <typename Facet>
const Facet& get_facet(detail::locale_ref loc)
{
    auto stdloc = loc.get<std::locale>();
    SCN_EXPECT(std::has_facet<Facet>(stdloc));
    return std::use_facet<Facet>(stdloc);
}

template <typename Facet>
const Facet& get_or_add_facet(std::locale& stdloc)
{
    if (std::has_facet<Facet>(stdloc)) {
        return std::use_facet<Facet>(stdloc);
    }
    stdloc = std::locale(stdloc, new Facet{});
    return std::use_facet<Facet>(stdloc);
}

class clocale_restorer {
public:
    clocale_restorer(int cat) : m_category(cat)
    {
        const auto loc = std::setlocale(cat, nullptr);
        std::strcpy(m_locbuf, loc);
    }
    ~clocale_restorer()
    {
        // Restore locale to what it was before
        std::setlocale(m_category, m_locbuf);
    }

    clocale_restorer(const clocale_restorer&) = delete;
    clocale_restorer(clocale_restorer&&) = delete;
    clocale_restorer& operator=(const clocale_restorer&) = delete;
    clocale_restorer& operator=(clocale_restorer&&) = delete;

private:
    // For whatever reason, this cannot be stored in the heap if
    // setlocale hasn't been called before, or msan errors with
    // 'use-of-unitialized-value' when resetting the locale
    // back. POSIX specifies that the content of loc may not be
    // static, so we need to save it ourselves
    char m_locbuf[64] = {0};

    int m_category;
};

class set_clocale_classic_guard {
public:
    set_clocale_classic_guard(int cat) : m_restorer(cat)
    {
        std::setlocale(cat, "C");
    }

private:
    clocale_restorer m_restorer;
};
}  // namespace impl

namespace impl {
struct classic_with_thsep_tag {};

template <typename CharT>
struct localized_number_formatting_options {
    localized_number_formatting_options() = default;

    localized_number_formatting_options(classic_with_thsep_tag)
    {
        grouping = "\3";
        thousands_sep = CharT{','};
    }

    localized_number_formatting_options(detail::locale_ref loc)
    {
        auto stdloc = loc.get<std::locale>();
        const auto& numpunct = get_or_add_facet<std::numpunct<CharT>>(stdloc);
        grouping = numpunct.grouping();
        thousands_sep =
            grouping.length() != 0 ? numpunct.thousands_sep() : CharT{0};
        decimal_point = numpunct.decimal_point();
    }

    std::string grouping{};
    CharT thousands_sep{0};
    CharT decimal_point{CharT{'.'}};
};
}  // namespace impl

#else

namespace impl {
struct set_clocale_classic_guard {
    set_clocale_classic_guard(int) {}
};

struct classic_with_thsep_tag {};

template <typename CharT>
struct localized_number_formatting_options {
    localized_number_formatting_options() = default;

    localized_number_formatting_options(classic_with_thsep_tag)
    {
        grouping = "\3";
        thousands_sep = CharT{','};
    }

    std::string grouping{};
    CharT thousands_sep{0};
    CharT decimal_point{CharT{'.'}};
};
}  // namespace impl

#endif  // !SCN_DISABLE_LOCALE

/////////////////////////////////////////////////////////////////
// Range reading algorithms
/////////////////////////////////////////////////////////////////

namespace impl {

SCN_PUBLIC std::string_view::iterator find_classic_space_narrow_fast(
    std::string_view source);

SCN_PUBLIC std::string_view::iterator find_classic_nonspace_narrow_fast(
    std::string_view source);

SCN_PUBLIC std::string_view::iterator find_nondecimal_digit_narrow_fast(
    std::string_view source);

template <typename Range>
auto read_all(Range range) -> ranges::const_iterator_t<Range>
{
    return ranges::next(range.begin(), range.end());
}

template <typename Range>
auto read_exactly_n_code_units(Range range, std::ptrdiff_t count)
    -> eof_expected<ranges::const_iterator_t<Range>>
{
    SCN_EXPECT(count >= 0);

    if constexpr (ranges::sized_range<Range>) {
        const auto sz = static_cast<std::ptrdiff_t>(range.size());
        if (sz < count) {
            return unexpected(eof_error::eof);
        }

        return ranges::next(range.begin(), count);
    }
    else {
        auto it = range.begin();
        if (guaranteed_minimum_size(range) >= static_cast<std::size_t>(count)) {
            return ranges::next(it, count);
        }

        for (std::ptrdiff_t i = 0; i < count; ++i, (void)++it) {
            if (it == range.end()) {
                return unexpected(eof_error::eof);
            }
        }

        return it;
    }
}

template <typename CharT>
class codepoint_string {
public:
    using value_type = CharT;

    static constexpr std::integral_constant<std::size_t, 4 / sizeof(CharT)>
        max_size{};

    constexpr codepoint_string() = default;

    template <std::size_t N, std::enable_if_t<N <= max_size + 1>>
    /*implicit*/ constexpr codepoint_string(const CharT (&d)[N])
        : codepoint_string(d, N - 1)
    {
    }

    /*implicit*/ constexpr codepoint_string(std::basic_string_view<CharT> s)
        : codepoint_string(s.data(), s.size())
    {
    }

    template <typename Iterator,
              std::enable_if_t<ranges::input_iterator<Iterator>>* = nullptr>
    constexpr codepoint_string(Iterator d, std::size_t n)
        : m_size(static_cast<unsigned char>(n))
    {
        SCN_EXPECT(n <= 4);
        std::copy_n(d, n, m_data);
    }

    template <typename Iterator,
              std::enable_if_t<ranges::forward_iterator<Iterator>>* = nullptr>
    constexpr codepoint_string(Iterator b, Iterator e)
        : m_size(static_cast<unsigned char>(std::distance(b, e)))
    {
        SCN_EXPECT(m_size <= 4);
        std::copy(b, e, m_data);
    }

    constexpr CharT* data()
    {
        return m_data;
    }
    constexpr const CharT* data() const
    {
        return m_data;
    }

    SCN_NODISCARD constexpr std::size_t size() const
    {
        return m_size;
    }

    SCN_NODISCARD constexpr bool empty() const
    {
        return size() == 0;
    }

    constexpr CharT* begin()
    {
        return data();
    }
    constexpr const CharT* begin() const
    {
        return data();
    }

    constexpr CharT* end()
    {
        return data() + size();
    }
    constexpr const CharT* end() const
    {
        return data() + size();
    }

    constexpr bool operator==(const codepoint_string& other) const
    {
        return size() == other.size() &&
               std::equal(begin(), end(), other.begin());
    }
    constexpr bool operator!=(const codepoint_string& other) const
    {
        return !(*this == other);
    }

private:
    CharT m_data[max_size]{};
    unsigned char m_size{};
};

template <typename Iterator, typename CharT>
struct read_code_point_into_result {
    Iterator iterator;
    codepoint_string<CharT> codepoint;

    SCN_NODISCARD bool is_valid() const
    {
        return !codepoint.empty();
    }
};

template <typename Range>
auto read_code_point_into(Range range)
    -> read_code_point_into_result<ranges::const_iterator_t<Range>,
                                   detail::char_t<Range>>
{
    SCN_EXPECT(!is_range_eof(range));
    using string_type = codepoint_string<detail::char_t<Range>>;

    auto it = range.begin();
    const auto len = detail::code_point_length_by_starting_code_unit(*it);

    if (SCN_UNLIKELY(len == 0)) {
        ++it;
        it = get_start_for_next_code_point(ranges::subrange{it, range.end()});
        return {it, {}};
    }

    if (len == 1) {
        ++it;
        return {it, string_type(range.begin(), 1)};
    }

    ranges::advance(it, static_cast<std::ptrdiff_t>(len), range.end());
    return {it, string_type{range.begin(), it}};
}

template <typename Range>
auto read_code_point(Range range) -> ranges::const_iterator_t<Range>
{
    return read_code_point_into(range).iterator;
}

template <typename Range>
auto read_exactly_n_code_points(Range range, std::ptrdiff_t count)
    -> eof_expected<ranges::const_iterator_t<Range>>
{
    SCN_EXPECT(count >= 0);

    if (count > 0) {
        if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }
    }

    auto it = range.begin();
    for (std::ptrdiff_t i = 0; i < count; ++i) {
        auto rng = ranges::subrange{it, range.end()};

        if (auto e = eof_check(rng); SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }

        it = read_code_point(rng);
    }

    return it;
}

template <typename Range>
auto read_until_code_unit(Range range,
                          detail::mp_identity_t<detail::char_t<Range>> cu)
    -> ranges::const_iterator_t<Range>
{
    if constexpr (ranges::common_range<Range>) {
        return std::find(range.begin(), range.end(), cu);
    }
    else {
        auto first = range.begin();
        for (; first != range.end(); ++first) {
            if (*first == cu) {
                return first;
            }
        }
        return first;
    }
}

template <typename Range>
auto read_until_code_unit(Range range,
                          function_ref<bool(detail::char_t<Range>)> pred)
    -> ranges::const_iterator_t<Range>
{
    if constexpr (ranges::common_range<Range>) {
        return std::find_if(range.begin(), range.end(), pred);
    }
    else {
        auto first = range.begin();
        for (; first != range.end(); ++first) {
            if (pred(*first)) {
                return first;
            }
        }
        return first;
    }
}

template <typename Range>
auto read_while_code_unit(Range range,
                          detail::mp_identity_t<detail::char_t<Range>> cu)
    -> ranges::const_iterator_t<Range>
{
    auto first = range.begin();
    for (; first != range.end(); ++first) {
        if (*first != cu) {
            return first;
        }
    }
    return first;
}

template <typename Range>
auto read_while_code_unit(Range range,
                          function_ref<bool(detail::char_t<Range>)> pred)
    -> ranges::const_iterator_t<Range>
{
    return read_until_code_unit(range, std::not_fn(pred));
}

template <typename Range>
auto read_until1_code_unit(Range range,
                           function_ref<bool(detail::char_t<Range>)> pred)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto it = read_until_code_unit(range, pred);
    if (it == range.begin()) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_while1_code_unit(Range range,
                           function_ref<bool(detail::char_t<Range>)> pred)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto it = read_while_code_unit(range, pred);
    if (it == range.begin()) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range, typename CodeUnits>
auto read_until_code_units(Range range, const CodeUnits& needle)
    -> ranges::const_iterator_t<Range>
{
    static_assert(ranges::common_range<CodeUnits>);

    if constexpr (ranges::common_range<Range>) {
        return std::search(range.begin(), range.end(), needle.begin(),
                           needle.end());
    }
    else {
        auto first = range.begin();
        while (true) {
            auto it = first;
            for (auto needle_it = needle.begin();; ++it, (void)++needle_it) {
                if (needle_it == needle.end()) {
                    it = SCN_MOVE(first);
                    return it;
                }
                if (it == range.end()) {
                    return it;
                }
                if (*it != *needle_it) {
                    break;
                }
            }
            ++first;
        }
    }
}

template <typename Range, typename CodeUnits>
auto read_while_code_units(Range range, const CodeUnits& needle)
    -> ranges::const_iterator_t<Range>
{
    static_assert(ranges::common_range<CodeUnits>);

    auto it = range.begin();
    while (it != range.end()) {
        auto r = read_exactly_n_code_units(
            ranges::subrange{it, range.end()},
            static_cast<std::ptrdiff_t>(needle.size()));
        if (!r) {
            return it;
        }
        static_assert(
            std::is_same_v<decltype(it), detail::remove_cvref_t<decltype(*r)>>);
        if (!std::equal(it, *r, needle.begin())) {
            return it;
        }
        it = *r;
    }
    SCN_ENSURE(it == range.end());
    return it;
}

template <typename Range>
auto read_until_code_point(Range range, function_ref<bool(char32_t)> pred)
    -> ranges::const_iterator_t<Range>
{
    auto it = range.begin();
    while (it != range.end()) {
        const auto val =
            read_code_point_into(ranges::subrange{it, range.end()});
        if (SCN_LIKELY(val.is_valid())) {
            const auto cp = detail::decode_code_point_exhaustive(
                std::basic_string_view<detail::char_t<Range>>{
                    val.codepoint.data(), val.codepoint.size()});
            if (pred(cp)) {
                return it;
            }
        }
        it = val.iterator;
    }

    return it;
}

template <typename Range>
auto read_while_code_point(Range range, function_ref<bool(char32_t)> pred)
    -> ranges::const_iterator_t<Range>
{
    return read_until_code_point(range, std::not_fn(pred));
}

template <typename Range>
auto read_until_classic_space(Range range) -> ranges::const_iterator_t<Range>
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(range);
        auto it = find_classic_space_narrow_fast(buf.view());
        return ranges::next(range.begin(),
                            ranges::distance(buf.view().begin(), it));
    }
    else {
        auto it = range.begin();

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_space_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges::next(it, ranges::distance(seg.begin(), seg_it));
            }
            ranges::advance(it, static_cast<std::ptrdiff_t>(seg.size()));
        }

        return read_until_code_point(
            ranges::subrange{it, range.end()},
            [](char32_t cp) noexcept { return detail::is_cp_space(cp); });
    }
}

template <typename Range>
auto read_while_classic_space(Range range) -> ranges::const_iterator_t<Range>
{
    if constexpr (ranges::contiguous_range<Range> &&
                  ranges::sized_range<Range> &&
                  std::is_same_v<detail::char_t<Range>, char>) {
        auto buf = make_contiguous_buffer(range);
        auto it = find_classic_nonspace_narrow_fast(buf.view());
        return ranges::next(range.begin(),
                            ranges::distance(buf.view().begin(), it));
    }
    else {
        auto it = range.begin();

        if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
            auto seg = get_contiguous_beginning(range);
            if (auto seg_it = find_classic_nonspace_narrow_fast(seg);
                seg_it != seg.end()) {
                return ranges::next(it, ranges::distance(seg.begin(), seg_it));
            }
            ranges::advance(it, static_cast<std::ptrdiff_t>(seg.size()));
        }

        SCN_UNUSED(it);
        return read_while_code_point(range, [](char32_t cp) noexcept {
            return detail::is_cp_space(cp);
        });
    }
}

template <typename Range>
auto read_matching_code_unit(Range range, detail::char_t<Range> ch)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
        return unexpected(make_eof_parse_error(e));
    }

    if (SCN_UNLIKELY(*range.begin() !=
                     static_cast<detail::char_t<Range>>(ch))) {
        return unexpected(parse_error::error);
    }

    return ranges::next(range.begin());
}

template <typename Range>
auto read_matching_code_point(Range range, char32_t cp)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    auto val = read_code_point_into(range);
    if (!val.is_valid()) {
        return unexpected(parse_error::error);
    }
    auto decoded_cp = decode_code_point_exhaustive(val.codepoint);
    if (SCN_UNLIKELY(cp != decoded_cp)) {
        return unexpected(parse_error::error);
    }
    return val.iterator;
}

template <typename Range>
auto read_matching_string(Range range,
                          std::basic_string_view<detail::char_t<Range>> str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    SCN_TRY(it, read_exactly_n_code_units(
                    range, static_cast<std::ptrdiff_t>(str.size()))
                    .transform_error(make_eof_parse_error));

    auto sv = make_contiguous_buffer(ranges::subrange{range.begin(), it});
    if (SCN_UNLIKELY(sv.view() != str)) {
        return unexpected(parse_error::error);
    }
    return it;
}

template <typename Range>
auto read_matching_string_classic(Range range, std::string_view str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    SCN_TRY(it, read_exactly_n_code_units(
                    range, static_cast<std::ptrdiff_t>(str.size()))
                    .transform_error(make_eof_parse_error));

    if constexpr (std::is_same_v<detail::char_t<Range>, char>) {
        auto sv = make_contiguous_buffer(ranges::subrange{range.begin(), it});
        if (SCN_UNLIKELY(sv.view() != str)) {
            return unexpected(parse_error::error);
        }
        return it;
    }
    else {
        auto range_it = range.begin();
        for (size_t i = 0; i < str.size(); ++i, (void)++range_it) {
            if (SCN_UNLIKELY(*range_it !=
                             static_cast<detail::char_t<Range>>(str[i]))) {
                return unexpected(parse_error::error);
            }
        }
        return it;
    }
}

// Ripped from fast_float
constexpr bool fast_streq_nocase(const char* a, const char* b, size_t len)
{
    unsigned char running_diff{0};
    for (size_t i = 0; i < len; ++i) {
        running_diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return running_diff == 0 || running_diff == 32;
}

template <typename Range>
auto read_matching_string_classic_nocase(Range range, std::string_view str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    using char_type = detail::char_t<Range>;

    if constexpr (ranges::contiguous_range<Range> &&
                  std::is_same_v<char_type, char>) {
        if (range.size() < str.size()) {
            return unexpected(make_eof_parse_error(eof_error::eof));
        }
        if (!fast_streq_nocase(range.data(), str.data(), str.size())) {
            return unexpected(parse_error::error);
        }
        return ranges::next(range.begin(),
                            static_cast<std::ptrdiff_t>(str.size()));
    }
    else {
        auto ascii_tolower = [](char_type ch) -> char_type {
            if (ch < 'A' || ch > 'Z') {
                return ch;
            }
            return static_cast<char_type>(ch +
                                          static_cast<char_type>('a' - 'A'));
        };

        SCN_TRY(it, read_exactly_n_code_units(
                        range, static_cast<std::ptrdiff_t>(str.size()))
                        .transform_error(make_eof_parse_error));

        if (SCN_UNLIKELY(!std::equal(
                range.begin(), it, str.begin(), [&](auto a, auto b) {
                    return ascii_tolower(a) ==
                           static_cast<detail::char_t<Range>>(b);
                }))) {
            return unexpected(parse_error::error);
        }

        return it;
    }
}

template <typename Range>
auto read_one_of_code_unit(Range range, std::string_view str)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
        return unexpected(make_eof_parse_error(e));
    }

    for (auto ch : str) {
        if (*range.begin() == static_cast<detail::char_t<Range>>(ch)) {
            return std::next(range.begin());
        }
    }

    return unexpected(parse_error::error);
}

template <typename Range, template <class> class Expected, typename Iterator>
auto apply_opt(Expected<Iterator>&& result, Range range)
    -> std::enable_if_t<detail::is_expected<Expected<Iterator>>::value,
                        ranges::const_iterator_t<Range>>
{
    if (!result) {
        return range.begin();
    }
    return *result;
}

/////////////////////////////////////////////////////////////////
// Text width calculation
/////////////////////////////////////////////////////////////////

constexpr std::size_t calculate_text_width_for_fmt_v10(char32_t cp)
{
    if (cp >= 0x1100 &&
        (cp <= 0x115f ||  // Hangul Jamo init. consonants
         cp == 0x2329 ||  // LEFT-POINTING ANGLE BRACKET
         cp == 0x232a ||  // RIGHT-POINTING ANGLE BRACKET
         // CJK ... Yi except IDEOGRAPHIC HALF FILL SPACE:
         (cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f) ||
         (cp >= 0xac00 && cp <= 0xd7a3) ||    // Hangul Syllables
         (cp >= 0xf900 && cp <= 0xfaff) ||    // CJK Compatibility Ideographs
         (cp >= 0xfe10 && cp <= 0xfe19) ||    // Vertical Forms
         (cp >= 0xfe30 && cp <= 0xfe6f) ||    // CJK Compatibility Forms
         (cp >= 0xff00 && cp <= 0xff60) ||    // Fullwidth Forms
         (cp >= 0xffe0 && cp <= 0xffe6) ||    // Fullwidth Forms
         (cp >= 0x20000 && cp <= 0x2fffd) ||  // CJK
         (cp >= 0x30000 && cp <= 0x3fffd) ||
         // Miscellaneous Symbols and Pictographs + Emoticons:
         (cp >= 0x1f300 && cp <= 0x1f64f) ||
         // Supplemental Symbols and Pictographs:
         (cp >= 0x1f900 && cp <= 0x1f9ff))) {
        return 2;
    }
    return 1;
}

constexpr std::size_t calculate_valid_text_width(char32_t cp)
{
    return calculate_text_width_for_fmt_v10(cp);
}

template <typename CharT>
std::size_t calculate_valid_text_width(std::basic_string_view<CharT> input)
{
    size_t count{0};
    for_each_code_point_valid(input, [&count](char32_t cp) {
        count += calculate_text_width_for_fmt_v10(cp);
    });
    return count;
}

constexpr std::size_t calculate_text_width(char32_t cp)
{
    return calculate_text_width_for_fmt_v10(cp);
}

template <typename CharT>
std::size_t calculate_text_width(std::basic_string_view<CharT> input)
{
    size_t count{0};
    for_each_code_point(input, [&count](char32_t cp) {
        count += calculate_text_width_for_fmt_v10(cp);
    });
    return count;
}

namespace counted_width_iterator_impl {
template <typename It, typename S>
class counted_width_iterator {
    static_assert(ranges::forward_iterator<It>);
    static_assert(ranges::sentinel_for<S, It>);

    template <typename OtherIt, typename OtherS>
    friend class counted_width_iterator;

public:
    using iterator = It;
    using sentinel = S;
    using value_type = ranges::iter_value_t<It>;
    using pointer = value_type*;
    using reference = value_type&;
    using difference_type = ranges::iter_difference_t<It>;
    using iterator_category =
        std::conditional_t<ranges::bidirectional_iterator<It>,
                           std::bidirectional_iterator_tag,
                           std::forward_iterator_tag>;

    constexpr counted_width_iterator() = default;

    constexpr counted_width_iterator(iterator x, sentinel s, difference_type n)
        : m_current(x), m_end(s), m_count(n)
    {
    }

    template <typename OtherIt,
              typename OtherS,
              std::enable_if_t<std::is_convertible_v<OtherIt, It> &&
                               std::is_convertible_v<OtherS, S>>* = nullptr>
    constexpr counted_width_iterator(
        const counted_width_iterator<OtherIt, OtherS>& other)
        : m_current(other.m_current),
          m_end(other.m_end),
          m_count(other.m_count),
          m_multibyte_left(other.m_multibyte_left)
    {
    }

    template <typename OtherIt, typename OtherS>
    constexpr auto operator=(
        const counted_width_iterator<OtherIt, OtherS>& other)
        -> std::enable_if_t<std::is_convertible_v<OtherIt, It> &&
                                std::is_convertible_v<OtherS, S>,
                            counted_width_iterator&>
    {
        m_current = other.m_current;
        m_end = other.m_end;
        m_count = other.m_count;
        m_multibyte_left = other.m_multibyte_left;
        return *this;
    }

    constexpr It base() const
    {
        return m_current;
    }
    constexpr difference_type count() const
    {
        return m_count;
    }
    constexpr difference_type multibyte_left() const
    {
        return m_multibyte_left;
    }

    bool is_current_double_wide() const
    {
        assert(count() != 0 || multibyte_left() != 0);
        return _get_width_at_current_cp_start(_get_cp_length_at_current()) == 2;
    }

    constexpr decltype(auto) operator*()
    {
        return *m_current;
    }
    constexpr decltype(auto) operator*() const
    {
        return *m_current;
    }

    constexpr counted_width_iterator& operator++()
    {
        SCN_EXPECT(m_current != m_end);
        _increment_current();
        return *this;
    }

    constexpr counted_width_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    template <typename Iter = It>
    constexpr auto operator--()
        -> std::enable_if_t<ranges::bidirectional_iterator<Iter>,
                            counted_width_iterator&>
    {
        _decrement_current();
        return *this;
    }

    template <typename Iter = It>
    constexpr auto operator--(int)
        -> std::enable_if_t<ranges::bidirectional_iterator<Iter>,
                            counted_width_iterator>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    // TODO: optimize, make better than forward, if possible
#if 0
                template <typename Iter = It>
                constexpr auto operator+(difference_type n) -> std::enable_if_t<
                    ranges_std::random_access_iterator<Iter>,
                    counted_width_iterator>
                {
                    // TODO
                    return counted_width_iterator(m_current + n, m_count - n);
                }

                template <typename Iter = It,
                          std::enable_if_t<ranges_std::random_access_iterator<
                              Iter>>* = nullptr>
                friend constexpr counted_width_iterator operator+(
                    ranges_std::iter_difference_t<Iter> n,
                    const counted_width_iterator<Iter>& x)
                {
                    return x + n;
                }

                template <typename Iter = It>
                constexpr auto operator+=(difference_type n)
                    -> std::enable_if_t<
                        ranges_std::random_access_iterator<Iter>,
                        counted_width_iterator&>
                {
                    // TODO
                    m_current += n;
                    m_count -= n;
                    return *this;
                }

                template <typename Iter = It>
                constexpr auto operator-(difference_type n) -> std::enable_if_t<
                    ranges_std::random_access_iterator<Iter>,
                    counted_width_iterator>
                {
                    // TODO
                    return counted_width_iterator(m_current - n, m_count + n);
                }

                template <typename Iter = It,
                          std::enable_if_t<ranges_std::random_access_iterator<
                              Iter>>* = nullptr>
                constexpr decltype(auto) operator[](difference_type n) const
                {
                    return m_current[n];
                }
#endif

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator==(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) == SCN_DECLVAL(const OtherIt&))
    {
        return a.m_current == b.m_current;
    }
    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator!=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) == SCN_DECLVAL(const OtherIt&))
    {
        return !(a == b);
    }

    friend constexpr bool operator==(const counted_width_iterator& x,
                                     ranges::default_sentinel_t)
    {
        return (x.count() == 0 && x.multibyte_left() == 0) ||
               (x.count() == 1 && x.multibyte_left() == 0 &&
                x.is_current_double_wide());
    }
    friend constexpr bool operator==(ranges::default_sentinel_t s,
                                     const counted_width_iterator& x)
    {
        return x == s;
    }

    friend constexpr bool operator!=(const counted_width_iterator& a,
                                     ranges::default_sentinel_t b)
    {
        return !(a == b);
    }
    friend constexpr bool operator!=(ranges::default_sentinel_t a,
                                     const counted_width_iterator& b)
    {
        return !(a == b);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator<(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
    {
        if (a.count() == b.count()) {
            return a.multibyte_left() > b.multibyte_left();
        }

        return a.count() > b.count();
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator>(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
    {
        return !(b < a);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator<=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
    {
        return !(b < a);
    }

    template <typename OtherIt, typename OtherS>
    friend constexpr auto operator>=(
        const counted_width_iterator& a,
        const counted_width_iterator<OtherIt, OtherS>& b)
        -> decltype(SCN_DECLVAL(const It&) < SCN_DECLVAL(const OtherIt&))
    {
        return !(a < b);
    }

#if 0
                template <typename OtherIt, typename OtherS>
                friend constexpr auto operator-(
                    const counted_width_iterator& a,
                    const counted_width_iterator<OtherIt, OtherS>& b)
                    -> std::enable_if_t<ranges_std::common_with<OtherIt, It>,
                                        ranges_std::iter_difference_t<OtherIt>>
                {
                    // TODO
                }

                friend constexpr ranges_std::iter_difference_t<It> operator-(
                    const counted_width_iterator& x,
                    ranges_std::default_sentinel_t)
                {
                    // TODO
                }

                friend constexpr ranges_std::iter_difference_t<It> operator-(
                    ranges_std::default_sentinel_t,
                    const counted_width_iterator& x)
                {
                    // TODO
                }
#endif

#if 0
                template <typename Iter = It>
                constexpr auto operator-=(difference_type n)
                    -> std::enable_if_t<
                        ranges_std::random_access_iterator<Iter>,
                        counted_width_iterator&>
                {
                    // TODO
                    m_current -= n;
                    m_count += n;
                    return *this;
                }
#endif

private:
    difference_type _get_cp_length_at_current() const
    {
        return static_cast<difference_type>(
            detail::code_point_length_by_starting_code_unit(*m_current));
    }

    difference_type _get_width_at_current_cp_start(difference_type cplen) const
    {
        if (SCN_UNLIKELY(cplen == 0)) {
            return 0;
        }

        if (cplen == 1) {
            SCN_EXPECT(m_current != m_end);
            auto cp = static_cast<char32_t>(*m_current);
            return static_cast<difference_type>(calculate_valid_text_width(cp));
        }

        auto r = read_exactly_n_code_units(ranges::subrange{m_current, m_end},
                                           cplen);
        if (SCN_UNLIKELY(!r)) {
            return 0;
        }

        auto cp_str = std::basic_string<value_type>{m_current, *r};
        return static_cast<difference_type>(calculate_text_width(
            std::basic_string_view<value_type>{cp_str.data(), cp_str.size()}));
    }

    void _increment_current()
    {
        if (m_multibyte_left == 0) {
            auto cplen = _get_cp_length_at_current();
            m_multibyte_left = cplen - 1;
            m_count -= _get_width_at_current_cp_start(cplen);
        }
        else {
            --m_multibyte_left;
        }

        ++m_current;
    }

    void _decrement_current()
    {
        --m_current;

        auto cplen = _get_cp_length_at_current();
        if (cplen == 0) {
            ++m_multibyte_left;
        }
        else {
            m_count += _get_width_at_current_cp_start(cplen);
            m_multibyte_left = cplen - 1;
        }
    }

    It m_current{};
    S m_end{};
    difference_type m_count{0};
    difference_type m_multibyte_left{0};
};

template <typename I, typename S>
counted_width_iterator(I, S, ranges::iter_difference_t<I>)
    -> counted_width_iterator<I, S>;
}  // namespace counted_width_iterator_impl

using counted_width_iterator_impl::counted_width_iterator;

template <typename View, typename = void>
struct take_width_view_storage;

template <typename View>
struct take_width_view_storage<View,
                               std::enable_if_t<ranges::borrowed_range<View>>> {
    take_width_view_storage(const View& v) : view(v) {}

    const View& get() const
    {
        return view;
    }

    View view;
};

template <typename View>
struct take_width_view_storage<
    View,
    std::enable_if_t<!ranges::borrowed_range<View>>> {
    take_width_view_storage(const View& v) : view(&v) {}

    const View& get() const
    {
        return *view;
    }

    const View* view;
};

template <typename View>
class take_width_view : public ranges::view_interface<take_width_view<View>> {
    template <bool IsConst>
    class sentinel {
        friend class sentinel<!IsConst>;
        using Base = std::conditional_t<IsConst, const View, View>;
        using CWI = counted_width_iterator<ranges::iterator_t<Base>,
                                           ranges::sentinel_t<Base>>;
        using underlying = ranges::sentinel_t<Base>;

    public:
        constexpr sentinel() = default;

        constexpr explicit sentinel(underlying s) : m_end(SCN_MOVE(s)) {}

        template <
            typename S,
            std::enable_if_t<std::is_same_v<S, sentinel<!IsConst>>>* = nullptr,
            bool C = IsConst,
            typename VV = View,
            std::enable_if_t<C && std::is_convertible_v<ranges::sentinel_t<VV>,
                                                        underlying>>* = nullptr>
        constexpr explicit sentinel(S s) : m_end(SCN_MOVE(s.m_end))
        {
        }

        constexpr underlying base() const
        {
            return m_end;
        }

        friend constexpr bool operator==(const CWI& y, const sentinel& x)
        {
            return (y.count() == 0 && y.multibyte_left() == 0) ||
                   y.base() == x.m_end ||
                   (y.count() == 1 && y.multibyte_left() == 0 &&
                    y.is_current_double_wide());
        }

        friend constexpr bool operator==(const sentinel& x, const CWI& y)
        {
            return y == x;
        }

        friend constexpr bool operator!=(const CWI& y, const sentinel& x)
        {
            return !(y == x);
        }

        friend constexpr bool operator!=(const sentinel& x, const CWI& y)
        {
            return !(y == x);
        }

    private:
        SCN_NO_UNIQUE_ADDRESS underlying m_end{};
    };

public:
    using value_type = ranges::range_value_t<View>;

    take_width_view() = default;

    constexpr take_width_view(const View& base, std::ptrdiff_t count)
        : m_base(base), m_count(count)
    {
    }

    constexpr View base() const
    {
        return m_base;
    }

    constexpr auto begin() const
    {
        return counted_width_iterator{m_base.get().begin(), m_base.get().end(),
                                      m_count};
    }

    constexpr auto end() const
    {
        return sentinel<true>{m_base.get().end()};
    }

private:
    take_width_view_storage<View> m_base{};
    std::ptrdiff_t m_count{0};
};

template <typename R>
take_width_view(R&&, std::ptrdiff_t) -> take_width_view<R>;

struct _take_width_fn {
    template <typename R>
    constexpr auto operator()(const R& r, std::ptrdiff_t n) const
        -> decltype(take_width_view{r, n})
    {
        return take_width_view{r, n};
    }
};

inline constexpr _take_width_fn take_width{};
}  // namespace impl

namespace ranges {
template <typename R>
inline constexpr bool enable_borrowed_range<::scn::impl::take_width_view<R>> =
    enable_borrowed_range<R>;
}

/////////////////////////////////////////////////////////////////
// contiguous_scan_context
/////////////////////////////////////////////////////////////////

namespace impl {

template <typename CharT>
class basic_contiguous_scan_context
    : public detail::scan_context_base<basic_scan_args<
          basic_scan_context<detail::buffer_range_tag, CharT>>> {
    using base = detail::scan_context_base<
        basic_scan_args<basic_scan_context<detail::buffer_range_tag, CharT>>>;

    using parent_context_type =
        basic_scan_context<detail::buffer_range_tag, CharT>;
    using args_type = basic_scan_args<parent_context_type>;
    using arg_type = basic_scan_arg<parent_context_type>;

public:
    using char_type = CharT;
    using range_type = ranges::subrange<const char_type*, const char_type*>;
    using iterator = const char_type*;
    using sentinel = const char_type*;
    using parse_context_type = basic_scan_parse_context<char_type>;

    template <typename Range,
              std::enable_if_t<ranges::contiguous_range<Range> &&
                               ranges::borrowed_range<Range>>* = nullptr>
    constexpr basic_contiguous_scan_context(Range&& r,
                                            args_type a,
                                            detail::locale_ref loc = {})
        : base(SCN_MOVE(a), loc),
          m_original_begin(ranges::begin(r)),
          m_current(ranges::begin(r)),
          m_end(ranges::end(r))
    {
    }

    constexpr iterator begin() const
    {
        return m_current;
    }

    constexpr sentinel end() const
    {
        return m_end;
    }

    constexpr iterator original_begin() const
    {
        return m_original_begin;
    }

    constexpr auto range() const
    {
        return ranges::subrange{begin(), end()};
    }

    void advance_to(iterator it)
    {
        SCN_EXPECT(it <= end());
        if constexpr (detail::is_comparable_with_nullptr<iterator>) {
            if (it == nullptr) {
                it = end();
            }
        }
        m_current = SCN_MOVE(it);
    }

    void advance_to(const typename parent_context_type::iterator& it)
    {
        SCN_EXPECT(it.position() <= std::distance(m_original_begin, m_end));
        m_current = m_original_begin + it.position();
    }

private:
    iterator m_original_begin;
    iterator m_current;
    SCN_NO_UNIQUE_ADDRESS sentinel m_end{};
};

struct reader_error_handler {
    constexpr void on_error(const char* msg)
    {
        SCN_UNLIKELY_ATTR
        m_msg = msg;
    }
    explicit constexpr operator bool() const
    {
        return m_msg == nullptr;
    }

    const char* m_msg{nullptr};
};

/////////////////////////////////////////////////////////////////
// General reading support
/////////////////////////////////////////////////////////////////

template <typename SourceRange>
auto skip_classic_whitespace(const SourceRange& range,
                             bool allow_exhaustion = false)
    -> eof_expected<ranges::const_iterator_t<SourceRange>>
{
    if (!allow_exhaustion) {
        auto it = read_while_classic_space(range);
        if (auto e = eof_check(ranges::subrange{it, range.end()});
            SCN_UNLIKELY(!e)) {
            return unexpected(e);
        }

        return it;
    }

    return read_while_classic_space(range);
}

template <typename SourceCharT, typename DestCharT>
scan_expected<void> transcode_impl(std::basic_string_view<SourceCharT> src,
                                   std::basic_string<DestCharT>& dst)
{
    dst.clear();
    transcode_valid_to_string(src, dst);
    return {};
}

template <typename SourceCharT, typename DestCharT>
scan_expected<void> transcode_if_necessary(
    const contiguous_range_factory<SourceCharT>& source,
    std::basic_string<DestCharT>& dest)
{
    if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
        dest.assign(source.view());
    }
    else {
        return transcode_impl(source.view(), dest);
    }

    return {};
}

template <typename SourceCharT, typename DestCharT>
scan_expected<void> transcode_if_necessary(
    contiguous_range_factory<SourceCharT>&& source,
    std::basic_string<DestCharT>& dest)
{
    if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
        if (source.stores_allocated_string()) {
            dest.assign(SCN_MOVE(source.get_allocated_string()));
        }
        else {
            dest.assign(source.view());
        }
    }
    else {
        return transcode_impl(source.view(), dest);
    }

    return {};
}

template <typename SourceCharT, typename DestCharT>
scan_expected<void> transcode_if_necessary(
    string_view_wrapper<SourceCharT> source,
    std::basic_string<DestCharT>& dest)
{
    if constexpr (std::is_same_v<SourceCharT, DestCharT>) {
        dest.assign(source.view());
    }
    else {
        return transcode_impl(source.view(), dest);
    }

    return {};
}

/////////////////////////////////////////////////////////////////
// Reader base classes etc.
/////////////////////////////////////////////////////////////////

template <typename Derived, typename CharT>
class reader_base {
public:
    using char_type = CharT;

    constexpr reader_base() = default;

    bool skip_ws_before_read() const
    {
        return true;
    }

    scan_expected<void> check_specs(const detail::format_specs& specs)
    {
        reader_error_handler eh{};
        get_derived().check_specs_impl(specs, eh);
        if (SCN_UNLIKELY(!eh)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_format_string, eh.m_msg);
        }
        return {};
    }

private:
    Derived& get_derived()
    {
        return static_cast<Derived&>(*this);
    }
    const Derived& get_derived() const
    {
        return static_cast<const Derived&>(*this);
    }
};

template <typename CharT>
class reader_impl_for_monostate {
public:
    constexpr reader_impl_for_monostate() = default;

    bool skip_ws_before_read() const
    {
        return true;
    }

    static scan_expected<void> check_specs(const detail::format_specs&)
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Range>
    auto read_default(Range, monostate&, detail::locale_ref)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    template <typename Range>
    auto read_specs(Range,
                    const detail::format_specs&,
                    monostate&,
                    detail::locale_ref)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }
};

/////////////////////////////////////////////////////////////////
// Numeric reader support
/////////////////////////////////////////////////////////////////

enum class sign_type { default_sign = -1, minus_sign = 0, plus_sign = 1 };

inline constexpr std::array<uint8_t, 256> char_to_int_table = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255,
    255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
    20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
    35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,
    18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
    33,  34,  35,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255};

SCN_NODISCARD SCN_FORCE_INLINE constexpr uint8_t char_to_int(char ch)
{
    return char_to_int_table[static_cast<unsigned char>(ch)];
}
SCN_NODISCARD SCN_FORCE_INLINE constexpr uint8_t char_to_int(wchar_t ch)
{
#if WCHAR_MIN < 0
    if (ch >= 0 && ch <= 255) {
#else
    if (ch <= 255) {
#endif
        return char_to_int(static_cast<char>(ch));
    }
    return 255;
}

template <typename Range>
auto parse_numeric_sign(Range range)
    -> eof_expected<std::pair<ranges::const_iterator_t<Range>, sign_type>>
{
    auto r = read_one_of_code_unit(range, "+-");
    if (!r) {
        if (r.error() == parse_error::error) {
            return std::pair{range.begin(), sign_type::default_sign};
        }
        return unexpected(eof_error::eof);
    }

    auto& it = *r;
    if (*range.begin() == '-') {
        return std::pair{it, sign_type::minus_sign};
    }
    return std::pair{it, sign_type::plus_sign};
}

template <typename CharT>
class numeric_reader {
public:
    contiguous_range_factory<CharT> m_buffer{};
};

/////////////////////////////////////////////////////////////////
// Integer reader
/////////////////////////////////////////////////////////////////

template <typename Iterator>
struct parse_integer_prefix_result {
    SCN_NO_UNIQUE_ADDRESS Iterator iterator;
    int parsed_base{0};
    sign_type sign{sign_type::default_sign};
    bool is_zero{false};
};

template <typename Range>
auto parse_integer_bin_base_prefix(Range range)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    return read_matching_string_classic_nocase(range, "0b");
}

template <typename Range>
auto parse_integer_hex_base_prefix(Range range)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    return read_matching_string_classic_nocase(range, "0x");
}

template <typename Range>
auto parse_integer_oct_base_prefix(Range range, bool& zero_parsed)
    -> parse_expected<ranges::const_iterator_t<Range>>
{
    if (auto r = read_matching_string_classic_nocase(range, "0o")) {
        return *r;
    }

    if (auto r = read_matching_code_unit(range, '0')) {
        zero_parsed = true;
        return *r;
    }

    return unexpected(parse_error::error);
}

template <typename Range>
auto parse_integer_base_prefix_for_detection(Range range)
    -> std::tuple<ranges::const_iterator_t<Range>, int, bool>
{
    if (auto r = parse_integer_hex_base_prefix(range)) {
        return {*r, 16, false};
    }
    if (auto r = parse_integer_bin_base_prefix(range)) {
        return {*r, 2, false};
    }
    {
        bool zero_parsed{false};
        if (auto r = parse_integer_oct_base_prefix(range, zero_parsed)) {
            return {*r, 8, zero_parsed};
        }
    }
    return {range.begin(), 10, false};
}

template <typename Range>
auto parse_integer_base_prefix(Range range, int base)
    -> std::tuple<ranges::const_iterator_t<Range>, int, bool>
{
    switch (base) {
        case 2:
            // allow 0b/0B
            return {apply_opt(parse_integer_bin_base_prefix(range), range), 2,
                    false};

        case 8: {
            // allow 0o/0O/0
            bool zero_parsed = false;
            auto it = apply_opt(
                parse_integer_oct_base_prefix(range, zero_parsed), range);
            return {it, 8, zero_parsed};
        }

        case 16:
            // allow 0x/0X
            return {apply_opt(parse_integer_hex_base_prefix(range), range), 16,
                    false};

        case 0:
            // detect base
            return parse_integer_base_prefix_for_detection(range);

        default:
            // no base prefix allowed
            return {range.begin(), base, false};
    }
}

template <typename Range>
auto parse_integer_prefix(Range range, int base) -> eof_expected<
    parse_integer_prefix_result<ranges::const_iterator_t<Range>>>
{
    SCN_TRY(sign_result, parse_numeric_sign(range));
    auto [base_prefix_begin_it, sign] = sign_result;

    auto [digits_begin_it, parsed_base, parsed_zero] =
        parse_integer_base_prefix(
            ranges::subrange{base_prefix_begin_it, range.end()}, base);

    if (parsed_zero) {
        if (digits_begin_it == range.end() ||
            char_to_int(*digits_begin_it) >= 8) {
            digits_begin_it = base_prefix_begin_it;
        }
        else {
            parsed_zero = false;
        }
    }
    else {
        if (digits_begin_it == range.end() ||
            char_to_int(*digits_begin_it) >= parsed_base) {
            digits_begin_it = base_prefix_begin_it;
        }
    }

    if (sign == sign_type::default_sign) {
        sign = sign_type::plus_sign;
    }
    return parse_integer_prefix_result<ranges::const_iterator_t<Range>>{
        digits_begin_it, parsed_base, sign, parsed_zero};
}

template <typename Range>
auto parse_integer_digits_without_thsep(Range range, int base)
    -> scan_expected<ranges::const_iterator_t<Range>>
{
    SCN_UNUSED(base);
    using char_type = detail::char_t<Range>;

    if constexpr (ranges::contiguous_range<Range>) {
        if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Failed to parse integer: No digits found");
        }
        return range.end();
    }
    else {
        return read_while1_code_unit(range,
                                     [&](char_type ch) noexcept {
                                         return char_to_int(ch) < base;
                                     })
            .transform_error(map_parse_error_to_scan_error(
                scan_error::invalid_scanned_value,
                "Failed to parse integer: No digits found"));
    }
}

template <typename Range, typename CharT>
auto parse_integer_digits_with_thsep(
    Range range,
    int base,
    const localized_number_formatting_options<CharT>& locale_options)
    -> scan_expected<std::tuple<ranges::const_iterator_t<Range>,
                                std::basic_string<CharT>,
                                std::string>>
{
    std::basic_string<CharT> output;
    std::string thsep_indices;
    auto it = range.begin();
    bool digit_matched = false;
    for (; it != range.end(); ++it) {
        if (*it == locale_options.thousands_sep) {
            thsep_indices.push_back(
                static_cast<char>(ranges::distance(range.begin(), it)));
        }
        else if (char_to_int(*it) >= base) {
            break;
        }
        else {
            output.push_back(*it);
            digit_matched = true;
        }
    }
    if (SCN_UNLIKELY(!digit_matched)) {
        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Failed to parse integer: No digits found");
    }
    return std::tuple{it, output, thsep_indices};
}

template <typename CharT, typename T>
SCN_PUBLIC auto parse_integer_value(std::basic_string_view<CharT> source,
                                    T& value,
                                    sign_type sign,
                                    int base)
    -> scan_expected<typename std::basic_string_view<CharT>::iterator>;

template <typename T>
SCN_PUBLIC void parse_integer_value_exhaustive_valid(std::string_view source,
                                                     T& value);

#define SCN_DECLARE_INTEGER_READER_TEMPLATE(CharT, IntT)                    \
    extern template SCN_PUBLIC auto parse_integer_value(                    \
        std::basic_string_view<CharT> source, IntT& value, sign_type sign,  \
        int base)                                                           \
        -> scan_expected<typename std::basic_string_view<CharT>::iterator>; \
    extern template SCN_PUBLIC void parse_integer_value_exhaustive_valid(   \
        std::string_view, IntT&);

#if !SCN_DISABLE_TYPE_SCHAR
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, signed char)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, signed char)
#endif
#if !SCN_DISABLE_TYPE_SHORT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, short)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, short)
#endif
#if !SCN_DISABLE_TYPE_INT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, int)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, int)
#endif
#if !SCN_DISABLE_TYPE_LONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, long)
#endif
#if !SCN_DISABLE_TYPE_LONG_LONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, long long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, long long)
#endif
#if SCN_HAS_INT128 && !SCN_DISABLE_TYPE_INT128
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, int128)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, int128)
#endif
#if !SCN_DISABLE_TYPE_UCHAR
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned char)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned char)
#endif
#if !SCN_DISABLE_TYPE_USHORT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned short)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned short)
#endif
#if !SCN_DISABLE_TYPE_UINT
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned int)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned int)
#endif
#if !SCN_DISABLE_TYPE_ULONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long)
#endif
#if !SCN_DISABLE_TYPE_ULONG_LONG
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, unsigned long long)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, unsigned long long)
#endif
#if SCN_HAS_INT128 && !SCN_DISABLE_TYPE_UINT128
SCN_DECLARE_INTEGER_READER_TEMPLATE(char, uint128)
SCN_DECLARE_INTEGER_READER_TEMPLATE(wchar_t, uint128)
#endif

#undef SCN_DECLARE_INTEGER_READER_TEMPLATE

template <typename CharT>
class reader_impl_for_int
    : public reader_base<reader_impl_for_int<CharT>, CharT> {
public:
    constexpr reader_impl_for_int() = default;

    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_int_type_specs(specs, eh);
    }

    template <typename Range, typename T>
    auto read_default_with_base(Range range, T& value, int base)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_TRY(prefix_result, parse_integer_prefix(range, base)
                                   .transform_error(make_eof_scan_error));

        if constexpr (!std::is_signed_v<T>) {
            if (prefix_result.sign == sign_type::minus_sign) {
                return detail::unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Unexpected '-' sign when parsing an "
                    "unsigned value");
            }
        }

        if (prefix_result.is_zero) {
            value = T{0};
            return std::next(prefix_result.iterator);
        }

        SCN_TRY(after_digits_it,
                parse_integer_digits_without_thsep(
                    ranges::subrange{prefix_result.iterator, range.end()},
                    prefix_result.parsed_base));

        auto buf = make_contiguous_buffer(
            ranges::subrange{prefix_result.iterator, after_digits_it});
        SCN_TRY(result_it,
                parse_integer_value(buf.view(), value, prefix_result.sign,
                                    prefix_result.parsed_base));

        return ranges::next(prefix_result.iterator,
                            ranges::distance(buf.view().begin(), result_it));
    }

    template <typename Range, typename T>
    auto read_default(Range range, T& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        return read_default_with_base(range, value, 10);
    }

    template <typename Range, typename T>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    T& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_TRY(prefix_result, parse_integer_prefix(range, specs.get_base())
                                   .transform_error(make_eof_scan_error));

        if (prefix_result.sign == sign_type::minus_sign) {
            if constexpr (!std::is_signed_v<T>) {
                return detail::unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Unexpected '-' sign when parsing an "
                    "unsigned value");
            }
            else {
                if (specs.type ==
                    detail::presentation_type::int_unsigned_decimal) {
                    return detail::unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "'u'-option disallows negative values");
                }
            }
        }

        if (prefix_result.is_zero) {
            value = T{0};
            return std::next(prefix_result.iterator);
        }

        if (SCN_LIKELY(!specs.localized)) {
            SCN_TRY(after_digits_it,
                    parse_integer_digits_without_thsep(
                        ranges::subrange{prefix_result.iterator, range.end()},
                        prefix_result.parsed_base));

            auto buf = make_contiguous_buffer(
                ranges::subrange{prefix_result.iterator, after_digits_it});
            SCN_TRY(result_it,
                    parse_integer_value(buf.view(), value, prefix_result.sign,
                                        prefix_result.parsed_base));

            return ranges::next(
                prefix_result.iterator,
                ranges::distance(buf.view().begin(), result_it));
        }

        auto locale_options =
#if SCN_DISABLE_LOCALE
            localized_number_formatting_options<CharT>{};
        SCN_UNUSED(loc);
#else
            localized_number_formatting_options<CharT>{loc};
#endif

        SCN_TRY(parse_digits_result,
                parse_integer_digits_with_thsep(
                    ranges::subrange{prefix_result.iterator, range.end()},
                    prefix_result.parsed_base, locale_options));
        const auto& [after_digits_it, nothsep_source, thsep_indices] =
            parse_digits_result;
        SCN_UNUSED(after_digits_it);

        auto nothsep_source_view = std::basic_string_view<CharT>{
            nothsep_source.data(), nothsep_source.size()};
        SCN_TRY(
            nothsep_source_it,
            parse_integer_value(nothsep_source_view, value, prefix_result.sign,
                                prefix_result.parsed_base));

        return ranges::next(
            prefix_result.iterator,
            ranges::distance(nothsep_source_view.begin(), nothsep_source_it) +
                ranges::ssize(thsep_indices));
    }
};

/////////////////////////////////////////////////////////////////
// Custom floating-point parser
/////////////////////////////////////////////////////////////////

struct float_rounding_guard {
    float_rounding_guard()
    {
        m_mode = std::fegetround();
        if (m_mode != FE_TONEAREST) {
            std::fesetround(FE_TONEAREST);
        }
    }

    ~float_rounding_guard()
    {
        if (m_mode != FE_TONEAREST) {
            std::fesetround(m_mode);
        }
    }

private:
    int m_mode{};
};

struct uint128_polyfill {
    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wreorder")

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wreorder-ctor")

#if !SCN_IS_BIG_ENDIAN
    std::uint64_t low{};
    std::uint64_t high{};
#else
    std::uint64_t high{};
    std::uint64_t low{};
#endif

    constexpr uint128_polyfill() = default;

    template <typename T,
              std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> &&
                               sizeof(T) <= 8>* = nullptr>
    constexpr uint128_polyfill(T x)
        : high(0u), low(static_cast<std::uint64_t>(x))
    {
    }

    template <typename T,
              std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> &&
                               (sizeof(T) <= 16 && sizeof(T) > 8)>* = nullptr>
    constexpr uint128_polyfill(T x)
        : high(static_cast<std::uint64_t>(x >> 64u)),
          low(static_cast<std::uint64_t>(x))
    {
    }

    template <typename T, std::enable_if_t<std::is_same_v<T, int>>* = nullptr>
    constexpr explicit uint128_polyfill(T x)
        : uint128_polyfill(static_cast<std::uint64_t>(x))
    {
        SCN_EXPECT(x >= 0);
    }

    constexpr friend bool operator==(const uint128_polyfill& a,
                                     const uint128_polyfill& b)
    {
        return a.high == b.high && a.low == b.low;
    }

    constexpr friend bool operator!=(const uint128_polyfill& a,
                                     const uint128_polyfill& b)
    {
        return !(a == b);
    }

    constexpr friend bool operator<(const uint128_polyfill& a,
                                    const uint128_polyfill& b)
    {
        if (a.high == b.high) {
            return a.low < b.low;
        }
        return a.high < b.high;
    }

    constexpr friend bool operator>(const uint128_polyfill& a,
                                    const uint128_polyfill& b)
    {
        return b < a;
    }

    constexpr friend bool operator<=(const uint128_polyfill& a,
                                     const uint128_polyfill& b)
    {
        return !(b > a);
    }

    constexpr friend bool operator>=(const uint128_polyfill& a,
                                     const uint128_polyfill& b)
    {
        return !(b < a);
    }

    constexpr explicit operator bool() const
    {
        return *this != uint128_polyfill{0u};
    }

    constexpr friend uint128_polyfill operator+(const uint128_polyfill& a,
                                                const uint128_polyfill& b)
    {
        uint128_polyfill result;
        result.high = a.high + b.high + (a.low + b.low < a.low);
        result.low = a.low + b.low;
        return result;
    }

    constexpr friend uint128_polyfill& operator+=(uint128_polyfill& self,
                                                  const uint128_polyfill& x)
    {
        self = self + x;
        return self;
    }

    constexpr friend uint128_polyfill operator-(const uint128_polyfill& a,
                                                const uint128_polyfill& b)
    {
        uint128_polyfill result;
        result.high = a.high - b.high - (a.low - b.low < a.low);
        result.low = a.low - b.low;
        return result;
    }

    constexpr friend uint128_polyfill& operator-=(uint128_polyfill& self,
                                                  const uint128_polyfill& x)
    {
        self = self - x;
        return self;
    }

    constexpr friend uint128_polyfill operator&(const uint128_polyfill& a,
                                                const uint128_polyfill& b)
    {
        uint128_polyfill result;
        result.high = a.high & b.high;
        result.low = a.low & b.low;
        return result;
    }

    constexpr friend uint128_polyfill& operator&=(uint128_polyfill& self,
                                                  const uint128_polyfill& x)
    {
        self = self & x;
        return self;
    }

    constexpr friend uint128_polyfill operator|(const uint128_polyfill& a,
                                                const uint128_polyfill& b)
    {
        uint128_polyfill result;
        result.high = a.high | b.high;
        result.low = a.low | b.low;
        return result;
    }

    constexpr friend uint128_polyfill& operator|=(uint128_polyfill& self,
                                                  const uint128_polyfill& x)
    {
        self = self | x;
        return self;
    }

    constexpr friend uint128_polyfill operator~(const uint128_polyfill& x)
    {
        uint128_polyfill result;
        result.high = ~x.high;
        result.low = ~x.low;
        return result;
    }

    constexpr friend uint128_polyfill operator^(const uint128_polyfill& a,
                                                const uint128_polyfill& b)
    {
        uint128_polyfill result;
        result.high = a.high ^ b.high;
        result.low = a.low ^ b.low;
        return result;
    }

    constexpr friend uint128_polyfill& operator^=(uint128_polyfill& self,
                                                  const uint128_polyfill& x)
    {
        self = self ^ x;
        return self;
    }

    constexpr friend uint128_polyfill operator<<(const uint128_polyfill& a,
                                                 const uint128_polyfill& b)
    {
        if (b.high || b.low >= 128) {
            return {};
        }

        const auto shift = b.low;
        if (shift == 0) {
            return a;
        }

        uint128_polyfill result{};
        if (shift < 64) {
            result.high = (a.high << shift) + (a.low >> (64 - shift));
            result.low = a.low << shift;
        }
        else {
            result.high = a.low << (shift - 64);
        }
        return result;
    }

    constexpr friend uint128_polyfill& operator<<=(uint128_polyfill& self,
                                                   const uint128_polyfill& x)
    {
        self = self << x;
        return self;
    }

    constexpr friend uint128_polyfill operator>>(const uint128_polyfill& a,
                                                 const uint128_polyfill& b)
    {
        if (b.high || b.low >= 128) {
            return {};
        }

        const auto shift = b.low;
        if (shift == 0) {
            return a;
        }

        uint128_polyfill result{};
        if (shift < 64) {
            result.high = a.high >> shift;
            result.low = (a.high << (64 - shift)) + (a.low >> shift);
        }
        else {
            result.low = a.high >> (shift - 64);
        }
        return result;
    }

    constexpr friend uint128_polyfill& operator>>=(uint128_polyfill& self,
                                                   const uint128_polyfill& x)
    {
        self = self >> x;
        return self;
    }

    constexpr explicit operator unsigned() const
    {
        return static_cast<unsigned>(low);
    }

    SCN_CLANG_POP    // -Wreorder-ctor
        SCN_GCC_POP  // -Wreorder
};

#if SCN_HAS_INT128
using uint128_always_t = uint128;
#else
using uint128_always_t = uint128_polyfill;
#endif

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wconversion")

template <typename F>
struct float_traits_impl_f32 {
    using type = F;
    // can hold the significand/nan payload (only) + 4 bits
    using significand_int_type = std::uint32_t;

    static constexpr unsigned exponent_bits = 8;
    static constexpr unsigned significand_bits = 23;
    static constexpr unsigned fraction_bits = significand_bits;

    struct value_repr {
#if !SCN_IS_BIG_ENDIAN
        unsigned significand : 23;
        unsigned exponent : 8;
        unsigned sign : 1;
#else
        unsigned sign : 1;
        unsigned exponent : 8;
        unsigned significand : 23;
#endif

        void apply_exponent(int e)
        {
            exponent = static_cast<unsigned>(e);
        }

        void apply_significand(significand_int_type s)
        {
            significand = s;
        }
    };

    static constexpr unsigned nan_payload_bits = 22;

    struct nan_repr {
#if !SCN_IS_BIG_ENDIAN
        unsigned payload : 22;
        unsigned quiet_nan : 1;
        unsigned exponent : 8;
        unsigned sign : 1;
#else
        unsigned sign : 1;
        unsigned exponent : 8;
        unsigned quiet_nan : 1;
        unsigned payload : 22;
#endif

        void apply_payload(significand_int_type p)
        {
            SCN_EXPECT(quiet_nan == 1);
            SCN_EXPECT(exponent == (1u << exponent_bits) - 1u);
            payload = p;
        }
    };
};

template <typename F>
struct float_traits_impl_f64 {
    using type = F;
    using significand_int_type = std::uint64_t;

    static constexpr unsigned exponent_bits = 11;
    static constexpr unsigned significand_bits = 52;
    static constexpr unsigned fraction_bits = significand_bits;

    struct value_repr {
#if !SCN_IS_BIG_ENDIAN && !SCN_IS_FLOAT_BIG_ENDIAN
        unsigned significand1 : 32;
        unsigned significand0 : 20;
        unsigned exponent : 11;
        unsigned sign : 1;
#elif !SCN_IS_BIG_ENDIAN && SCN_IS_FLOAT_BIG_ENDIAN
        unsigned significand0 : 20;
        unsigned exponent : 11;
        unsigned sign : 1;
        unsigned significand1 : 32;
#else
        unsigned sign : 1;
        unsigned exponent : 11;
        unsigned significand0 : 20;
        unsigned significand1 : 32;
#endif

        void apply_exponent(int e)
        {
            exponent = static_cast<unsigned>(e);
        }

        void apply_significand(significand_int_type s)
        {
            significand0 = static_cast<unsigned>(s >> 32);
            significand1 = static_cast<unsigned>(s);
        }
    };

    static constexpr unsigned nan_payload_bits = 51;

    struct nan_repr {
#if !SCN_IS_BIG_ENDIAN && !SCN_IS_FLOAT_BIG_ENDIAN
        unsigned payload1 : 32;
        unsigned payload0 : 19;
        unsigned quiet_nan : 1;
        unsigned exponent : 11;
        unsigned sign : 1;
#elif !SCN_IS_BIG_ENDIAN && SCN_IS_FLOAT_BIG_ENDIAN
        unsigned payload0 : 20;
        unsigned quiet_nan : 1;
        unsigned exponent : 11;
        unsigned sign : 1;
        unsigned payload1 : 32;
#else
        unsigned sign : 1;
        unsigned exponent : 11;
        unsigned quiet_nan : 1;
        unsigned payload0 : 20;
        unsigned payload1 : 32;
#endif

        void apply_payload(significand_int_type p)
        {
            SCN_EXPECT(quiet_nan == 1);
            SCN_EXPECT(exponent == (1u << exponent_bits) - 1u);
            payload0 = static_cast<unsigned>(p >> 32);
            payload1 = static_cast<unsigned>(p);
        }
    };
};

template <typename F>
struct float_traits_impl_f80 {
    using type = F;
    using significand_int_type = uint128_always_t;

    static constexpr unsigned exponent_bits = 15;
    static constexpr unsigned significand_bits = 64;
    static constexpr unsigned fraction_bits = significand_bits - 1;

    struct value_repr {
#if !SCN_IS_BIG_ENDIAN && !SCN_IS_FLOAT_BIG_ENDIAN
        unsigned significand1 : 32;
        unsigned significand0 : 31;
        unsigned one : 1;
        unsigned exponent : 15;
        unsigned sign : 1;
#elif !SCN_IS_BIG_ENDIAN && SCN_IS_FLOAT_BIG_ENDIAN
        unsigned exponent : 11;
        unsigned sign : 1;
        unsigned padding : 16;
        unsigned significand0 : 31;
        unsigned one : 1;
        unsigned significand1 : 32;
#else
        unsigned padding : 16;
        unsigned sign : 1;
        unsigned exponent : 15;
        unsigned one : 1;
        unsigned significand0 : 31;
        unsigned significand1 : 32;
#endif

        void apply_exponent(int e)
        {
            exponent = static_cast<unsigned>(e);
        }

        void apply_significand(significand_int_type s)
        {
            significand0 = static_cast<unsigned>(s >> 32);
            significand1 = static_cast<unsigned>(s);
            one = exponent != 0;
        }
    };

    static constexpr unsigned nan_payload_bits = 62;

    struct nan_repr {
#if !SCN_IS_BIG_ENDIAN && !SCN_IS_FLOAT_BIG_ENDIAN
        unsigned payload1 : 32;
        unsigned payload0 : 30;
        unsigned quiet_nan : 1;
        unsigned one : 1;
        unsigned exponent : 15;
        unsigned sign : 1;
#elif !SCN_IS_BIG_ENDIAN && SCN_IS_FLOAT_BIG_ENDIAN
        unsigned exponent : 15;
        unsigned sign : 1;
        unsigned padding : 16;
        unsigned payload0 : 30;
        unsigned quiet_nan : 1;
        unsigned one : 1;
        unsigned payload1 : 32;
#else
        unsigned padding : 16;
        unsigned sign : 1;
        unsigned exponent : 15;
        unsigned one : 1;
        unsigned quiet_nan : 1;
        unsigned payload0 : 30;
        unsigned payload1 : 32;
#endif

        void apply_exponent(int e)
        {
            exponent = static_cast<unsigned>(e);
        }

        void apply_payload(significand_int_type p)
        {
            SCN_EXPECT(quiet_nan == 1);
            SCN_EXPECT(exponent == (1u << exponent_bits) - 1u);
            one = 1;
            payload0 = static_cast<unsigned>(p >> 32);
            payload1 = static_cast<unsigned>(p);
        }
    };
};

template <typename F>
struct float_traits_impl_f128 {
    using type = F;
    using significand_int_type = uint128_always_t;

    static constexpr unsigned exponent_bits = 15;
    static constexpr unsigned significand_bits = 112;
    static constexpr unsigned fraction_bits = significand_bits;

    struct value_repr {
#if !SCN_IS_BIG_ENDIAN && !SCN_IS_FLOAT_BIG_ENDIAN
        unsigned significand3 : 32;
        unsigned significand2 : 32;
        unsigned significand1 : 32;
        unsigned significand0 : 16;
        unsigned exponent : 15;
        unsigned sign : 1;
#elif !SCN_IS_BIG_ENDIAN && SCN_IS_FLOAT_BIG_ENDIAN
        unsigned significand0 : 16;
        unsigned exponent : 15;
        unsigned sign : 1;
        unsigned significand1 : 32;
        unsigned significand2 : 32;
        unsigned significand3 : 32;
#else
        unsigned sign : 1;
        unsigned exponent : 15;
        unsigned significand0 : 16;
        unsigned significand1 : 32;
        unsigned significand2 : 32;
        unsigned significand3 : 32;
#endif

        void apply_exponent(int e)
        {
            exponent = static_cast<unsigned>(e);
        }

        void apply_significand(significand_int_type s)
        {
            significand0 = static_cast<unsigned>(s >> 96u);
            significand1 = static_cast<unsigned>(s >> 64u);
            significand2 = static_cast<unsigned>(s >> 32u);
            significand3 = static_cast<unsigned>(s);
        }
    };

    static constexpr unsigned nan_payload_bits = 111;

    struct nan_repr {
#if !SCN_IS_BIG_ENDIAN && !SCN_IS_FLOAT_BIG_ENDIAN
        unsigned payload3 : 32;
        unsigned payload2 : 32;
        unsigned payload1 : 32;
        unsigned payload0 : 15;
        unsigned quiet_nan : 1;
        unsigned exponent : 15;
        unsigned sign : 1;
#elif !SCN_IS_BIG_ENDIAN && SCN_IS_FLOAT_BIG_ENDIAN
        unsigned payload0 : 15;
        unsigned quiet_nan : 1;
        unsigned exponent : 15;
        unsigned sign : 1;
        unsigned payload1 : 32;
        unsigned payload2 : 32;
        unsigned payload3 : 32;
#else
        unsigned sign : 1;
        unsigned exponent : 15;
        unsigned quiet_nan : 1;
        unsigned payload0 : 15;
        unsigned payload1 : 32;
        unsigned payload2 : 32;
        unsigned payload3 : 32;
#endif

        void apply_payload(significand_int_type p)
        {
            SCN_EXPECT(quiet_nan == 1);
            SCN_EXPECT(exponent == (1u << exponent_bits) - 1u);
            payload0 = static_cast<unsigned>(p >> 96u);
            payload1 = static_cast<unsigned>(p >> 64u);
            payload2 = static_cast<unsigned>(p >> 32u);
            payload3 = static_cast<unsigned>(p);
        }
    };
};

template <typename F>
struct float_traits_impl_doubledouble {
    using type = F;
    using significand_int_type = uint128_always_t;

    using base = float_traits_impl_f64<double>;

    static constexpr unsigned exponent_bits = base::exponent_bits * 1;
    static constexpr unsigned significand_bits = base::significand_bits * 2;
    static constexpr unsigned fraction_bits = base::fraction_bits * 2;

    struct value_repr {
        base::value_repr high;
        base::value_repr low;

        void apply_exponent(int e)
        {
            high.apply_exponent(e);
            low.apply_exponent(e);
        }

        void apply_significand(significand_int_type s)
        {
            high.apply_significand(
                static_cast<base::significand_int_type>(s >> 64u));
            low.apply_significand(static_cast<base::significand_int_type>(s));
        }
    };

    static constexpr unsigned nan_payload_bits = base::nan_payload_bits * 2;

    struct nan_repr {
        base::nan_repr high;
        base::nan_repr low;

        void apply_payload(significand_int_type p)
        {
            high.apply_payload(
                static_cast<base::significand_int_type>(p >> 64u));
            low.apply_payload(static_cast<base::significand_int_type>(p));
        }
    };
};

template <typename F>
struct float_traits_impl_f16 {
    using type = F;
    using significand_int_type = std::uint16_t;

    static constexpr unsigned exponent_bits = 5;
    static constexpr unsigned significand_bits = 10;
    static constexpr unsigned fraction_bits = significand_bits;

    struct value_repr {
#if !SCN_IS_BIG_ENDIAN
        unsigned significand : 10;
        unsigned exponent : 5;
        unsigned sign : 1;
#else
        unsigned sign : 1;
        unsigned exponent : 5;
        unsigned significand : 10;
#endif

        void apply_exponent(int e)
        {
            exponent = static_cast<unsigned>(e);
        }

        void apply_significand(significand_int_type s)
        {
            significand = s;
        }
    };

    static constexpr unsigned nan_payload_bits = 9;

    struct nan_repr {
#if !SCN_IS_BIG_ENDIAN
        unsigned payload : 9;
        unsigned quiet_nan : 1;
        unsigned exponent : 5;
        unsigned sign : 1;
#else
        unsigned sign : 1;
        unsigned exponent : 5;
        unsigned quiet_nan : 1;
        unsigned payload : 9;
#endif

        void apply_payload(significand_int_type p)
        {
            SCN_EXPECT(quiet_nan == 1);
            SCN_EXPECT(exponent == (1u << exponent_bits) - 1u);
            payload = p;
        }
    };
};

template <typename F>
struct float_traits_impl_bf16 {
    using type = F;
    using significand_int_type = std::uint16_t;

    static constexpr unsigned exponent_bits = 8;
    static constexpr unsigned significand_bits = 7;
    static constexpr unsigned fraction_bits = significand_bits;

    struct value_repr {
#if !SCN_IS_BIG_ENDIAN
        unsigned significand : 7;
        unsigned exponent : 8;
        unsigned sign : 1;
#else
        unsigned sign : 1;
        unsigned exponent : 8;
        unsigned significand : 7;
#endif

        void apply_exponent(int e)
        {
            exponent = static_cast<unsigned>(e);
        }

        void apply_significand(significand_int_type s)
        {
            significand = s;
        }
    };

    static constexpr unsigned nan_payload_bits = 6;

    struct nan_repr {
#if !SCN_IS_BIG_ENDIAN
        unsigned payload : 6;
        unsigned quiet_nan : 1;
        unsigned exponent : 8;
        unsigned sign : 1;
#else
        unsigned sign : 1;
        unsigned exponent : 8;
        unsigned quiet_nan : 1;
        unsigned payload : 6;
#endif

        void apply_payload(significand_int_type p)
        {
            SCN_EXPECT(quiet_nan == 1);
            SCN_EXPECT(exponent == (1u << exponent_bits) - 1u);
            payload = p;
        }
    };
};

struct float_traits_impl_none_base {};

template <typename...>
struct float_traits_impl_none : float_traits_impl_none_base {};

template <typename F>
struct float_traits;

template <>
struct float_traits<float> : float_traits_impl_f32<float> {};

template <>
struct float_traits<double> : float_traits_impl_f64<double> {};

template <>
struct float_traits<long double>
    : detail::mp_cond<
          // long double is equivalent to a double,
          // true on (non-mingw) Windows, Arm32, and Apple Arm64
          detail::mp_bool<sizeof(double) == sizeof(long double)>,
          detail::mp_defer<float_traits_impl_f64, long double>,
          // x87 long double, on non-Windows x86
          detail::mp_bool<std::numeric_limits<long double>::digits == 64>,
          detail::mp_defer<float_traits_impl_f80, long double>,
          // binary128 long double,
          // true on non-Apple non-Windows Arm64
          detail::mp_bool<std::numeric_limits<long double>::digits == 113>,
          detail::mp_defer<float_traits_impl_f128, long double>,
          // double-double (two regular doubles next to each other),
          // true on PowerPC
          detail::mp_bool<std::numeric_limits<long double>::digits == 106>,
          detail::mp_defer<float_traits_impl_doubledouble, long double>,
          // Exotic long double, no payload setting support
          std::true_type,
          detail::mp_defer<float_traits_impl_none>>::type {};

#if SCN_HAS_STD_F16
template <>
struct float_traits<std::float16_t> : float_traits_impl_f16<std::float16_t> {};
#endif

#if SCN_HAS_STD_F32
template <>
struct float_traits<std::float32_t> : float_traits_impl_f32<std::float32_t> {};
#endif

#if SCN_HAS_STD_F64
template <>
struct float_traits<std::float64_t> : float_traits_impl_f64<std::float64_t> {};
#endif

#if SCN_HAS_STD_F128
template <>
struct float_traits<std::float128_t> : float_traits_impl_f128<std::float128_t> {
};
#endif

#if SCN_HAS_STD_BF16
template <>
struct float_traits<std::bfloat16_t> : float_traits_impl_bf16<std::bfloat16_t> {
};
#endif

SCN_GCC_POP  // -Wconversion

    namespace
{
}

template <typename CharT>
struct float_parts {
    std::basic_string_view<CharT> before_radix_point;
    std::basic_string_view<CharT> after_radix_point;
    std::basic_string_view<CharT> exponent;
    typename std::basic_string_view<CharT>::iterator end;
    bool exponent_positive{true};
};

// Simple Decimal Conversion (SDC), as described in
// https://nigeltao.github.io/blog/2020/parse-number-f64-simple.html

// Table generated with
// ```
// seq 60 | sed 's/^/5^/' | bc |
// awk 'BEGIN{ print "\t{ 0, \"\" }," }
// {
//     log2 = log(2)/log(10)
//     printf("\t{ %d, \"%s\" },\t// * %d\n",
//            int(log2 * NR + 1), $0, 2 ** NR)
// }'
// ```

// clang-format off
constexpr std::pair<std::uint8_t, std::string_view> left_shift_table[] = {
    {0, ""},
    {1, "5"},                                            // * 2
    {1, "25"},                                           // * 4
    {1, "125"},                                          // * 8
    {2, "625"},                                          // * 16
    {2, "3125"},                                         // * 32
    {2, "15625"},                                        // * 64
    {3, "78125"},                                        // * 128
    {3, "390625"},                                       // * 256
    {3, "1953125"},                                      // * 512
    {4, "9765625"},                                      // * 1024
    {4, "48828125"},                                     // * 2048
    {4, "244140625"},                                    // * 4096
    {4, "1220703125"},                                   // * 8192
    {5, "6103515625"},                                   // * 16384
    {5, "30517578125"},                                  // * 32768
    {5, "152587890625"},                                 // * 65536
    {6, "762939453125"},                                 // * 131072
    {6, "3814697265625"},                                // * 262144
    {6, "19073486328125"},                               // * 524288
    {7, "95367431640625"},                               // * 1048576
    {7, "476837158203125"},                              // * 2097152
    {7, "2384185791015625"},                             // * 4194304
    {7, "11920928955078125"},                            // * 8388608
    {8, "59604644775390625"},                            // * 16777216
    {8, "298023223876953125"},                           // * 33554432
    {8, "1490116119384765625"},                          // * 67108864
    {9, "7450580596923828125"},                          // * 134217728
    {9, "37252902984619140625"},                         // * 268435456
    {9, "186264514923095703125"},                        // * 536870912
    {10, "931322574615478515625"},                       // * 1073741824
    {10, "4656612873077392578125"},                      // * 2147483648
    {10, "23283064365386962890625"},                     // * 4294967296
    {10, "116415321826934814453125"},                    // * 8589934592
    {11, "582076609134674072265625"},                    // * 17179869184
    {11, "2910383045673370361328125"},                   // * 34359738368
    {11, "14551915228366851806640625"},                  // * 68719476736
    {12, "72759576141834259033203125"},                  // * 137438953472
    {12, "363797880709171295166015625"},                 // * 274877906944
    {12, "1818989403545856475830078125"},                // * 549755813888
    {13, "9094947017729282379150390625"},                // * 1099511627776
    {13, "45474735088646411895751953125"},               // * 2199023255552
    {13, "227373675443232059478759765625"},              // * 4398046511104
    {13, "1136868377216160297393798828125"},             // * 8796093022208
    {14, "5684341886080801486968994140625"},             // * 17592186044416
    {14, "28421709430404007434844970703125"},            // * 35184372088832
    {14, "142108547152020037174224853515625"},           // * 70368744177664
    {15, "710542735760100185871124267578125"},           // * 140737488355328
    {15, "3552713678800500929355621337890625"},          // * 281474976710656
    {15, "17763568394002504646778106689453125"},         // * 562949953421312
    {16, "88817841970012523233890533447265625"},         // * 1125899906842624
    {16, "444089209850062616169452667236328125"},        // * 2251799813685248
    {16, "2220446049250313080847263336181640625"},       // * 4503599627370496
    {16, "11102230246251565404236316680908203125"},      // * 9007199254740992
    {17, "55511151231257827021181583404541015625"},      // * 18014398509481984
    {17, "277555756156289135105907917022705078125"},     // * 36028797018963968
    {17, "1387778780781445675529539585113525390625"},    // * 72057594037927936
    {18, "6938893903907228377647697925567626953125"},    // * 144115188075855872
    {18, "34694469519536141888238489627838134765625"},   // * 288230376151711744
    {18, "173472347597680709441192448139190673828125"},  // * 576460752303423488
    {19, "867361737988403547205962240695953369140625"},  // * 1152921504606846976
};
// clang-format on

struct high_precision_decimal {
    std::uint32_t num_digits{};
    std::int32_t decimal_point{};
    bool truncated{false};
    std::array<std::uint8_t, 800> digits;

    high_precision_decimal() = default;
    ~high_precision_decimal() = default;

    high_precision_decimal(const high_precision_decimal&) = delete;
    high_precision_decimal(high_precision_decimal&&) = delete;
    high_precision_decimal& operator=(const high_precision_decimal&) = delete;
    high_precision_decimal& operator=(high_precision_decimal&&) = delete;

    template <typename CharT>
    static scan_expected<void> populate(high_precision_decimal& hpd,
                                        const float_parts<CharT>& parts)
    {
        std::size_t idx = 0;
        for (CharT ch : parts.before_radix_point) {
            if (idx < hpd.digits.size()) {
                hpd.digits[idx] = char_to_int(ch);
                SCN_ENSURE(hpd.digits[idx] < std::uint8_t{10});
            }
            else {
                hpd.truncated = true;
            }
            idx++;
        }
        hpd.decimal_point = static_cast<std::int32_t>(idx);

        for (CharT ch : parts.after_radix_point) {
            if (idx < hpd.digits.size()) {
                hpd.digits[idx] = char_to_int(ch);
                SCN_ENSURE(hpd.digits[idx] < std::uint8_t{10});
            }
            else {
                hpd.truncated = true;
            }
            idx++;
        }
        hpd.num_digits = static_cast<std::uint32_t>(idx);

        if (!parts.exponent.empty()) {
            std::int32_t exponent{};
            auto r = parse_integer_value(parts.exponent, exponent,
                                         parts.exponent_positive
                                             ? sign_type::plus_sign
                                             : sign_type::minus_sign,
                                         10);
            if (!r) {
                return detail::unexpected_scan_error(
                    r.error().code(),
                    "Invalid exponent value for floating-point value");
            }

            hpd.decimal_point = static_cast<std::int32_t>(
                static_cast<std::int64_t>(hpd.decimal_point) +
                static_cast<std::int64_t>(exponent));
        }

        hpd.trim();

        return {};
    }

    template <typename T>
    struct rounded_significand_type {
        using type = typename float_traits<T>::significand_int_type;
        type significand;
        bool rounded_up;
    };

    template <typename T>
    SCN_NODISCARD rounded_significand_type<T> rounded_significand() const
    {
        if (num_digits == 0) {
            return {0u, false};
        }
        if (decimal_point < 0) {
            return {0u, false};
        }

        using result_type = typename float_traits<T>::significand_int_type;
        using accumulator_type =
            std::conditional_t<sizeof(result_type) <= sizeof(std::uint64_t),
                               std::uint64_t, uint128_always_t>;

        if (decimal_point >= std::numeric_limits<T>::max_digits10) {
            return {static_cast<result_type>(~accumulator_type{}), false};
        }

        // Build up the integral part,
        // including any implicit trailing zeroes after `num_digits`.
        accumulator_type accumulator{};
        std::size_t i = 0;
        for (; i < static_cast<std::size_t>(decimal_point); ++i) {
            // Multiply by 10 without using operator*,
            // which uint128_polyfill doesn't have.
            // `<< 3` is `* 8`, and `<< 1` is `* 2`
            const auto accumulator_before = accumulator;
            accumulator = (accumulator << 3u) + (accumulator << 1u) +
                          (i < num_digits ? digits[i] : 0u);
            SCN_EXPECT(accumulator > accumulator_before);  // check for overflow
        }

        // Check the decimal part (if any) for rounding
        bool round_up = false;
        if (i < num_digits) {
            round_up = digits[i] >= 5;

            // If the entire decimal part is ".5":
            //  - if we're truncated, round up,
            //  - otherwise, round to even
            if (digits[i] == 5 && num_digits == i + 1) {
                round_up = truncated || (i > 0 && digits[i - 1] % 2 != 0);
            }
        }
        if (round_up) {
            accumulator += 1u;
        }
        return {static_cast<result_type>(accumulator), round_up};
    }

    void trim()
    {
        while (num_digits > 0 && digits[num_digits - 1] == 0) {
            --num_digits;
        }
    }

    void left_shift(std::uint32_t shift)
    {
        if (num_digits == 0) {
            return;
        }

        while (shift > 60u) {
            left_shift(60u);
            shift -= 60u;
        }

        SCN_EXPECT(shift <= 60);

        const auto num_new_digits = count_new_digits(shift);

        std::int32_t read_index = static_cast<std::int32_t>(num_digits) - 1;
        std::uint32_t write_index = num_digits + num_new_digits;
        std::uint64_t accumulator{};

        // Accumulate `accumulator = digit * 2 ^ shift + carry`,
        // write `accumulator % 10` to the write position,
        // carry `accumulator / 10`
        while (read_index >= 0) {
            accumulator += static_cast<std::uint64_t>(
                               digits[static_cast<std::size_t>(read_index)])
                           << shift;

            const auto quotient = accumulator / 10u;
            const auto remainder = accumulator % 10u;

            --write_index;
            if (write_index < digits.size()) {
                digits[write_index] = static_cast<std::uint8_t>(remainder);
            }
            else if (remainder > 0) {
                truncated = true;
            }
            accumulator = quotient;
            --read_index;
        }

        // Drain any remaining carry
        while (accumulator > 0) {
            const auto quotient = accumulator / 10u;
            const auto remainder = accumulator % 10u;

            --write_index;
            if (write_index < digits.size()) {
                digits[write_index] = static_cast<std::uint8_t>(remainder);
            }
            else if (remainder > 0) {
                truncated = true;
            }
            accumulator = quotient;
        }

        num_digits += num_new_digits;
        if (num_digits > static_cast<std::uint32_t>(digits.size())) {
            num_digits = static_cast<std::uint32_t>(digits.size());
        }
        decimal_point += static_cast<std::int32_t>(num_new_digits);

        trim();
    }

    void right_shift(std::uint32_t shift)
    {
        while (shift > 60u) {
            right_shift(60u);
            shift -= 60u;
        }

        SCN_EXPECT(shift <= 60);

        std::uint32_t read_index{};
        std::uint32_t write_index{};
        std::uint64_t accumulator{};

        // Accumulate digits until accumulator >= 2 ^ shift
        // (i.e., we can extract a digit)
        while (accumulator >> shift == 0) {
            if (read_index < num_digits) {
                accumulator = 10 * accumulator + digits[read_index];
            }
            else {
                accumulator *= 10;
            }
            ++read_index;
        }
        // The leading read_index digits collapsed into accumulator
        // without producing output,
        // so the decimal point shifts left by (read_index - 1)
        decimal_point -= static_cast<std::int32_t>(read_index) - 1;

        // Extract quotient digit (accumulator >> shift),
        // carry remainder (accumulator - digit * 2 ^ shift),
        // bring in next output digit
        while (read_index < num_digits) {
            const auto digit = accumulator >> shift;
            accumulator -= digit << shift;
            accumulator = 10 * accumulator + digits[read_index++];
            digits[write_index++] = static_cast<std::uint8_t>(digit);
        }

        // Drain remaining fractional digits
        while (accumulator > 0) {
            const auto digit = accumulator >> shift;
            accumulator -= digit << shift;
            if (write_index < digits.size()) {
                digits[write_index++] = static_cast<std::uint8_t>(digit);
            }
            else if (digit > 0) {
                truncated = true;
            }
            accumulator *= 10;
        }

        num_digits = write_index;
        trim();
    }

private:
    SCN_NODISCARD std::uint32_t count_new_digits(std::uint32_t shift) const
    {
        const auto& [delta, pow5] = left_shift_table[shift];
        for (std::size_t i = 0; i < pow5.size(); ++i) {
            if (i >= num_digits) {
                return delta - 1;
            }
            if (const auto digit = char_to_int(pow5[i]); digits[i] != digit) {
                return digits[i] < digit ? delta - 1 : delta;
            }
        }
        return delta;
    }
};

constexpr std::array<std::uint8_t, 19> powers_of_two = {
    0, 3, 6, 9, 13, 16, 19, 23, 26, 29, 33, 36, 39, 43, 46, 49, 53, 56, 59};

template <typename T>
scan_expected<void> simple_decimal_conversion(T& result,
                                              high_precision_decimal& value)
{
    SCN_EXPECT(std::fegetround() == FE_TONEAREST);

    if (value.num_digits == 0) {
        result = static_cast<T>(0.0);
        return {};
    }

    constexpr std::int32_t exp_bias =
        (1 << (float_traits<T>::exponent_bits - 1)) - 1;
    std::int32_t exp2{};

    // Scale by powers of two (i.e., shift left or right),
    // until we're in the range [1, 2).

    // "[R]epeatedly do Non-Rounding right-shifts until the HPD represents
    // a number less than 1 (i.e.[,] its decimal_point field is non-positive)"
    while (value.decimal_point > 0) {
        const auto shift =
            static_cast<std::size_t>(value.decimal_point) < powers_of_two.size()
                ? powers_of_two[static_cast<std::size_t>(value.decimal_point)]
                : 60u;
        value.right_shift(shift);
        exp2 += static_cast<int32_t>(shift);
    }

    // "Repeatedly do Non-Rounding left-shifts until the HPD represents
    // a number in the range [1/2 ... 1] (i.e.[,] its decimal_point field
    // is zero and its digits[0] is at least 5)"
    SCN_EXPECT(value.num_digits > 0);
    while (value.decimal_point < 0) {
        const auto shift =
            static_cast<std::size_t>(-value.decimal_point) <
                    powers_of_two.size()
                ? powers_of_two[static_cast<std::size_t>(-value.decimal_point)]
                : 60u;
        value.left_shift(shift);
        exp2 -= static_cast<int32_t>(shift);
    }
    // We're in [0.1, 0.5), repeatedly shift left until we're in [0.5, 1)
    while (value.decimal_point == 0 && value.digits[0] < 5u) {
        value.left_shift(1);
        exp2 -= 1;
    }

    // Shift once to get us in [1, 2)
    value.left_shift(1);
    exp2 -= 1;

    constexpr auto max_exponent = exp_bias + 1;
    if (exp2 >= max_exponent) {
        // "If the exponent was too large then the parsed [value] is infinite"
        return detail::unexpected_scan_error(
            scan_error::value_positive_overflow,
            "Parsed float exponent too large");
    }

    auto biased_exponent = exp2 + exp_bias;
    // constexpr auto max_biased_exponent =
    //     (1 << float_traits<T>::exponent_bits) - 1;
    bool was_subnormal = false;

    if (biased_exponent <= 0) {
        // "If the exponent was too small then we would N-R right shift
        // the mantissa (and adjust the biased base-2 exponent) until
        // the exponent was in range (positive).
        // This might end with the mantissa below 1/2,
        // so that the parsed [value] is zero,
        // but otherwise we'd return a subnormal [value].

        was_subnormal = true;
        while (biased_exponent < 0) {
            value.right_shift(1);
            biased_exponent += 1;
        }
        value.right_shift(1);
    }

    // "Multiply by (2 ** precision) and round to get [the] mantissa"
    value.left_shift(float_traits<T>::fraction_bits);
    auto [significand, significand_rounded_up] = value.rounded_significand<T>();

    if (was_subnormal &&
        (significand >> float_traits<T>::fraction_bits) != 0u) {
        // Previously subnormal number became normal with the right-shifts
        ++biased_exponent;
    }

    // Check that the mantissa fits (rounding may have added an extra bit).
    // If it doesn't, shift down by one.
    // if (significand_rounded_up &&
    //    (significand == 0u || count_trailing_zeroes(significand) ==
    //                              float_traits<T>::fraction_bits)) {
    //    significand >>= 1u;
    //    ++biased_exponent;
    //    if (biased_exponent >= max_biased_exponent) {
    //        return detail::unexpected_scan_error(
    //            scan_error::value_positive_overflow,
    //            "Parsed float exponent too large");
    //    }
    //}

    if (significand == 0u && biased_exponent == 0 && value.decimal_point < 0) {
        // Value rounded to zero, but the input was non-zero
        //  -> underflow
        return detail::unexpected_scan_error(
            scan_error::value_positive_underflow,
            "Parsed float rounds to zero, but is non-zero");
    }

    // Build resulting value
    using repr_type = typename float_traits<T>::value_repr;
    repr_type repr{};
    SCN_ENSURE(biased_exponent >= 0);
    repr.apply_exponent(biased_exponent);
    repr.apply_significand(significand);

    std::memcpy(&result, &repr, sizeof(repr_type));
    return {};
}

/////////////////////////////////////////////////////////////////
// Floating-point parser implementation
/////////////////////////////////////////////////////////////////

namespace float_conversion {
enum flags_type {
    allow_hex = 1,
    allow_scientific = 2,
    allow_fixed = 4,
};

template <typename CharT>
struct parts {
    using string_view_type = std::basic_string_view<CharT>;

    string_view_type prefix;
    string_view_type before_radix_point;
    string_view_type after_radix_point;
    string_view_type exponent;

    CharT exponent_delimiter{};
    CharT exponent_sign{};
};

enum class kind_type {
    decimal,              // fixed or scientific
    fixed,                // aaa.bbb (no exponent)
    scientific,           // aaa.bbb(e|E)ccc
    hex_without_prefix,   // aaa.bbb[(p|P)ccc]
    hex_with_prefix,      // 0(x|X)aaa.bbb[(p|P)ccc]
    inf_short,            // inf
    inf_long,             // infinity
    nan_without_payload,  // nan
    nan_with_payload,     // nan(abc)
};

template <typename CharT>
struct options {
    unsigned flags{allow_hex | allow_scientific | allow_fixed};
    CharT radix_point{'.'};
    std::optional<CharT> group_separator{};
    std::string grouping{};
};

template <typename Iterator, typename Sentinel>
auto parse_nan(ranges::subrange<Iterator, Sentinel> source)
    -> scan_expected<Iterator>
{
    SCN_EXPECT(source.begin() != source.end());
    using char_type = ranges::iter_value_t<Iterator>;

    auto it = source.begin();
    const auto after_nan = read_matching_string_classic_nocase(source, "nan");
    if (!after_nan) {
        // Not a NaN, detected by not advancing the start
        return source.begin();
    }

    it = *after_nan;
    if (it == source.end() || *it != char_type{'('}) {
        return it;
    }
    ++it;

    for (; it != source.end(); ++it) {
        if (char_to_int(*it) >= 36 && *it != char_type{'_'}) {
            break;
        }
    }
    if (it == source.end() || *it != char_type{')'}) {
        return detail::unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Invalid NaN payload value");
    }
    SCN_ENSURE(*it == char_type{')'});
    ++it;
    return it;
}

template <typename Iterator>
struct prepare_read_source_result {
    kind_type kind{};
    sign_type sign{};
    Iterator start{};
    std::optional<Iterator> definite_end{};
};

template <typename Source, typename CharT = detail::char_t<Source>>
auto prepare_read_source(Source& source, unsigned flags, CharT radix_point)
    -> scan_expected<prepare_read_source_result<ranges::iterator_t<Source>>>
{
    using result_type = prepare_read_source_result<ranges::iterator_t<Source>>;

    SCN_TRY(sign_result,
            parse_numeric_sign(source).transform_error(make_eof_scan_error));
    const auto start = sign_result.first;
    if (start == source.end()) {
        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Expected a floating point value after a sign, "
            "got end of input instead");
    }

    if (auto inf_short = read_matching_string_classic_nocase(
            ranges::subrange{start, source.end()}, "inf")) {
        if (auto inf_long = read_matching_string_classic_nocase(
                ranges::subrange{*inf_short, source.end()}, "inity")) {
            return result_type{kind_type::inf_long, sign_result.second, start,
                               *inf_long};
        }
        return result_type{kind_type::inf_short, sign_result.second, start,
                           *inf_short};
    }

    if (auto nan = parse_nan(ranges::subrange{start, source.end()}); !nan) {
        return unexpected(nan.error());
    }
    else if (ranges::distance(start, *nan) == 3) {
        return result_type{kind_type::nan_without_payload, sign_result.second,
                           start, *nan};
    }
    else if (ranges::distance(start, *nan) > 3) {
        return result_type{kind_type::nan_with_payload, sign_result.second,
                           start, *nan};
    }

    const bool allowed_hex = flags & allow_hex;
    const bool allowed_nonhex = flags & ~allow_hex;

    auto check_following_digit = [&](auto iter,
                                     int base) -> scan_expected<void> {
        if (iter == source.end()) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Expected a floating-point value, got end of input instead");
        }
        const auto ch = *iter;
        if (ch == radix_point) {
            return {};
        }
        if (is_ascii_char(ch) && char_to_int(ch) < base) {
            return {};
        }
        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Expected a floating-point value, "
            "got a non-numeric character instead");
    };

    if (allowed_hex && !allowed_nonhex) {
        // Only hex allowed:
        // Prefix "0x" allowed, but not required
        if (auto after_prefix = read_matching_string_classic_nocase(
                ranges::subrange{start, source.end()}, "0x");
            after_prefix && check_following_digit(*after_prefix, 16)) {
            return result_type{kind_type::hex_with_prefix, sign_result.second,
                               start, std::nullopt};
        }
        return result_type{kind_type::hex_without_prefix, sign_result.second,
                           start, std::nullopt};
    }
    if (allowed_hex && allowed_nonhex) {
        // Both hex and nonhex allowed:
        // Check for a "0x" prefix -> hex,
        // nonhex otherwise
        if (auto after_prefix = read_matching_string_classic_nocase(
                ranges::subrange{start, source.end()}, "0x");
            after_prefix && check_following_digit(*after_prefix, 16)) {
            return result_type{kind_type::hex_with_prefix, sign_result.second,
                               start, std::nullopt};
        }
        // fallthrough
    }

    if (auto r = check_following_digit(start, 10); !r) {
        return unexpected(r.error());
    }

    return result_type{kind_type::decimal, sign_result.second, start,
                       std::nullopt};
}

template <typename CharT>
struct source_reader {
    explicit source_reader(const options<CharT>& o) : m_options(o) {}

    struct state_type {
        std::basic_string<CharT> normalized_string{};
        std::string thsep_positions{};
        std::size_t significand_digits_count{};
        std::size_t whole_part_digits_count{};
        std::size_t fractional_part_digits_count{};
        std::size_t exponent_digits_count{};
        bool has_radix_point{};
        bool has_exponent_sign{};
    };

    template <typename Iterator>
    struct result_type {
        Iterator end{};
        kind_type kind{};
    };

    const state_type& state()
    {
        return m_state;
    }

    template <typename Iterator, typename Sentinel>
    scan_expected<result_type<Iterator>> read(
        ranges::subrange<Iterator, Sentinel> source,
        kind_type initial_kind)
    {
        auto it = source.begin();

        if (initial_kind == kind_type::decimal) {
            // Nonhex
            const bool exponent_allowed = m_options.flags & allow_scientific;
            const bool exponent_required =
                exponent_allowed && !(m_options.flags & allow_fixed);

            if (auto res = parse_significand<10>(source)) {
                it = SCN_MOVE(*res);
            }
            else {
                return unexpected(res.error());
            }

            if (exponent_allowed) {
                if (auto after_exp = parse_exponent(
                        ranges::subrange{it, source.end()}, "eE")) {
                    return result_type<Iterator>{*after_exp,
                                                 kind_type::scientific};
                }
                if (exponent_required) {
                    return detail::unexpected_scan_error(
                        scan_error::invalid_scanned_value,
                        "No exponent given for a floating-point value in the "
                        "scientific format");
                }
            }

            return result_type<Iterator>{it, kind_type::fixed};
        }

        {
            // Hex
            SCN_EXPECT(initial_kind == kind_type::hex_with_prefix ||
                       initial_kind == kind_type::hex_without_prefix);

            if (initial_kind == kind_type::hex_with_prefix) {
                m_state.normalized_string.push_back('0');
                m_state.normalized_string.push_back('x');
                ranges::advance(it, 2);
            }

            if (auto res =
                    parse_significand<16>(ranges::subrange{it, source.end()})) {
                it = SCN_MOVE(*res);
            }
            else {
                return unexpected(res.error());
            }

            if (auto after_exp =
                    parse_exponent(ranges::subrange{it, source.end()}, "pP")) {
                return result_type<Iterator>{*after_exp, initial_kind};
            }
            return result_type<Iterator>{it, initial_kind};
        }
    }

private:
    template <int Base, typename Iterator, typename Sentinel>
    Iterator parse_whole_part(ranges::subrange<Iterator, Sentinel> source)
    {
        auto it = source.begin();
        for (; it != source.end(); ++it) {
            // Handle leading zeroes:
            // count them as `significand_digits` so `0.` will be valid,
            // but don't count them as `whole_part_digits`
            // and don't put them in `normalized_string`,
            // so we can skip them easily
            if (*it == CharT{'0'}) {
                ++m_state.significand_digits_count;
            }
            if (*it == m_options.group_separator) {
                m_state.thsep_positions.push_back(
                    static_cast<char>(m_state.significand_digits_count));
                continue;
            }
            break;
        }
        for (; it != source.end(); ++it) {
            if (*it == m_options.group_separator) {
                m_state.thsep_positions.push_back(
                    static_cast<char>(m_state.significand_digits_count));
                continue;
            }
            if (char_to_int(*it) < Base) {
                ++m_state.whole_part_digits_count;
                ++m_state.significand_digits_count;
                m_state.normalized_string.push_back(*it);
                continue;
            }
            break;
        }
        return it;
    }

    template <int Base, typename Iterator, typename Sentinel>
    scan_expected<Iterator> parse_significand(
        ranges::subrange<Iterator, Sentinel> source)
    {
        auto it = parse_whole_part<Base>(source);

        if (auto point = read_matching_code_unit(
                ranges::subrange{it, source.end()}, m_options.radix_point)) {
            m_state.normalized_string.push_back(CharT{'.'});
            m_state.has_radix_point = true;
            it = *point;
        }

        for (; it != source.end(); ++it) {
            if (char_to_int(*it) < Base) {
                ++m_state.significand_digits_count;
                ++m_state.fractional_part_digits_count;
                m_state.normalized_string.push_back(*it);
                continue;
            }
            break;
        }

        if (SCN_UNLIKELY(m_state.significand_digits_count == 0)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "No significand digits in float");
        }

        return it;
    }

    template <typename Iterator, typename Sentinel>
    std::optional<Iterator> parse_exponent(
        ranges::subrange<Iterator, Sentinel> source,
        std::string_view prefix)
    {
        SCN_EXPECT(prefix.size() == 2);
        if (auto after_exp = read_one_of_code_unit(source, prefix)) {
            std::basic_string<CharT> normalized_exponent_string{
                *source.begin()};
            auto it = *after_exp;

            if (auto sign = read_one_of_code_unit(
                    ranges::subrange{it, source.end()}, "+-")) {
                normalized_exponent_string.push_back(*it);
                m_state.has_exponent_sign = true;
                it = *sign;
            }

            for (; it != source.end(); ++it) {
                if (char_to_int(*it) < 10) {
                    normalized_exponent_string.push_back(*it);
                    ++m_state.exponent_digits_count;
                    continue;
                }
                break;
            }

            if (SCN_UNLIKELY(m_state.exponent_digits_count == 0)) {
                return std::nullopt;
            }
            m_state.normalized_string.append(normalized_exponent_string);
            return it;
        }
        return std::nullopt;
    }

    const options<CharT>& m_options;
    state_type m_state{};
};

struct SCN_MAYBE_UNUSED convert_traits_sample {
    // If `false`, `kind_type::tbd` is allowed,
    // has a `unsigned flags` parameter in `convert`,
    // and `convert` returns an iterator (not `void`)
    static constexpr bool need_definite_kind = false;
    // If `true`, the `source` parameter has type `const CharT*`.
    static constexpr bool need_null_termination = false;
    // If `true`, `convert` has a `source_reader<CharT>::state_type` parameter,
    // and needs `need_definite_kind` to be `true`.
    static constexpr bool need_reader_state = false;
    // If `true`, supports hexfloats
    static constexpr bool support_hexfloat = false;

    template <typename CharT, typename T>
    static constexpr bool enabled = false;
};

// Implementation using strtod (and variants) from the C stdlib.
// Used as the final fallback for standard floating-point types
// that don't follow IEEE 754.

struct convert_strtod_traits {
#if !SCN_DISABLE_STRTOD
private:
    template <typename CharT>
    static constexpr bool enabled_char =
        std::is_same_v<CharT, char> || std::is_same_v<CharT, wchar_t>;
    template <typename T>
    static constexpr bool enabled_float =
        std::is_same_v<T, float> || std::is_same_v<T, double> ||
        std::is_same_v<T, long double>
#if SCN_HAS_STD_F16 && defined(__HAVE_FLOAT16) && __HAVE_FLOAT16
        || std::is_same_v<T, std::float16_t>
#endif
        ;

public:
    static constexpr bool need_definite_kind = true;
    static constexpr bool need_null_termination = true;
    static constexpr bool need_reader_state = false;
    static constexpr bool support_hexfloat = true;

    template <typename CharT, typename T>
    static constexpr bool enabled = enabled_char<CharT> && enabled_float<T>;

    template <typename CharT, typename T>
    static scan_expected<void> convert(const CharT* source,
                                       T& value,
                                       kind_type kind,
                                       bool& can_fallback);
#else
    template <typename CharT, typename T>
    static constexpr bool enabled = false;
#endif
};

// Custom implementation using Simple Decimal Conversion for decimal values,
// and another algorithm for hexfloats.
// Used as a fallback for all CharT, and all IEEE FloatT

struct convert_custom_traits {
    static constexpr bool need_definite_kind = true;
    static constexpr bool need_null_termination = false;
    static constexpr bool need_reader_state = true;
    static constexpr bool support_hexfloat = true;

    template <typename CharT, typename T>
    static constexpr bool enabled = std::numeric_limits<T>::is_iec559;

    template <typename CharT, typename T>
    static scan_expected<void> convert(
        std::basic_string_view<CharT> source,
        T& value,
        kind_type kind,
        const typename source_reader<CharT>::state_type& state,
        bool& can_fallback);
};

// Implementation using std::from_chars.
// Used as a fallback if supported by the stdlib,
// for CharT=char

#if SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS
template <typename T, typename = void>
inline constexpr bool has_charconv_for = false;

template <typename T>
inline constexpr bool has_charconv_for<
    T,
    std::void_t<decltype(std::from_chars(SCN_DECLVAL(const char*),
                                         SCN_DECLVAL(const char*),
                                         SCN_DECLVAL(T&)))>> = true;

#if SCN_STDLIB_GLIBCXX

#if SCN_HAS_STD_F16
template <>
inline constexpr bool has_charconv_for<std::float16_t, void> = false;
#endif

#if SCN_HAS_STD_BF16
template <>
inline constexpr bool has_charconv_for<std::bfloat16_t, void> = false;
#endif

#endif  // GLIBCXX

#endif

struct convert_from_chars_traits {
#if SCN_HAS_FLOAT_CHARCONV && !SCN_DISABLE_FROM_CHARS
    static constexpr bool need_definite_kind = false;
    static constexpr bool need_null_termination = false;
    static constexpr bool need_reader_state = false;
    static constexpr bool support_hexfloat = true;

    template <typename CharT, typename T>
    static constexpr bool enabled =
        std::is_same_v<CharT, char> && has_charconv_for<T>;

    template <typename T>
    static scan_expected<std::string_view::iterator> convert(
        std::string_view source,
        T& value,
        kind_type kind,
        unsigned flags,
        bool& can_fallback);
#else
    template <typename CharT, typename T>
    static constexpr bool enabled = false;
#endif
};

// Implementation using fast_float.
// Used if the float type is supported by the library
// (i.e., float, double, or an extended type, not long double)

struct convert_fast_float_traits {
#if !SCN_DISABLE_FAST_FLOAT
    static constexpr bool need_definite_kind = false;
    static constexpr bool need_null_termination = false;
    static constexpr bool need_reader_state = false;
    static constexpr bool support_hexfloat = false;

    template <typename CharT, typename T>
    static constexpr bool enabled =
        fast_float::is_supported_char_type<CharT>() &&
        fast_float::is_supported_float_type<T>();

    template <typename CharT, typename T>
    static scan_expected<typename std::basic_string_view<CharT>::iterator>
    convert(std::basic_string_view<CharT> source,
            T& value,
            kind_type kind,
            unsigned flags,
            bool& can_fallback);
#else
    template <typename CharT, typename T>
    static constexpr bool enabled = false;
#endif
};

template <typename F>
void apply_nan_payload(F& value,
                       typename float_traits<F>::significand_int_type payload)
{
    static_assert(
        !std::is_base_of_v<float_traits<F>, float_traits_impl_none<F>>, "");

    using traits = float_traits<F>;
    typename traits::nan_repr repr{};
    std::memcpy(&repr, &value, sizeof(repr));
    repr.apply_payload(payload);
    std::memcpy(&value, &repr, sizeof(repr));
}

template <typename CharT>
struct convert_float {
    convert_float() = default;

    template <typename Source, typename T>
    auto convert_default(Source& source, T& value)
        -> scan_expected<ranges::iterator_t<Source&>>
    {
        return do_convert<false>(source, value);
    }

    template <typename Source, typename T>
    auto convert_specs(Source& source,
                       T& value,
                       const detail::format_specs& specs,
                       detail::locale_ref loc)
        -> scan_expected<ranges::iterator_t<Source&>>
    {
        switch (specs.type) {
            case detail::presentation_type::float_fixed:
                m_options.flags = allow_fixed;
                break;

            case detail::presentation_type::float_scientific:
                m_options.flags = allow_scientific;
                break;

            case detail::presentation_type::float_hex:
                m_options.flags = allow_hex;
                break;

            case detail::presentation_type::float_general:
                m_options.flags = allow_scientific | allow_fixed;
                break;

            case detail::presentation_type::none:
                m_options.flags = allow_scientific | allow_fixed | allow_hex;
                break;

            default:
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
        }

        if (specs.localized) {
#if !SCN_DISABLE_LOCALE
            auto stdloc = loc.get<std::locale>();
            const auto& numpunct =
                get_or_add_facet<std::numpunct<CharT>>(stdloc);
            m_options.radix_point = numpunct.decimal_point();
            m_options.grouping = numpunct.grouping();
            if (!m_options.grouping.empty()) {
                m_options.group_separator = numpunct.thousands_sep();
            }
#else
            m_options.group_separator.emplace(CharT{','});
            m_options.grouping = "\3";
#endif
        }

        if (specs.localized) {
            return do_convert<true>(source, value);
        }
        return do_convert<false>(source, value);
    }

private:
    template <bool ForceReaderUse, typename Source, typename T>
    auto do_convert(Source& source, T& value)
        -> scan_expected<ranges::iterator_t<Source>>
    {
        auto it = source.begin();

        if (auto res = prepare_read_source(source, m_options.flags,
                                           m_options.radix_point)) {
            m_kind = res->kind;
            m_sign = res->sign;
            it = SCN_MOVE(res->start);

            switch (m_kind) {
                case kind_type::inf_short:
                case kind_type::inf_long: {
                    if constexpr (std::numeric_limits<T>::has_infinity) {
                        value = m_sign == sign_type::minus_sign
                                    ? -std::numeric_limits<T>::infinity()
                                    : std::numeric_limits<T>::infinity();
                        SCN_ENSURE(res->definite_end);
                        return *res->definite_end;
                    }
                    else {
                        return detail::unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Floating-point type doesn't support infinities");
                    }
                }

                case kind_type::nan_without_payload: {
                    if constexpr (std::numeric_limits<T>::has_quiet_NaN) {
                        value = m_sign == sign_type::minus_sign
                                    ? -std::numeric_limits<T>::quiet_NaN()
                                    : std::numeric_limits<T>::quiet_NaN();
                        SCN_ENSURE(res->definite_end);
                        return *res->definite_end;
                    }
                    else {
                        return detail::unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Floating-point type doesn't support NaNs");
                    }
                }

                case kind_type::nan_with_payload: {
                    if constexpr (std::numeric_limits<T>::has_quiet_NaN) {
                        value = m_sign == sign_type::minus_sign
                                    ? -std::numeric_limits<T>::quiet_NaN()
                                    : std::numeric_limits<T>::quiet_NaN();

                        SCN_ENSURE(res->definite_end);
                        if constexpr (std::numeric_limits<T>::is_iec559) {
                            std::string nan_payload;
                            ranges::advance(it, std::strlen("nan("));
                            for (; it != *res->definite_end; ++it) {
                                if (*it == CharT{'_'}) {
                                    continue;
                                }
                                if (*it == CharT{')'}) {
                                    break;
                                }
                                SCN_EXPECT(is_ascii_char(*it) &&
                                           char_to_int(*it) < 36);
                                nan_payload.push_back(static_cast<char>(*it));
                            }
                            SCN_ENSURE(std::next(it) == *res->definite_end);

                            using payload_type =
                                typename float_traits<T>::significand_int_type;
                            payload_type parsed_payload{};
                            if (auto result =
                                    reader_impl_for_int<CharT>{}.read_default(
                                        std::string_view{nan_payload},
                                        parsed_payload, {})) {
                                apply_nan_payload(value, parsed_payload);
                            }
                            else if (result.error().code() ==
                                     scan_error::value_positive_overflow) {
                                apply_nan_payload(
                                    value,
                                    std::numeric_limits<payload_type>::max());
                            }
                        }

                        return *res->definite_end;
                    }
                    else {
                        return detail::unexpected_scan_error(
                            scan_error::invalid_scanned_value,
                            "Floating-point type doesn't support NaNs");
                    }
                }

                default:
                    SCN_ENSURE(!res->definite_end);
            }
        }
        else {
            return unexpected(res.error());
        }

        if (auto res =
                iterate_traits<ForceReaderUse, convert_fast_float_traits,
                               convert_from_chars_traits, convert_custom_traits,
                               convert_strtod_traits>(
                    ranges::subrange{it, source.end()}, value)) {
            SCN_ENSURE(value == std::copysign(value, static_cast<T>(1.0)));
            if (m_sign == sign_type::minus_sign) {
                value = std::copysign(value, static_cast<T>(-1.0));
            }
            return *res;
        }
        else {
            if (res.error().code() == scan_error::value_positive_overflow &&
                m_sign == sign_type::minus_sign) {
                return detail::unexpected_scan_error(
                    scan_error::value_negative_overflow, res.error().msg());
            }
            if (res.error().code() == scan_error::value_positive_underflow &&
                m_sign == sign_type::minus_sign) {
                return detail::unexpected_scan_error(
                    scan_error::value_negative_underflow, res.error().msg());
            }
            return unexpected(res.error());
        }
    }

    template <bool ForceReaderUse,
              typename Traits,
              typename... OtherTraits,
              typename Iterator,
              typename Sentinel,
              typename T,
              std::enable_if_t<Traits::template enabled<CharT, T>>* = nullptr>
    scan_expected<Iterator> iterate_traits(
        ranges::subrange<Iterator, Sentinel> source,
        T& value)
    {
        if (auto r = read_source_for<ForceReaderUse, Traits>(source); !r) {
            return unexpected(r.error());
        }

        m_can_fall_back = false;
        auto res = invoke_convert_for<Traits, Iterator>(source, value);
        if (!res) {
            if constexpr (sizeof...(OtherTraits) > 0 &&
                          (OtherTraits::template enabled<CharT, T> || ...)) {
                if (m_can_fall_back) {
                    return iterate_traits<ForceReaderUse, OtherTraits...>(
                        source, value);
                }
            }

            return unexpected(res.error());
        }

        if constexpr (std::is_same_v<decltype(res), scan_expected<Iterator>>) {
            return *res;
        }
        else {
            SCN_ENSURE(m_definite_length != 0);
            return std::next(source.begin(), m_definite_length);
        }
    }

    template <bool ForceReaderUse,
              typename Traits,
              typename... OtherTraits,
              typename Iterator,
              typename Sentinel,
              typename T,
              std::enable_if_t<!Traits::template enabled<CharT, T>>* = nullptr>
    scan_expected<Iterator> iterate_traits(
        ranges::subrange<Iterator, Sentinel> source,
        T& value)
    {
        static_assert(sizeof...(OtherTraits) > 0);
        static_assert((OtherTraits::template enabled<CharT, T> || ...));

        return iterate_traits<ForceReaderUse, OtherTraits...>(source, value);
    }

    template <bool ForceReaderUse, typename Traits, typename Source>
    scan_expected<void> read_source_for(Source& source)
    {
        if constexpr (Traits::need_reader_state || Traits::need_definite_kind ||
                      !ranges::contiguous_range<Source> || ForceReaderUse) {
            if (m_reader) {
                return {};
            }

            m_reader.emplace(m_options);
            auto res = m_reader->read(source, m_kind);
            if (!res) {
                return unexpected(res.error());
            }
            m_definite_length = ranges::distance(source.begin(), res->end);
            m_kind = res->kind;
        }

        return {};
    }

    template <typename Traits, typename Iterator, typename Sentinel, typename T>
    auto invoke_convert_for(ranges::subrange<Iterator, Sentinel> source,
                            T& value) -> scan_expected<Iterator>
    {
        if constexpr (!Traits::support_hexfloat) {
            if (m_kind == kind_type::hex_with_prefix ||
                m_kind == kind_type::hex_without_prefix) {
                m_can_fall_back = true;
                return detail::unexpected_scan_error(
                    scan_error::invalid_scanned_value,
                    "Hexfloats not supported by the implementation");
            }
        }

        auto src = std::invoke([&] {
            if constexpr (Traits::need_null_termination) {
                SCN_EXPECT(m_reader);
                return m_reader->state().normalized_string.c_str();
            }
            else {
                if constexpr (ranges::contiguous_range<decltype(source)>) {
                    if (m_reader) {
                        SCN_EXPECT(m_definite_length != 0);
                        return std::basic_string_view<CharT>{
                            m_reader->state().normalized_string};
                    }
                    return std::basic_string_view<CharT>(
                        ranges::data(source),
                        ranges::data(source) + ranges::size(source));
                }
                else {
                    SCN_EXPECT(m_reader);
                    SCN_EXPECT(m_definite_length != 0);
                    return std::basic_string_view<CharT>{
                        m_reader->state().normalized_string};
                }
            }
        });

        auto make_return = [&](auto res) -> scan_expected<Iterator> {
            if (!res) {
                return unexpected(res.error());
            }

            if (m_definite_length == 0) {
                if constexpr (!std::is_same_v<decltype(res),
                                              scan_expected<void>>) {
                    if constexpr (std::is_pointer_v<decltype(src)>) {
                        m_definite_length = ranges::distance(src, *res);
                    }
                    else {
                        m_definite_length = ranges::distance(src.begin(), *res);
                    }
                }
                else {
                    SCN_EXPECT(false);
                }
            }

            SCN_ENSURE(m_definite_length != 0);
            return std::next(source.begin(), m_definite_length);
        };

        if constexpr (Traits::need_reader_state) {
            SCN_EXPECT(m_reader);
            return make_return(Traits::convert(
                src, value, m_kind, m_reader->state(), m_can_fall_back));
        }
        else if constexpr (!Traits::need_definite_kind) {
            return make_return(Traits::convert(
                src, value, m_kind, m_options.flags, m_can_fall_back));
        }
        else {
            return make_return(
                Traits::convert(src, value, m_kind, m_can_fall_back));
        }
    }

    options<CharT> m_options{};
    std::ptrdiff_t m_definite_length{};
    kind_type m_kind{};
    sign_type m_sign{};
    bool m_can_fall_back{};

    std::optional<source_reader<CharT>> m_reader{};
};

}  // namespace float_conversion

template <typename CharT>
class reader_impl_for_float
    : public reader_base<reader_impl_for_float<CharT>, CharT> {
public:
    constexpr reader_impl_for_float() = default;

    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_float_type_specs(specs, eh);
    }

    template <typename Range, typename T>
    auto read_default(Range range, T& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        return float_conversion::convert_float<CharT>{}.convert_default(range,
                                                                        value);
    }

    template <typename Range, typename T>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    T& value,
                    detail::locale_ref loc)
    {
        return float_conversion::convert_float<CharT>{}.convert_specs(
            range, value, specs, loc);
    }
};

/////////////////////////////////////////////////////////////////
// Regex reader
/////////////////////////////////////////////////////////////////

#if !SCN_DISABLE_REGEX

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
constexpr auto make_regex_flags(detail::regex_flags flags)
    -> scan_expected<std::regex_constants::syntax_option_type>
{
    std::regex_constants::syntax_option_type result{};
    if ((flags & detail::regex_flags::multiline) != detail::regex_flags::none) {
#if SCN_HAS_STD_REGEX_MULTILINE
        result |= std::regex_constants::multiline;
#else
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "/m flag for regex isn't supported by regex backend");
#endif
    }
    if ((flags & detail::regex_flags::singleline) !=
        detail::regex_flags::none) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "/s flag for regex isn't supported by regex backend");
    }
    if ((flags & detail::regex_flags::nocase) != detail::regex_flags::none) {
        result |= std::regex_constants::icase;
    }
    if ((flags & detail::regex_flags::nocapture) != detail::regex_flags::none) {
        result |= std::regex_constants::nosubs;
    }
    return result;
}
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
constexpr auto make_regex_flags(detail::regex_flags flags)
    -> boost::regex_constants::syntax_option_type
{
    boost::regex_constants::syntax_option_type result{};
    if ((flags & detail::regex_flags::multiline) == detail::regex_flags::none) {
        result |= boost::regex_constants::no_mod_m;
    }
    if ((flags & detail::regex_flags::singleline) !=
        detail::regex_flags::none) {
        result |= boost::regex_constants::mod_s;
    }
    if ((flags & detail::regex_flags::nocase) != detail::regex_flags::none) {
        result |= boost::regex_constants::icase;
    }
    if ((flags & detail::regex_flags::nocapture) != detail::regex_flags::none) {
        result |= boost::regex_constants::nosubs;
    }
    return result;
}

template <typename CharT>
auto make_regex(std::basic_string_view<CharT> pattern,
                boost::regex_constants::syntax_option_type flags)
{
#if SCN_REGEX_BOOST_USE_ICU
    return boost::make_u32regex(pattern.data(), pattern.data() + pattern.size(),
                                flags);
#else
    return boost::basic_regex<CharT>(pattern.data(),
                                     pattern.data() + pattern.size(), flags);
#endif
}

template <typename CharT, typename Input>
auto make_regex_input(Input& input)
{
    auto common = ranges::views::common(input);
    static_assert(ranges::borrowed_range<decltype(common)>);

#if SCN_REGEX_BOOST_USE_ICU
    if constexpr (sizeof(CharT) == sizeof(char32_t)) {
        return common;
    }
    // If we pass a non-UTF-32 range to Boost.Regex with ICU,
    // the internals of that library will decrement the end iterator
    // while doing range checking.
    // We can't support that, so we need to pass in a UTF-32 range.
    // Luckily, Boost.Regex provides a clean interface to make that happen.
    else if constexpr (sizeof(CharT) == sizeof(char16_t)) {
        using I = decltype(common.begin());
        return ranges::subrange{boost::u16_to_u32_iterator<I>{common.begin()},
                                boost::u16_to_u32_iterator<I>{common.end()}};
    }
    else {
        using I = decltype(common.begin());
        return ranges::subrange{boost::u8_to_u32_iterator<I>{common.begin()},
                                boost::u8_to_u32_iterator<I>{common.end()}};
    }
#else
    return common;
#endif
}

template <typename Regex, typename Input, typename Matches>
bool do_regex_search(Regex& regex, Input& input, Matches& matches)
{
#if SCN_REGEX_BOOST_USE_ICU
    return boost::u32regex_search(input.begin(), input.end(), matches, regex,
                                  boost::match_continuous);
#else
    return boost::regex_search(input.begin(), input.end(), matches, regex,
                               boost::match_continuous);
#endif
}

#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
inline auto make_regex_flags(detail::regex_flags flags)
    -> std::pair<RE2::Options, std::string_view>
{
    RE2::Options opt{RE2::Quiet};
    std::string_view stringflags{};

    if ((flags & detail::regex_flags::multiline) == detail::regex_flags::none) {
        stringflags = "(?m)";
    }
    if ((flags & detail::regex_flags::singleline) !=
        detail::regex_flags::none) {
        opt.set_dot_nl(true);
    }
    if ((flags & detail::regex_flags::nocase) != detail::regex_flags::none) {
        opt.set_case_sensitive(false);
    }
    if ((flags & detail::regex_flags::nocapture) != detail::regex_flags::none) {
        opt.set_never_capture(true);
    }

    return {opt, stringflags};
}
#endif  // SCN_REGEX_BACKEND == ...

template <typename CharT, typename Input>
auto read_regex_string_impl(std::basic_string_view<CharT> pattern,
                            detail::regex_flags flags,
                            Input input)
    -> scan_expected<ranges::iterator_t<Input>>
{
    static_assert(std::is_same_v<ranges::range_value_t<Input>, CharT>);

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
    std::basic_regex<CharT> re{};
    try {
        SCN_TRY(re_flags, make_regex_flags(flags));
        re = std::basic_regex<CharT>{pattern.data(), pattern.size(),
                                     re_flags | std::regex_constants::nosubs};
    }
    catch (const std::regex_error&) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Invalid regex");
    }

    auto common_input = ranges::views::common(input);
    std::match_results<decltype(common_input.begin())> matches{};
    try {
        bool found =
            std::regex_search(common_input.begin(), common_input.end(), matches,
                              re, std::regex_constants::match_continuous);
        if (!found || matches.prefix().matched) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Regular expression didn't match");
        }
    }
    catch (const std::regex_error&) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Regex matching failed with an error");
    }

    SCN_ENSURE(matches.ready());
    return std::next(input.begin(), matches.length(0));
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
    auto re =
        make_regex(pattern, make_regex_flags(flags) | boost::regex::no_except |
                                boost::regex::nosubs);
    if (re.status() != 0) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Invalid regex");
    }

    auto regex_input = make_regex_input<CharT>(input);
    boost::match_results<decltype(regex_input.begin())> matches{};
    try {
        bool found = do_regex_search(re, regex_input, matches);
        if (!found || matches.prefix().matched) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Regular expression didn't match");
        }
    }
    catch (const std::runtime_error&) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Regex matching failed with an error");
    }
    catch (const std::out_of_range&) {
        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Regex matching failed with invalid encoding");
    }

#if SCN_REGEX_BOOST_USE_ICU
    auto get_source_iterator = [&](auto it) {
        if constexpr (sizeof(CharT) == sizeof(char32_t)) {
            return it;
        }
        else {
            return it.base();
        }
    };

    return std::next(input.begin(),
                     std::distance(get_source_iterator(regex_input.begin()),
                                   get_source_iterator(matches[0].second)));
#else
    return std::next(input.begin(), matches.length(0));
#endif
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
    static_assert(std::is_same_v<CharT, char>);
    std::string flagged_pattern{};
    auto re = [&]() {
        auto [opts, flagstr] = make_regex_flags(flags);
        opts.set_never_capture(true);
        if (flagstr.empty()) {
            return re2::RE2{pattern, opts};
        }
        flagged_pattern.reserve(flagstr.size() + pattern.size());
        flagged_pattern.append(flagstr);
        flagged_pattern.append(pattern);
        return re2::RE2{flagged_pattern, opts};
    }();
    if (!re.ok()) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Failed to parse regular expression");
    }

    if (!is_entire_source_contiguous(input)) {
        return detail::unexpected_scan_error(
            scan_error::insufficient_source,
            "Regex scanning with re2 requires a contiguous source");
    }
    auto contiguous_input = get_as_contiguous(input);
    auto new_input = detail::make_string_view_from_pointers(
        detail::to_address(contiguous_input.begin()),
        detail::to_address(contiguous_input.end()));
    const auto original_begin = new_input.begin();
    bool found = re2::RE2::Consume(&new_input, re);
    if (!found) {
        return detail::unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
    }
    return std::next(input.begin(),
                     std::distance(original_begin, new_input.begin()));
#endif  // SCN_REGEX_BACKEND == ...
}

template <typename CharT, typename Input>
auto read_regex_matches_impl(std::basic_string_view<CharT> pattern,
                             detail::regex_flags flags,
                             Input input,
                             basic_regex_matches<CharT>& value)
    -> scan_expected<ranges::iterator_t<Input>>
{
    static_assert(std::is_same_v<ranges::range_value_t<Input>, CharT>);

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
    std::basic_regex<CharT> re{};
    try {
        SCN_TRY(re_flags, make_regex_flags(flags));
        re = std::basic_regex<CharT>{pattern.data(), pattern.size(), re_flags};
    }
    catch (const std::regex_error&) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Invalid regex");
    }

    auto common_input = ranges::views::common(input);
    std::match_results<decltype(common_input.begin())> matches{};
    try {
        bool found =
            std::regex_search(common_input.begin(), common_input.end(), matches,
                              re, std::regex_constants::match_continuous);
        if (!found || matches.prefix().matched) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Regular expression didn't match");
        }
    }
    catch (const std::regex_error&) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Regex matching failed with an error");
    }

    SCN_ENSURE(matches.ready());

    auto full_match = matches[0].str();
    std::vector<typename basic_regex_matches<CharT>::submatch> submatches(
        matches.size() - 1);

    for (std::size_t i = 1; i < matches.size(); ++i) {
        if (!matches[i].matched) {
            continue;
        }
        submatches[i - 1].pos = static_cast<std::size_t>(matches.position(i));
        submatches[i - 1].len = static_cast<std::size_t>(matches.length(i));
    }

    value =
        basic_regex_matches<CharT>{SCN_MOVE(full_match), SCN_MOVE(submatches)};

    return std::next(input.begin(), matches.length(0));
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
    std::vector<std::basic_string<CharT>> names;
    for (size_t i = 0; i < pattern.size();) {
        if constexpr (std::is_same_v<CharT, char>) {
            i = pattern.find("(?<", i);
        }
        else {
            i = pattern.find(L"(?<", i);
        }

        if (i == std::basic_string_view<CharT>::npos) {
            break;
        }
        if (i > 0 && pattern[i - 1] == CharT{'\\'}) {
            if (i == 1 || pattern[i - 2] != CharT{'\\'}) {
                i += 3;
                continue;
            }
        }

        i += 3;
        auto end_i = pattern.find(CharT{'>'}, i);
        if (end_i == std::basic_string_view<CharT>::npos) {
            break;
        }
        names.emplace_back(pattern.substr(i, end_i - i));
    }

    auto re =
        make_regex(pattern, make_regex_flags(flags) | boost::regex::no_except);
    if (re.status() != 0) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Invalid regex");
    }

    auto regex_input = make_regex_input<CharT>(input);
    boost::match_results<decltype(regex_input.begin())> matches{};

    try {
        bool found = do_regex_search(re, regex_input, matches);
        if (!found || matches.prefix().matched) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Regular expression didn't match");
        }
    }
    catch (const std::runtime_error&) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Regex matching failed with an error");
    }
    catch (const std::out_of_range&) {
        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Regex matching failed with invalid encoding");
    }

    std::vector<typename basic_regex_matches<CharT>::submatch> submatches(
        matches.size() - 1);

#if !SCN_REGEX_BOOST_USE_ICU
    auto full_match = matches[0].str();

    for (std::size_t i = 1; i < matches.size(); ++i) {
        if (!matches[static_cast<int>(i)].matched) {
            continue;
        }

        submatches[i - 1].pos = static_cast<std::size_t>(
            matches.position(static_cast<unsigned>(i)));
        submatches[i - 1].len =
            static_cast<std::size_t>(matches.length(static_cast<int>(i)));
    }
#else
    auto u32_full_match = matches[0].str();
    std::basic_string<CharT> full_match{};
    if constexpr (sizeof(CharT) == sizeof(char32_t)) {
        full_match.assign(reinterpret_cast<const CharT*>(u32_full_match.data()),
                          u32_full_match.size());
    }
    else {
        transcode_to_string(
            std::basic_string_view<
                typename decltype(u32_full_match)::value_type>{u32_full_match},
            full_match);
    }

    auto get_source_iterator = [&](auto it) {
        if constexpr (sizeof(CharT) == sizeof(char32_t)) {
            return it;
        }
        else {
            return it.base();
        }
    };
    const auto src_beg = get_source_iterator(regex_input.begin());

    for (std::size_t i = 1; i < matches.size(); ++i) {
        auto& match = matches[static_cast<int>(i)];
        if (!match.matched) {
            continue;
        }

        const auto beg = get_source_iterator(match.first);
        const auto end = get_source_iterator(match.second);
        submatches[i - 1].pos =
            static_cast<std::size_t>(std::distance(src_beg, beg));
        submatches[i - 1].len =
            static_cast<std::size_t>(std::distance(beg, end));
    }
#endif
    for (auto& name : names) {
        auto idx = matches.named_subexpression_index(name.data(),
                                                     name.data() + name.size());
        if (idx > 0) {
            submatches[static_cast<std::size_t>(idx - 1)].name = SCN_MOVE(name);
        }
    }
    names.clear();

    const auto n = static_cast<std::ptrdiff_t>(full_match.size());
    value =
        basic_regex_matches<CharT>{SCN_MOVE(full_match), SCN_MOVE(submatches)};

    return std::next(input.begin(), n);
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
    static_assert(std::is_same_v<CharT, char>);
    std::string flagged_pattern{};
    auto re = [&]() {
        auto [opts, flagstr] = make_regex_flags(flags);
        if (flagstr.empty()) {
            return re2::RE2{pattern, opts};
        }
        flagged_pattern.reserve(flagstr.size() + pattern.size());
        flagged_pattern.append(flagstr);
        flagged_pattern.append(pattern);
        return re2::RE2{flagged_pattern, opts};
    }();
    if (!re.ok()) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Failed to parse regular expression");
    }

    if (!is_entire_source_contiguous(input)) {
        return detail::unexpected_scan_error(
            scan_error::insufficient_source,
            "Regex scanning with re2 requires a contiguous source");
    }
    auto contiguous_input = get_as_contiguous(input);
    auto new_input = detail::make_string_view_from_pointers(
        detail::to_address(contiguous_input.begin()),
        detail::to_address(contiguous_input.end()));

    const auto max_matches_n =
        static_cast<std::size_t>(re.NumberOfCapturingGroups());
    std::optional<std::string_view>* matches{};
    re2::RE2::Arg* match_args{};
    re2::RE2::Arg** match_argptrs{};

    const auto round_up = [](std::size_t n, std::size_t multiple) {
        return (n + multiple - 1) & ~(multiple - 1);
    };
    const auto memory_needed_for = [&](auto* ptr) {
        using T = std::remove_pointer_t<decltype(ptr)>;
        return round_up(sizeof(T), alignof(T));
    };
    const auto memory_needed =
        round_up(memory_needed_for(matches) * max_matches_n,
                 alignof(std::max_align_t)) +
        round_up(memory_needed_for(match_args) * max_matches_n,
                 alignof(std::max_align_t)) +
        (memory_needed_for(match_argptrs) * max_matches_n);
    auto memory =
        std::unique_ptr<unsigned char[]>(new unsigned char[memory_needed]);

    {
        auto* ptr = memory.get();
        matches = static_cast<std::optional<std::string_view>*>(
            SCN_ASSUME_ALIGNED(ptr, alignof(std::optional<std::string_view>)));
        ptr += round_up(memory_needed_for(matches) * max_matches_n,
                        alignof(std::max_align_t));
        match_args = static_cast<re2::RE2::Arg*>(
            SCN_ASSUME_ALIGNED(ptr, alignof(re2::RE2::Arg)));
        ptr += round_up(memory_needed_for(match_args) * max_matches_n,
                        alignof(std::max_align_t));
        match_argptrs = static_cast<re2::RE2::Arg**>(
            SCN_ASSUME_ALIGNED(ptr, alignof(re2::RE2::Arg*)));

        for (std::size_t i = 0; i < max_matches_n; ++i) {
            auto* match = ::new (&matches[i]) std::optional<std::string_view>{};
            auto* arg = ::new (&match_args[i]) re2::RE2::Arg{match};
            ::new (&match_argptrs[i]) re2::RE2::Arg* {arg};
        }
    }

    bool found = re2::RE2::ConsumeN(&new_input, re, match_argptrs,
                                    static_cast<int>(max_matches_n));
    if (!found) {
        return detail::unexpected_scan_error(scan_error::invalid_scanned_value,
                                             "Regular expression didn't match");
    }

    std::basic_string<CharT> full_match{};
    full_match.assign(contiguous_input.data(),
                      static_cast<std::size_t>(std::distance(
                          contiguous_input.data(), new_input.data())));

    std::vector<typename basic_regex_matches<CharT>::submatch> submatches{
        max_matches_n};
    for (std::size_t i = 0; i < max_matches_n; ++i) {
        auto match = matches[i];
        {
            using argptr_t = re2::RE2::Arg*;
            match_argptrs[i].~argptr_t();
            using arg_t = re2::RE2::Arg;
            match_args[i].~arg_t();
            using match_t = std::optional<std::string_view>;
            matches[i].~match_t();
        }

        if (!match) {
            continue;
        }

        submatches[i].pos = static_cast<std::size_t>(
            std::distance(contiguous_input.data(), match->data()));
        submatches[i].len = match->size();

        if (auto it = re.CapturingGroupNames().find(static_cast<int>(i + 1));
            it != re.CapturingGroupNames().end()) {
            submatches[i].name = it->second;
        }
    }

    value =
        basic_regex_matches<CharT>{SCN_MOVE(full_match), SCN_MOVE(submatches)};

    return std::next(input.begin(),
                     std::distance(contiguous_input.data(), new_input.data()));
#endif  // SCN_REGEX_BACKEND == ...
}

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wrestrict")

inline std::string get_unescaped_regex_pattern(std::string_view pattern)
{
    std::string result{pattern};
    for (size_t n = 0; (n = result.find("\\/", n)) != std::string::npos;) {
        result.replace(n, 2, "/");
        ++n;
    }
    return result;
}
inline std::wstring get_unescaped_regex_pattern(std::wstring_view pattern)
{
    std::wstring result{pattern};
    for (size_t n = 0; (n = result.find(L"\\/", n)) != std::wstring::npos;) {
        result.replace(n, 2, L"/");
        ++n;
    }
    return result;
}

SCN_GCC_POP

template <typename SourceCharT>
struct regex_matches_reader
    : public reader_base<regex_matches_reader<SourceCharT>, SourceCharT> {
    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_regex_type_specs(specs, eh);
        (void)specs.get_charset_string<SourceCharT>();
    }

    template <typename Range, typename DestCharT>
    auto read_default(Range,
                      basic_regex_matches<DestCharT>&,
                      detail::locale_ref = {})
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "No regex given in format string for scanning regex_matches");
    }

    template <typename Range, typename DestCharT>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    basic_regex_matches<DestCharT>& value,
                    detail::locale_ref = {})
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(range);
        if constexpr (!std::is_same_v<SourceCharT, DestCharT>) {
            return detail::unexpected_scan_error(
                scan_error::invalid_format_string,
                "Cannot transcode with regex_matches_reader");
        }
        else if constexpr (!SCN_REGEX_SUPPORTS_WIDE_STRINGS &&
                           !std::is_same_v<SourceCharT, char>) {
            return detail::unexpected_scan_error(
                scan_error::invalid_format_string,
                "Regex backend doesn't support wide strings as input");
        }
        else {
            return impl(range,
                        specs.type == detail::presentation_type::regex_escaped,
                        specs.get_charset_string<SourceCharT>(),
                        specs.regexp_flags, value);
        }
    }

private:
    template <typename Range, typename DestCharT>
    auto impl(Range input,
              bool is_escaped,
              std::basic_string_view<SourceCharT> pattern,
              detail::regex_flags flags,
              basic_regex_matches<DestCharT>& value)
    {
        if constexpr (detail::is_type_disabled<
                          basic_regex_matches<DestCharT>>) {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
        else {
            if (is_escaped) {
                return read_regex_matches_impl<SourceCharT>(
                    get_unescaped_regex_pattern(pattern), flags, input, value);
            }
            return read_regex_matches_impl(pattern, flags, input, value);
        }
    }
};

template <typename CharT>
struct reader_impl_for_regex_matches : public regex_matches_reader<CharT> {};

#endif  // !SCN_DISABLE_REGEX

/////////////////////////////////////////////////////////////////
// String reader
/////////////////////////////////////////////////////////////////

template <typename Range, typename Iterator, typename ValueCharT>
auto read_string_impl(Range range,
                      Iterator&& result,
                      std::basic_string<ValueCharT>& value)
    -> scan_expected<ranges::const_iterator_t<Range>>
{
    static_assert(ranges::forward_iterator<detail::remove_cvref_t<Iterator>>);

    auto src = make_contiguous_buffer(ranges::subrange{range.begin(), result});
    if (!validate_unicode(src.view())) {
        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Invalid encoding in scanned string");
    }

    SCN_TRY_DISCARD(transcode_if_necessary(SCN_MOVE(src), value));
    return SCN_MOVE(result);
}

template <typename Range, typename Iterator, typename ValueCharT>
auto read_string_view_impl(Range range,
                           Iterator&& result,
                           std::basic_string_view<ValueCharT>& value)
    -> scan_expected<ranges::const_iterator_t<Range>>
{
    static_assert(ranges::forward_iterator<detail::remove_cvref_t<Iterator>>);

    auto src = [&]() {
        if constexpr (detail::is_specialization_of_v<Range, take_width_view>) {
            return make_contiguous_buffer(
                ranges::subrange{range.begin().base(), result.base()});
        }
        else {
            return make_contiguous_buffer(
                ranges::subrange{range.begin(), result});
        }
    }();
    using src_type = decltype(src);

    if (src.stores_allocated_string()) {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Cannot read a string_view from this source range (not "
            "contiguous)");
    }
    if constexpr (!std::is_same_v<typename src_type::char_type, ValueCharT>) {
        return detail::unexpected_scan_error(scan_error::invalid_format_string,
                                             "Cannot read a string_view from "
                                             "this source range (would require "
                                             "transcoding)");
    }
    else {
        const auto view = src.view();
        value = std::basic_string_view<ValueCharT>(view.data(), view.size());

        if (!validate_unicode(value)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value,
                "Invalid encoding in scanned string_view");
        }

        return SCN_MOVE(result);
    }
}

template <typename SourceCharT>
class word_reader_impl {
public:
    template <typename Range, typename ValueCharT>
    auto read(Range range, std::basic_string<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        return read_string_impl(range, read_until_classic_space(range), value);
    }

    template <typename Range, typename ValueCharT>
    auto read(Range range, std::basic_string_view<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        return read_string_view_impl(range, read_until_classic_space(range),
                                     value);
    }
};

template <typename SourceCharT>
class custom_word_reader_impl {
public:
    template <typename Range, typename ValueCharT>
    auto read(Range range,
              const detail::format_specs& specs,
              std::basic_string<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (specs.fill.size() <= sizeof(SourceCharT)) {
            return read_string_impl(
                range,
                read_until_code_unit(
                    range, specs.fill.template get_code_unit<SourceCharT>()),
                value);
        }
        return read_string_impl(
            range,
            read_until_code_units(
                range, specs.fill.template get_code_units<SourceCharT>()),
            value);
    }

    template <typename Range, typename ValueCharT>
    auto read(Range range,
              const detail::format_specs& specs,
              std::basic_string_view<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (specs.fill.size() <= sizeof(SourceCharT)) {
            return read_string_view_impl(
                range,
                read_until_code_unit(
                    range, specs.fill.template get_code_unit<SourceCharT>()),
                value);
        }
        return read_string_view_impl(
            range,
            read_until_code_units(
                range, specs.fill.template get_code_units<SourceCharT>()),
            value);
    }
};

#if !SCN_DISABLE_REGEX
template <typename SourceCharT>
class regex_string_reader_impl {
public:
    template <typename Range, typename ValueCharT>
    auto read(Range range,
              std::basic_string_view<SourceCharT> pattern,
              detail::regex_flags flags,
              std::basic_string<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_TRY(it, impl(range, pattern, flags));
        return read_string_impl(range, it, value);
    }

    template <typename Range, typename ValueCharT>
    auto read(Range range,
              std::basic_string_view<SourceCharT> pattern,
              detail::regex_flags flags,
              std::basic_string_view<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_TRY(it, impl(range, pattern, flags));
        return read_string_view_impl(range, it, value);
    }

private:
    template <typename Range>
    auto impl(Range range,
              std::basic_string_view<SourceCharT> pattern,
              detail::regex_flags flags)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if constexpr (!SCN_REGEX_SUPPORTS_WIDE_STRINGS &&
                      !std::is_same_v<SourceCharT, char>) {
            return detail::unexpected_scan_error(
                scan_error::invalid_format_string,
                "Regex backend doesn't support wide strings as input");
        }
        else {
            return read_regex_string_impl<SourceCharT>(pattern, flags, range);
        }
    }
};
#endif

template <typename SourceCharT>
class character_reader_impl {
public:
    // Note: no localized version,
    // since it's equivalent in behavior

    template <typename Range, typename ValueCharT>
    auto read(Range range, std::basic_string<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        return read_impl(
            range,
            [&](const auto& rng) {
                return read_string_impl(rng, read_all(rng), value);
            },
            detail::priority_tag<1>{});
    }

    template <typename Range, typename ValueCharT>
    auto read(Range range, std::basic_string_view<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        return read_impl(
            range,
            [&](const auto& rng) {
                return read_string_view_impl(rng, read_all(rng), value);
            },
            detail::priority_tag<1>{});
    }

private:
    template <typename View, typename ReadCb>
    static auto read_impl(const take_width_view<View>& range,
                          ReadCb&& read_cb,
                          detail::priority_tag<1>)
        -> scan_expected<ranges::const_iterator_t<take_width_view<View>&>>
    {
        return read_cb(range);
    }

    template <typename Range, typename ReadCb>
    static auto read_impl(Range, ReadCb&&, detail::priority_tag<0>)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        return detail::unexpected_scan_error(
            scan_error::invalid_format_string,
            "Cannot read characters {:c} without maximum field width");
    }
};

struct nonascii_specs_handler {
    void on_charset_single(char32_t cp)
    {
        on_charset_range(cp, cp + 1);
    }

    void on_charset_range(char32_t begin, char32_t end)
    {
        if (end <= 127) {
            return;
        }

        for (auto& elem : extra_ranges) {
            // TODO: check for overlap
            if (elem.first == end) {
                elem.first = begin;
                return;
            }

            if (elem.second == begin) {
                elem.second = end;
                return;
            }
        }

        extra_ranges.push_back(std::make_pair(begin, end));
    }

    constexpr void on_charset_inverted() const
    {
        // no-op
    }

    constexpr void on_error(const char* msg)
    {
        on_error(scan_error{scan_error::invalid_format_string, msg});
    }
    constexpr void on_error(scan_error e)
    {
        SCN_UNLIKELY_ATTR
        err = unexpected(e);
    }

    constexpr scan_expected<void> get_error() const
    {
        return err;
    }

    std::vector<std::pair<char32_t, char32_t>> extra_ranges;
    scan_expected<void> err;
};

template <typename SourceCharT>
class character_set_reader_impl {
public:
    template <typename Range, typename ValueCharT>
    auto read(Range range,
              const detail::format_specs& specs,
              std::basic_string<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        auto it = read_source_impl(range, {specs});
        if (SCN_UNLIKELY(!it)) {
            return unexpected(it.error());
        }

        return read_string_impl(range, *it, value);
    }

    template <typename Range, typename ValueCharT>
    auto read(Range range,
              const detail::format_specs& specs,
              std::basic_string_view<ValueCharT>& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        auto it = read_source_impl(range, {specs});
        if (SCN_UNLIKELY(!it)) {
            return unexpected(it.error());
        }

        return read_string_view_impl(range, *it, value);
    }

private:
    struct specs_helper {
        constexpr specs_helper(const detail::format_specs& s) : specs(s) {}

        constexpr bool is_char_set_in_literals(char ch) const
        {
            SCN_EXPECT(is_ascii_char(ch));
            const auto val =
                static_cast<unsigned>(static_cast<unsigned char>(ch));
            return (static_cast<unsigned>(specs.charset_literals[val / 8]) >>
                    (val % 8)) &
                   1u;
        }

        bool is_char_set_in_extra_literals(char32_t cp) const
        {
            // TODO: binary search?
            if (nonascii.extra_ranges.empty()) {
                return false;
            }

            const auto cp_val = static_cast<uint32_t>(cp);
            return std::find_if(
                       nonascii.extra_ranges.begin(),
                       nonascii.extra_ranges.end(),
                       [cp_val](const auto& pair) noexcept {
                           return static_cast<uint32_t>(pair.first) <= cp_val &&
                                  static_cast<uint32_t>(pair.second) > cp_val;
                       }) != nonascii.extra_ranges.end();
        }

        scan_expected<void> handle_nonascii()
        {
            if (!specs.charset_has_nonascii) {
                return {};
            }

            auto charset_string = specs.get_charset_string<SourceCharT>();
            auto it = detail::to_address(charset_string.begin());
            auto set = detail::parse_presentation_set(
                it, detail::to_address(charset_string.end()), nonascii);
            SCN_TRY_DISCARD(nonascii.get_error());
            SCN_ENSURE(it == detail::to_address(charset_string.end()));
            SCN_ENSURE(set == charset_string);

            std::sort(nonascii.extra_ranges.begin(),
                      nonascii.extra_ranges.end());
            return {};
        }

        const detail::format_specs& specs;
        nonascii_specs_handler nonascii;
    };

    struct read_source_callback {
        SCN_NODISCARD bool on_ascii_only(SourceCharT ch) const
        {
            if (!is_ascii_char(ch)) {
                return false;
            }

            return helper.is_char_set_in_literals(static_cast<char>(ch));
        }

        SCN_NODISCARD bool on_classic_with_extra_ranges(char32_t cp) const
        {
            if (!is_ascii_char(cp)) {
                return helper.is_char_set_in_extra_literals(cp);
            }

            return helper.is_char_set_in_literals(static_cast<char>(cp));
        }

        const specs_helper& helper;
        detail::locale_ref loc{};
    };

    template <typename Range>
    auto read_source_impl(Range range, specs_helper helper) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        const bool is_inverted = helper.specs.charset_is_inverted;
        const bool accepts_nonascii = helper.specs.charset_has_nonascii;

        SCN_TRY_DISCARD(helper.handle_nonascii());

        read_source_callback cb_wrapper{helper};

        if (accepts_nonascii) {
            const auto cb = [&](char32_t cp) {
                return cb_wrapper.on_classic_with_extra_ranges(cp);
            };

            if (is_inverted) {
                return read_until_code_point(range, cb);
            }
            return read_while_code_point(range, cb);
        }

        const auto cb = [&](SourceCharT ch) {
            return cb_wrapper.on_ascii_only(ch);
        };

        if (is_inverted) {
            return read_until_code_unit(range, cb);
        }
        return read_while_code_unit(range, cb);
    }
};

template <typename SourceCharT>
class string_reader
    : public reader_base<string_reader<SourceCharT>, SourceCharT> {
public:
    constexpr string_reader() = default;

    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_string_type_specs(specs, eh);

        SCN_GCC_PUSH
        SCN_GCC_IGNORE("-Wswitch")
        SCN_GCC_IGNORE("-Wswitch-default")
        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wswitch")
        SCN_CLANG_IGNORE("-Wswitch-default")

        switch (specs.type) {
            case detail::presentation_type::none:
                m_type = reader_type::word;
                break;

            case detail::presentation_type::string: {
                if (specs.align == detail::align_type::left ||
                    specs.align == detail::align_type::center) {
                    m_type = reader_type::custom_word;
                }
                else {
                    m_type = reader_type::word;
                }
                break;
            }

            case detail::presentation_type::character:
                m_type = reader_type::character;
                break;

            case detail::presentation_type::string_set:
                m_type = reader_type::character_set;
                break;

#if !SCN_DISABLE_REGEX
            case detail::presentation_type::regex:
                m_type = reader_type::regex;
                break;

            case detail::presentation_type::regex_escaped:
                m_type = reader_type::regex_escaped;
                break;
#endif
        }

        SCN_CLANG_POP
        SCN_GCC_POP
    }

    bool skip_ws_before_read() const
    {
        return m_type == reader_type::word;
    }

    template <typename Range, typename Value>
    auto read_default(Range range, Value& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        return word_reader_impl<SourceCharT>{}.read(range, value);
    }

    template <typename Range, typename Value>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    Value& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        return read_impl(range, specs, value);
    }

protected:
    enum class reader_type {
        word,
        custom_word,
        character,
        character_set,
#if !SCN_DISABLE_REGEX
        regex,
        regex_escaped,
#endif
    };

    template <typename Range, typename Value>
    auto read_impl(Range range, const detail::format_specs& specs, Value& value)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_CLANG_PUSH
        SCN_CLANG_IGNORE("-Wcovered-switch-default")

        switch (m_type) {
            case reader_type::word:
                return word_reader_impl<SourceCharT>{}.read(range, value);

            case reader_type::custom_word:
                return custom_word_reader_impl<SourceCharT>{}.read(range, specs,
                                                                   value);

            case reader_type::character:
                return character_reader_impl<SourceCharT>{}.read(range, value);

            case reader_type::character_set:
                return character_set_reader_impl<SourceCharT>{}.read(
                    range, specs, value);

#if !SCN_DISABLE_REGEX
            case reader_type::regex:
                return regex_string_reader_impl<SourceCharT>{}.read(
                    range, specs.get_charset_string<SourceCharT>(),
                    specs.regexp_flags, value);

            case reader_type::regex_escaped:
                return regex_string_reader_impl<SourceCharT>{}.read(
                    range,
                    get_unescaped_regex_pattern(
                        specs.get_charset_string<SourceCharT>()),
                    specs.regexp_flags, value);
#endif

            default:
                SCN_EXPECT(false);
                SCN_UNREACHABLE;
        }

        SCN_CLANG_POP
    }

    reader_type m_type{reader_type::word};
};

template <typename SourceCharT>
class reader_impl_for_string : public string_reader<SourceCharT> {};

/////////////////////////////////////////////////////////////////
// Boolean reader
/////////////////////////////////////////////////////////////////

struct bool_reader_base {
    enum options_type { allow_text = 1, allow_numeric = 2 };

    constexpr bool_reader_base() = default;
    constexpr bool_reader_base(unsigned opt) : m_options(opt) {}

    template <typename Range>
    auto read_classic(Range range, bool& value) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        scan_error err{scan_error::invalid_scanned_value,
                       "Failed to read boolean"};

        if (m_options & allow_numeric) {
            if (auto r = read_numeric(range, value)) {
                return *r;
            }
            else {
                err = r.error();
            }
        }

        if (m_options & allow_text) {
            if (auto r = read_textual_classic(range, value)) {
                return *r;
            }
            else {
                err = r.error();
            }
        }

        return unexpected(err);
    }

protected:
    template <typename Range>
    auto read_numeric(Range range, bool& value) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (auto r = read_matching_code_unit(range, '0')) {
            value = false;
            return *r;
        }
        if (auto r = read_matching_code_unit(range, '1')) {
            value = true;
            return *r;
        }

        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Failed to read numeric boolean value: No match");
    }

    template <typename Range>
    auto read_textual_classic(Range range, bool& value) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (auto r = read_matching_string_classic_nocase(range, "true")) {
            value = true;
            return *r;
        }
        if (auto r = read_matching_string_classic_nocase(range, "false")) {
            value = false;
            return *r;
        }

        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Failed to read textual boolean value: No match");
    }

    unsigned m_options{allow_text | allow_numeric};
};

template <typename CharT>
struct bool_reader : public bool_reader_base {
    using bool_reader_base::bool_reader_base;

#if !SCN_DISABLE_LOCALE
    template <typename Range>
    auto read_localized(Range range, detail::locale_ref loc, bool& value) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        scan_error err{scan_error::invalid_scanned_value,
                       "Failed to read boolean"};

        if (m_options & allow_numeric) {
            if (auto r = read_numeric(range, value)) {
                return *r;
            }
            else {
                err = r.error();
            }
        }

        if (m_options & allow_text) {
            auto stdloc = loc.get<std::locale>();
            const auto& numpunct =
                get_or_add_facet<std::numpunct<CharT>>(stdloc);
            const auto truename = numpunct.truename();
            const auto falsename = numpunct.falsename();

            if (auto r =
                    read_textual_custom(range, value, truename, falsename)) {
                return *r;
            }
            else {
                err = r.error();
            }
        }

        return unexpected(err);
    }
#endif

protected:
    template <typename Range>
    auto read_textual_custom(Range range,
                             bool& value,
                             std::basic_string_view<CharT> truename,
                             std::basic_string_view<CharT> falsename) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        const auto is_truename_shorter = truename.size() <= falsename.size();
        const auto shorter = std::pair{
            is_truename_shorter ? truename : falsename, is_truename_shorter};
        const auto longer = std::pair{
            !is_truename_shorter ? truename : falsename, !is_truename_shorter};

        if (auto r = read_matching_string(range, shorter.first)) {
            value = shorter.second;
            return *r;
        }
        if (auto r = read_matching_string(range, longer.first)) {
            value = longer.second;
            return *r;
        }

        return detail::unexpected_scan_error(
            scan_error::invalid_scanned_value,
            "Failed to read textual boolean: No match");
    }
};

template <typename CharT>
class reader_impl_for_bool
    : public reader_base<reader_impl_for_bool<CharT>, CharT> {
public:
    reader_impl_for_bool() = default;

    void check_specs_impl(const detail::format_specs& specs,
                          reader_error_handler& eh)
    {
        detail::check_bool_type_specs(specs, eh);
    }

    template <typename Range>
    auto read_default(Range range, bool& value, detail::locale_ref loc) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);

        return bool_reader<CharT>{}.read_classic(range, value);
    }

    template <typename Range>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    bool& value,
                    detail::locale_ref loc) const
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        const auto rd = bool_reader<CharT>{get_options(specs)};

#if !SCN_DISABLE_LOCALE
        if (specs.localized) {
            return rd.read_localized(range, loc, value);
        }
#else
        SCN_UNUSED(loc);
#endif

        return rd.read_classic(range, value);
    }

    static constexpr unsigned get_options(const detail::format_specs& specs)
    {
        SCN_GCC_COMPAT_PUSH
        SCN_GCC_COMPAT_IGNORE("-Wswitch-enum")

        switch (specs.type) {
            case detail::presentation_type::string:
                return bool_reader_base::allow_text;

            case detail::presentation_type::int_generic:
            case detail::presentation_type::int_binary:
            case detail::presentation_type::int_decimal:
            case detail::presentation_type::int_hex:
            case detail::presentation_type::int_octal:
            case detail::presentation_type::int_unsigned_decimal:
                return bool_reader_base::allow_numeric;

            default:
                return bool_reader_base::allow_text |
                       bool_reader_base::allow_numeric;
        }

        SCN_GCC_COMPAT_POP  // -Wswitch-enum
    }
};

/////////////////////////////////////////////////////////////////
// Character (code unit, code point) reader
/////////////////////////////////////////////////////////////////

template <typename CharT>
class code_unit_reader {
public:
    template <typename SourceRange>
    auto read(const SourceRange& range, CharT& ch)
        -> scan_expected<ranges::const_iterator_t<SourceRange>>
    {
        if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
            return unexpected(make_eof_scan_error(e));
        }
        ch = *range.begin();
        return std::next(range.begin());
    }
};

template <typename CharT>
class code_point_reader;

template <>
class code_point_reader<char32_t> {
public:
    template <typename SourceRange>
    auto read(const SourceRange& range, char32_t& cp)
        -> scan_expected<ranges::const_iterator_t<SourceRange>>
    {
        auto result = read_code_point_into(range);
        if (SCN_UNLIKELY(!result.is_valid())) {
            return detail::unexpected_scan_error(
                scan_error::invalid_scanned_value, "Invalid code point");
        }
        cp = detail::decode_code_point_exhaustive_valid(
            std::basic_string_view<detail::char_t<SourceRange>>{
                result.codepoint.data(), result.codepoint.size()});
        return result.iterator;
    }
};

template <>
class code_point_reader<wchar_t> {
public:
    template <typename SourceRange>
    auto read(const SourceRange& range, wchar_t& ch)
        -> scan_expected<ranges::const_iterator_t<SourceRange>>
    {
        code_point_reader<char32_t> reader{};
        char32_t cp{};
        auto ret = reader.read(range, cp);
        if (SCN_UNLIKELY(!ret)) {
            return unexpected(ret.error());
        }

        SCN_TRY(encoded_ch, encode_code_point_as_wide_character(cp, true));
        ch = encoded_ch;
        return *ret;
    }
};

template <typename ValueCharT>
class char_reader_base {
public:
    constexpr char_reader_base() = default;

    bool skip_ws_before_read() const
    {
        return false;
    }

    static scan_expected<void> check_specs(const detail::format_specs& specs)
    {
        reader_error_handler eh{};
        if constexpr (std::is_same_v<ValueCharT, char32_t>) {
            detail::check_code_point_type_specs(specs, eh);
        }
        else {
            detail::check_char_type_specs(specs, eh);
        }
        if (SCN_UNLIKELY(!eh)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_format_string, eh.m_msg);
        }
        return {};
    }
};

template <typename CharT>
class reader_impl_for_char : public char_reader_base<char> {
public:
    template <typename Range>
    auto read_default(Range range, char& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        if constexpr (std::is_same_v<CharT, char>) {
            return code_unit_reader<char>{}.read(range, value);
        }
        else {
            SCN_UNUSED(range);
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
    }

    template <typename Range>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    char& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (specs.type == detail::presentation_type::none ||
            specs.type == detail::presentation_type::character) {
            return read_default(range, value, loc);
        }

        reader_impl_for_int<CharT> reader{};
        signed char tmp_value{};
        auto ret = reader.read_specs(range, specs, tmp_value, loc);
        value = static_cast<char>(tmp_value);
        return ret;
    }
};

template <typename CharT>
class reader_impl_for_wchar : public char_reader_base<wchar_t> {
public:
    template <typename Range>
    auto read_default(Range range, wchar_t& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        if constexpr (std::is_same_v<CharT, char>) {
            return code_point_reader<wchar_t>{}.read(range, value);
        }
        else {
            return code_unit_reader<wchar_t>{}.read(range, value);
        }
    }

    template <typename Range>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    wchar_t& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        if (specs.type == detail::presentation_type::none ||
            specs.type == detail::presentation_type::character) {
            return read_default(range, value, loc);
        }

        reader_impl_for_int<CharT> reader{};
        using integer_type =
            std::conditional_t<sizeof(wchar_t) == 2, int16_t, int32_t>;
        integer_type tmp_value{};
        auto ret = reader.read_specs(range, specs, tmp_value, loc);
        value = static_cast<wchar_t>(tmp_value);
        return ret;
    }
};

template <typename CharT>
class reader_impl_for_code_point : public char_reader_base<char32_t> {
public:
    template <typename Range>
    auto read_default(Range range, char32_t& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(loc);
        return code_point_reader<char32_t>{}.read(range, value);
    }

    template <typename Range>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    char32_t& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(specs);
        return read_default(range, value, loc);
    }
};

/////////////////////////////////////////////////////////////////
// Pointer reader
/////////////////////////////////////////////////////////////////

template <typename CharT>
class reader_impl_for_voidptr {
public:
    constexpr reader_impl_for_voidptr() = default;

    bool skip_ws_before_read() const
    {
        return true;
    }

    static scan_expected<void> check_specs(const detail::format_specs& specs)
    {
        reader_error_handler eh{};
        detail::check_pointer_type_specs(specs, eh);
        if (SCN_UNLIKELY(!eh)) {
            return detail::unexpected_scan_error(
                scan_error::invalid_format_string, eh.m_msg);
        }
        return {};
    }

    template <typename Range>
    auto read_default(Range range, void*& value, detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        detail::format_specs specs{};
        specs.type = detail::presentation_type::int_hex;

        std::uintptr_t intvalue{};
        SCN_TRY(result, reader_impl_for_int<CharT>{}.read_specs(range, specs,
                                                                intvalue, loc));
        value = reinterpret_cast<void*>(intvalue);
        return result;
    }

    template <typename Range>
    auto read_specs(Range range,
                    const detail::format_specs& specs,
                    void*& value,
                    detail::locale_ref loc)
        -> scan_expected<ranges::const_iterator_t<Range>>
    {
        SCN_UNUSED(specs);
        return read_default(range, value, loc);
    }
};

/////////////////////////////////////////////////////////////////
// Argument readers
/////////////////////////////////////////////////////////////////

template <typename Range>
auto skip_ws_before_if_required(bool is_required, Range range)
    -> eof_expected<ranges::iterator_t<Range>>
{
    if (auto e = eof_check(range); SCN_UNLIKELY(!e)) {
        return unexpected(e);
    }

    if (!is_required) {
        return range.begin();
    }

    return skip_classic_whitespace(range);
}

template <typename T, typename CharT>
constexpr auto make_reader()
{
    if constexpr (std::is_same_v<T, bool>) {
        return reader_impl_for_bool<CharT>{};
    }
    else if constexpr (std::is_same_v<T, char>) {
        return reader_impl_for_char<CharT>{};
    }
    else if constexpr (std::is_same_v<T, wchar_t>) {
        return reader_impl_for_wchar<CharT>{};
    }
    else if constexpr (std::is_same_v<T, char32_t>) {
        return reader_impl_for_code_point<CharT>{};
    }
    else if constexpr (std::is_same_v<T, std::string_view> ||
                       std::is_same_v<T, std::wstring_view>) {
        return reader_impl_for_string<CharT>{};
    }
    else if constexpr (std::is_same_v<T, std::string> ||
                       std::is_same_v<T, std::wstring>) {
        return reader_impl_for_string<CharT>{};
    }
#if !SCN_DISABLE_REGEX
    else if constexpr (std::is_same_v<T, regex_matches> ||
                       std::is_same_v<T, wregex_matches>) {
        return reader_impl_for_regex_matches<CharT>{};
    }
#endif
    else if constexpr (std::is_same_v<T, void*>) {
        return reader_impl_for_voidptr<CharT>{};
    }
    else if constexpr (std::is_floating_point_v<T>) {
        return reader_impl_for_float<CharT>{};
    }
    else if constexpr (std::is_same_v<T, signed char> ||
                       std::is_same_v<T, short> || std::is_same_v<T, int> ||
                       std::is_same_v<T, long> ||
                       std::is_same_v<T, long long> ||
                       std::is_same_v<T, unsigned char> ||
                       std::is_same_v<T, unsigned short> ||
                       std::is_same_v<T, unsigned int> ||
                       std::is_same_v<T, unsigned long> ||
                       std::is_same_v<T, unsigned long long>
#if SCN_HAS_INT128
                       || std::is_same_v<T, int128> ||
                       std::is_same_v<T, uint128>
#endif
    ) {
        return reader_impl_for_int<CharT>{};
    }
    else {
        return reader_impl_for_monostate<CharT>{};
    }
}

template <typename Ctx>
inline constexpr bool is_contiguous_context =
    ranges::contiguous_range<typename Ctx::range_type>;

template <typename Context>
struct default_arg_reader {
    using context_type = Context;
    using char_type = typename context_type::char_type;
    using args_type = basic_scan_args<detail::default_context<char_type>>;

    using range_type = typename context_type::range_type;
    using iterator = ranges::iterator_t<range_type>;

    template <typename Reader, typename Range, typename T>
    auto impl(Reader& rd, Range rng, T& value)
        -> scan_expected<ranges::iterator_t<Range>>
    {
        SCN_TRY(it, skip_ws_before_if_required(rd.skip_ws_before_read(), rng)
                        .transform_error(make_eof_scan_error));
        return rd.read_default(ranges::subrange{it, rng.end()}, value, loc);
    }

    template <typename T>
    scan_expected<iterator> operator()(T& value)
    {
        if constexpr (!detail::is_type_disabled<T> &&
                      is_contiguous_context<Context>) {
            auto rd = make_reader<T, char_type>();
            return impl(rd, range, value);
        }
        else if constexpr (!detail::is_type_disabled<T>) {
            auto rd = make_reader<T, char_type>();
            if (!is_entire_source_contiguous(range)) {
                return impl(rd, range, value);
            }
            auto crange = get_as_contiguous(range);
            SCN_TRY(it, impl(rd, crange, value));
            return ranges::next(range.begin(),
                                ranges::distance(crange.begin(), it));
        }
        else {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
    }

    detail::default_context<char_type> make_custom_ctx()
    {
        if constexpr (is_contiguous_context<Context>) {
            auto it = typename detail::basic_scan_buffer<char_type>::iterator{
                std::basic_string_view<char_type>(range.data(), range.size()),
                0};
            return {it, ranges::default_sentinel, args, loc};
        }
        else {
            return {range, args, loc};
        }
    }

    scan_expected<iterator> operator()(
        typename basic_scan_arg<detail::default_context<char_type>>::handle h)
    {
        if constexpr (!detail::is_type_disabled<void>) {
            basic_scan_parse_context<char_type> parse_ctx{
                source_tag<range_type>, {}};
            auto ctx = make_custom_ctx();
            SCN_TRY_DISCARD(h.scan(parse_ctx, ctx));

            if constexpr (is_contiguous_context<Context>) {
                return range.begin() + ctx.begin().position();
            }
            else {
                return ctx.begin();
            }
        }
        else {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
    }

    range_type range;
    args_type args;
    detail::locale_ref loc;
};

template <typename Iterator>
using skip_fill_result = std::pair<Iterator, std::ptrdiff_t>;

template <typename Range>
auto skip_fill(Range range,
               std::ptrdiff_t max_width,
               const detail::fill_type& fill,
               bool want_skipped_width)
    -> scan_expected<skip_fill_result<ranges::iterator_t<Range>>>
{
    using char_type = detail::char_t<Range>;
    using result_type = skip_fill_result<ranges::iterator_t<Range>>;

    if (fill.size() <= sizeof(char_type)) {
        const auto fill_ch = fill.template get_code_unit<char_type>();
        const auto pred = [=](char_type ch) { return ch == fill_ch; };

        if (max_width == 0) {
            auto it = read_while_code_unit(range, pred);

            if (want_skipped_width) {
                auto prefix_width =
                    static_cast<std::ptrdiff_t>(
                        calculate_text_width(static_cast<char32_t>(fill_ch))) *
                    ranges::distance(range.begin(), it);
                return result_type{it, prefix_width};
            }
            return result_type{it, 0};
        }

        auto max_width_view = take_width(range, max_width);
        auto w_it = read_while_code_unit(max_width_view, pred);

        if (want_skipped_width) {
            return result_type{w_it.base(), max_width - w_it.count()};
        }
        return result_type{w_it.base(), 0};
    }

    const auto fill_chars = fill.template get_code_units<char_type>();
    if (max_width == 0) {
        auto it = read_while_code_units(range, fill_chars);

        if (want_skipped_width) {
            auto prefix_width =
                static_cast<std::ptrdiff_t>(calculate_text_width(fill_chars)) *
                ranges::distance(range.begin(), it) / ranges::ssize(fill_chars);
            return result_type{it, prefix_width};
        }
        return result_type{it, 0};
    }

    auto max_width_view = take_width(range, max_width);
    auto w_it = read_while_code_units(max_width_view, fill_chars);

    if (want_skipped_width) {
        return result_type{w_it.base(), max_width - w_it.count()};
    }
    return result_type{w_it.base(), 0};
}

SCN_MAYBE_UNUSED constexpr scan_expected<void> check_widths_for_arg_reader(
    const detail::format_specs& specs,
    std::ptrdiff_t prefix_width,
    std::ptrdiff_t value_width,
    std::ptrdiff_t postfix_width)
{
    if (specs.width != 0) {
        if (prefix_width + value_width + postfix_width < specs.width) {
            return detail::unexpected_scan_error(
                scan_error::length_too_short,
                "Scanned value too narrow, width did not exceed what "
                "was specified in the format string");
        }
    }
    if (specs.precision != 0) {
        // Ensured by take_width_view
        SCN_ENSURE(prefix_width + value_width + postfix_width <=
                   specs.precision);
    }
    return {};
}

template <typename Context>
struct arg_reader {
    using context_type = Context;
    using char_type = typename context_type::char_type;

    using range_type = typename context_type::range_type;
    using iterator = ranges::iterator_t<range_type>;

    template <typename Range>
    auto impl_prefix(Range rng, bool rd_skip_ws_before_read)
        -> scan_expected<skip_fill_result<ranges::iterator_t<Range>>>
    {
        const bool need_skipped_width =
            specs.width != 0 || specs.precision != 0;
        using result_type = skip_fill_result<ranges::iterator_t<Range>>;

        // Read prefix
        if (specs.align == detail::align_type::right ||
            specs.align == detail::align_type::center) {
            return skip_fill(rng, specs.precision, specs.fill,
                             need_skipped_width);
        }
        if (specs.align == detail::align_type::none && rd_skip_ws_before_read) {
            // Default alignment:
            // Skip preceding whitespace, if required by the reader
            if (specs.precision != 0) {
                auto max_width_view = take_width(rng, specs.precision);
                SCN_TRY(w_it, skip_classic_whitespace(max_width_view)
                                  .transform_error(make_eof_scan_error));
                return result_type{w_it.base(), specs.precision - w_it.count()};
            }
            SCN_TRY(it, skip_classic_whitespace(rng).transform_error(
                            make_eof_scan_error));

            if (need_skipped_width) {
                return result_type{
                    it,
                    calculate_text_width(make_contiguous_buffer(
                                             ranges::subrange{rng.begin(), it})
                                             .view())};
            }
            return result_type{it, 0};
        }

        return result_type{rng.begin(), 0};
    }

    template <typename Range>
    auto impl_postfix(Range rng,
                      bool rd_skip_ws_before_read,
                      std::ptrdiff_t prefix_width,
                      std::ptrdiff_t value_width)
        -> scan_expected<skip_fill_result<ranges::iterator_t<Range>>>
    {
        const bool need_skipped_width =
            specs.width != 0 || specs.precision != 0;
        using result_type = skip_fill_result<ranges::iterator_t<Range>>;

        if (specs.align == detail::align_type::left ||
            specs.align == detail::align_type::center) {
            if (specs.precision != 0 &&
                specs.precision - value_width - prefix_width == 0) {
                return result_type{rng.begin(), 0};
            }
            return skip_fill(rng, specs.precision - value_width - prefix_width,
                             specs.fill, need_skipped_width);
        }
        if (specs.align == detail::align_type::none &&
            !rd_skip_ws_before_read &&
            ((specs.width != 0 && prefix_width + value_width < specs.width) ||
             (specs.precision != 0 &&
              prefix_width + value_width < specs.precision))) {
            if (specs.precision != 0) {
                const auto initial_width =
                    specs.precision - prefix_width - value_width;
                auto max_width_view = take_width(rng, initial_width);
                SCN_TRY(w_it, skip_classic_whitespace(max_width_view, true)
                                  .transform_error(make_eof_scan_error));
                return result_type{w_it.base(), initial_width - w_it.count()};
            }
            SCN_TRY(it, skip_classic_whitespace(rng, true).transform_error(
                            make_eof_scan_error));

            if (need_skipped_width) {
                return result_type{
                    it,
                    calculate_text_width(make_contiguous_buffer(
                                             ranges::subrange{rng.begin(), it})
                                             .view())};
            }
            return result_type{it, 0};
        }
        return result_type{rng.begin(), 0};
    }

    template <typename Reader, typename Range, typename T>
    auto impl(Reader& rd, Range rng, T& value)
        -> scan_expected<ranges::iterator_t<Range>>
    {
        const bool need_skipped_width =
            specs.width != 0 || specs.precision != 0;

        // Read prefix
        auto it = rng.begin();
        std::ptrdiff_t prefix_width = 0;
        if (specs.precision != 0) {
            auto max_width_view = take_width(rng, specs.precision);
            SCN_TRY(prefix_result,
                    impl_prefix(max_width_view, rd.skip_ws_before_read()));
            it = prefix_result.first.base();
            prefix_width = prefix_result.second;
        }
        else {
            SCN_TRY(prefix_result, impl_prefix(rng, rd.skip_ws_before_read()));
            std::tie(it, prefix_width) = prefix_result;
        }
        auto prefix_end_it = it;

        // Read value
        std::ptrdiff_t value_width = 0;
        if (specs.precision != 0) {
            if (specs.precision <= prefix_width) {
                return detail::unexpected_scan_error(
                    scan_error::invalid_fill,
                    "Too many fill characters before value, "
                    "precision exceeded before reading value");
            }

            const auto initial_width = specs.precision - prefix_width;
            auto max_width_view =
                take_width(ranges::subrange{it, rng.end()}, initial_width);
            SCN_TRY(w_it, rd.read_specs(max_width_view, specs, value, loc));
            it = w_it.base();
            value_width = initial_width - w_it.count();
        }
        else {
            SCN_TRY_ASSIGN(it, rd.read_specs(ranges::subrange{it, rng.end()},
                                             specs, value, loc));

            if (need_skipped_width) {
                value_width = static_cast<std::ptrdiff_t>(calculate_text_width(
                    make_contiguous_buffer(ranges::subrange{prefix_end_it, it})
                        .view()));
            }
        }

        // Read postfix
        std::ptrdiff_t postfix_width = 0;
        if (it != rng.end()) {
            SCN_TRY(postfix_result,
                    impl_postfix(ranges::subrange{it, rng.end()},
                                 rd.skip_ws_before_read(), prefix_width,
                                 value_width));
            std::tie(it, postfix_width) = postfix_result;
        }

        SCN_TRY_DISCARD(check_widths_for_arg_reader(
            specs, prefix_width, value_width, postfix_width));
        return it;
    }

    template <typename T>
    scan_expected<iterator> operator()(T& value)
    {
        if constexpr (!detail::is_type_disabled<T> &&
                      is_contiguous_context<Context>) {
            auto rd = make_reader<T, char_type>();
            SCN_TRY_DISCARD(rd.check_specs(specs));
            return impl(rd, range, value);
        }
        else if constexpr (!detail::is_type_disabled<T>) {
            auto rd = make_reader<T, char_type>();
            SCN_TRY_DISCARD(rd.check_specs(specs));

            if (!is_entire_source_contiguous(range) || specs.precision != 0 ||
                specs.width != 0) {
                return impl(rd, range, value);
            }
            auto crange = get_as_contiguous(range);
            SCN_TRY(it, impl(rd, crange, value));
            return ranges::next(range.begin(),
                                ranges::distance(crange.begin(), it));
        }
        else {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
    }

    scan_expected<iterator> operator()(
        typename basic_scan_arg<detail::default_context<char_type>>::handle)
        const
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    range_type range;
    const detail::format_specs& specs;
    detail::locale_ref loc;
};

template <typename Context>
struct custom_reader {
    using context_type = Context;
    using char_type = typename context_type::char_type;
    using parse_context_type = typename context_type::parse_context_type;
    using iterator = typename context_type::iterator;

    template <typename T>
    scan_expected<iterator> operator()(T&) const
    {
        SCN_EXPECT(false);
        SCN_UNREACHABLE;
    }

    scan_expected<iterator> operator()(
        typename basic_scan_arg<detail::default_context<char_type>>::handle h)
        const
    {
        SCN_TRY_DISCARD(h.scan(parse_ctx, ctx));
        return {ctx.begin()};
    }

    parse_context_type& parse_ctx;
    context_type& ctx;
};
}  // namespace impl

SCN_END_NAMESPACE
}  // namespace scn

namespace std {

template <>
struct numeric_limits<::scn::impl::uint128_polyfill> {
    static constexpr bool is_specialized = true;

    static constexpr ::scn::impl::uint128_polyfill min() noexcept
    {
        return {0u};
    }

    static constexpr ::scn::impl::uint128_polyfill max() noexcept
    {
        ::scn::impl::uint128_polyfill result;
        result.low = std::numeric_limits<uint64_t>::max();
        result.high = std::numeric_limits<uint64_t>::max();
        return result;
    }
};

}  // namespace std
