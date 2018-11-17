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

#include "../expected-lite/expected.h"
#include "../span-lite/span.h"

#include "locale.h"

namespace scn {
    enum class error {
        good,
        end_of_stream,
        invalid_format_string,
        invalid_scanned_value,
        unrecoverable_stream_error,
        stream_source_error,
        unrecoverable_stream_source_error,
        putback_all_not_available
    };

    inline bool is_recoverable_error(error e)
    {
        return e == error::unrecoverable_stream_error ||
               e == error::unrecoverable_stream_source_error ||
               e == error::putback_all_not_available;
    }

    namespace detail {
        template <typename Context>
        struct custom_value {
            using fn_type = expected<void, error>(void*, Context&);

            void* value;
            fn_type* scan;
        };
    }  // namespace detail

    template <typename Context>
    expected<void, error> skip_stream_whitespace(Context& ctx)
    {
        while (true) {
            auto ch = ctx.stream().read_char();
            if (!ch) {
                return make_unexpected(ch.error());
            }
            if (!ctx.locale().is_space(ch.value())) {
                auto pb = ctx.stream().putback(ch.value());
                if (!pb) {
                    return pb;
                }
                break;
            }
        }
        return {};
    }
    template <typename Context>
    expected<void, error> parse_whitespace(Context& ctx)
    {
        bool found = false;
        while (ctx.locale().is_space(*ctx.parse_context().begin())) {
            if (!found) {
                auto ret = skip_stream_whitespace(ctx);
                if (!ret) {
                    return ret;
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

    template <typename CharT, typename T, typename Enable = void>
    struct basic_value_scanner;

    template <typename Stream>
    class basic_context {
    public:
        using stream_type = Stream;
        using char_type = typename stream_type::char_type;
        using parse_context_type = basic_parse_context<char_type>;
        using locale_type = basic_locale_ref<char_type>;

        template <typename T>
        using value_scanner_type = basic_value_scanner<char_type, T>;

        basic_context(stream_type s,
                      basic_string_view<char_type> f,
                      locale_type locale = locale_type())
            : m_stream(std::move(s)),
              m_parse_ctx(std::move(f)),
              m_locale(locale)
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
        locale_type locale() const
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