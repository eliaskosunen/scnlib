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

#include <scn/detail/config.h>

#include <cassert>

#define SCN_STRINGIFY_APPLY(x) #x
#define SCN_STRINGIFY(x)       SCN_STRINGIFY_APPLY(x)

// Warning control
#if SCN_GCC
#define SCN_PRAGMA_APPLY(x) _Pragma(#x)

#define SCN_GCC_PUSH        _Pragma("GCC diagnostic push")
#define SCN_GCC_POP         _Pragma("GCC diagnostic pop")

#define SCN_GCC_IGNORE(x)   SCN_PRAGMA_APPLY(GCC diagnostic ignored x)
#else
#define SCN_GCC_PUSH
#define SCN_GCC_POP
#define SCN_GCC_IGNORE(x)
#endif

#if SCN_CLANG
#define SCN_PRAGMA_APPLY(x) _Pragma(#x)

#define SCN_CLANG_PUSH      _Pragma("clang diagnostic push")
#define SCN_CLANG_POP       _Pragma("clang diagnostic pop")

#define SCN_CLANG_IGNORE(x) SCN_PRAGMA_APPLY(clang diagnostic ignored x)

#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
#define SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE \
    SCN_CLANG_PUSH SCN_CLANG_IGNORE("-Wundefined-func-template")
#define SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE SCN_CLANG_POP
#else
#define SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
#define SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
#endif

#else
#define SCN_CLANG_PUSH
#define SCN_CLANG_POP
#define SCN_CLANG_IGNORE(x)
#define SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
#define SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
#endif

#if SCN_GCC_COMPAT && defined(SCN_PRAGMA_APPLY)
#define SCN_GCC_COMPAT_PUSH      SCN_PRAGMA_APPLY(GCC diagnostic push)
#define SCN_GCC_COMPAT_POP       SCN_PRAGMA_APPLY(GCC diagnostic pop)
#define SCN_GCC_COMPAT_IGNORE(x) SCN_PRAGMA_APPLY(GCC diagnostic ignored x)
#else
#define SCN_GCC_COMPAT_PUSH
#define SCN_GCC_COMPAT_POP
#define SCN_GCC_COMPAT_IGNORE(x)
#endif

#if SCN_MSVC
#define SCN_MSVC_PUSH      __pragma(warning(push))
#define SCN_MSVC_POP       __pragma(warning(pop))

#define SCN_MSVC_IGNORE(x) __pragma(warning(disable : x))
#else
#define SCN_MSVC_PUSH
#define SCN_MSVC_POP
#define SCN_MSVC_IGNORE(x)
#endif

// SCN_CONSTEVAL
#if SCN_HAS_CONSTEVAL
#define SCN_CONSTEVAL consteval
#else
#define SCN_CONSTEVAL /*consteval*/
#endif

// SCN_NODISCARD
#if SCN_HAS_NODISCARD
#define SCN_NODISCARD [[nodiscard]]
#else
#define SCN_NODISCARD /*nodiscard*/
#endif

// SCN_MAYBE_UNUSED
#if SCN_HAS_MAYBE_UNUSED
#define SCN_MAYBE_UNUSED [[maybe_unused]]
#else
#define SCN_MAYBE_UNUSED /*maybe_unused*/
#endif

// SCN_NO_UNIQUE_ADDRESS
#if SCN_HAS_NO_UNIQUE_ADDRESS
#define SCN_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define SCN_NO_UNIQUE_ADDRESS /*no_unique_address*/
#endif

// SCN_FALLTHROUGH
#if SCN_HAS_FALLTHROUGH_CPPATTRIBUTE
#define SCN_FALLTHROUGH [[fallthrough]]
#elif SCN_HAS_FALLTHROUGH_CPPGNUATTRIBUTE
#define SCN_FALLTHROUGH [[gnu::fallthrough]]
#elif SCN_HAS_FALLTHROUGH_CPPCLANGATTRIBUTE
#define SCN_FALLTHROUGH [[clang::fallthrough]]
#elif SCN_HAS_FALLTHROUGH_GCCATTRIBUTE
#define SCN_FALLTHROUGH __attribute__((fallthrough))
#else
#define SCN_FALLTHROUGH    \
    do { /* fallthrough */ \
    } while (false)
