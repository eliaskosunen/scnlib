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

        template <typename Iterator>
        class ranges_result : public result<int> {
        public:
            using iterator_type = Iterator;

            constexpr ranges_result(iterator_type it, result<int>&& base)
                : result<int>(std::move(base)), m_it(it)
            {
            }

            constexpr iterator_type iterator() const noexcept
            {
                return m_it;
            }

        private:
            iterator_type m_it;
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
        ranges_result<::ranges::iterator_t<const Range>> scan(
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
            using context_type = basic_context<stream_type>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, f, args);
            auto result = vscan(ctx);
            auto n = static_cast<std::ptrdiff_t>(stream.chars_read());
            return {::ranges::begin(range) + n, std::move(result)};
        }

        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>> scan(
            options opt,
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
            using context_type = basic_context<stream_type>;

            auto args = make_args<context_type>(a...);
            auto ctx = context_type(stream, f, args, opt);
            auto result = vscan(ctx);
            return {::ranges::begin(range) +
                        static_cast<std::ptrdiff_t>(stream.chars_read()),
                    std::move(result)};
        }

        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>> scanf(
            const Range& range,
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
            return {::ranges::begin(range) +
                        static_cast<std::ptrdiff_t>(stream.chars_read()),
                    std::move(result)};
        }
        template <typename Range, typename... Args>
        ranges_result<::ranges::iterator_t<const Range>> scanf(
            options opt,
            const Range& range,
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
            auto ctx = context_type(stream, f, args, opt);
            auto result = vscan(ctx);
            return {::ranges::begin(range) +
                        static_cast<std::ptrdiff_t>(stream.chars_read()),
                    std::move(result)};
        }

        SCN_END_NAMESPACE
    }  // namespace ranges
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && \
    !defined(SCN_RANGES_VSCAN_CPP)
#include "ranges/vscan.cpp"
#endif

#endif  // SCN_DETAIL_RANGES_RANGES_H

