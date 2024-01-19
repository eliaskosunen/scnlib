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

#include <scn/detail/istream_scanner.h>
#include <scn/detail/scan.h>

namespace scn {
SCN_BEGIN_NAMESPACE

/**
 * \defgroup xchar Wide character APIs
 *
 * \brief Scanning interfaces taking wide strings (`wchar_t`).
 *
 * The header `<scn/xchar.h>` needs to be included for these APIs.
 */

// vscan

/**
 * \ingroup xchar
 *
 * \see vscan()
 */
template <typename Range>
auto vscan(Range&& range, std::wstring_view format, wscan_args args)
    -> vscan_result<Range>
{
    return detail::vscan_generic(SCN_FWD(range), format, args);
}

/**
 * \ingroup xchar
 *
 * \see vscan()
 */
template <typename Range,
          typename Locale,
          std::void_t<decltype(Locale::classic())>* = nullptr>
auto vscan(const Locale& loc,
           Range&& range,
           std::wstring_view format,
           wscan_args args) -> vscan_result<Range>
{
    return detail::vscan_localized_generic(loc, SCN_FWD(range), format, args);
}

/**
 * \ingroup xchar
 *
 * \see vscan_value()
 */
template <typename Range>
auto vscan_value(Range&& range, basic_scan_arg<wscan_context> arg)
    -> vscan_result<Range>
{
    return detail::vscan_value_generic(SCN_FWD(range), arg);
}

// scan

/**
 * \ingroup xchar
 *
 * \see scan()
 */
template <typename... Args,
          typename Source,
          std::enable_if_t<detail::is_wide_range<Source>>* = nullptr>
SCN_NODISCARD auto scan(Source&& source,
                        wscan_format_string<Source, Args...> format)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_impl<wchar_t, Args...>(SCN_FWD(source), format, {});
}

/**
 * \ingroup xchar
 *
 * \see scan()
 */
template <typename... Args,
          typename Source,
          std::enable_if_t<detail::is_wide_range<Source>>* = nullptr>
SCN_NODISCARD auto scan(Source&& source,
                        wscan_format_string<Source, Args...> format,
                        std::tuple<Args...>&& args)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_impl<wchar_t, Args...>(SCN_FWD(source), format,
                                               SCN_MOVE(args));
}

/**
 * \ingroup xchar
 *
 * \see scan()
 */
template <typename... Args,
          typename Locale,
          typename Source,
          std::enable_if_t<detail::is_wide_range<Source>>* = nullptr,
          std::void_t<decltype(Locale::classic())>* = nullptr>
SCN_NODISCARD auto scan(const Locale& loc,
                        Source&& source,
                        wscan_format_string<Source, Args...> format)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_localized_impl<wchar_t, Args...>(loc, SCN_FWD(source),
                                                         format, {});
}

/**
 * \ingroup xchar
 *
 * \see scan()
 */
template <typename... Args,
          typename Locale,
          typename Source,
          std::enable_if_t<detail::is_wide_range<Source>>* = nullptr,
          std::void_t<decltype(Locale::classic())>* = nullptr>
SCN_NODISCARD auto scan(const Locale& loc,
                        Source&& source,
                        wscan_format_string<Source, Args...> format,
                        std::tuple<Args...>&& args)
    -> scan_result_type<Source, Args...>
{
    return detail::scan_localized_impl<wchar_t, Args...>(loc, SCN_FWD(source),
                                                         format, args);
}

/**
 * \ingroup xchar
 *
 * \see scan_value()
 */
template <typename T,
          typename Source,
          std::enable_if_t<detail::is_wide_range<Source>>* = nullptr>
SCN_NODISCARD auto scan_value(Source&& source) -> scan_result_type<Source, T>
{
    return detail::scan_value_impl<wchar_t>(SCN_FWD(source), T{});
}

/**
 * \ingroup xchar
 *
 * \see scan_value()
 */
template <typename T,
          typename Source,
          std::enable_if_t<detail::is_wide_range<Source>>* = nullptr>
SCN_NODISCARD auto scan_value(Source&& source, T default_value)
    -> scan_result_type<Source, T>
{
    return detail::scan_value_impl<wchar_t>(SCN_FWD(source),
                                            SCN_MOVE(default_value));
}

namespace detail {
SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(wscan_context)
#undef SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX
}  // namespace detail

SCN_END_NAMESPACE
}  // namespace scn
