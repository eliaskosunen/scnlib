// Copyright 2017-2018 Elias Kosunen
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

#ifndef SCN_CONFIG_H
#define SCN_CONFIG_H

#include <cstddef>

#define SCN_STD_11 201103L
#define SCN_STD_14 201402L
#define SCN_STD_17 201703L

#define SCN_COMPILER(major, minor, patch) \
    ((major)*10000000 /* 10,000,000 */ + (minor)*10000 /* 10,000 */ + (patch))

#ifdef __INTEL_COMPILER
// Intel
#define SCN_INTEL                                                      \
    SCN_COMPILER(__INTEL_COMPILER / 100, (__INTEL_COMPILER / 10) % 10, \
                 __INTEL_COMPILER % 10)
#elif defined(_MSC_VER) && defined(_MSC_FULL_VER)
// MSVC
#if _MSC_VER == _MSC_FULL_VER / 10000
#define SCN_MSVC \
    SCN_COMPILER(_MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 10000)
#else
#define SCN_MSVC                                                \
    SCN_COMPILER(_MSC_VER / 100, (_MSC_FULL_VER / 100000) % 10, \
                 _MSC_FULL_VER % 100000)
#endif  // _MSC_VER == _MSC_FULL_VER / 10000
#elif defined(__clang__) && defined(__clang_minor__) && \
    defined(__clang_patchlevel__)
// Clang
#define SCN_CLANG \
    SCN_COMPILER(__clang_major__, __clang_minor__, __clang_patchlevel__)
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && \
    defined(__GNUC_PATCHLEVEL__)
#define SCN_GCC SCN_COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#endif

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

#ifdef __has_include
#define SCN_HAS_INCLUDE(x) __has_include(x)
#else
#define SCN_HAS_INCLUDE(x) 0
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

// Detect constexpr
#if defined(__cpp_constexpr)
#if __cpp_constexpr >= 201304
#define SCN_HAS_RELAXED_CONSTEXPR 1
#else
#define SCN_HAS_RELAXED_CONSTEXPR 0
#endif
#endif

#ifndef SCN_HAS_RELAXED_CONSTEXPR
#if SCN_HAS_FEATURE(cxx_relaxed_constexpr) || \
    SCN_MSVC >= SCN_COMPILER(19, 10, 0) ||    \
    ((SCN_GCC >= SCN_COMPILER(6, 0, 0) ||     \
      SCN_INTEL >= SCN_COMPILER(17, 0, 0)) && \
     __cplusplus >= SCN_STD_14)
#define SCN_HAS_RELAXED_CONSTEXPR 1
#else
#define SCN_HAS_RELAXED_CONSTEXPR 0
#endif
#endif

#if SCN_HAS_RELAXED_CONSTEXPR
#define SCN_CONSTEXPR constexpr
#define SCN_CONSTEXPR14 constexpr
#define SCN_CONSTEXPR_DECL constexpr
#else
#define SCN_CONSTEXPR constexpr
#define SCN_CONSTEXPR14 inline
#define SCN_CONSTEXPR_DECL
#endif

// Detect [[nodiscard]]
#if (SCN_HAS_CPP_ATTRIBUTE(nodiscard) && __cplusplus >= SCN_STD_17) ||      \
    (SCN_MSVC >= SCN_COMPILER(19, 11, 0) && SCN_MSVC_LANG >= SCN_STD_17) || \
    ((SCN_GCC >= SCN_COMPILER(7, 0, 0) ||                                   \
      SCN_INTEL >= SCN_COMPILER(18, 0, 0)) &&                               \
     __cplusplus >= SCN_STD_17)
#define SCN_NODISCARD [[nodiscard]]
#else
#define SCN_NODISCARD /*nodiscard*/
#endif

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY
#define SCN_FUNC inline
#else
#define SCN_FUNC
#endif

// Detect <charconv>
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201606
#define SCN_HAS_CHARCONV 1
#else
#define SCN_HAS_CHARCONV 0
#endif

#endif  // SCN_CONFIG_H
