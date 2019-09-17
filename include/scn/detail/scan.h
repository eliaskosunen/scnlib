// Copyright 2017-2019 Elias Kosunen
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

#ifndef SCN_DETAIL_SCAN_H
#define SCN_DETAIL_SCAN_H

#include "vscan.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Range>
        struct scan_result_for_range {
            using type =
                scan_result<typename range_wrapper_for_t<Range>::return_type>;
        };
        template <typename Range>
        using scan_result_for_range_t =
            typename scan_result_for_range<Range>::type;
    }  // namespace detail

    template <typename Range, typename Format, typename... Args>
    auto scan(const Range& r, Format f, Args&... a)
        -> detail::scan_result_for_range_t<const Range&>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type =
            basic_context<detail::range_wrapper_for_t<const Range&>>;
        auto args = make_args<context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(r), args);
        auto pctx =
            basic_parse_context<typename context_type::locale_type>(f, ctx);
        return vscan(ctx, pctx);
    }
    template <typename Range, typename Format, typename... Args>
    auto scan(Range&& r, Format f, Args&... a) ->
        typename std::enable_if<!std::is_reference<Range>::value,
                                detail::scan_result_for_range_t<Range>>::type
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<detail::range_wrapper_for_t<Range>>;
        auto args = make_args<context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(std::move(r)), args);
        auto pctx =
            basic_parse_context<typename context_type::locale_type>(f, ctx);
        return vscan(ctx, pctx);
    }

    // default format

    template <typename Range, typename... Args>
    auto scan(const Range& r, detail::default_t, Args&... a)
        -> detail::scan_result_for_range_t<const Range&>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type =
            basic_context<detail::range_wrapper_for_t<const Range&>>;
        auto args = make_args<context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(r), args);
        auto pctx =
            basic_empty_parse_context<typename context_type::locale_type>(
                static_cast<int>(sizeof...(Args)), ctx);
        return vscan(ctx, pctx);
    }
    template <typename Range, typename... Args>
    auto scan(Range&& r, detail::default_t, Args&... a) ->
        typename std::enable_if<!std::is_reference<Range>::value,
                                detail::scan_result_for_range_t<Range>>::type
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<detail::range_wrapper_for_t<Range>>;
        auto args = make_args<context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(std::move(r)), args);
        auto pctx =
            basic_empty_parse_context<typename context_type::locale_type>(
                static_cast<int>(sizeof...(Args)), ctx);
        return vscan(ctx, pctx);
    }

    // scanf

    template <typename Range, typename Format, typename... Args>
    auto scanf(const Range& r, Format f, Args&... a)
        -> detail::scan_result_for_range_t<const Range&>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type =
            basic_context<detail::range_wrapper_for_t<const Range&>>;
        auto args = make_args<context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(r), args);
        auto pctx =
            basic_scanf_parse_context<typename context_type::locale_type>(f,
                                                                          ctx);
        return vscan(ctx, pctx);
    }
    template <typename Range, typename Format, typename... Args>
    auto scanf(Range&& r, Format f, Args&... a) ->
        typename std::enable_if<!std::is_reference<Range>::value,
                                detail::scan_result_for_range_t<Range>>::type
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<detail::range_wrapper_for_t<Range>>;
        auto args = make_args<context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(std::move(r)), args);
        auto pctx =
            basic_scanf_parse_context<typename context_type::locale_type>(f,
                                                                          ctx);
        return vscan(ctx, pctx);
    }

    // getline

    namespace detail {
        template <typename WrappedRange, typename String, typename CharT>
        auto getline_impl(WrappedRange& r, String& str, CharT until)
            -> scan_result<typename WrappedRange::return_type, error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            auto s = read_until_space_zero_copy(r, until_pred, false);
            if (!s) {
                return {std::move(s.error()), r.get_return()};
            }
            if (s.value().size() != 0) {
                str.assign(s.value().data(), s.value().size());
                return {{}, r.get_return()};
            }

            String tmp;
            auto out = std::back_inserter(tmp);
            auto e = read_until_space(r, out, until_pred, false);
            if (!e) {
                return {std::move(e), r.get_return()};
            }
            str.assign(tmp);
            return {{}, r.get_return()};
        }
    }  // namespace detail

    template <typename Range, typename String, typename CharT>
    auto getline(const Range& r, String& str, CharT until) -> scan_result<
        typename detail::range_wrapper_for_t<const Range&>::return_type,
        error>
    {
        auto wrapped = detail::make_range_wrapper(r);
        return getline_impl(wrapped, str, until);
    }
    template <typename Range,
              typename String,
              typename CharT,
              typename std::enable_if<!std::is_reference<Range>::value>::type* =
                  nullptr>
    auto getline(Range&& r, String& str, CharT until)
        -> scan_result<typename detail::range_wrapper_for_t<Range>::return_type,
                       error>
    {
        auto wrapped = detail::make_range_wrapper(std::move(r));
        return getline_impl(wrapped, str, until);
    }

    template <typename Range,
              typename String,
              typename CharT = typename detail::extract_char_type<
                  detail::range_wrapper_for_t<Range>>::type>
    auto getline(Range&& r, String& str)
        -> decltype(getline(std::forward<Range>(r),
                            str,
                            detail::default_widen<CharT>::widen('\n')))
    {
        return getline(std::forward<Range>(r), str,
                       detail::default_widen<CharT>::widen('\n'));
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_SCAN_H
