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

#ifndef __cplusplus
#error "scnlib is a C++ library"
#endif

#ifdef __has_include
#define SCN_HAS_INCLUDE(x) __has_include(x)
#else
#define SCN_HAS_INCLUDE(x) 0
#endif

#define SCN_STD_17 201703L
#define SCN_STD_20 202002L
#define SCN_STD_23 202302L

#if SCN_HAS_INCLUDE(<version>)
#include <version>

#ifdef _MSC_VER
// _ITERATOR_DEBUG_LEVEL isn't defined in <version> (needed below)
// <ciso646> is removed in C++20, and including it will cause a warning
// So, include <yvals.h> from the STL directly, if available, or otherwise fall
// back on a small C header
#if SCN_HAS_INCLUDE(<yvals.h>)
#include <yvals.h>
#else
#include <cstddef>
#endif
#endif  // _MSC_VER

#else  // has_include(<version>)
#include <ciso646>
#endif

#define SCN_COMPILER(major, minor, patch) \
    ((major)*10'000'000 + (minor)*10'000 + (patch))

#ifdef __INTEL_COMPILER
// Intel
#define SCN_INTEL                                                      \
    SCN_COMPILER(__INTEL_COMPILER / 100, (__INTEL_COMPILER / 10) % 10, \
                 __INTEL_COMPILER % 10)
#elif defined(__clang__) && defined(__clang_minor__) && \
    defined(__clang_patchlevel__)
// Clang
#define SCN_CLANG \
    SCN_COMPILER(__clang_major__, __clang_minor__, __clang_patchlevel__)
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && \
    defined(__GNUC_PATCHLEVEL__)
// GCC
#define SCN_GCC SCN_COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#endif

#if defined(_MSC_VER) && defined(_MSC_FULL_VER)
// MSVC
#if _MSC_VER == _MSC_FULL_VER / 10000
#define SCN_MSVC \
    SCN_COMPILER(_MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 10000)
#else
#define SCN_MSVC                                                \
    SCN_COMPILER(_MSC_VER / 100, (_MSC_FULL_VER / 100000) % 10, \
                 _MSC_FULL_VER % 100000)
#endif  // _MSC_VER == _MSC_FULL_VER / 10000
#endif  // _MSC_VER

#ifndef SCN_INTEL
#define SCN_INTEL 0
#endif
#ifndef SCN_MSVC
#define SCN_MSVC 0
#endif
#ifndef SCN_CLANG
#define SCN_CLANG 0
#endif
#ifndef SCN_GCC
#define SCN_GCC 0
#endif

#if SCN_CLANG && SCN_MSVC
#define SCN_MSVC_CLANG 1
#else
#define SCN_MSVC_CLANG 0
#endif

// Pretending to be gcc (clang, icc, etc.)
#ifdef __GNUC__

#ifdef __GNUC_MINOR__
#define SCN_GCC_COMPAT_MINOR __GNUC_MINOR__
#else
#define SCN_GCC_COMPAT_MINOR 0
#endif

#ifdef __GNUC_PATCHLEVEL__
#define SCN_GCC_COMPAT_PATCHLEVEL __GNUC_PATCHLEVEL__
#else
#define SCN_GCC_COMPAT_PATCHLEVEL 0
#endif

#define SCN_GCC_COMPAT \
    SCN_COMPILER(__GNUC__, SCN_GCC_COMPAT_MINOR, SCN_GCC_COMPAT_PATCHLEVEL)
#else
#define SCN_GCC_COMPAT 0
#endif  // #ifdef __GNUC__

// Stdlib detect: libstdc++
#ifdef _GLIBCXX_RELEASE
#define SCN_STDLIB_GLIBCXX _GLIBCXX_RELEASE
#elif defined(__GLIBCXX__)
#define SCN_STDLIB_GLIBCXX 1
#else
#define SCN_STDLIB_GLIBCXX 0
#endif

// libc++
#ifdef _LIBCPP_VERSION
#define SCN_STDLIB_LIBCPP _LIBCPP_VERSION
#else
#define SCN_STDLIB_LIBCPP 0
#endif

// MSVC STL
#ifdef _MSVC_STL_VERSION
#define SCN_STDLIB_MS_STL _MSVC_STL_VERSION
#else
#define SCN_STDLIB_MS_STL 0
#endif

// MSVC debug iterators
#if SCN_STDLIB_MS_STL && defined(_ITERATOR_DEBUG_LEVEL) && \
    _ITERATOR_DEBUG_LEVEL != 0
#define SCN_MSVC_DEBUG_ITERATORS 1
#else
#define SCN_MSVC_DEBUG_ITERATORS 0
#endif

// POSIX
#if defined(__unix__) || defined(__APPLE__)
#define SCN_POSIX 1
#else
#define SCN_POSIX 0
#endif

#if defined(__APPLE__)
#define SCN_APPLE 1
#else
#define SCN_APPLE 0
#endif

// Windows
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32) || \
     defined(_WIN64)) &&                                      \
    !defined(__CYGWIN__)
