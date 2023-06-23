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

    // vscan_impl

    namespace detail {
        scan_result<std::wstring_view> vscan_impl(
            std::wstring_view source,
            std::wstring_view format,
            scan_args_for<std::wstring_view, wchar_t> args);
        scan_result<werased_subrange> vscan_impl(
            werased_subrange source,
            std::wstring_view format,
            scan_args_for<werased_subrange, wchar_t> args);
#if SCN_USE_IOSTREAMS
        scan_result<wistreambuf_subrange> vscan_impl(
            wistreambuf_subrange source,
            std::wstring_view format,
            scan_args_for<wistreambuf_subrange, wchar_t> args);
#endif

        template <typename Locale>
        scan_result<std::wstring_view> vscan_impl(
            const Locale& loc,
            std::wstring_view source,
            std::wstring_view format,
            scan_args_for<std::wstring_view, wchar_t> args);
        template <typename Locale>
        scan_result<werased_subrange> vscan_impl(
            const Locale& loc,
            werased_subrange source,
            std::wstring_view format,
            scan_args_for<werased_subrange, wchar_t> args);
#if SCN_USE_IOSTREAMS
        template <typename Locale>
        scan_result<wistreambuf_subrange> vscan_impl(
            const Locale& loc,
            wistreambuf_subrange source,
            std::wstring_view format,
            scan_args_for<wistreambuf_subrange, wchar_t> args);
#endif

        scan_result<std::wstring_view> vscan_value_impl(
            std::wstring_view source,
            scan_arg_for<std::wstring_view, wchar_t> arg);
        scan_result<werased_subrange> vscan_value_impl(
            werased_subrange source,
            scan_arg_for<werased_subrange, wchar_t> arg);
#if SCN_USE_IOSTREAMS
        scan_result<wistreambuf_subrange> vscan_value_impl(
            wistreambuf_subrange source,
            scan_arg_for<wistreambuf_subrange, wchar_t> arg);
#endif

#if SCN_USE_IOSTREAMS
        scan_result<wistreambuf_subrange> vscan_and_sync_impl(
            wistreambuf_subrange source,
            std::wstring_view format,
            scan_args_for<wistreambuf_subrange, wchar_t> args);
#endif
    }  // namespace detail

    // vscan_impl

    template <typename Range>
    auto vscan(Range&& range,
               std::wstring_view format,
               scan_args_for<Range, wchar_t> args)
    {
        auto mapped_range = detail::scan_map_input_range(range);
        auto result = detail::vscan_impl(mapped_range, format, args);
        auto result_range =
            detail::map_scan_result_range(SCN_FWD(range), result.range());
        return scan_result{SCN_MOVE(result_range), result.error()};
    }

    template <typename Range,
              typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    auto vscan(const Locale& loc,
               Range&& range,
               std::wstring_view format,
               scan_args_for<Range, wchar_t> args)
    {
        auto mapped_range = detail::scan_map_input_range(range);
        auto result =
            detail::vscan_localized_impl(loc, mapped_range, format, args);
        auto result_range =
            detail::map_scan_result_range(SCN_FWD(range), result.range());
        return scan_result{SCN_MOVE(result_range), result.error()};
    }

    template <typename Range>
    auto vscan_value(Range&& range, scan_arg_for<Range, wchar_t> arg)
    {
        auto mapped_range = detail::scan_map_input_range(range);
        auto result = detail::vscan_value_impl(mapped_range, arg);
        auto result_range =
            detail::map_scan_result_range(SCN_FWD(range), result.range());
        return scan_result{SCN_MOVE(result_range), result.error()};
    }

    template <typename Range>
    auto vscan_and_sync(Range&& range,
                        std::wstring_view format,
                        scan_args_for<Range, wchar_t> args)
    {
        auto mapped_range = detail::scan_map_input_range(range);
        auto result = detail::vscan_and_sync_impl(mapped_range, format, args);
        auto result_range =
            detail::map_scan_result_range(SCN_FWD(range), result.range());
        return scan_result{SCN_MOVE(result_range), result.error()};
    }

    // scan

    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>>
    SCN_NODISCARD auto scan(Source&& source, wformat_string<Args...> format)
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format, {});
    }

    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>>
    SCN_NODISCARD auto scan(Source&& source,
                            wformat_string<Args...> format,
                            std::tuple<Args...>&& args)
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format,
                                          SCN_MOVE(args));
    }

    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(Locale& loc,
                            Source&& source,
                            wformat_string<Args...> format)
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, {});
    }

    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(Locale& loc,
                            Source&& source,
                            wformat_string<Args...> format,
                            std::tuple<Args...>&& args)
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, args);
    }

#if SCN_USE_IOSTREAMS
    namespace detail {
        scn::wistreambuf_view& internal_wide_stdin();
    }

    template <typename... Args>
    SCN_NODISCARD auto input(wformat_string<Args...> format)
    {
        return detail::input_impl<Args...>(detail::internal_wide_stdin(),
                                           format);
    }

    template <typename... Args>
    SCN_NODISCARD auto prompt(const wchar_t* msg,
                              wformat_string<Args...> format)
    {
        std::wprintf(L"%s", msg);
        return detail::input_impl<Args...>(detail::internal_wide_stdin(),
                                           format);
    }
#endif

    // istream_range

    namespace detail {
        extern template bool
        basic_input_istreambuf_view<wchar_t>::read_next_char() const;
        extern template bool
        basic_input_istreambuf_view<wchar_t>::iterator::is_at_end() const;
    }  // namespace detail

    extern template void basic_istreambuf_view<wchar_t>::sync(iterator);
    extern template void basic_istreambuf_subrange<wchar_t>::sync();

    // istream_scanner streambuf

    namespace detail {
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(std::wstring_view)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(wistreambuf_subrange)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(werased_subrange)
    }  // namespace detail

    // scanner_builtin

    namespace detail {
        namespace scanner_scan_contexts {
            using wsv = basic_scan_context<std::wstring_view, wchar_t>;
            using wes = basic_scan_context<werased_subrange, wchar_t>;
#if SCN_USE_IOSTREAMS
            using wis = basic_scan_context<wistreambuf_subrange, wchar_t>;
#endif
        }  // namespace scanner_scan_contexts

        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wsv)
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wes)
#if SCN_USE_IOSTREAMS
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wis)
#endif

#undef SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX
#undef SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE

    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
