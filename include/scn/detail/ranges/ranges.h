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

#ifndef SCN_DETAIL_RANGES_RANGES_H
#define SCN_DETAIL_RANGES_RANGES_H

#include "../vscan.h"
#include "types.h"

namespace scn {
    namespace ranges {
        SCN_BEGIN_NAMESPACE

        template <typename Iterator, typename Sentinel>
        class ranges_result : public result<int> {
        public:
            using iterator_type = Iterator;
            using sentinel_type = Sentinel;
            using difference_type = ::ranges::difference_type_t<iterator_type>;

            constexpr ranges_result(iterator_type it,
                                    sentinel_type end,
                                    result<int>&& base)
                : result<int>(std::move(base)), m_it(it), m_end(end)
            {
            }
            template <typename Range>
            constexpr ranges_result(const Range& r,
                                    difference_type n,
                                    result<int>&& base)
                : result<int>(std::move(base)),
                  m_it(::ranges::next(::ranges::begin(r), n)),
                  m_end(::ranges::end(r))
            {
            }

            constexpr iterator_type iterator() const noexcept
            {
                return m_it;
            }

            constexpr auto view() const noexcept
            {
                return ::ranges::make_subrange(iterator(), m_end);
            }

        private:
            iterator_type m_it;
            sentinel_type m_end;
        };

        template <typename Iterator, typename Range>
        auto subrange_from(Iterator it, const Range& r)
        {
            return ::ranges::make_subrange(it, ::ranges::end(r));
        }

        template <typename CharT>
        using basic_erased_stream_context =
            basic_context<erased_range_stream<CharT>>;
        template <typename CharT>
        using basic_erased_sized_stream_context =
            basic_context<erased_sized_range_stream<CharT>>;

        using erased_stream_context = basic_erased_stream_context<char>;
        using werased_stream_context = basic_erased_stream_context<wchar_t>;
        using erased_sized_stream_context =
            basic_erased_sized_stream_context<char>;
        using werased_sized_stream_context =
            basic_erased_sized_stream_context<wchar_t>;

        template <typename Stream>
        struct erased_stream_context_type {
            using char_type = typename Stream::char_type;
            using type = typename std::conditional<
                is_sized_stream<Stream>::value,
                basic_erased_sized_stream_context<char_type>,
                basic_erased_stream_context<char_type>>::type;
        };