#define SCN_WINDOWS 1
#else
#define SCN_WINDOWS 0
#endif

#if SCN_WINDOWS && defined(_WIN64)
#define SCN_WINDOWS_64BIT 1
#else
#define SCN_WINDOWS_64BIT 0
#endif

// MinGW
#if defined(__MINGW32__) || defined(__MINGW64__)
#define SCN_MINGW 1
#else
#define SCN_MINGW 0
#endif

#ifdef _MSVC_LANG
#define SCN_MSVC_LANG _MSVC_LANG
#else
#define SCN_MSVC_LANG 0
#endif

// Standard version
#if SCN_MSVC
#define SCN_STD SCN_MSVC_LANG
#else
#define SCN_STD __cplusplus
#endif

#ifdef __has_cpp_attribute
#define SCN_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define SCN_HAS_CPP_ATTRIBUTE(x) 0
#endif

#ifdef __has_feature
#define SCN_HAS_FEATURE(x) __has_feature(x)
#else
#define SCN_HAS_FEATURE(x) 0
#endif

#ifdef __has_builtin
#define SCN_HAS_BUILTIN(x) __has_builtin(x)
#else
#define SCN_HAS_BUILTIN(x) 0
#endif

#if defined(_SCN_DOXYGEN) && _SCN_DOXYGEN
#define SCN_DOXYGEN 1
#else
#define SCN_DOXYGEN 0
#endif

// Detect exceptions
#ifdef __cpp_exceptions
#define SCN_HAS_EXCEPTIONS 1
#endif

#if !defined(SCN_HAS_EXCEPTIONS) && defined(__EXCEPTIONS)
#define SCN_HAS_EXCEPTIONS 1
#endif

#if !defined(SCN_HAS_EXCEPTIONS) && defined(_HAS_EXCEPTIONS)
#if _HAS_EXCEPTIONS
#define SCN_HAS_EXCEPTIONS 1
#else
#define SCN_HAS_EXCEPTIONS 0
#endif
#endif

#if !defined(SCN_HAS_EXCEPTIONS) && !defined(_CPPUNWIND)
#define SCN_HAS_EXCEPTIONS 0
#endif

#ifndef SCN_HAS_EXCEPTIONS
#define SCN_HAS_EXCEPTIONS 0
#endif

#if SCN_GCC >= SCN_COMPILER(7, 0, 0)
#define SCN_HAS_CPP17_ATTRIBUTES 1
#elif SCN_CLANG >= SCN_COMPILER(3, 9, 0)
#define SCN_HAS_CPP17_ATTRIBUTES 1
#elif SCN_MSVC >= SCN_COMPILER(19, 11, 0)
#define SCN_HAS_CPP17_ATTRIBUTES 1
#elif SCN_INTEL >= SCN_COMPILER(18, 0, 0)
#define SCN_HAS_CPP17_ATTRIBUTES 1
#else
#define SCN_HAS_CPP17_ATTRIBUTES 0
#endif

// Detect [[nodiscard]]
#if SCN_HAS_CPP_ATTRIBUTE(nodiscard) >= 201603L
#define SCN_HAS_NODISCARD 1
#elif SCN_STD >= SCN_STD_17 && SCN_HAS_CPP17_ATTRIBUTES
#define SCN_HAS_NODISCARD 1
#else
#define SCN_HAS_NODISCARD 0
#endif

// Detect [[maybe_unused]]
#if SCN_HAS_CPP_ATTRIBUTE(maybe_unused) >= 201603L
#define SCN_HAS_MAYBE_UNUSED 1
#elif SCN_STD >= SCN_STD_17 && SCN_HAS_CPP17_ATTRIBUTES
#define SCN_HAS_MAYBE_UNUSED 1
#else
#define SCN_HAS_MAYBE_UNUSED 0
#endif

// Detect [[no_unique_address]]
#if SCN_MSVC >= SCN_COMPILER(19, 29, 0) && SCN_STD >= SCN_STD_20
#define SCN_HAS_NO_UNIQUE_ADDRESS_MSVC 1
#define SCN_HAS_NO_UNIQUE_ADDRESS_STD  0
#elif SCN_HAS_CPP_ATTRIBUTE(no_unique_address) >= 201803L && \
    SCN_STD >= SCN_STD_20
#define SCN_HAS_NO_UNIQUE_ADDRESS_MSVC 0
#define SCN_HAS_NO_UNIQUE_ADDRESS_STD  1
#else
#define SCN_HAS_NO_UNIQUE_ADDRESS_MSVC 0
#define SCN_HAS_NO_UNIQUE_ADDRESS_STD  0
#endif

