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
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(r), args);
        auto pctx = parse_context_type(f, ctx);
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
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(std::move(r)), args);
        auto pctx = parse_context_type(f, ctx);
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
        using parse_context_type =
            basic_empty_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(r), args);
        auto pctx = parse_context_type(static_cast<int>(sizeof...(Args)), ctx);
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
        using parse_context_type =
            basic_empty_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(std::move(r)), args);
        auto pctx = parse_context_type(static_cast<int>(sizeof...(Args)), ctx);
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
        using parse_context_type =
            basic_scanf_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(r), args);
        auto pctx = parse_context_type(f, ctx);
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
        using parse_context_type =
            basic_scanf_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::make_range_wrapper(std::move(r)), args);
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx);
    }

    // getline

    namespace detail {
        template <typename WrappedRange, typename String, typename CharT>
        auto getline_impl(WrappedRange& r, String& str, CharT until)
            -> scan_result<typename WrappedRange::return_type, error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            auto s = read_until_space_zero_copy(r, until_pred, true);
            if (!s) {
                return {std::move(s.error()), r.get_return()};
            }
            if (s.value().size() != 0) {
                auto size = s.value().size();
                if (until_pred(s.value()[size - 1])) {
                    --size;
                }
                str.clear();
                str.resize(size);
                std::memcpy(&str[0], s.value().data(), size * sizeof(CharT));
                return {{}, r.get_return()};
            }

            String tmp;
            auto out = std::back_inserter(tmp);
            auto e = read_until_space(r, out, until_pred, true);
            if (!e) {
                return {std::move(e), r.get_return()};
            }
            if (until_pred(str.back())) {
                str.pop_back();
            }
            str = std::move(tmp);
            return {{}, r.get_return()};
        }
    }  // namespace detail

    template <typename Range, typename String, typename CharT>
    auto getline(const Range& r, String& str, CharT until) -> scan_result<
        typename detail::range_wrapper_for_t<const Range&>::return_type,
        error>
    {
        auto wrapped = detail::make_range_wrapper(r);
        auto ret = getline_impl(wrapped, str, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
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

    // ignore

    namespace detail {
        template <typename CharT>
        struct ignore_iterator {
            using value_type = CharT;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::output_iterator_tag;

            constexpr ignore_iterator() = default;

            constexpr const ignore_iterator& operator=(CharT) const noexcept
            {
                return *this;
            }

            constexpr const ignore_iterator& operator*() const noexcept
            {
                return *this;
            }
            constexpr const ignore_iterator& operator++() const noexcept
            {
                return *this;
            }
        };

        template <typename CharT>
        struct ignore_iterator_n {
            using value_type = CharT;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::output_iterator_tag;

            ignore_iterator_n() = default;
            ignore_iterator_n(difference_type n) : i(n) {}

            constexpr const ignore_iterator_n& operator=(CharT) const noexcept
            {
                return *this;
            }

            constexpr const ignore_iterator_n& operator*() const noexcept
            {
                return *this;
            }

            SCN_CONSTEXPR14 ignore_iterator_n& operator++() noexcept
            {
                ++i;
                return *this;
            }

            constexpr bool operator==(const ignore_iterator_n& o) const noexcept
            {
                return i == o.i;
            }
            constexpr bool operator!=(const ignore_iterator_n& o) const noexcept
            {
                return !(*this == o);
            }

            difference_type i{0};
        };

        template <typename WrappedRange,
                  typename CharT = typename detail::extract_char_type<
                      detail::range_wrapper_for_t<WrappedRange>>::type>
        auto ignore_until_impl(WrappedRange& r, CharT until)
            -> scan_result<typename WrappedRange::return_type, error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            ignore_iterator<CharT> it{};
            auto e = read_until_space(r, it, until_pred, false);
            if (!e) {
                return {std::move(e), r.get_return()};
            }
            return {{}, r.get_return()};
        }

        template <typename WrappedRange,
                  typename CharT = typename detail::extract_char_type<
                      detail::range_wrapper_for_t<WrappedRange>>::type>
        auto ignore_until_n_impl(WrappedRange& r,
                                 ranges::range_difference_t<WrappedRange> n,
                                 CharT until)
            -> scan_result<typename WrappedRange::return_type, error>
        {
            auto until_pred = [until](CharT ch) { return ch == until; };
            ignore_iterator_n<CharT> begin{}, end{n};
            auto e = read_until_space_ranged(r, begin, end, until_pred, false);
            if (!e) {
                return {std::move(e), r.get_return()};
            }
            return {{}, r.get_return()};
        }
    }  // namespace detail

    template <typename Range, typename CharT>
    auto ignore_until(const Range& r, CharT until) -> scan_result<
        typename detail::range_wrapper_for_t<const Range&>::return_type,
        error>
    {
        auto wrapped = detail::make_range_wrapper(r);
        auto ret = detail::ignore_until_impl(wrapped, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
    }
    template <typename Range,
              typename CharT,
              typename std::enable_if<!std::is_reference<Range>::value>::type* =
                  nullptr>
    auto ignore_until(Range&& r, CharT until)
        -> scan_result<typename detail::range_wrapper_for_t<Range>::return_type,
                       error>
    {
        auto wrapped = detail::make_range_wrapper(std::move(r));
        auto ret = detail::ignore_until_impl(wrapped, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
    }

    template <typename Range, typename CharT>
    auto ignore_until_n(const Range& r,
                        detail::ranges::range_difference_t<Range> n,
                        CharT until)
        -> scan_result<
            typename detail::range_wrapper_for_t<const Range&>::return_type,
            error>
    {
        auto wrapped = detail::make_range_wrapper(r);
        auto ret = detail::ignore_until_n_impl(wrapped, n, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
    }
    template <typename Range,
              typename CharT,
              typename std::enable_if<!std::is_reference<Range>::value>::type* =
                  nullptr>
    auto ignore_until_n(Range&& r,
                        detail::ranges::range_difference_t<Range> n,
                        CharT until)
        -> scan_result<typename detail::range_wrapper_for_t<Range>::return_type,
                       error>
    {
        auto wrapped = detail::make_range_wrapper(std::move(r));
        auto ret = detail::ignore_until_n_impl(wrapped, n, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_SCAN_H
