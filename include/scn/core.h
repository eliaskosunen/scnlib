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

#ifndef SCN_CORE_H
#define SCN_CORE_H

#include "expected-lite/expected.h"
#include "span-lite/span.h"

#include "locale.h"

namespace scn {
    enum class error {
        end_of_stream = 1,
        invalid_format_string,
        invalid_scanned_value
    };

    namespace detail {
        template <typename Context>
        struct custom_value {
            using fn_type = expected<void, error>(void*, Context&);

            void* value;
            fn_type* scan;
        };
    }  // namespace detail

    template <typename Context>
    expected<void, error> parse_whitespace(Context& ctx)
    {
        bool found = false;
        while (detail::contains(*ctx.parse_context().begin(),
                                ctx.locale().space)) {
            if (!found) {
                auto next = ctx.stream().read_char();
                if (!next) {
                    return make_unexpected(next.error());
                }
                if (detail::contains(next.value(), ctx.locale().space)) {
                    ctx.stream().putback(next.value());
                    found = true;
                }
            }
            ctx.parse_context().advance();
        }
        return {};
    }

    template <typename Char>
    class basic_parse_context {
    public:
        using char_type = Char;
        using iterator = typename basic_string_view<char_type>::iterator;

        explicit SCN_CONSTEXPR basic_parse_context(
            basic_string_view<char_type> f)
            : m_str(f)
        {
        }

        SCN_CONSTEXPR iterator begin() const noexcept
        {
            return m_str.begin();
        }
        SCN_CONSTEXPR iterator end() const noexcept
        {
            return m_str.end();
        }

        SCN_CONSTEXPR14 iterator advance()
        {
            m_str.remove_prefix(1);
            return begin();
        }
        SCN_CONSTEXPR14 void advance_to(iterator it)
        {
            m_str.remove_refix(static_cast<size_t>(std::distance(begin(), it)));
        }

    private:
        basic_string_view<char_type> m_str;
    };

    template <typename Char, typename Source, typename Enable = void>
    class basic_stream;

    template <typename Char, typename Container>
    class basic_stream<Char, Container> {
    public:
        using char_type = Char;
        using source_type = Container;
        using iterator = typename source_type::const_iterator;

        basic_stream(const source_type& s)
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
        bool putback(char_type)
        {
            --m_next;
            // TODO: Check underflow
            // TODO: Check if given char is correct
            return true;
        }
        bool putback_all()
        {
            m_next = begin();
            return true;
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
    class basic_stream<Char, span<const Char>> {
    public:
        using char_type = Char;
        using source_type = span<const Char>;
        using iterator = typename source_type::const_iterator;

        basic_stream(source_type s) : m_source(s), m_next(begin()) {}

        expected<char_type, error> read_char()
        {
            if (m_next == end()) {
                return make_unexpected(error::end_of_stream);
            }
            auto ch = *m_next;
            ++m_next;
            return ch;
        }
        bool putback(char_type)
        {
            --m_next;
            // TODO: Check underflow
            // TODO: Check if given char is correct
            return true;
        }
        bool putback_all()
        {
            m_next = begin();
            return true;
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

    template <typename CharT, typename T, typename Enable = void>
    struct basic_value_scanner;

    template <typename Stream>
    class basic_context {
    public:
        using stream_type = Stream;
        using char_type = typename stream_type::char_type;
        using parse_context_type = basic_parse_context<char_type>;
        using locale_type = basic_locale<char_type>;

        template <typename T>
        using value_scanner_type = basic_value_scanner<char_type, T>;

        basic_context(stream_type s,
                      basic_string_view<char_type> f,
                      locale_type locale)
            : m_stream(std::move(s)),
              m_parse_ctx(std::move(f)),
              m_locale(std::move(locale))
        {
        }

        parse_context_type& parse_context()
        {
            return m_parse_ctx;
        }
        stream_type& stream()
        {
            return m_stream;
        }
        const locale_type& locale() const
        {
            return m_locale;
        }

    private:
        stream_type m_stream;
        parse_context_type m_parse_ctx;
        locale_type m_locale;
    };
}  // namespace scn

#endif  // SCN_CORE_H