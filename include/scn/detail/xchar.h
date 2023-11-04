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

    /// \ingroup xchar
    template <typename Range>
    auto vscan(Range&& range,
               std::wstring_view format,
               scan_args_for<Range, wchar_t> args) -> vscan_result<Range>
    {
        return detail::vscan_generic(SCN_FWD(range), format, args);
    }

    /// \ingroup xchar
    template <typename Range,
              typename Locale,
              typename = std::void_t<decltype(Locale::classic())>>
    auto vscan(const Locale& loc,
               Range&& range,
               std::wstring_view format,
               scan_args_for<Range, wchar_t> args) -> vscan_result<Range>
    {
        return detail::vscan_localized_generic(loc, SCN_FWD(range), format,
                                               args);
    }

    /// \ingroup xchar
    template <typename Range>
    auto vscan_value(Range&& range, scan_arg_for<Range, wchar_t> arg)
        -> vscan_result<Range>
    {
        return detail::vscan_value_generic(SCN_FWD(range), arg);
    }

#if !SCN_DISABLE_IOSTREAM
    namespace detail {
        template <typename Range>
        auto vscan_and_sync(Range&& range,
                            std::wstring_view format,
                            scan_args_for<Range, wchar_t> args)
            -> vscan_result<Range>
        {
            return detail::vscan_and_sync_generic(SCN_FWD(range), format, args);
        }
    }  // namespace detail
#endif

    // scan

    /// \ingroup xchar
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>>
    SCN_NODISCARD auto scan(Source&& source, wformat_string<Args...> format)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format, {});
    }

    /// \ingroup xchar
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>>
    SCN_NODISCARD auto scan(Source&& source,
                            wformat_string<Args...> format,
                            std::tuple<Args...>&& args)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format,
                                          SCN_MOVE(args));
    }

    /// \ingroup xchar
    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(const Locale& loc,
                            Source&& source,
                            wformat_string<Args...> format)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, {});
    }

    /// \ingroup xchar
    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<detail::char_t<Source>, wchar_t>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(const Locale& loc,
                            Source&& source,
                            wformat_string<Args...> format,
                            std::tuple<Args...>&& args)
        -> scan_result_type<Source, Args...>
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, args);
    }

#if !SCN_DISABLE_IOSTREAM
    namespace detail {
        scn::wistreambuf_view& internal_wide_stdin();
    }

    /// \ingroup xchar
    template <typename... Args>
    SCN_NODISCARD auto input(wformat_string<Args...> format)
        -> scan_result_type<wistreambuf_view&, Args...>
    {
        return detail::input_impl<Args...>(detail::internal_wide_stdin(),
                                           format);
    }

    /// \ingroup xchar
    template <typename... Args>
    SCN_NODISCARD auto prompt(const wchar_t* msg,
                              wformat_string<Args...> format)
        -> scan_result_type<wistreambuf_view&, Args...>
    {
        std::wprintf(L"%s", msg);
        return detail::input_impl<Args...>(detail::internal_wide_stdin(),
                                           format);
    }

    // istream_range

    namespace detail {
        extern template bool
        basic_input_istreambuf_view<wchar_t>::read_next_char() const;
        extern template bool
        basic_input_istreambuf_view<wchar_t>::iterator::is_at_end() const;
    }  // namespace detail

    extern template void basic_istreambuf_view<wchar_t>::sync(iterator);
    extern template void basic_istreambuf_subrange<wchar_t>::sync();
#endif

    // istream_scanner streambuf

#if !SCN_DISABLE_IOSTREAM
    namespace detail {
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(std::wstring_view)
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(wistreambuf_subrange)
#if !SCN_DISABLE_ERASED_RANGE
        SCN_DECLARE_EXTERN_RANGE_STREAMBUF(werased_subrange)
#endif
    }   // namespace detail
#endif  // !SCN_DISABLE_IOSTREAM

    // scanner_builtin

    namespace detail {
        namespace scanner_scan_contexts {
            using wsv = basic_scan_context<std::wstring_view, wchar_t>;
#if !SCN_DISABLE_ERASED_RANGE
            using wes = basic_scan_context<werased_subrange, wchar_t>;
#endif
#if !SCN_DISABLE_IOSTREAM
            using wis = basic_scan_context<wistreambuf_subrange, wchar_t>;
#endif
        }  // namespace scanner_scan_contexts

        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wsv)
#if !SCN_DISABLE_ERASED_RANGE
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wes)
#endif
#if !SCN_DISABLE_IOSTREAM
        SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX(scanner_scan_contexts::wis)
#endif

#undef SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_CTX
#undef SCN_DECLARE_EXTERN_SCANNER_SCAN_FOR_TYPE

    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
