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

#include <scn/detail/pp_detect.h>

#define SCN_VERSION SCN_COMPILER(2, 0, 0)

// SCN_USE_EXCEPTIONS
// If 0, removes all `noexcept` annotations,
// and exception handling around stdlib facilities.
#ifndef SCN_USE_EXCEPTIONS
#define SCN_USE_EXCEPTIONS 1
#endif

// SCN_USE_TRIVIAL_ABI
// If 1, uses [[clang::trivial_abi]] in some classes, if available.
#ifndef SCN_USE_TRIVIAL_ABI
#define SCN_USE_TRIVIAL_ABI 1
#endif

// SCN_USE_STD_RANGES
// If 1, uses stdlib <ranges>, if available.
// If 0, always falls back to bundled nanorange
// (include/scn/external/nanorange).
#ifndef SCN_USE_STD_RANGES
#define SCN_USE_STD_RANGES 1
#endif

#if 0
// TODO: SCN_USE_DYNAMIC_ALLOCATION
// If 0, removes all instances of dynamic allocation from the library,
// including references to standard facilities that may allocate.
#ifndef SCN_USE_DYNAMIC_ALLOCATION
#define SCN_USE_DYNAMIC_ALLOCATION 1
#endif
#endif

// SCN_DISABLE_REGEX
// If 1, disabled regular expression support
#ifndef SCN_DISABLE_REGEX
#define SCN_DISABLE_REGEX 0
#endif

// SCN_REGEX_BOOST_USE_ICU
// If 1, use ICU for full Unicode support with the regex backend
// Only effective when SCN_REGEX_BACKEND is Boost
#ifndef SCN_REGEX_BOOST_USE_ICU
#define SCN_REGEX_BOOST_USE_ICU 0
#endif

// std::regex
#define SCN_REGEX_BACKEND_STD   0
// Boost.Regex
#define SCN_REGEX_BACKEND_BOOST 1
// Google RE2
#define SCN_REGEX_BACKEND_RE2   2
// TODO: PCRE (Perl Compatible Regular Expressions)
// #define SCN_REGEX_BACKEND_PCRE  3
// TODO: CTRE (Compile-Time Regular Expressions)
// #define SCN_REGEX_BACKEND_CTRE  4

// Default to std::regex
#ifndef SCN_REGEX_BACKEND
#define SCN_REGEX_BACKEND SCN_REGEX_BACKEND_STD
#endif

#if SCN_REGEX_BACKEND < SCN_REGEX_BACKEND_STD || \
    SCN_REGEX_BACKEND > SCN_REGEX_BACKEND_RE2
#error "Invalid regex backend"
#endif

#if SCN_REGEX_BOOST_USE_ICU && SCN_REGEX_BACKEND != SCN_REGEX_BACKEND_BOOST
#error "SCN_REGEX_BOOST_USE_ICU requires the Boost SCN_REGEX_BACKEND"
#endif

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
#define SCN_REGEX_SUPPORTS_NAMED_CAPTURES 0
#else
#define SCN_REGEX_SUPPORTS_NAMED_CAPTURES 1
#endif

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
#define SCN_REGEX_SUPPORTS_WIDE_STRINGS 0
#else
#define SCN_REGEX_SUPPORTS_WIDE_STRINGS 1
#endif

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2 || SCN_REGEX_BOOST_USE_ICU
#define SCN_REGEX_SUPPORTS_UTF8_CLASSIFICATION 1
#else
#define SCN_REGEX_SUPPORTS_UTF8_CLASSIFICATION 0
#endif

// SCN_DISABLE_IOSTREAM
// If 1, removes all references and functionality related to standard streams.
#ifndef SCN_DISABLE_IOSTREAM
#define SCN_DISABLE_IOSTREAM 0
#endif

// SCN_DISABLE_TRANSCODING
// If 1, removes the ability to read narrow data from wide sources,
// and vice versa
#ifndef SCN_DISABLE_TRANSCODING
#define SCN_DISABLE_TRANSCODING 0
#endif

// SCN_DISABLE_LOCALE
// If 1, removes all references to std::locale, and C locale
#ifndef SCN_DISABLE_LOCALE
#define SCN_DISABLE_LOCALE 0
#endif

// SCN_DISABLE_FROM_CHARS
// If 1, disallows the float scanner from falling back on std::from_chars,
// even if it were available
#ifndef SCN_DISABLE_FROM_CHARS
#define SCN_DISABLE_FROM_CHARS 0
#endif

// SCN_DISABLE_STRTOD
// If 1, disallows the float scanner from falling back on std::strtod,
// even if it were available
#ifndef SCN_DISABLE_STRTOD
#define SCN_DISABLE_STRTOD 0
#endif

// SCN_DISABLE_TYPE_*
// If 1, removes ability to scan type
#ifndef SCN_DISABLE_TYPE_SCHAR
#define SCN_DISABLE_TYPE_SCHAR 0
#endif
#ifndef SCN_DISABLE_TYPE_SHORT
#define SCN_DISABLE_TYPE_SHORT 0
#endif
#ifndef SCN_DISABLE_TYPE_INT
#define SCN_DISABLE_TYPE_INT 0
#endif
#ifndef SCN_DISABLE_TYPE_LONG
#define SCN_DISABLE_TYPE_LONG 0
#endif
#ifndef SCN_DISABLE_TYPE_LONG_LONG
#define SCN_DISABLE_TYPE_LONG_LONG 0
#endif
#ifndef SCN_DISABLE_TYPE_UCHAR
#define SCN_DISABLE_TYPE_UCHAR 0
#endif
#ifndef SCN_DISABLE_TYPE_USHORT
#define SCN_DISABLE_TYPE_USHORT 0
#endif
#ifndef SCN_DISABLE_TYPE_UINT
#define SCN_DISABLE_TYPE_UINT 0
#endif
#ifndef SCN_DISABLE_TYPE_ULONG
#define SCN_DISABLE_TYPE_ULONG 0
#endif
#ifndef SCN_DISABLE_TYPE_ULONG_LONG
#define SCN_DISABLE_TYPE_ULONG_LONG 0
#endif
#ifndef SCN_DISABLE_TYPE_POINTER
#define SCN_DISABLE_TYPE_POINTER 0
#endif
#ifndef SCN_DISABLE_TYPE_BOOL
#define SCN_DISABLE_TYPE_BOOL 0
#endif
#ifndef SCN_DISABLE_TYPE_CHAR
#define SCN_DISABLE_TYPE_CHAR 0
#endif
#ifndef SCN_DISABLE_TYPE_CHAR32
#define SCN_DISABLE_TYPE_CHAR32 0
#endif
#ifndef SCN_DISABLE_TYPE_FLOAT
#define SCN_DISABLE_TYPE_FLOAT 0
#endif
#ifndef SCN_DISABLE_TYPE_DOUBLE
#define SCN_DISABLE_TYPE_DOUBLE 0
#endif
#ifndef SCN_DISABLE_TYPE_LONG_DOUBLE
#define SCN_DISABLE_TYPE_LONG_DOUBLE 0
#endif
#ifndef SCN_DISABLE_TYPE_STRING
#define SCN_DISABLE_TYPE_STRING 0
#endif
#ifndef SCN_DISABLE_TYPE_STRING_VIEW
#define SCN_DISABLE_TYPE_STRING_VIEW 0
#endif
#ifndef SCN_DISABLE_TYPE_CUSTOM
#define SCN_DISABLE_TYPE_CUSTOM 0
#endif
