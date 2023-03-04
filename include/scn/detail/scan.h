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
        namespace _context_type_map {
            struct fn {
                template <typename CharT>
                basic_scan_context<std::basic_string_view<CharT>, CharT>
                operator()(tag_type<CharT>,
                           const std::basic_string_view<CharT>&,
                           priority_tag<1>) const SCN_NOEXCEPT;

                template <typename CharT>
                basic_scan_context<basic_istreambuf_subrange<CharT>, CharT>
                operator()(tag_type<CharT>,
                           const basic_istreambuf_view<CharT>&,
                           priority_tag<1>) const SCN_NOEXCEPT;

                template <typename CharT>
                basic_scan_context<basic_erased_subrange<CharT>, CharT>
                operator()(tag_type<CharT>,
                           const basic_erased_subrange<CharT>&,
                           priority_tag<1>) const SCN_NOEXCEPT;

                template <typename CharT>
                basic_scan_context<basic_istreambuf_subrange<CharT>, CharT>
                operator()(tag_type<CharT>,
                           const basic_istreambuf_subrange<CharT>&,
                           priority_tag<0>) const SCN_NOEXCEPT;

                template <typename CharT>
                basic_scan_context<basic_erased_subrange<CharT>, CharT>
                operator()(tag_type<CharT>,
                           const basic_erased_range<CharT>&,
                           priority_tag<0>) const SCN_NOEXCEPT;
            };
        }  // namespace _context_type_map

        template <typename Range>
        using mapped_context_type = decltype(_context_type_map::fn{}(
            tag_type<ranges::range_value_t<const Range&>>{},
            SCN_DECLVAL(const Range&),
            priority_tag<1>{}));
    }  // namespace detail

    template <typename SourceRange, typename ResultRange, typename... Args>
    using scan_result_type = scan_result_tuple<
        std::conditional_t<ranges::borrowed_range<SourceRange>,
                           decltype(detail::map_scan_result_range<
                                    ranges::range_value_t<ResultRange>>(
                               SCN_DECLVAL(SourceRange),
                               SCN_DECLVAL(const vscan_result<ResultRange>&))),
                           ranges::dangling>,
        Args...>;
    template <typename SourceRange, typename... Args>
    using scan_result_type_for =
        scan_result_type<SourceRange,
                         decltype(scan_map_input_range(
                             SCN_DECLVAL(const SourceRange&))),
                         Args...>;

    template <typename SourceRange,
              typename ResultRange,
              typename Context,
              typename... Args>
    scan_result_type<SourceRange, ResultRange, Args...> make_scan_result(
        SourceRange&& source,
        vscan_result<ResultRange>&& result,
        scan_arg_store<Context, Args...>&& args)
    {
        auto result_range =
            detail::map_scan_result_range<ranges::range_value_t<ResultRange>>(
                source, result);
        if (result.error) {
            return {scan_result{SCN_MOVE(result_range), {}},
                    SCN_MOVE(args.args())};
        }
        return {scan_result{SCN_MOVE(result_range), result.error}, {}};
    }

    namespace detail {
        template <typename... Args, typename Source, typename Format>
        auto scan_impl(Source&& source,
                       Format format,
                       std::tuple<Args...> args_default_values)
        {
            auto range = scan_map_input_range(source);
            auto args =
                make_scan_args<detail::mapped_context_type<decltype(range)>,
                               Args...>(std::move(args_default_values));
            auto result = vscan(range, format, args);
            return make_scan_result(SCN_FWD(source), SCN_MOVE(result),
                                    SCN_MOVE(args));
        }
    }  // namespace detail

    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<ranges::range_value_t<Source>, char>>>
    SCN_NODISCARD auto scan(Source&& source, format_string<Args...> format)
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format, {});
    }
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<ranges::range_value_t<Source>, wchar_t>>>
    SCN_NODISCARD auto scan(Source&& source, wformat_string<Args...> format)
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format, {});
    }

    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<ranges::range_value_t<Source>, char>>>
    SCN_NODISCARD auto scan(Source&& source,
                            format_string<Args...> format,
                            std::tuple<Args...>&& args)
    {
        return detail::scan_impl<Args...>(SCN_FWD(source), format,
                                          SCN_MOVE(args));
    }
    template <typename... Args,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<ranges::range_value_t<Source>, wchar_t>>>
    SCN_NODISCARD auto scan(Source&& source,
                            wformat_string<Args...> format,
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
            auto args =
                make_scan_args<detail::mapped_context_type<decltype(range)>,
                               Args...>(std::move(args_default_values));
            SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE
            auto result = vscan(loc, range, format, args);
            SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
            return make_scan_result(SCN_FWD(source), SCN_MOVE(result),
                                    SCN_MOVE(args));
        }
    }  // namespace detail

    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<ranges::range_value_t<Source>, char>>,
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
                  std::is_same_v<ranges::range_value_t<Source>, wchar_t>>,
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
                  std::is_same_v<ranges::range_value_t<Source>, char>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(Locale& loc,
                            Source&& source,
                            format_string<Args...> format,
                            std::tuple<Args...>&& args)
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, args);
    }
    template <typename... Args,
              typename Locale,
              typename Source,
              typename = std::enable_if_t<
                  std::is_same_v<ranges::range_value_t<Source>, wchar_t>>,
              typename = std::void_t<decltype(Locale::classic())>>
    SCN_NODISCARD auto scan(Locale& loc,
                            Source&& source,
                            wformat_string<Args...> format,
                            std::tuple<Args...>&& args)
    {
        return detail::scan_localized_impl<Args...>(loc, SCN_FWD(source),
                                                    format, args);
    }

    namespace detail {
        template <typename T, typename Source>
        auto scan_value_impl(Source&& source, T value)
        {
            auto range = scan_map_input_range(source);
            auto arg =
                detail::make_arg<detail::mapped_context_type<decltype(range)>>(
                    value);
            auto result = vscan_value(range, arg);
            auto result_range =
                detail::map_scan_result_range(SCN_FWD(source), result);
            return scan_result_type<Source, decltype(result_range), T>{
                scan_result{SCN_MOVE(result_range), result.error},
                std::tuple<T>{SCN_MOVE(value)}};
        }
    }  // namespace detail

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

    struct stdin_range_placeholder {
        constexpr stdin_range_placeholder() SCN_NOEXCEPT = default;
        template <typename... Args>
        constexpr stdin_range_placeholder(Args&&...) SCN_NOEXCEPT
        {
        }
    };

    namespace detail {
        scn::istreambuf_view& internal_narrow_stdin();
        scn::wistreambuf_view& internal_wide_stdin();

        template <typename... Args, typename Source, typename Format>
        scan_result_tuple<stdin_range_placeholder, Args...> input_impl(
            Source& source,
            Format format)
        {
            auto range = scan_map_input_range(source);
            auto args =
                make_scan_args<detail::mapped_context_type<decltype(range)>,
                               Args...>({});
            auto result = vscan_and_sync(range, format, args);
            return make_scan_result(SCN_FWD(source), SCN_MOVE(result),
                                    SCN_MOVE(args));
        }
    }  // namespace detail

    template <typename... Args>
    SCN_NODISCARD auto input(format_string<Args...> f)
    {
        return detail::input_impl<Args...>(detail::internal_narrow_stdin(), f);
    }
    template <typename... Args>
    SCN_NODISCARD auto input(wformat_string<Args...> f)
    {
        return detail::input_impl<Args...>(detail::internal_wide_stdin(), f);
    }

    template <typename... Args>
    SCN_NODISCARD auto prompt(const char* msg, format_string<Args...> f)
    {
        std::printf("%s", msg);
        return detail::input_impl<Args...>(detail::internal_narrow_stdin(), f);
    }
    template <typename... Args>
    SCN_NODISCARD auto prompt(const wchar_t* msg, wformat_string<Args...> f)
    {
        std::wprintf(L"%s", msg);
        return detail::input_impl<Args...>(detail::internal_wide_stdin(), f);
    }

#endif  // SCN_USE_IOSTREAMS

    SCN_END_NAMESPACE
}  // namespace scn
