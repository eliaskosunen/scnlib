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

    template <typename ResultIterator, typename Context, typename... Args>
    auto make_scan_result(scan_expected<ResultIterator>&& result,
                          scan_arg_store<Context, Args...>&& args)
        -> scan_expected<scan_result<ResultIterator, Args...>>
    {
        if (SCN_UNLIKELY(!result)) {
            return unexpected(result.error());
        }
        return scan_result{SCN_MOVE(*result), SCN_MOVE(args.args())};
    }

    template <typename Range, typename... Args>
    using scan_result_type =
        scan_expected<scan_result<ranges::borrowed_iterator_t<Range>, Args...>>;

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
     * Scan Args... from source, according to format.
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

    /// Scan using a locale
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

    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, char>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(Locale& loc,
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

    /// Scan a single value of type T from source, with default options
    template <typename T, typename Source>
    SCN_NODISCARD auto scan_value(Source&& source)
        -> scan_result_type<Source, T>
    {
        return detail::scan_value_impl(SCN_FWD(source), T{});
    }

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

    /// Scan Args... from stdin according to format
    /// Prefer this over constructing a view over std::cin, and using scan().
    /// Thread-safe.
    template <typename... Args>
    SCN_NODISCARD auto input(format_string<Args...> format)
        -> scan_result_type<scn::istreambuf_view&, Args...>
    {
        return detail::input_impl<Args...>(detail::internal_narrow_stdin(),
                                           format);
    }

    /// Write msg to stdout, and call input<Args...>(format)
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
