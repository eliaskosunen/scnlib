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

#ifndef SCN_DETAIL_PARSE_CONTEXT_H
#define SCN_DETAIL_PARSE_CONTEXT_H

#include "result.h"
#include "string_view.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

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

        SCN_CONSTEXPR14 size_t next_arg_id()
        {
            if (m_next_arg_id >= 0) {
                return m_next_arg_id++;
            }
            return 0;
        }
        SCN_CONSTEXPR14 bool check_arg_id(size_t)
        {
            if (m_next_arg_id > 0) {
                return false;
            }
            m_next_arg_id = static_cast<size_t>(-1);
            return true;
        }
        SCN_CONSTEXPR14 void check_arg_id(basic_string_view<Char>) {}

        template <typename Locale>
        bool should_skip_ws(const Locale& loc)
        {
            bool skip = false;
            while (loc.is_space(next())) {
                skip = true;
                advance();
            }
            return skip;
        }
        template <typename Locale>
        bool should_read_literal(const Locale& loc)
        {
            const auto brace = loc.widen('{');
            if (next() != brace) {
                if (next() == loc.widen('}')) {
                    advance();
                }
                return true;
            }
            if (SCN_UNLIKELY(m_str.size() > 1 &&
                             *(m_str.begin() + 1) == brace)) {
                advance();
                return true;
            }
            return false;
        }
        SCN_CONSTEXPR bool check_literal(char_type ch) const
        {
            return ch == next();
        }

        SCN_CONSTEXPR bool good() const
        {
            return !m_str.empty();
        }
        SCN_CONSTEXPR explicit operator bool() const
        {
            return good();
        }

        SCN_CONSTEXPR14 void advance(size_t n = 1) noexcept
        {
            SCN_EXPECT(good());
            m_str.remove_prefix(n);
        }
        SCN_CONSTEXPR char_type next() const
        {
            return m_str.front();
        }

        template <typename Locale>
        bool check_arg_begin(const Locale& loc) const
        {
            return next() == loc.widen('{');
        }
        template <typename Locale>
        bool check_arg_end(const Locale& loc) const
        {
            return next() == loc.widen('}');
        }

        void arg_begin()
        {
            advance();
        }
        void arg_end() {}

    private:
        basic_string_view<char_type> m_str;
        size_t m_next_arg_id{0};
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_PARSE_CONTEXT_H
