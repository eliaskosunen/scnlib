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

#include <vector>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename Range, typename E = result<ptrdiff_t>>
        struct scan_result_for_range {
            using type = scan_result<
                typename detail::range_wrapper_for_t<Range>::return_type,
                E>;
        };
        template <typename Range, typename E = result<ptrdiff_t>>
        using scan_result_for_range_t =
            typename scan_result_for_range<Range, E>::type;
    }  // namespace detail

    // scan

    template <typename Range, typename Format, typename... Args>
    auto scan(Range&& r, const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using range_type = detail::range_wrapper_for_t<Range>;
        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)), args);
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx);
    }

    // scan localized

    template <typename Locale,
              typename Range,
              typename Format,
              typename... Args>
    auto scan_localized(const Locale& loc,
                        Range&& r,
                        const Format& f,
                        Args&... a) -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using range_type = detail::range_wrapper_for_t<Range>;
        using locale_type = basic_locale_ref<typename range_type::char_type>;
        using context_type = basic_context<range_type, locale_type>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)), args,
                                {std::addressof(loc)});
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx);
    }

    // default format

    template <typename Range, typename... Args>
    auto scan(Range&& r, detail::default_t, Args&... a)
        -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using range_type = detail::range_wrapper_for_t<Range>;
        using context_type = basic_context<range_type>;
        using parse_context_type =
            basic_empty_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)), args);
        auto pctx = parse_context_type(static_cast<int>(sizeof...(Args)), ctx);
        return vscan(ctx, pctx);
    }

    // scanf

    template <typename Range, typename Format, typename... Args>
    auto scanf(Range&& r, const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<Range>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<detail::range_wrapper_for_t<Range>>;
        using parse_context_type =
            basic_scanf_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(std::forward<Range>(r)), args);
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx);
    }

    // input

    template <typename Format,
              typename... Args,
              typename CharT = detail::ranges::range_value_t<Format>>
    auto input(const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<decltype(stdin_range<CharT>())>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");

        using context_type = basic_context<
            detail::range_wrapper_for_t<decltype(stdin_range<CharT>())>>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(stdin_range<CharT>()), args);
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx);
    }

    // prompt

    template <typename Format,
              typename... Args,
              typename CharT = detail::ranges::range_value_t<Format>>
    auto prompt(const CharT* p, const Format& f, Args&... a)
        -> detail::scan_result_for_range_t<decltype(stdin_range<CharT>())>
    {
        static_assert(sizeof...(Args) > 0,
                      "Have to scan at least a single argument");
        SCN_EXPECT(p != nullptr);

        std::printf("%s", p);

        using context_type = basic_context<
            detail::range_wrapper_for_t<decltype(stdin_range<CharT>())>>;
        using parse_context_type =
            basic_parse_context<typename context_type::locale_type>;

        auto args = make_args<context_type, parse_context_type>(a...);
        auto ctx = context_type(detail::wrap(stdin_range<CharT>()), args);
        auto pctx = parse_context_type(f, ctx);
        return vscan(ctx, pctx);
    }

    // getline

    namespace detail {
        template <typename WrappedRange, typename String, typename CharT>
        auto getline_impl(WrappedRange& r, String& str, CharT until)
            -> detail::scan_result_for_range_t<WrappedRange, error>
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
                std::copy(s.value().begin(), s.value().begin() + size,
                          str.begin());
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
        template <typename WrappedRange, typename CharT>
        auto getline_impl(WrappedRange& r,
                          basic_string_view<CharT>& str,
                          CharT until)
            -> detail::scan_result_for_range_t<WrappedRange, error>
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
                str = basic_string_view<CharT>{s.value().data(), size};
                return {{}, r.get_return()};
            }
            return {
                error(
                    error::invalid_operation,
                    "Cannot getline a string_view from a non-contiguous range"),
                r.get_return()};
        }
    }  // namespace detail

    template <typename Range, typename String, typename CharT>
    auto getline(Range&& r, String& str, CharT until)
        -> decltype(detail::getline_impl(
            std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
            str,
            until))
    {
        auto wrapped = detail::wrap(std::forward<Range>(r));
        return getline_impl(wrapped, str, until);
    }

    template <typename Range,
              typename String,
              typename CharT =
                  typename detail::extract_char_type<detail::ranges::iterator_t<
                      detail::range_wrapper_for_t<Range>>>::type>
    auto getline(Range&& r, String& str) -> decltype(
        getline(std::forward<Range>(r), str, detail::ascii_widen<CharT>('\n')))
    {
        return getline(std::forward<Range>(r), str,
                       detail::ascii_widen<CharT>('\n'));
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
                      detail::range_wrapper_for_t<
                          typename WrappedRange::iterator>>::type>
        auto ignore_until_impl(WrappedRange& r, CharT until)
            -> scan_result<WrappedRange, error>
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
                      detail::range_wrapper_for_t<
                          typename WrappedRange::iterator>>::type>
        auto ignore_until_n_impl(WrappedRange& r,
                                 ranges::range_difference_t<WrappedRange> n,
                                 CharT until)
            -> scan_result<WrappedRange, error>
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
    auto ignore_until(Range&& r, CharT until)
        -> decltype(detail::ignore_until_impl(
            std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
            until))
    {
        auto wrapped = detail::wrap(std::forward<Range>(r));
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
    auto ignore_until_n(Range&& r,
                        detail::ranges::range_difference_t<Range> n,
                        CharT until)
        -> decltype(detail::ignore_until_n_impl(
            std::declval<decltype(detail::wrap(std::forward<Range>(r)))&>(),
            n,
            until))
    {
        auto wrapped = detail::wrap(std::forward<Range>(r));
        auto ret = detail::ignore_until_n_impl(wrapped, n, until);
        if (!ret) {
            auto e = wrapped.reset_to_rollback_point();
            if (!e) {
                return {std::move(e), wrapped.get_return()};
            }
        }
        return ret;
    }

    // list

    template <typename T, typename OutputIt>
    struct list {
        using value_type = T;
        using iterator = OutputIt;

        list(OutputIt i)
            : it(std::move(i)), has_separator(false), is_separator(nullptr)
        {
        }

        template <typename F>
        list(OutputIt i, F f)
            : it(std::move(i)),
              has_separator(true),
              is_separator(reinterpret_cast<void (*)()>(+f))
        {
        }

        template <typename CharT>
        auto get_separator_fn() -> bool (*)(CharT)
        {
            return reinterpret_cast<bool (*)(CharT)>(is_separator);
        }

        OutputIt it;
        bool has_separator;

        void (*is_separator)();
    };

    template <typename Container, typename... A>
    list<typename Container::value_type, std::back_insert_iterator<Container>>
    make_list(Container& c, A&&... a)
    {
        return {std::back_inserter(c), std::forward<A>(a)...};
    }
    template <typename T, typename... A>
    list<T, std::vector<T>*> make_list(std::vector<T>& vec, A&&... a)
    {
        return {std::addressof(vec), std::forward<A>(a)...};
    }

    namespace detail {
        template <typename T, typename OutputIt, typename InputIt>
        void list_append(InputIt b, InputIt e, list<T, OutputIt>& l)
        {
            std::copy(b, e, l.it);
        }
        template <typename T, typename InputIt>
        void list_append(InputIt b, InputIt e, list<T, std::vector<T>*>& l)
        {
            l.it->insert(l.it->end(), b, e);
        }
    }  // namespace detail

    template <typename CharT, typename T, typename OutputIt>
    struct scanner<CharT, list<T, OutputIt>> : public scanner<CharT, T> {
        template <typename Context>
        error scan(list<T, OutputIt>& val, Context& ctx)
        {
            detail::small_vector<T, 8> buf{};
            auto is_separator = val.template get_separator_fn<CharT>();

            while (true) {
                {
                    auto ret = skip_range_whitespace(ctx);
                    if (!ret) {
                        if (ret == scn::error::end_of_range) {
                            break;
                        }
                        return ret;
                    }
                }

                {
                    T tmp{};
                    auto ret = scanner<CharT, T>::scan(tmp, ctx);
                    if (!ret) {
                        if (ret == scn::error::end_of_range) {
                            break;
                        }
                        return ret;
                    }
                    buf.push_back(tmp);
                    *this = scanner<CharT, list<T, OutputIt>>{};
                }

                if (val.has_separator) {
                    auto sep_ret = read_char(ctx.range());
                    if (!sep_ret) {
                        if (sep_ret.error() == scn::error::end_of_range) {
                            break;
                        }
                        return sep_ret.error();
                    }
                    if (is_separator(sep_ret.value())) {
                        continue;
                    }
                    else {
                        return error(error::invalid_scanned_value,
                                     "Invalid separator character");
                    }
                }
            }
            detail::list_append(buf.begin(), buf.end(), val);
            return {};
        }
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_SCAN_H
