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

#include <scn/detail/context.h>
#include <scn/detail/istream_range.h>
#include <scn/detail/result.h>
#include <scn/util/expected.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename Range, typename CharT>
    using scan_arg_for = basic_scan_arg<
        basic_scan_context<detail::decayed_mapped_source_range<Range>, CharT>>;
    template <typename Range, typename CharT>
    using scan_args_for = basic_scan_args<
        basic_scan_context<detail::decayed_mapped_source_range<Range>, CharT>>;

    namespace detail {
        template <typename R>
        using vscan_impl_result = scan_expected<typename R::iterator>;

        vscan_impl_result<std::string_view> vscan_impl(
            std::string_view source,
            std::string_view format,
            scan_args_for<std::string_view, char> args);
        vscan_impl_result<erased_subrange> vscan_impl(
            erased_subrange source,
            std::string_view format,
            scan_args_for<erased_subrange, char> args);
#if SCN_USE_IOSTREAMS
        vscan_impl_result<istreambuf_subrange> vscan_impl(
            istreambuf_subrange source,
            std::string_view format,
            scan_args_for<istreambuf_subrange, char> args);
#endif

        vscan_impl_result<std::wstring_view> vscan_impl(
            std::wstring_view source,
            std::wstring_view format,
            scan_args_for<std::wstring_view, wchar_t> args);
        vscan_impl_result<werased_subrange> vscan_impl(
            werased_subrange source,
            std::wstring_view format,
            scan_args_for<werased_subrange, wchar_t> args);
#if SCN_USE_IOSTREAMS
        vscan_impl_result<wistreambuf_subrange> vscan_impl(
            wistreambuf_subrange source,
            std::wstring_view format,
            scan_args_for<wistreambuf_subrange, wchar_t> args);
#endif

        template <typename Locale>
        vscan_impl_result<std::string_view> vscan_localized_impl(
            const Locale& loc,
            std::string_view source,
            std::string_view format,
            scan_args_for<std::string_view, char> args);
        template <typename Locale>
        vscan_impl_result<erased_subrange> vscan_localized_impl(
            const Locale& loc,
            erased_subrange source,
            std::string_view format,
            scan_args_for<erased_subrange, char> args);
#if SCN_USE_IOSTREAMS
        template <typename Locale>
        vscan_impl_result<istreambuf_subrange> vscan_localized_impl(
            const Locale& loc,
            istreambuf_subrange source,
            std::string_view format,
            scan_args_for<istreambuf_subrange, char> args);
#endif

        template <typename Locale>
        vscan_impl_result<std::wstring_view> vscan_localized_impl(
            const Locale& loc,
            std::wstring_view source,
            std::wstring_view format,
            scan_args_for<std::wstring_view, wchar_t> args);
        template <typename Locale>
        vscan_impl_result<werased_subrange> vscan_localized_impl(
            const Locale& loc,
            werased_subrange source,
            std::wstring_view format,
            scan_args_for<werased_subrange, wchar_t> args);
#if SCN_USE_IOSTREAMS
        template <typename Locale>
        vscan_impl_result<wistreambuf_subrange> vscan_localized_impl(
            const Locale& loc,
            wistreambuf_subrange source,
            std::wstring_view format,
            scan_args_for<wistreambuf_subrange, wchar_t> args);
#endif

        vscan_impl_result<std::string_view> vscan_value_impl(
            std::string_view source,
            scan_arg_for<std::string_view, char> arg);
        vscan_impl_result<erased_subrange> vscan_value_impl(
            erased_subrange source,
            scan_arg_for<erased_subrange, char> arg);
#if SCN_USE_IOSTREAMS
        vscan_impl_result<istreambuf_subrange> vscan_value_impl(
            istreambuf_subrange source,
            scan_arg_for<istreambuf_subrange, char> arg);
#endif

        vscan_impl_result<std::wstring_view> vscan_value_impl(
            std::wstring_view source,
            scan_arg_for<std::wstring_view, wchar_t> arg);
        vscan_impl_result<werased_subrange> vscan_value_impl(
            werased_subrange source,
            scan_arg_for<werased_subrange, wchar_t> arg);
#if SCN_USE_IOSTREAMS
        vscan_impl_result<wistreambuf_subrange> vscan_value_impl(
            wistreambuf_subrange source,
            scan_arg_for<wistreambuf_subrange, wchar_t> arg);
#endif

#if SCN_USE_IOSTREAMS
        vscan_impl_result<istreambuf_subrange> vscan_and_sync_impl(
            istreambuf_subrange source,
            std::string_view format,
            scan_args_for<istreambuf_subrange, char> args);
#endif

#if SCN_USE_IOSTREAMS
        vscan_impl_result<wistreambuf_subrange> vscan_and_sync_impl(
            wistreambuf_subrange source,
            std::wstring_view format,
            scan_args_for<wistreambuf_subrange, wchar_t> args);
#endif
    }  // namespace detail

    template <typename Range>
    using vscan_result = scan_expected<borrowed_ssubrange_t<Range>>;

    namespace detail {
        template <typename Range, typename Format, typename Args>
        auto vscan_generic(Range&& range, Format format, Args args)
            -> vscan_result<Range>
        {
            auto mapped_range = scan_map_input_range(range);

            auto result = vscan_impl(mapped_range, format, args);
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
        }

        template <typename Locale,
                  typename Range,
                  typename Format,
                  typename Args>
        auto vscan_localized_generic(const Locale& loc,
                                     Range&& range,
                                     Format format,
                                     Args args) -> vscan_result<Range>
        {
            auto mapped_range = scan_map_input_range(range);

            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            auto result = vscan_localized_impl(loc, mapped_range, format, args);
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
        }

        template <typename Range, typename Arg>
        auto vscan_value_generic(Range&& range, Arg arg) -> vscan_result<Range>
        {
            auto mapped_range = scan_map_input_range(range);

            auto result = vscan_value_impl(mapped_range, arg);
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
        }

#if SCN_USE_IOSTREAMS
        template <typename Range, typename Format, typename Args>
        auto vscan_and_sync_generic(Range&& range, Format format, Args args)
            -> vscan_result<Range>
        {
            auto mapped_range = scan_map_input_range(range);

            auto result = vscan_and_sync_impl(mapped_range, format, args);
            if (SCN_UNLIKELY(!result)) {
                return unexpected(result.error());
            }
            return map_scan_result_range(SCN_FWD(range), mapped_range.begin(),
                                         *result);
        }
#endif
    }  // namespace detail

    SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wnoexcept")

    template <typename Range>
    auto vscan(Range&& range,
               std::string_view format,
               scan_args_for<Range, char> args) -> vscan_result<Range>
    {
        return detail::vscan_generic(SCN_FWD(range), format, args);
    }

    template <typename Range,
              typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    auto vscan(const Locale& loc,
               Range&& range,
               std::string_view format,
               scan_args_for<Range, char> args) -> vscan_result<Range>
    {
        return detail::vscan_localized_generic(loc, SCN_FWD(range), format,
                                               args);
    }

    template <typename Range>
    auto vscan_value(Range&& range, scan_arg_for<Range, char> arg)
        -> vscan_result<Range>
    {
        return detail::vscan_value_generic(SCN_FWD(range), arg);
    }

#if SCN_USE_IOSTREAMS
    namespace detail {
        template <typename Range>
        auto vscan_and_sync(Range&& range,
                            std::string_view format,
                            scan_args_for<Range, char> args)
            -> vscan_result<Range>
        {
            return detail::vscan_and_sync_generic(SCN_FWD(range), format, args);
        }
    }  // namespace detail
#endif

    SCN_GCC_POP  // -Wnoexcept

        SCN_END_NAMESPACE
}  // namespace scn
