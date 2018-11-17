// Copyright 2017-2018 Elias Kosunen
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

#ifndef SCN_STREAM_H
#define SCN_STREAM_H

#include "string_view.h"

#include <array>
#include <vector>

namespace scn {
    template <typename Char, typename Source, typename Enable = void>
    class basic_static_container_stream;

    template <typename Char, typename Container>
    class basic_static_container_stream<Char, Container> {
    public:
        using char_type = Char;
        using source_type = Container;
        using iterator = typename source_type::const_iterator;

        basic_static_container_stream(const source_type& s)
            : m_source(std::addressof(s)), m_next(begin())
        {
        }

        expected<char_type, error> read_char()
        {
            if (m_next == end()) {
                return make_unexpected(error::end_of_stream);
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        expected<void, error> putback(char_type)
        {
            --m_next;
            // TODO: Check underflow
            // TODO: Check if given char is correct
            return {};
        }
        expected<void, error> putback_all()
        {
            m_next = begin();
            return {};
        }

    private:
        iterator begin() const
        {
            using std::begin;
            return begin(*m_source);
        }
        iterator end() const
        {
            using std::end;
            return end(*m_source);
        }

        const source_type* m_source;
        iterator m_next{};
    };
    template <typename Char>
    class basic_static_container_stream<Char, span<const Char>> {
    public:
        using char_type = Char;
        using source_type = span<const Char>;
        using iterator = typename source_type::const_iterator;

        basic_static_container_stream(source_type s)
            : m_source(s), m_next(begin())
        {
        }

        expected<char_type, error> read_char()
        {
            if (m_next == end()) {
                return make_unexpected(error::end_of_stream);
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        expected<void, error> putback(char_type)
        {
            --m_next;
            // TODO: Check underflow
            // TODO: Check if given char is correct
            return {};
        }
        expected<void, error> putback_all()
        {
            m_next = begin();
            return {};
        }

    private:
        iterator begin()
        {
            using std::begin;
            return begin(m_source);
        }
        iterator end()
        {
            using std::end;
            return end(m_source);
        }

        source_type m_source;
        iterator m_next{};
    };

    template <typename Iterator>
    struct basic_bidirectional_iterator_stream {
        using char_type = typename std::iterator_traits<Iterator>::value_type;

        basic_bidirectional_iterator_stream(Iterator begin, Iterator end)
            : m_begin(begin), m_end(end), m_next(begin)
        {
        }

        expected<char_type, error> read_char()
        {
            if (m_next == m_end) {
                return make_unexpected(error::end_of_stream);
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        expected<void, error> putback(char_type)
        {
            --m_next;
            // TODO: Check underflow
            // TODO: Check if given char is correct
            return {};
        }
        expected<void, error> putback_all()
        {
            m_next = m_begin;
            return {};
        }

    private:
        Iterator m_begin, m_end, m_next;
    };

    template <typename CharT>
    scn::basic_static_container_stream<CharT, span<const CharT>> make_stream(
        span<const CharT> s)
    {
        return s;
    }
    template <typename CharT>
    scn::basic_static_container_stream<CharT, std::basic_string<CharT>>
    make_stream(const std::basic_string<CharT>& s)
    {
        return s;
    }
    template <typename CharT>
    scn::basic_static_container_stream<CharT, std::vector<CharT>> make_stream(
        const std::vector<CharT>& s)
    {
        return s;
    }
    template <typename CharT, size_t N>
    scn::basic_static_container_stream<CharT, std::array<CharT, N>> make_stream(
        const std::array<CharT, N>& s)
    {
        return s;
    }

    template <typename Iterator>
    scn::basic_bidirectional_iterator_stream<Iterator> make_stream(
        Iterator begin,
        Iterator end)
    {
        return {begin, end};
    }
}  // namespace scn

#endif  // SCN_STREAM_H