// Detect [[fallthrough]]
#if SCN_HAS_CPP_ATTRIBUTE(fallthrough) >= 201603L
#define SCN_HAS_FALLTHROUGH_CPPATTRIBUTE 1
#elif SCN_STD >= SCN_STD_17 && \
    (SCN_HAS_CPP17_ATTRIBUTES || SCN_MSVC >= SCN_COMPILER(19, 10, 0))
#define SCN_HAS_FALLTHROUGH_CPPATTRIBUTE 1
#endif

#if SCN_HAS_CPP_ATTRIBUTE(gnu::fallthrough)
#define SCN_HAS_FALLTHROUGH_CPPGNUATTRIBUTE 1
#endif
#if SCN_HAS_CPP_ATTRIBUTE(clang::fallthrough)
#define SCN_HAS_FALLTHROUGH_CPPCLANGATTRIBUTE 1
#endif

#if SCN_GCC >= SCN_COMPILER(7, 0, 0)
#define SCN_HAS_FALLTHROUGH_GCCATTRIBUTE 1
#endif

#ifndef SCN_HAS_FALLTHROUGH_CPPATTRIBUTE
#define SCN_HAS_FALLTHROUGH_CPPATTRIBUTE 0
#endif
#ifndef SCN_HAS_FALLTHROUGH_CPPGNUATTRIBUTE
#define SCN_HAS_FALLTHROUGH_CPPGNUATTRIBUTE 0
#endif
#ifndef SCN_HAS_FALLTHROUGH_CPPCLANGATTRIBUTE
#define SCN_HAS_FALLTHROUGH_CPPCLANGATTRIBUTE 0
#endif
#ifndef SCN_HAS_FALLTHROUGH_GCCATTRIBUTE
#define SCN_HAS_FALLTHROUGH_GCCATTRIBUTE 0
#endif

// Detect [[likely]] and [[unlikely]]
#if SCN_STD >= SCN_STD_20

#if SCN_HAS_CPP_ATTRIBUTE(likely) >= 201803 && \
    SCN_HAS_CPP_ATTRIBUTE(unlikely) >= 201803
#define SCN_HAS_LIKELY_ATTR 1
#elif SCN_GCC >= SCN_COMPILER(9, 0, 0)
#define SCN_HAS_LIKELY_ATTR 1
#elif SCN_CLANG >= SCN_COMPILER(12, 0, 0)
#define SCN_HAS_LIKELY_ATTR 1
#elif SCN_MSVC >= SCN_COMPILER(19, 26, 0)
#define SCN_HAS_LIKELY_ATTR 1
#else
#define SCN_HAS_LIKELY_ATTR 0
#endif  // has_attr(likely && unlikely)

#else
#define SCN_HAS_LIKELY_ATTR 0
#endif  // SCN_STD >= 20

// Detect [[clang::trivial_abi]]
#if SCN_HAS_CPP_ATTRIBUTE(clang::trivial_abi)
#define SCN_HAS_TRIVIAL_ABI 1
#else
#define SCN_HAS_TRIVIAL_ABI 0
#endif

// Detect <charconv>

#if SCN_STD >= SCN_STD_17

// libstdc++
#if SCN_STDLIB_GLIBCXX

#if SCN_STDLIB_GLIBCXX >= 9
#define SCN_HAS_INTEGER_CHARCONV 1
#else
#define SCN_HAS_INTEGER_CHARCONV 0
#endif

#if SCN_STDLIB_GLIBCXX >= 11
#define SCN_HAS_FLOAT_CHARCONV 1
#else
#define SCN_HAS_FLOAT_CHARCONV 0
#endif

// MSVC
#elif SCN_MSVC >= SCN_COMPILER(19, 14, 0)

#define SCN_HAS_INTEGER_CHARCONV 1

#if SCN_MSVC >= SCN_COMPILER(19, 21, 0)
#define SCN_HAS_FLOAT_CHARCONV 1
#else
#define SCN_HAS_FLOAT_CHARCONV 0
#endif

// libc++
#elif SCN_STDLIB_LIBCPP

#define SCN_HAS_FLOAT_CHARCONV 0

#if SCN_STDLIB_LIBCPP >= 7000
#define SCN_HAS_INTEGER_CHARCONV 1
#else
#define SCN_HAS_INTEGER_CHARCONV 0
#endif

// other
#elif defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201606L
#define SCN_HAS_INTEGER_CHARCONV 1
#define SCN_HAS_FLOAT_CHARCONV   1
#endif  // _GLIBCXX_RELEASE

#endif  // std >= 17

#ifndef SCN_HAS_INTEGER_CHARCONV
#define SCN_HAS_INTEGER_CHARCONV 0
#define SCN_HAS_FLOAT_CHARCONV   0
#endif

