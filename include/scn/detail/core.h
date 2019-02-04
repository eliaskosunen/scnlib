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
     * error::end_of_stream if `false`
     */
    template <typename Context>
    error skip_stream_whitespace(Context& ctx) noexcept
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
    error parse_whitespace(Context& ctx) noexcept
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

        SCN_CONSTEXPR14 iterator advance() noexcept
        {
            m_str.remove_prefix(1);
            return begin();
        }
        SCN_CONSTEXPR14 void advance_to(iterator it) noexcept
        {
            m_str.remove_prefix(
                static_cast<size_t>(std::distance(begin(), it)));
        }

        SCN_CONSTEXPR14 unsigned next_arg_id()
        {
            if (m_next_arg_id >= 0) {
                return m_next_arg_id++;
            }
            return 0;
        }
        SCN_CONSTEXPR14 bool check_arg_id(unsigned)
        {
            if (m_next_arg_id > 0) {
                return false;
            }
            m_next_arg_id = -1;
            return true;
        }
        SCN_CONSTEXPR14 void check_arg_id(basic_string_view<Char>) {}

    private:
        basic_string_view<char_type> m_str;
        int m_next_arg_id{0};
    };

    template <typename CharT, typename T, typename Enable = void>
    struct basic_value_scanner;
}  // namespace scn

#endif  // SCN_DETAIL_CORE_H
