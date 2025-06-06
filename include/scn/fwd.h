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

#include <scn/macros.h>

#if defined(SCN_MODULE) && defined(SCN_IMPORT_STD)
#include <cassert>
import std;
#else
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#endif

/////////////////////////////////////////////////////////////////
// Forward declarations
/////////////////////////////////////////////////////////////////

/**
 * scnlib namespace, containing the library interface
 */
namespace scn {
SCN_BEGIN_NAMESPACE

/// Placeholder monostate type
struct monostate {};

template <typename Context>
class basic_scan_arg;
template <typename Context>
class basic_scan_args;

template <typename Range, typename CharT>
class basic_scan_context;

namespace detail {
struct buffer_range_tag {};

template <typename CharT>
using default_context = basic_scan_context<buffer_range_tag, CharT>;
}  // namespace detail

using scan_context = basic_scan_context<detail::buffer_range_tag, char>;
using wscan_context = basic_scan_context<detail::buffer_range_tag, wchar_t>;

using scan_args = basic_scan_args<scan_context>;
using wscan_args = basic_scan_args<wscan_context>;

class scan_error;

/**
 * A C++23-like `expected`.
 *
 * \ingroup result
 */
template <typename T, typename E>
class expected;

template <typename CharT>
struct basic_runtime_format_string;
template <typename CharT, typename Source, typename... Args>
class basic_scan_format_string;

namespace detail {
template <typename T>
struct type_identity {
    using type = T;
};
template <typename T>
using type_identity_t = typename type_identity<T>::type;
}  // namespace detail

template <typename Source, typename... Args>
using scan_format_string =
    basic_scan_format_string<char,
                             detail::type_identity_t<Source>,
                             detail::type_identity_t<Args>...>;
template <typename Source, typename... Args>
using wscan_format_string =
    basic_scan_format_string<wchar_t,
                             detail::type_identity_t<Source>,
                             detail::type_identity_t<Args>...>;

struct invalid_input_range;

#if !SCN_DISABLE_IOSTREAM

template <typename CharT>
struct basic_istream_scanner;

///
using istream_scanner = basic_istream_scanner<char>;
///
using wistream_scanner = basic_istream_scanner<wchar_t>;
#endif  // SCN_USE_IOSTREAMS

template <typename CharT>
class basic_scan_parse_context;

///
using scan_parse_context = basic_scan_parse_context<char>;
///
using wscan_parse_context = basic_scan_parse_context<wchar_t>;

namespace detail {
template <typename CharT>
class compile_parse_context;
}

template <typename Iterator, typename... Args>
class scan_result;

struct file_marker {
    constexpr file_marker() noexcept = default;
    template <typename... Args>
    constexpr file_marker(Args&&...) noexcept
    {
    }
};

namespace detail {
template <typename CharT>
class basic_scan_buffer;

using scan_buffer = basic_scan_buffer<char>;
using wscan_buffer = basic_scan_buffer<wchar_t>;
}  // namespace detail

/**
 * Scanner type, can be customized to enable scanning of user-defined types
 *
 * \ingroup ctx
 */
template <typename T, typename CharT = char, typename Enable = void>
struct scanner {
    /// Default fallback implementation, not constructible, always an error.
    scanner() = delete;

    /**
     * Parse the format string contained in `pctx`, and populate `*this` with
     * the parsed format specifier values, to be used later in `scan()`.
     *
     * Should be `constexpr` to allow for compile-time format string checking.
     *
     * A common pattern is to inherit a `scanner` implementation from another
     * `scanner`, while only overriding `scan()`, and keeping the same
     * `parse()`, or at least delegating to it.
     *
     * To report errors, an exception derived from `std::exception` can be
     * thrown, or `ParseContext::on_error` can be called.
     *
     * \return On success, an iterator pointing to the `}` character at the end
     * of the replacement field in the format string.
     * Will cause an error, if the returned iterator doesn't point to a `}`
     * character.
     */
    template <typename ParseContext>
    constexpr auto parse(ParseContext& pctx) ->
        typename ParseContext::iterator = delete;

    /**
     * Scan a value of type `T` from `ctx` into `value`,
     * using the format specs in `*this`, populated by `parse()`.
     *
     * `value` is guaranteed to only be default initialized.
     *
     * \return On success, an iterator pointing past the last character consumed
     * from `ctx`.
     */
    template <typename Context>
    auto scan(T& value, Context& ctx) const
        -> expected<typename Context::iterator, scan_error> = delete;
};

template <typename T>
struct discard;

namespace detail {
template <typename T, size_t N>
class basic_buffer;
}  // namespace detail

namespace detail {
template <typename T, typename = void>
struct pointer_traits;
}  // namespace detail

namespace detail {
struct dummy_type {};

template <typename T>
struct tag_type {
    using type = T;
};

template <typename...>
struct dependent_false : std::false_type {};

template <typename T>
struct remove_reference {
    using type = T;
};
template <typename T>
struct remove_reference<T&> {
    using type = T;
};
template <typename T>
struct remove_reference<T&&> {
    using type = T;
};

template <std::size_t I>
struct priority_tag : priority_tag<I - 1> {};
template <>
struct priority_tag<0> {};

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T, typename Self>
inline constexpr bool is_not_self = !std::is_same_v<remove_cvref_t<T>, Self>;
}  // namespace detail

template <typename CharT>
class basic_regex_match;
template <typename CharT>
class basic_regex_matches;

using regex_match = basic_regex_match<char>;
using wregex_match = basic_regex_match<wchar_t>;

using regex_matches = basic_regex_matches<char>;
using wregex_matches = basic_regex_matches<wchar_t>;

#if SCN_HAS_INT128

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wpedantic")

using int128 = __int128;
using uint128 = unsigned __int128;

SCN_GCC_POP

#endif

SCN_END_NAMESPACE
}  // namespace scn
