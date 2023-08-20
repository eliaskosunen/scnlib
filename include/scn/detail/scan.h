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
     * auto args = scn::make_scan_args<Range, Args...>();
     * auto result = scn::vscan(std::forward<Range>(range), format, args);
     *
     * return scn::make_scan_result(std::move(result), std::move(args));
     * \endcode
     */
    template <typename ResultRange, typename Context, typename... Args>
    auto make_scan_result(scan_expected<ResultRange>&& result,
                          scan_arg_store<Context, Args...>&& args)
        -> scan_expected<scan_result<ResultRange, Args...>>
    {
        if (SCN_UNLIKELY(!result)) {
            return unexpected(result.error());
        }
        return scan_result{SCN_MOVE(*result), SCN_MOVE(args.args())};
    }

    /**
     * The return type of `scan`, based on the type of the input range, and the
     * types of the scanned arguments.
     */
    template <typename Range, typename... Args>
    using scan_result_type =
        scan_expected<scan_result<borrowed_ssubrange_t<Range>, Args...>>;

    namespace detail {
        // Boilerplate for scan()
        template <typename... Args, typename Source, typename Format>
        auto scan_impl(Source&& source,
                       Format format,
                       std::tuple<Args...> default_values)
            -> scan_result_type<Source, Args...>
        {
            auto args =
                make_scan_args<Source, Args...>(SCN_MOVE(default_values));
            auto result = vscan(SCN_FWD(source), format, args);
            return make_scan_result(SCN_MOVE(result), SCN_MOVE(args));
        }
    }  // namespace detail

    /**
     * Scans `Args...` from the range given to it (`source`), according to the
     * specifications given in the format string (`format`).
     * Returns the resulting values in an object of type `scan_result`,
     * alongside a `subrange` pointing to the unused input.
     *
     * Example:
     * \code{.cpp}
     * if (auto result = scn::scan<int>("123", "{}"))
     *     int value = result->value();
     * \endcode
     */
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>>
    SCN_NODISCARD auto scan(Source&& source, format_string<Args...> format)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format, {});
    }

    /**
     * `scan` with explicitly supplied default values
     *
     * Can be used, for example, for preallocating a scanned string:
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
     */
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>>
    SCN_NODISCARD auto scan(Source&& source,
                            format_string<Args...> format,
                            std::tuple<Args...>&& default_args)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format,
                                          SCN_MOVE(default_args));
    }

    namespace detail {
        // Boilerplate for scan()
        template <typename... Args,
                  typename Locale,
                  typename Source,
                  typename Format>
        auto scan_localized_impl(const Locale& loc,
                                 Source&& source,
                                 Format format,
                                 std::tuple<Args...> default_values)
            -> scan_result_type<Source, Args...>
        {
            auto args =
                make_scan_args<Source, Args...>(SCN_MOVE(default_values));
            auto result = vscan(loc, SCN_FWD(source), format, args);
            return make_scan_result(SCN_MOVE(result), SCN_MOVE(args));
        }
    }  // namespace detail

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
     */
    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(const Locale& loc,
                            Source&& source,
                            format_string<Args...> format)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, {});
    }

    /// `scan` with a locale and default values
    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(const Locale& loc,
                            Source&& source,
                            format_string<Args...> format,
                            std::tuple<Args...>&& default_args)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_localized_impl<Args...>(
            loc, SCN_FWD(source), format, SCN_MOVE(default_args));
    }

    namespace detail {
        template <typename Range, typename CharT>
        using context_type_for_impl =
            basic_scan_context<decayed_mapped_source_range<Range>, CharT>;

        template <typename Range>
        using context_type_for =
            context_type_for_impl<mapped_source_range<Range>,
                                  detail::char_t<Range>>;

        template <typename T, typename Source>
        auto scan_value_impl(Source&& source, T value)
            -> scan_result_type<Source, T>
        {
            auto arg =
                detail::make_arg<detail::context_type_for<Source>>(value);
            return vscan_value(SCN_FWD(source), arg).transform([&](auto&& it) {
                return scan_result{SCN_MOVE(it), std::tuple{SCN_MOVE(value)}};
            });
        }
    }  // namespace detail

    /**
     * `scan` a single value, with default options.
     *
     * Essentially equivalent to: `scn::scan<T>(source, "{}")`,
     * except it can skip parsing the format string, gaining performance.
     */
    template <typename T, typename Source>
    SCN_NODISCARD auto scan_value(Source&& source)
        -> scan_result_type<Source, T>
    {
        return detail::scan_value_impl(SCN_FWD(source), T{});
    }

    /// `scan` a single value, with default options, and a default value.
    template <typename T, typename Source>
    SCN_NODISCARD auto scan_value(Source&& source, T default_value)
        -> scan_result_type<Source, T>
    {
        return detail::scan_value_impl(SCN_FWD(source),
                                       SCN_MOVE(default_value));
    }

#if SCN_USE_IOSTREAMS

    namespace detail {
        scn::istreambuf_view& internal_narrow_stdin();

        template <typename... Args, typename Source, typename Format>
        auto input_impl(Source& source, Format format)
            -> scan_result_type<Source&, Args...>
        {
            auto args = make_scan_args<decltype(source), Args...>();
            auto result = detail::vscan_and_sync(SCN_FWD(source), format, args);
            return make_scan_result(SCN_MOVE(result), SCN_MOVE(args));
        }
    }  // namespace detail

    /**
     * Scan from stdin.
     *
     * Prefer this over constructing a view over `std::cin`,
     * and using `scan` with it: `input` deals with synchronization with `stdin`
     * and `std::cin`, and with multiple threads.
     *
     * \code{.cpp}
     * auto result = scn::input<int>("{}");
     * \endcode
     */
    template <typename... Args>
    SCN_NODISCARD auto input(format_string<Args...> format)
        -> scan_result_type<scn::istreambuf_view&, Args...>
    {
        return detail::input_impl<Args...>(detail::internal_narrow_stdin(),
                                           format);
    }

    /// Write msg to stdout, and call `input<Args...>(format)`
    template <typename... Args>
    SCN_NODISCARD auto prompt(const char* msg, format_string<Args...> format)
        -> scan_result_type<scn::istreambuf_view&, Args...>
    {
        std::printf("%s", msg);
        return detail::input_impl<Args...>(detail::internal_narrow_stdin(),
                                           format);
    }

#endif  // SCN_USE_IOSTREAMS

    SCN_END_NAMESPACE
}  // namespace scn
