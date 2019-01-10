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

#ifndef SCN_DETAIL_CORE_H
#define SCN_DETAIL_CORE_H

#include "result.h"
#include "span.h"
#include "string_view.h"

namespace scn {
    /**
     * Skip any whitespace from the stream.
     * Next read_char() will return the first non-whitespace character of EOF.
     * \param ctx Stream and locale to use
     * \param allow_eof In case of EOF, return error::good if `true` or
     * error::end_of_stream if `false`
     */
    template <typename Context>
    error skip_stream_whitespace(Context& ctx)
    {
        while (true) {
            auto ch = ctx.stream().read_char();
            if (!ch) {
                return ch.get_error();
            }
#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif
            if (!ctx.locale().is_space(ch.value())) {
                auto pb = ctx.stream().putback(ch.value());
                if (!pb) {
                    return pb;
                }
                break;
            }
#if SCN_CLANG >= SCN_COMPILER(3, 9, 0)
#pragma clang diagnostic pop
#endif
        }
        return {};
    }
    template <typename Context>
    error parse_whitespace(Context& ctx)
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

    namespace detail {
        template <typename Context>
        struct custom_value {
            using fn_type = error (*)(void*, Context&);

            void* value;
            fn_type scan;
        };
    }  // namespace detail

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
}  // namespace scn

#endif  // SCN_DETAIL_CORE_H