// Detect std::launder
#if defined(__cpp_lib_launder) && __cpp_lib_launder >= 201606
#define SCN_HAS_LAUNDER 1
#else
#define SCN_HAS_LAUNDER 0
#endif

// Detect <bit> operations
#if defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L && \
    SCN_STD >= SCN_STD_20
#define SCN_HAS_BITOPS 1
#else
#define SCN_HAS_BITOPS 0
#endif

// Detect __assume
#if SCN_INTEL || SCN_MSVC
#define SCN_HAS_ASSUME 1
#else
#define SCN_HAS_ASSUME 0
#endif

// Detect __builtin_assume
#if SCN_HAS_BUILTIN(__builtin_assume)
#define SCN_HAS_BUILTIN_ASSUME 1
#else
#define SCN_HAS_BUILTIN_ASSUME 0
#endif

// Detect std::assume_aligned
#if defined(__cpp_lib_assume_aligned) && \
    __cpp_lib_assume_aligned >= 201811L && SCN_STD >= SCN_STD_20
#define SCN_HAS_STD_ASSUME_ALIGNED 1
#else
#define SCN_HAS_STD_ASSUME_ALIGNED 0
#endif

// Detect __builtin_assume_aligned
#if SCN_HAS_BUILTIN(__builtin_assume_aligned) || SCN_GCC
#define SCN_HAS_BUILTIN_ASSUME_ALIGNED 1
#else
#define SCN_HAS_BUILTIN_ASSUME_ALIGNED 0
#endif

// Detect __assume_aligned
#if SCN_HAS_BUILTIN(__assume_aligned) || SCN_INTEL
#define SCN_HAS_ASSUME_ALIGNED 1
#else
#define SCN_HAS_ASSUME_ALIGNED 0
#endif

// Detect std::unreachable
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L && \
    SCN_STD >= SCN_STD_23
#define SCN_HAS_STD_UNREACHABLE 1
#else
#define SCN_HAS_STD_UNREACHABLE 0
#endif

// Detect __builtin_unreachable
#if SCN_HAS_BUILTIN(__builtin_unreachable) || SCN_GCC
#define SCN_HAS_BUILTIN_UNREACHABLE 1
#else
#define SCN_HAS_BUILTIN_UNREACHABLE 0
#endif

// Detect __builtin_expect
#if SCN_HAS_BUILTIN(__builtin_expect) || SCN_GCC
#define SCN_HAS_BUILTIN_EXPECT 1
#else
#define SCN_HAS_BUILTIN_EXPECT 0
#endif

// Detect __builtin_add_overflow etc.
#if SCN_HAS_BUILTIN(__builtin_add_overflow) || SCN_GCC
#define SCN_HAS_BUILTIN_OVERFLOW 1
#else
#define SCN_HAS_BUILTIN_OVERFLOW 0
#endif

// Detect concepts
#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
#define SCN_HAS_CONCEPTS 1
#else
#define SCN_HAS_CONCEPTS 0
#endif

// Detect ranges (202110 = owning_view, P2415, C++20 DR)
#if defined(__cpp_lib_ranges) && __cpp_lib_ranges >= 202110L
#define SCN_HAS_RANGES 1
#else
#define SCN_HAS_RANGES 0
#endif

// Detect char8_t
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811L
#define SCN_HAS_CHAR8 1
#else
#define SCN_HAS_CHAR8 0
#endif

// Detect consteval
#if defined(__cpp_consteval) && __cpp_consteval >= 201811L && \
    SCN_STD >= SCN_STD_20
#define SCN_HAS_CONSTEVAL 1
#else
#define SCN_HAS_CONSTEVAL 0
#endif

// Detect std::span
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L && \
    SCN_STD >= SCN_STD_20
#define SCN_HAS_STD_SPAN 1
#else
#define SCN_HAS_STD_SPAN 0
#endif

// Detect std::regex_constants::multiline
#if SCN_STDLIB_LIBCPP || SCN_STDLIB_GLIBCXX >= 11
#define SCN_HAS_STD_REGEX_MULTILINE 1
#else
#define SCN_HAS_STD_REGEX_MULTILINE 0
#endif

// Detect endianness
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SCN_IS_BIG_ENDIAN 1
#else
#define SCN_IS_BIG_ENDIAN 0
#endif

#elif SCN_WINDOWS
#define SCN_IS_BIG_ENDIAN 0
#else

#if SCN_APPLE
#include <machine/endian.h>
#elif defined(sun) || defined(__sun)
#include <sys/byteorder.h>
#elif SCN_HAS_INCLUDE(<endian.h>)
#include <endian.h>
#endif

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__)
#define SCN_IS_BIG_ENDIAN 0
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SCN_IS_BIG_ENDIAN 0
#else
#define SCN_IS_BIG_ENDIAN 1
#endif

#endif  // defined __BYTE_ORDER__ && defined __ORDER_BIG_ENDIAN__

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
