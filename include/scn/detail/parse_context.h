// Copyright 2017 Elias Kosunen
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

#include "locale.h"
#include "result.h"

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        class parse_context_base {
        public:
            SCN_CONSTEXPR14 std::ptrdiff_t next_arg_id()
            {
                return m_next_arg_id >= 0 ? m_next_arg_id++ : 0;
            }
            SCN_CONSTEXPR14 bool check_arg_id(std::ptrdiff_t)
            {
                if (m_next_arg_id > 0) {
                    return false;
                }
                m_next_arg_id = -1;
                return true;
            }

        protected:
            parse_context_base() = default;

            static SCN_CONSTEXPR14 expected<utf8::code_point> next_cp_impl(
                const string_view& sv)
            {
                utf8::code_point cp{};
                auto it = utf8::parse_code_point(sv.begin(), sv.end(), cp);
                if (!it) {
                    return it.error();
                }
                return {cp};
            }
            static SCN_CONSTEXPR14 expected<wchar_t> next_cp_impl(
                const wstring_view& sv)
            {
                return sv.front();
            }

            static SCN_CONSTEXPR14 error advance_cp_impl(string_view& sv)
            {
                utf8::code_point cp{};
                auto it = utf8::parse_code_point(sv.begin(), sv.end(), cp);
                if (!it) {
                    return it.error();
                }
                sv.remove_prefix(static_cast<size_t>(it.value() - sv.begin()));
                return {};
            }
            static constexpr error advance_cp_impl(wstring_view& sv)
            {
                sv.remove_prefix(1);
                return {};
            }

            static SCN_CONSTEXPR14 expected<utf8::code_point> peek_cp_impl(
                const string_view& sv)
            {
                utf8::code_point cp{};
                auto it = utf8::parse_code_point(sv.begin(), sv.end(), cp);
                if (!it) {
                    return it.error();
                }
                it = utf8::parse_code_point(it.value(), sv.end(), cp);
                if (!it) {
                    return it.error();
                }
                return {cp};
            }
            static SCN_CONSTEXPR14 expected<wchar_t> peek_cp_impl(
                const wstring_view& sv)
            {
                return {sv[1]};
            }

            std::ptrdiff_t m_next_arg_id{0};
        };
    }  // namespace detail

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    template <typename CharT>
    class basic_parse_context : public detail::parse_context_base {
    public:
        using char_type = CharT;
        using cp_type =
            typename std::conditional<std::is_same<CharT, char>::value,
                                      utf8::code_point,
                                      char_type>::type;
        using locale_type = basic_locale_ref<CharT>;
        using string_view_type = basic_string_view<char_type>;
        using iterator = typename string_view_type::iterator;

        constexpr basic_parse_context(basic_string_view<char_type> f,
                                      locale_type& loc)
            : m_str(f), m_locale(loc)
        {
        }

        bool should_skip_ws()
        {
            bool skip = false;
            while (*this && m_locale.get_static().is_space(next_char())) {
                skip = true;
                advance_char();
            }
            return skip;
        }
        bool should_read_literal()
        {
            const auto brace = detail::ascii_widen<char_type>('{');
            if (next_char() != brace) {
                if (next_char() == detail::ascii_widen<char_type>('}')) {
                    advance_char();
                }
                return true;
            }
            if (SCN_UNLIKELY(m_str.size() > 1 &&
                             *(m_str.begin() + 1) == brace)) {
                advance_char();
                return true;
            }
            return false;
        }
        SCN_NODISCARD constexpr bool check_literal(char_type ch) const
        {
            return ch == next_char();
        }
        SCN_NODISCARD SCN_CONSTEXPR14 bool check_literal(
            span<const char_type> ch) const
        {
            if (chars_left() < ch.size()) {
                return false;
            }
            for (size_t i = 0; i < ch.size(); ++i) {
                if (ch[i] != m_str[i]) {
                    return false;
                }
            }
            return true;
        }
        SCN_NODISCARD SCN_CONSTEXPR14 expected<bool> check_literal_cp(
            cp_type cp) const
        {
            auto next = next_cp();
            if (!next) {
                return next.error();
            }
            return cp == next.value();
        }

        constexpr bool good() const
        {
            return !m_str.empty();
        }
        constexpr explicit operator bool() const
        {
            return good();
        }

        constexpr char_type next_char() const
        {
            return m_str.front();
        }
        SCN_NODISCARD SCN_CONSTEXPR14 expected<cp_type> next_cp() const
        {
            return next_cp_impl(m_str);
        }

        constexpr std::size_t chars_left() const noexcept
        {
            return m_str.size();
        }
        SCN_NODISCARD SCN_CONSTEXPR14 expected<std::size_t> cp_left()
            const noexcept
        {
            auto d = utf8::code_point_distance(m_str.begin(), m_str.end());
            if (!d) {
                return d.error();
            }
            return {static_cast<std::size_t>(d.value())};
        }

        SCN_CONSTEXPR14 void advance_char(std::ptrdiff_t n = 1) noexcept
        {
            SCN_EXPECT(chars_left() >= static_cast<std::size_t>(n));
            m_str.remove_prefix(static_cast<std::size_t>(n));
        }
        SCN_NODISCARD SCN_CONSTEXPR14 error advance_cp() noexcept
        {
            SCN_EXPECT(cp_left() && cp_left().value() >= 1);
            return advance_cp_impl(m_str);
        }

        constexpr bool can_peek_char(std::size_t n = 1) const noexcept
        {
            return chars_left() > n;
        }
        SCN_NODISCARD SCN_CONSTEXPR14 expected<bool> can_peek_cp()
            const noexcept
        {
            return chars_left() > 1;
        }

        SCN_CONSTEXPR14 char_type peek_char(std::size_t n = 1) const noexcept
        {
            SCN_EXPECT(n < chars_left());
            return m_str[n];
        }
        SCN_NODISCARD SCN_CONSTEXPR14 expected<cp_type> peek_cp() const noexcept
        {
            SCN_EXPECT(can_peek_cp());
            return peek_cp_impl(m_str);
        }

        SCN_CONSTEXPR14 iterator begin() const noexcept
        {
            return m_str.begin();
        }
        SCN_CONSTEXPR14 iterator end() const noexcept
        {
            return m_str.end();
        }

        SCN_CONSTEXPR14 bool check_arg_begin() const
        {
            SCN_EXPECT(good());
            return next_char() == detail::ascii_widen<char_type>('{');
        }
        SCN_CONSTEXPR14 bool check_arg_end() const
        {
            SCN_EXPECT(good());
            return next_char() == detail::ascii_widen<char_type>('}');
        }

        using parse_context_base::check_arg_id;
        SCN_CONSTEXPR14 void check_arg_id(basic_string_view<CharT>) {}

        SCN_CONSTEXPR14 void arg_begin() const noexcept {}
        SCN_CONSTEXPR14 void arg_end() const noexcept {}

        SCN_CONSTEXPR14 void arg_handled() const noexcept {}

        const locale_type& locale() const
        {
            return m_locale;
        }

        template <typename Scanner>
        error parse(Scanner& s)
        {
            return s.parse(*this);
        }

        bool has_arg_id()
        {
            SCN_EXPECT(good());
            if (m_str.size() == 1) {
                return true;
            }
            if (m_str[1] == detail::ascii_widen<char_type>('}')) {
                advance_char();
                return false;
            }
            if (m_str[1] == detail::ascii_widen<char_type>(':')) {
                advance_char(2);
                return false;
            }
            return true;
        }
        expected<string_view_type> parse_arg_id()
        {
            SCN_EXPECT(good());
            advance_char();
            if (SCN_UNLIKELY(!good())) {
                return error(error::invalid_format_string,
                             "Unexpected end of format argument");
            }
            auto it = m_str.begin();
            for (std::ptrdiff_t i = 0; good(); ++i, (void)advance_char()) {
                if (check_arg_end()) {
                    return string_view_type{
                        it,
                        static_cast<typename string_view_type::size_type>(i)};
                }
                if (next_char() == detail::ascii_widen<char_type>(':')) {
                    advance_char();
                    return string_view_type{
                        it,
                        static_cast<typename string_view_type::size_type>(i)};
                }
            }
            return error(error::invalid_format_string,
                         "Unexpected end of format argument");
        }

    private:
        string_view_type m_str;
        locale_type& m_locale;
    };

    template <typename CharT>
    class basic_empty_parse_context : public detail::parse_context_base {
    public:
        using char_type = CharT;
        using cp_type =
            typename std::conditional<std::is_same<CharT, char>::value,
                                      utf8::code_point,
                                      char_type>::type;
        using locale_type = basic_locale_ref<char_type>;
        using string_view_type = basic_string_view<char_type>;

        constexpr basic_empty_parse_context(int args, locale_type& loc)
            : m_args_left(args), m_locale(loc)
        {
        }

        SCN_CONSTEXPR14 bool should_skip_ws()
        {
            if (m_should_skip_ws) {
                m_should_skip_ws = false;
                return true;
            }
            return false;
        }
        constexpr bool should_read_literal() const
        {
            return false;
        }
        constexpr bool check_literal(char_type) const
        {
            return false;
        }
        constexpr bool check_literal(span<const char_type>) const
        {
            return false;
        }
        constexpr bool check_literal_cp(cp_type) const
        {
            return false;
        }

        constexpr bool good() const
        {
            return m_args_left > 0;
        }
        constexpr explicit operator bool() const
        {
            return good();
        }

        SCN_CONSTEXPR14 void advance_char(std::ptrdiff_t = 1) const noexcept {}
        SCN_CONSTEXPR14 error advance_cp() const noexcept
        {
            return {};
        }

        char_type next_char() const
        {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
        expected<std::pair<cp_type, std::ptrdiff_t>> next_cp() const
        {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }

        std::size_t chars_left() const noexcept
        {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
        std::size_t cp_left() const noexcept
        {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }

        constexpr bool can_peek_char() const noexcept
        {
            return false;
        }
        constexpr bool can_peek_cp() const noexcept
        {
            return false;
        }

        char_type peek_char(std::ptrdiff_t = 1) const noexcept
        {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }
        expected<cp_type> peek_cp() const noexcept
        {
            SCN_EXPECT(false);
            SCN_UNREACHABLE;
        }

        constexpr bool check_arg_begin() const
        {
            return true;
        }
        constexpr bool check_arg_end() const
        {
            return true;
        }

        using parse_context_base::check_arg_id;
        SCN_CONSTEXPR14 void check_arg_id(basic_string_view<CharT>) {}

        SCN_CONSTEXPR14 void arg_begin() const noexcept {}
        SCN_CONSTEXPR14 void arg_end() const noexcept {}

        SCN_CONSTEXPR14 void arg_handled()
        {
            m_should_skip_ws = true;
            --m_args_left;
        }

        const locale_type& locale() const
        {
            return m_locale;
        }

        template <typename Scanner>
        constexpr error parse(Scanner&) const
        {
            return {};
        }

        constexpr bool has_arg_id() const
        {
            return false;
        }
        SCN_CONSTEXPR14 expected<string_view_type> parse_arg_id() const
        {
            SCN_EXPECT(good());
            return string_view_type{};
        }

        void reset_args_left(int n)
        {
            m_args_left = n;
            parse_context_base::m_next_arg_id = 0;
            m_should_skip_ws = false;
        }

    private:
        int m_args_left;
        bool m_should_skip_ws{false};
        locale_type& m_locale;
    };

    namespace detail {
        template <typename CharT>
        basic_parse_context<CharT> make_parse_context_impl(
            basic_string_view<CharT> f,
            basic_locale_ref<CharT>& loc)
        {
            return {f, loc};
        }
        template <typename CharT>
        basic_empty_parse_context<CharT> make_parse_context_impl(
            int i,
            basic_locale_ref<CharT>& loc)
        {
            return {i, loc};
        }

        template <typename CharT>
        struct parse_context_template_for_format<basic_string_view<CharT>> {
            template <typename T>
            using type = basic_parse_context<T>;
        };
        template <>
        struct parse_context_template_for_format<int> {
            template <typename CharT>
            using type = basic_empty_parse_context<CharT>;
        };
    }  // namespace detail

    template <typename F, typename CharT>
    auto make_parse_context(F f, basic_locale_ref<CharT>& locale)
        -> decltype(detail::make_parse_context_impl(f, locale))
    {
        return detail::make_parse_context_impl(f, locale);
    }

    SCN_CLANG_POP

    SCN_END_NAMESPACE
}  // namespace scn

#endif  // SCN_DETAIL_PARSE_CONTEXT_H
