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

#include <scn/detail/args.h>
#include <scn/detail/format_string.h>
#include <scn/detail/result.h>
#include <scn/detail/scanner_builtin.h>
#include <scn/detail/vscan.h>

namespace scn {
SCN_BEGIN_NAMESPACE

/**
 * Convenience function to construct the value to return from `scan`,
 * based on the return value of `vscan`, and the argument store.
 *
 * Takes its arguments by rvalue reference to disallow extraneous copying.
 *
 * Note: Because `vscan` places the values it scanned into the argument
 * store passed to it, the call to `make_scan_result` needs to happen
 * strictly after calling `vscan`. This means, that this is UB:
 * `return scn::make_scan_result(scn::vscan(...), std::move(args));`
 *
 * Example:
 * \code{.cpp}
 * auto args = scn::make_scan_args<Source, Args...>();
 * auto result = scn::vscan(std::forward<Source>(source), format, args);
 *
 * return scn::make_scan_result(std::move(result), std::move(args));
 * \endcode
 *
 * \ingroup result
 */
template <typename Result, typename Context, typename... Args>
auto make_scan_result(scan_expected<Result>&& result,
                      scan_arg_store<Context, Args...>&& args)
    -> scan_expected<scan_result<Result, Args...>>
{
    if (SCN_UNLIKELY(!result)) {
        return unexpected(result.error());
    }
    return scan_result{SCN_MOVE(*result), SCN_MOVE(args.args())};
}

/**
 * The return type of `scan`, based on the type of the source, and the
 * types of the scanned arguments.
 */
template <typename Source, typename... Args>
using scan_result_type =
    scan_expected<scan_result<detail::scan_result_value_type<Source>, Args...>>;

namespace detail {
// Boilerplate for scan()
template <typename CharT, typename... Args, typename Source, typename Format>
auto scan_impl(Source&& source,
               Format format,
               std::tuple<Args...> default_values)
    -> scan_result_type<Source, Args...>
{
    auto args = make_scan_args<basic_scan_context<CharT>, Args...>(
        SCN_MOVE(default_values));
    auto result = vscan(SCN_FWD(source), format, args);
    return make_scan_result(SCN_MOVE(result), SCN_MOVE(args));
}
}  // namespace detail

/**
 * \defgroup scan Basic scanning API
 *
 * \brief The core public-facing interface of the library
 *
 * The following functions use a format string syntax similar to that of
 * `std::format`. See more at \ref format-string.
 *
 * When these functions take a `source` as input, it must
 * model the `scannable_source` concept. See more at \ref scannable.
 */

/**
 * Scans `Args...` from `source`, according to the
 * specifications given in the format string (`format`).
 * Returns the resulting values in an object of type `scan_result`,
 * alongside a `subrange` pointing to the unused input.
 *
 * Example:
 * \code{.cpp}
 * if (auto result = scn::scan<int>("123", "{}"))
 *     int value = result->value();
 * \endcode
 *
 * \ingroup scan
 */
template <typename... Args,
          typename Source,
          typename = std::enable_if_t<detail::is_file_or_narrow_range<Source>>>
SCN_NODISCARD auto scan(Source&& source,
                        scan_format_string<Source, Args...> format)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_impl<char, Args...>(SCN_FWD(source), format, {});
}

/**
 * `scan` with explicitly supplied default values
 *
 * Can be used, for example, for pre-allocating a scanned string:
 *
 * \code{.cpp}
 * std::string str;
 * str.reserve(64);
 *
 * // As long as the read string fits in `str`,
 * // does not allocate
 * auto result = scn::scan<std::string>(source, "{}",
 *                                      {std::move(str)});
 * // Access the read string with result->value()
 * \endcode
 *
 * \ingroup scan
 */
template <typename... Args,
          typename Source,
          typename = std::enable_if_t<detail::is_file_or_narrow_range<Source>>>
SCN_NODISCARD auto scan(Source&& source,
                        scan_format_string<Source, Args...> format,
                        std::tuple<Args...>&& default_args)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_impl<char, Args...>(SCN_FWD(source), format,
                                            SCN_MOVE(default_args));
}

namespace detail {
// Boilerplate for scan(const locale&)
template <typename CharT,
          typename... Args,
          typename Locale,
          typename Source,
          typename Format>
auto scan_localized_impl(const Locale& loc,
                         Source&& source,
                         Format format,
                         std::tuple<Args...> default_values)
    -> scan_result_type<Source, Args...>
{
    auto args = make_scan_args<basic_scan_context<CharT>, Args...>(
        SCN_MOVE(default_values));
    auto result = vscan(loc, SCN_FWD(source), format, args);
    return make_scan_result(SCN_MOVE(result), SCN_MOVE(args));
}
}  // namespace detail

/**
 * \defgroup locale Localization
 *
 * \brief Scanning APIs that allow passing in a locale
 */

/**
 * `scan` using an explicit locale.
 *
 * Has no effect on its own, locale-specific scanning still needs to be
 * opted-into on an argument-by-argument basis, with the `L` format string
 * specifier.
 *
 * \code{.cpp}
 * auto result = scn::scan<double>(
 *     std::locale{"fi_FI.UTF-8"}, "3,14, "{:L}");
 * // result->value() == 3.14
 * \endcode
 *
 * \ingroup locale
 */
template <typename... Args,
          typename Locale,
          typename Source,
          typename = std::enable_if_t<detail::is_file_or_narrow_range<Source>>,
          typename = std::void_t<decltype(Locale::classic())>>