#endif

// SCN_TRIVIAL_ABI
#if SCN_HAS_TRIVIAL_ABI && SCN_USE_TRIVIAL_ABI
#define SCN_TRIVIAL_ABI [[clang::trivial_abi]]
#else
#define SCN_TRIVIAL_ABI /*trivial_abi*/
#endif

// SCN_LIKELY & SCN_UNLIKELY
#if SCN_HAS_BUILTIN_EXPECT
#define SCN_LIKELY(x)   __builtin_expect(!!(x), 1)
#define SCN_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SCN_LIKELY(x)   (x)
#define SCN_UNLIKELY(x) (x)
#endif

// SCN_ASSUME
#if SCN_HAS_ASSUME
#define SCN_ASSUME(x) __assume(x)
#elif SCN_HAS_BUILTIN_ASSUME
#define SCN_ASSUME(x) __builtin_assume(x)
#elif SCN_HAS_BUILTIN_UNREACHABLE
#define SCN_ASSUME(x) ((x) ? static_cast<void>(0) : __builtin_unreachable())
#else
#define SCN_ASSUME(x) static_cast<void>((x) ? 0 : 0)
#endif

// SCN_UNREACHABLE
#if SCN_HAS_BUILTIN_UNREACHABLE
#define SCN_UNREACHABLE __builtin_unreachable()
#else
#define SCN_UNREACHABLE SCN_ASSUME(0)
#endif

// SCN_NOEXCEPT
#if SCN_DOXYGEN || (SCN_HAS_EXCEPTIONS && SCN_USE_EXCEPTIONS)
#define SCN_NOEXCEPT        noexcept
#define SCN_NOEXCEPT_P(...) noexcept(__VA_ARGS__)
#else
#define SCN_NOEXCEPT        /*noexcept*/
#define SCN_NOEXCEPT_P(...) /*noexcept_p*/
#endif

// SCN_TRY and friends
#if SCN_HAS_EXCEPTIONS && SCN_USE_EXCEPTIONS
#define SCN_TRY      try
#define SCN_CATCH(x) catch (x)
#define SCN_THROW(x) throw x
#define SCN_RETHROW  throw
#else
#define SCN_TRY      if (true)
#define SCN_CATCH(x) if (false)
#define SCN_THROW(x) ::std::abort()
#define SCN_RETHROW  ::std::abort()
#endif

// clang currently can't process libstdc++ ranges::view_interface
#if SCN_CLANG && defined(_GLIBCXX_RELEASE)
#define SCN_ENVIRONMENT_SUPPORTS_RANGES 0
#else
#define SCN_ENVIRONMENT_SUPPORTS_RANGES 1
#endif

#if SCN_HAS_CONCEPTS && SCN_HAS_RANGES && SCN_ENVIRONMENT_SUPPORTS_RANGES
#define SCN_STD_RANGES 1
#else
#define SCN_STD_RANGES 0
#endif

#define SCN_UNUSED(x) static_cast<void>(sizeof(x))

// SCN_ASSERT
#ifdef NDEBUG
#define SCN_ASSERT(cond, msg)        \
    do {                             \
        SCN_CLANG_PUSH               \
        SCN_CLANG_IGNORE("-Wassume") \
        SCN_ASSUME(!!(cond));        \
        SCN_CLANG_POP                \
    } while (false)
#else
#define SCN_ASSERT(cond, msg) assert((cond) && msg)
#endif

#define SCN_EXPECT(cond) SCN_ASSERT(cond, "Precondition violation")
#define SCN_ENSURE(cond) SCN_ASSERT(cond, "Postcondition violation")

#define SCN_MOVE(x) \
    static_cast<    \
        typename ::scn::detail::remove_reference<decltype(x)>::type&&>(x)
#define SCN_FWD(x)          static_cast<decltype(x)&&>(x)
#define SCN_DECLVAL(T)      static_cast<T (*)()>(nullptr)()

#define SCN_BEGIN_NAMESPACE inline namespace v2 {
#define SCN_END_NAMESPACE   }
