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

    namespace detail {
        template <typename SourceRange, typename ResultRange, typename... Args>
        using scan_result_type = scan_result_tuple<
            std::conditional_t<ranges::borrowed_range<SourceRange>,
                               decltype(detail::map_scan_result_range(
                                   SCN_DECLVAL(const SourceRange&),
                                   SCN_DECLVAL(const ResultRange&))),
                               ranges::dangling>,
            Args...>;
    }

    /**
     * Make value to be returned by scan().
     *
     * Returns a scan_result_tuple<R, Args...>, where Args... are the scanned
     * arguments, and R is the appropriate result range, as determined by
     * SourceRange and ResultRange.
     *
     * If the range given to scan() is a borrowed_range, R is a view into that
     * range. Otherwise, R is ranges::dangling.
     *
     * Formally,
     *   - if SourceRange is not a borrowed_range, R is ranges::dangling
     *   - if ResultRange is a specialization of std::basic_string_view,
     *     R is ResultRange
     *   - if ResultRange is a specialization of basic_istreambuf_subrange,
     *     R is ResultRange
     *   - if ResultRange is a specialization of basic_erased_subrange, and:
     *     - SourceRange is a specialization of basic_erased_view, or
     *       basic_erased_subrange, R is ResultRange
     *     - otherwise, R is ranges::subrange<ranges::iterator_t<SourceRange>,
     *                                        ranges::sentinel_t<SourceRange>>
     *
     *
     * Example:
     *
     * template <typename... Args, typename Source>
     * auto my_scan(Source&& source, format_string<Args...> format) {
     *   auto range = scan_map_input_range(source);
     *   auto args = make_scan_args<decltype(range), Args...>();
     *   auto result = vscan(range, format, args);
     *   return make_scan_result(
     *            std::forward<Source>(source), result, std::move(args));
     * }
     *
     * @param source The source range given to scan(), forwarded.
     * @param result The result value returned by vscan()
     * @param args   The argument store returned by make_scan_args() and given
     *               to vscan()
     */
    template <typename SourceRange,
              typename ResultRange,
              typename Context,
              typename... Args>
    detail::scan_result_type<SourceRange, ResultRange, Args...>
    make_scan_result(SourceRange&& source,
                     scan_result<ResultRange> result,
                     scan_arg_store<Context, Args...>&& args)
    {
        auto result_range =
            detail::map_scan_result_range(source, result.range());
        if (SCN_LIKELY(result.good())) {
            return {scan_result{SCN_MOVE(result_range), {}},
                    SCN_MOVE(args.args())};
        }
        return {scan_result{SCN_MOVE(result_range), result.error()}, {}};
    }

    namespace detail {
        // Boilerplate for scan()
        template <typename... Args, typename Source, typename Format>
        auto scan_impl(Source&& source,
                       Format format,
                       std::tuple<Args...> args_default_values)
        {
            auto range = scan_map_input_range(source);
            auto args = make_scan_args<decltype(range), Args...>(
                std::move(args_default_values));
            auto result = vscan(range, format, args);
            return make_scan_result(SCN_FWD(source), SCN_MOVE(result),
                                    SCN_MOVE(args));
        }
    }  // namespace detail

    /**
     * Scan Args... from source, according to format.
     */
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>>
    SCN_NODISCARD auto scan(Source&& source, format_string<Args...> format)
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format, {});
    }

    /**
     * scan with explicitly supplied default values
     *
     * Can be used, for example, for preallocating a scanned string:
     *
     * std::string str;
     * str.reserve(64);
     *
     * auto [result, s] = scn::scan<std::string>(source, "{}",
     *                                           {std::move(str)});
     */
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>>
    SCN_NODISCARD auto scan(Source&& source,
                            format_string<Args...> format,
                            std::tuple<Args...>&& args)
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format,
                                          SCN_MOVE(args));
    }

    namespace detail {
        template <typename... Args,
                  typename Locale,
                  typename Source,
                  typename Format>
        auto scan_localized_impl(Locale& loc,
                                 Source&& source,
                                 Format format,
                                 std::tuple<Args...> args_default_values)
        {
            auto range = scan_map_input_range(source);
            auto args = make_scan_args<decltype(range), Args...>(
                std::move(args_default_values));
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            auto result = vscan(loc, range, format, args);
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
            return make_scan_result(SCN_FWD(source), SCN_MOVE(result),
                                    SCN_MOVE(args));
        }
    }  // namespace detail

    /// Scan using a locale
    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(Locale& loc,
                            Source&& source,
                            format_string<Args...> format)
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, {});
    }

    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(Locale& loc,
                            Source&& source,
                            format_string<Args...> format,
                            std::tuple<Args...>&& args)
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, args);
    }

    namespace detail {
        template <typename Range, typename CharT>
        using context_type_for_impl =
            basic_scan_context<decayed_input_range<Range, CharT>, CharT>;

        template <typename Range>
        using context_type_for =
            detail::context_type_for_impl<Range, detail::char_t<Range>>;

        template <typename T, typename Source>
        auto scan_value_impl(Source&& source, T value)
        {
            auto range = scan_map_input_range(source);
            auto arg =
                detail::make_arg<context_type_for<decltype(range)>>(value);
            auto result = vscan_value(range, arg);
            auto result_range =
                detail::map_scan_result_range(SCN_FWD(source), result.range());
            return scan_result_type<Source, decltype(result_range), T>{
                scan_result{SCN_MOVE(result_range), result.error()},
                std::tuple<T>{SCN_MOVE(value)}};
        }
    }  // namespace detail

    /// Scan a single value of type T from source, with default options
    template <typename T, typename Source>
    SCN_NODISCARD auto scan_value(Source&& source)
    {
        return detail::scan_value_impl(SCN_FWD(source), T{});
    }

    template <typename T, typename Source>
    SCN_NODISCARD auto scan_value(Source&& source, T default_value)
    {
        return detail::scan_value_impl(SCN_FWD(source),
                                       std::move(default_value));
    }

#if SCN_USE_IOSTREAMS

    namespace detail {
        scn::istreambuf_view& internal_narrow_stdin();

        template <typename... Args, typename Source, typename Format>
        scan_result_tuple<stdin_range_marker, Args...> input_impl(
            Source& source,
            Format format)
        {
            auto range = scan_map_input_range(source);
            auto args = make_scan_args<decltype(range), Args...>({});
            auto result = vscan_and_sync(range, format, args);
            return make_scan_result(SCN_FWD(source), SCN_MOVE(result),
                                    SCN_MOVE(args));
        }
    }  // namespace detail

    /// Scan Args... from stdin according to format
    /// Prefer this over constructing a view over std::cin, and using scan().
    /// Thread-safe.
    template <typename... Args>
    SCN_NODISCARD auto input(format_string<Args...> format)
    {
        return detail::input_impl<Args...>(detail::internal_narrow_stdin(),
                                           format);
    }

    /// Write msg to stdout, and call input<Args...>(format)
    template <typename... Args>
    SCN_NODISCARD auto prompt(const char* msg, format_string<Args...> format)
    {
        std::printf("%s", msg);
        return detail::input_impl<Args...>(detail::internal_narrow_stdin(),
                                           format);
    }

#endif  // SCN_USE_IOSTREAMS

    SCN_END_NAMESPACE
}  // namespace scn
