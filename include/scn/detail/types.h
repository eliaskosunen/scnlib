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

#ifndef SCN_DETAIL_TYPES_H
#define SCN_DETAIL_TYPES_H

#include "visitor.h"

#include <algorithm>
#include <vector>

SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

namespace scn {
    template <typename Stream,
              typename Traits,
              typename Allocator,
              typename CharT = typename Stream::char_type>
    error getline(Stream& s, std::basic_string<CharT, Traits, Allocator>& str)
    {
        using context_type = basic_context<Stream>;
        auto f = basic_string_view<CharT>();
        auto args = make_args<context_type>(str);
        auto ctx = context_type(s, f, args);

        str.clear();
        auto res = scan_chars(
            ctx, std::back_inserter(str),
            predicates::until<context_type>{ctx.locale().widen('\n')});
        if (!res) {
            return res.get_error();
        }
        return {};
    }
    template <typename Stream,
              typename Traits,
              typename Allocator,
              typename CharT = typename Stream::char_type>
    error getline(Stream& s,
                  std::basic_string<CharT, Traits, Allocator>& str,
                  CharT until)
    {
        using context_type = basic_context<Stream>;
        auto f = basic_string_view<CharT>();
        auto args = make_args<context_type>(str);
        auto ctx = context_type(s, f, args);

        str.clear();
        auto res = scan_chars(ctx, std::back_inserter(str),
                              predicates::until<context_type>{until});
        if (!res) {
            return res.get_error();
        }
        return {};
    }

    namespace detail {
        template <typename CharT>
        struct ignore_iterator {
            using value_type = CharT;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::input_iterator_tag;

            ignore_iterator() = default;
            explicit ignore_iterator(difference_type n) : i(n) {}

            reference operator*() const
            {
                static CharT c(0);
                return c;
            }
            pointer operator->() const
            {
                return &operator*();
            }

            ignore_iterator& operator++()
            {
                ++i;
                return *this;
            }
            ignore_iterator operator++(int)
            {
                auto tmp(*this);
                operator++();
                return tmp;
            }

            void swap(ignore_iterator& other)
            {
                using std::swap;
                swap(i, other.i);
            }

            bool operator==(ignore_iterator& other)
            {
                return i == other.i;
            }
            bool operator!=(ignore_iterator& other)
            {
                return !(*this == other);
            }

            static ignore_iterator max()
            {
                return {std::numeric_limits<difference_type>::max()};
            }

            difference_type i{0};
        };

        template <typename CharT>
        void swap(ignore_iterator<CharT>& a, ignore_iterator<CharT>& b)
        {
            a.swap(b);
        }
    }  // namespace detail

    template <typename Stream, typename CharT = typename Stream::char_type>
    auto ignore_all(Stream& s) ->
        typename std::enable_if<!is_bulk_stream<Stream>::value, error>::type
    {
        using context_type = basic_context<Stream>;
        auto f = basic_string_view<CharT>();

        auto ctx = context_type(s, f, basic_args<context_type>());

        auto res = scan_chars(ctx, detail::ignore_iterator<CharT>{},
                              predicates::propagate<context_type>{});
        if (!res && res.get_error() != error::end_of_stream) {
            return res.get_error();
        }
        return {};
    }
    template <typename Stream, typename CharT = typename Stream::char_type>
    auto ignore_all(Stream& s) ->
        typename std::enable_if<is_bulk_stream<Stream>::value, error>::type
    {
        return s.skip_all();
    }

    template <typename Stream>
    error ignore_until(Stream& s, typename Stream::char_type until)
    {
        using char_type = typename Stream::char_type;
        using context_type = basic_context<Stream>;
        auto f = basic_string_view<char_type>();

        auto ctx = context_type(s, f, basic_args<context_type>());

        auto res = scan_chars(ctx, detail::ignore_iterator<char_type>{},
                              predicates::until<context_type>{until});
        if (!res) {
            return res.get_error();
        }
        return {};
    }

    template <typename Stream, typename CharT = typename Stream::char_type>
    auto ignore_n(Stream& s, std::ptrdiff_t count) ->
        typename std::enable_if<!is_bulk_stream<Stream>::value, error>::type
    {
        using context_type = basic_context<Stream>;
        auto f = basic_string_view<CharT>();

        auto ctx = context_type(s, f, basic_args<context_type>());

        auto res = scan_chars_until(ctx, detail::ignore_iterator<CharT>{},
                                    detail::ignore_iterator<CharT>{count},
                                    predicates::propagate<context_type>{});
        if (!res) {
            return res.get_error();
        }
        return {};
    }
    template <typename Stream, typename CharT = typename Stream::char_type>
    auto ignore_n(Stream& s, std::ptrdiff_t count) ->
        typename std::enable_if<is_bulk_stream<Stream>::value, error>::type
    {
        return s.skip(static_cast<size_t>(count));
    }

    template <typename Stream>
    error ignore_n_until(Stream& s,
                         std::ptrdiff_t count,
                         typename Stream::char_type until)
    {
        using char_type = typename Stream::char_type;
        using context_type = basic_context<Stream>;
        auto f = basic_string_view<char_type>();

        auto ctx = context_type(s, f, basic_args<context_type>());

        auto res = scan_chars_until(ctx, detail::ignore_iterator<char_type>{},
                                    detail::ignore_iterator<char_type>{count},
                                    predicates::until<context_type>{until});
        if (!res) {
            return res.get_error();
        }
        return {};
    }

    template <typename Stream>
    result<typename Stream::char_type> getchar(Stream& s)
    {
        return s.read_char();
    }
}  // namespace scn

SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

#endif  // SCN_DETAIL_TYPES_H
