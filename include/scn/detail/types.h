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

SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename Stream,
              typename Traits,
              typename Allocator,
              typename CharT = typename Stream::char_type>
    error getline(Stream& s,
                  std::basic_string<CharT, Traits, Allocator>& str,
                  CharT until)
    {
        str.clear();
        auto res = read_into_if(s, std::back_inserter(str),
                                predicates::until<CharT>{until}, true);
        if (!res) {
            return res.error();
        }
        return {};
    }
    template <typename Stream,
              typename Traits,
              typename Allocator,
              typename CharT = typename Stream::char_type>
    error getline(Stream& s, std::basic_string<CharT, Traits, Allocator>& str)
    {
        return getline(s, str, detail::default_widen<CharT>::widen('\n'));
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
        typename std::enable_if<!is_sized_stream<Stream>::value, error>::type
    {
        auto res = read_into_if(s, detail::ignore_iterator<CharT>{},
                                predicates::propagate<CharT>{});
        if (!res && res.error() != error::end_of_stream) {
            return res.error();
        }
        return {};
    }
    template <typename Stream, typename CharT = typename Stream::char_type>
    auto ignore_all(Stream& s) ->
        typename std::enable_if<is_sized_stream<Stream>::value, error>::type
    {
        s.skip_all();
        return {};
    }

    template <typename Stream>
    error ignore_until(Stream& s, typename Stream::char_type until)
    {
        auto res = read_into_if(
            s, detail::ignore_iterator<typename Stream::char_type>{},
            predicates::until<typename Stream::char_type>{until});
        if (!res) {
            return res.error();
        }
        return {};
    }

    template <typename Stream, typename CharT = typename Stream::char_type>
    auto ignore_n(Stream& s, std::ptrdiff_t count) ->
        typename std::enable_if<!is_sized_stream<Stream>::value, error>::type
    {
        auto res = read_into_if(s, detail::ignore_iterator<CharT>{},
                                detail::ignore_iterator<CharT>{count},
                                predicates::propagate<CharT>{});
        if (!res) {
            return res.error();
        }
        return {};
    }
    template <typename Stream, typename CharT = typename Stream::char_type>
    auto ignore_n(Stream& s, std::ptrdiff_t count) ->
        typename std::enable_if<is_sized_stream<Stream>::value, error>::type
    {
        s.skip(static_cast<size_t>(count));
        return {};
    }

    template <typename Stream>
    error ignore_n_until(Stream& s,
                         std::ptrdiff_t count,
                         typename Stream::char_type until)
    {
        auto res = read_into_if(
            s, detail::ignore_iterator<typename Stream::char_type>{},
            detail::ignore_iterator<typename Stream::char_type>{count},
            predicates::until<typename Stream::char_type>{until});
        if (!res) {
            return res.error();
        }
        return {};
    }

    template <typename Stream,
              typename std::enable_if<is_sized_stream<Stream>::value>::type* =
                  nullptr>
    expected<typename Stream::char_type> getchar(Stream& s)
    {
        return s.read_char();
    }

    template <typename T, typename Stream>
    expected<T> get_value(Stream& s)
    {
        using context_type = basic_empty_context<Stream>;
        using char_type = typename Stream::char_type;

        auto args = make_args<context_type>();
        auto ctx = context_type(s, 1, args);

        auto ret = skip_stream_whitespace(ctx);
        if (!ret) {
            return ret;
        }

        T val{};
        scanner<char_type, T> sc;
        ret = sc.scan(val, ctx);
        if (!ret) {
            return ret;
        }
        return val;
    }

    SCN_END_NAMESPACE
}  // namespace scn

SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE

#endif  // SCN_DETAIL_TYPES_H