        result<int> vscan(erased_stream_context&);
        result<int> vscan(werased_stream_context&);
        result<int> vscan(erased_sized_stream_context&);
        result<int> vscan(werased_sized_stream_context&);

        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>,
                      ::ranges::sentinel_t<const Range>>
        scan(const Range& range,
             basic_string_view<
                 ::ranges::value_type_t<::ranges::iterator_t<Range>>> f,
             Args&... a)
        {
            static_assert(sizeof...(Args) > 0,
                          "Have to scan at least a single argument");
            SCN_EXPECT(!f.empty());

            auto stream = make_stream(range);

            using stream_type = decltype(stream);
            using context_type = basic_context<stream_type>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, f, args);
            auto result = vscan(ctx);
            auto n = static_cast<std::ptrdiff_t>(stream.chars_read());
            return {range, n, std::move(result)};
        }

        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>,
                      ::ranges::sentinel_t<const Range>>
        scan(options opt,
             const Range& range,
             basic_string_view<
                 ::ranges::value_type_t<::ranges::iterator_t<Range>>> f,
             Args&... a)
        {
            static_assert(sizeof...(Args) > 0,
                          "Have to scan at least a single argument");
            SCN_EXPECT(!f.empty());

            auto stream = make_stream(range);

            using stream_type = decltype(stream);
            using context_type =
                basic_context<stream_type,
                              basic_locale_ref<::ranges::value_type_t<
                                  ::ranges::iterator_t<Range>>>>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, f, args, opt);
            auto result = vscan(ctx);
            return {range, static_cast<std::ptrdiff_t>(stream.chars_read()),
                    std::move(result)};
        }

        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>,
                      ::ranges::sentinel_t<const Range>>
        scan_default(const Range& range, Args&... a)
        {
            static_assert(sizeof...(Args) > 0,
                          "Have to scan at least a single argument");

            auto stream = make_stream(range);

            using stream_type = decltype(stream);
            using context_type = basic_empty_context<stream_type>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, sizeof...(Args), args);
            auto result = vscan(ctx);
            auto n = static_cast<std::ptrdiff_t>(stream.chars_read());
            return {range, n, std::move(result)};
        }

        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>,
                      ::ranges::sentinel_t<const Range>>
        scan_default(options opt, const Range& range, Args&... a)
        {
            static_assert(sizeof...(Args) > 0,
                          "Have to scan at least a single argument");

            auto stream = make_stream(range);

            using stream_type = decltype(stream);
            using context_type =
                basic_empty_context<stream_type,
                                    basic_locale_ref<::ranges::value_type_t<
                                        ::ranges::iterator_t<Range>>>>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, sizeof...(Args), args, opt);
            auto result = vscan(ctx);
            return {range, static_cast<std::ptrdiff_t>(stream.chars_read()),
                    std::move(result)};
        }

        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>,
                      ::ranges::sentinel_t<const Range>>
        scanf(const Range& range,
              basic_string_view<
                  ::ranges::value_type_t<::ranges::iterator_t<Range>>> f,
              Args&... a)
        {
            static_assert(sizeof...(Args) > 0,
                          "Have to scan at least a single argument");

            auto stream = make_stream(range);

            using stream_type = decltype(stream);
            using context_type = basic_scanf_context<stream_type>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, f, args);
            auto result = vscan(ctx);
            return {range, static_cast<std::ptrdiff_t>(stream.chars_read()),
                    std::move(result)};
        }
        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>,
                      ::ranges::sentinel_t<const Range>>
        scanf(options opt,
              const Range& range,
              basic_string_view<
                  ::ranges::value_type_t<::ranges::iterator_t<Range>>> f,
              Args&... a)
        {
            static_assert(sizeof...(Args) > 0,
                          "Have to scan at least a single argument");

            auto stream = make_stream(range);

            using stream_type = decltype(stream);
            using context_type =
                basic_scanf_context<stream_type,
                                    basic_locale_ref<::ranges::value_type_t<
                                        ::ranges::iterator_t<Range>>>>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, f, args, opt);
            auto result = vscan(ctx);
            return {range, static_cast<std::ptrdiff_t>(stream.chars_read()),
                    std::move(result)};
        }

        SCN_END_NAMESPACE
#if 0
        template <typename Range>
        class scan_view : public ::ranges::view_interface<
                              scan_view<Range>,
                              ::ranges::is_finite<Range>::value
                                  ? ::ranges::finite
                                  : ::ranges::range_cardinality<Range>::value> {
            using _difference_type = ::ranges::difference_type_t<Range>;

        public:
            scan_view() = default;
            scan_view(Range r, _difference_type n = 0) : m_view(std::move(r), n)
            {
            }

        private:
            ::ranges::drop_view<Range> m_view;
        };

        SCN_END_NAMESPACE

        namespace view {
            SCN_BEGIN_NAMESPACE

            SCN_END_NAMESPACE
        }  // namespace view

        namespace action {
            SCN_BEGIN_NAMESPACE

            struct scan_fn {
            private:
                friend ::ranges::action::action_access;

                template <typename T,
                          typename E,
                          typename P = ::ranges::identity>
                static auto CPP_fun(bind)(scan_fn scan,
                                          T& value,
                                          E& err,
                                          P proj = P{})(
                    requires(!::ranges::Range<T>))
                {
                    return std::bind(scan, std::placeholders::_1, value, err,
                                     std::move(proj));
                }

            public:
                CPP_template(typename Range,
                             typename T,
                             typename P = ::ranges::identity)(
                    requires ::ranges::ForwardRange<Range>) Range
                operator()(Range&& rng, T& value, error& err, P = P{}) const
                {
                    auto ret = ::scn::ranges::scan_default(rng, value);
                    if (!ret) {
                        err = ret.error();
                        return std::move(rng);
                    }
                    auto dist = ::ranges::distance(::ranges::begin(rng),
                                                   ret.iterator());
                    ::ranges::action::drop(rng, dist);
                    return std::move(rng);
                }
            };

            RANGES_INLINE_VARIABLE(::ranges::action::action<scan_fn>, scan)

            SCN_END_NAMESPACE
        }  // namespace action
#endif
    }  // namespace ranges
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && \
    !defined(SCN_RANGES_VSCAN_CPP)
#include "ranges/vscan.cpp"
#endif

#endif  // SCN_DETAIL_RANGES_RANGES_H