SCN_NODISCARD auto scan(const Locale& loc,
                        Source&& source,
                        scan_format_string<Source, Args...> format)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_localized_impl<char, Args...>(loc, SCN_FWD(source),
                                                      format, {});
}

/**
 * `scan` with a locale and default values
 *
 * \ingroup locale
 */
template <typename... Args,
          typename Locale,
          typename Source,
          typename = std::enable_if_t<detail::is_file_or_narrow_range<Source>>,
          typename = std::void_t<decltype(Locale::classic())>>
SCN_NODISCARD auto scan(const Locale& loc,
                        Source&& source,
                        scan_format_string<Source, Args...> format,
                        std::tuple<Args...>&& default_args)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_localized_impl<char, Args...>(
        loc, SCN_FWD(source), format, SCN_MOVE(default_args));
}

namespace detail {
template <typename CharT, typename T, typename Source>
auto scan_value_impl(Source&& source, T value) -> scan_result_type<Source, T>
{
    auto arg = detail::make_arg<basic_scan_context<CharT>>(value);
    SCN_TRY(it, vscan_value(SCN_FWD(source), arg));
    return scan_result{SCN_MOVE(it), std::tuple{SCN_MOVE(value)}};
}
}  // namespace detail

/**
 * `scan` a single value, with default options.
 *
 * Essentially equivalent to: `scn::scan<T>(source, "{}")`,
 * except it can skip parsing the format string, gaining performance.
 *
 * \ingroup scan
 */
template <typename T,
          typename Source,
          typename = std::enable_if_t<detail::is_file_or_narrow_range<Source>>>
SCN_NODISCARD auto scan_value(Source&& source) -> scan_result_type<Source, T>
{
    return detail::scan_value_impl<char>(SCN_FWD(source), T{});
}

/**
 * `scan` a single value, with default options, and a default value.
 *
 * \ingroup scan
 */
template <typename T,
          typename Source,
          std::enable_if_t<detail::is_file_or_narrow_range<Source>>* = nullptr>
SCN_NODISCARD auto scan_value(Source&& source, T default_value)
    -> scan_result_type<Source, T>
{
    return detail::scan_value_impl<char>(SCN_FWD(source),
                                         SCN_MOVE(default_value));
}

/**
 * Scan from `stdin`.
 *
 * Equivalent to `scn::scan<...>(stdin, ...)`.
 *
 * \code{.cpp}
 * auto result = scn::input<int>("{}");
 * \endcode
 *
 * \ingroup scan
 */
template <typename... Args>
SCN_NODISCARD auto input(scan_format_string<std::FILE*, Args...> format)
    -> scan_result_type<std::FILE*, Args...>
{
    auto args = make_scan_args<scan_context, Args...>();
    auto err = vinput(format, args);
    if (SCN_UNLIKELY(!err)) {
        return unexpected(err);
    }
    return scan_result{stdin, SCN_MOVE(args.args())};
}

/**
 * Write msg to stdout, and call `input<Args...>(format)`
 *
 * \ingroup scan
 */
template <typename... Args>
SCN_NODISCARD auto prompt(const char* msg,
                          scan_format_string<std::FILE*, Args...> format)
    -> scan_result_type<std::FILE*, Args...>
{
    std::printf("%s", msg);
    std::fflush(stdout);
    return input<Args...>(format);
}

namespace detail {
template <typename T>
inline constexpr bool is_scan_int_type =
    std::is_integral_v<T> && !std::is_same_v<T, char> &&
    !std::is_same_v<T, wchar_t> && !std::is_same_v<T, char32_t> &&
    !std::is_same_v<T, bool>;
}

/**
 * Fast integer reading.
 *
 * Quickly reads an integer from a `std::string_view`. Skips preceding
 * whitespace.
 *
 * Reads in the specified base,
 * allowing a base prefix. Set `base` to `0` to detect the base from the
 * input. `base` must either be `0`, or in range `[2, 36]`.
 */
template <typename T, std::enable_if_t<detail::is_scan_int_type<T>>* = nullptr>
SCN_NODISCARD auto scan_int(std::string_view source, int base = 10)
    -> scan_result_type<std::string_view, T>
{
    T value{};
    SCN_TRY(it, detail::scan_int_impl(source, value, base));
    return scan_result{ranges::subrange{it, source.end()}, std::tuple{value}};
}

namespace detail {
template <bool Val, typename T>
inline constexpr bool dependent_bool = Val;
}

/**
 * Very fast integer reading.
 *
 * Quickly reads an integer from a `std::string_view`.
 *
 * Be very careful when using this one!
 * Its speed comes from some very heavy assumptions about the validity of
 * the input:
 *  - `source` must not be empty.
 *  - `source` contains nothing but the integer: no leading or trailing
 *    whitespace, no extra junk. Leading `-` is allowed for signed types,
 *    no `+` is allowed.
 *  - The parsed value does not overflow.
 *  - The input is a valid base-10 integer.
 * Breaking these assumptions will lead to UB.
 */
template <typename T, std::enable_if_t<detail::is_scan_int_type<T>>* = nullptr>
SCN_NODISCARD auto scan_int_exhaustive_valid(std::string_view source) -> T
{
    static_assert(
        detail::dependent_bool<!SCN_IS_BIG_ENDIAN, T>,
        "scan_int_exhaustive_valid requires a little endian environment");
    return detail::scan_int_exhaustive_valid_impl<T>(source);
}

SCN_END_NAMESPACE
}  // namespace scn
