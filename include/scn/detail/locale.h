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

#ifndef SCN_DETAIL_LOCALE_H
#define SCN_DETAIL_LOCALE_H

#include "result.h"
#include "string_view.h"

#include <cwchar>
#include <string>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace detail {
        template <typename CharT>
        class truename_falsename_storage {
        public:
            using char_type = CharT;
            using string_type = std::basic_string<char_type>;
            using string_view_type = basic_string_view<char_type>;

            truename_falsename_storage(const void* loc);

            constexpr const string_type& get_true_str() const
            {
                return m_truename;
            }
            constexpr const string_type& get_false_str() const
            {
                return m_falsename;
            }

            constexpr string_view_type get_true_view() const
            {
                return string_view_type(m_truename.data(), m_truename.size());
            }
            constexpr string_view_type get_false_view() const
            {
                return string_view_type(m_falsename.data(), m_falsename.size());
            }

        private:
            string_type m_truename;
            string_type m_falsename;
        };

        constexpr bool has_zero(uint64_t v)
        {
            return (v - UINT64_C(0x0101010101010101)) & ~v &
                   UINT64_C(0x8080808080808080);
        }

        // Hand write to avoid C locales and thus noticeable performance losses
        inline bool is_space(char ch) noexcept
        {
            static constexpr detail::array<bool, 256> lookup = {
                {false, false, false, false, false, false, false, false, false,
                 true,  true,  true,  true,  true,  false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, true,  false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false, false, false, false, false, false,
                 false, false, false, false}};
            const auto i =
                static_cast<unsigned char>((ch < 0) ? (ch + 256) : ch);
            return lookup[static_cast<size_t>(i)];
        }
        constexpr inline bool is_space(wchar_t ch) noexcept
        {
            return ch == 0x20 || (ch >= 0x09 && ch <= 0x0d);
        }

        constexpr inline bool is_digit(char ch) noexcept
        {
            return ch >= '0' && ch <= '9';
        }
        constexpr inline bool is_digit(wchar_t ch) noexcept
        {
            return ch >= L'0' && ch <= L'9';
        }

        template <typename CharT>
        struct default_widen;
        template <>
        struct default_widen<char> {
            static constexpr char widen(char ch) noexcept
            {
                return ch;
            }
        };
        template <>
        struct default_widen<wchar_t> {
            static wchar_t widen(char ch)
            {
                auto ret = std::btowc(static_cast<int>(ch));
                if (ret == WEOF) {
                    return static_cast<wchar_t>(-1);
                }
                return static_cast<wchar_t>(ret);
            }
        };

        template <typename CharT>
        struct default_narrow;
        template <>
        struct default_narrow<char> {
            static constexpr char narrow(char ch, char) noexcept
            {
                return ch;
            }
        };
        template <>
        struct default_narrow<wchar_t> {
            static char narrow(wchar_t ch, char def)
            {
                auto ret = std::wctob(static_cast<wint_t>(ch));
                if (ret == EOF) {
                    return def;
                }
                return static_cast<char>(ret);
            }
        };

        template <typename CharT>
        struct locale_defaults;
        template <>
        struct locale_defaults<char> {
            static constexpr string_view truename()
            {
                return {"true"};
            }
            static constexpr string_view falsename()
            {
                return {"false"};
            }
            static constexpr char decimal_point() noexcept
            {
                return '.';
            }
            static constexpr char thousands_separator() noexcept
            {
                return ',';
            }
        };
        template <>
        struct locale_defaults<wchar_t> {
            static constexpr wstring_view truename()
            {
                return {L"true"};
            }
            static constexpr wstring_view falsename()
            {
                return {L"false"};
            }
            static constexpr wchar_t decimal_point() noexcept
            {
                return L'.';
            }
            static constexpr wchar_t thousands_separator() noexcept
            {
                return L',';
            }
        };
    }  // namespace detail

    SCN_CLANG_PUSH_IGNORE_UNDEFINED_TEMPLATE

    SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")

    template <typename CharT>
    class basic_default_locale_ref {
    public:
        using char_type = CharT;
        using string_type = std::basic_string<char_type>;
        using string_view_type = basic_string_view<char_type>;
        using defaults = detail::locale_defaults<char_type>;

        constexpr basic_default_locale_ref() = default;

        constexpr bool is_space(char_type ch) const
        {
            return detail::is_space(ch);
        }
        constexpr bool is_digit(char_type ch) const
        {
            return detail::is_digit(ch);
        }

        constexpr char_type decimal_point() const
        {
            return defaults::decimal_point();
        }
        constexpr char_type thousands_separator() const
        {
            return defaults::thousands_separator();
        }

        constexpr string_view_type truename() const
        {
            return defaults::truename();
        }
        constexpr string_view_type falsename() const
        {
            return defaults::falsename();
        }

        CharT widen(char ch) const
        {
            return detail::default_widen<CharT>::widen(ch);
        }
        char narrow(CharT ch, char def) const
        {
            return detail::default_narrow<CharT>::narrow(ch, def);
        }

        template <typename T>
        expected<std::ptrdiff_t> read_num(T&, const string_type&)
        {
            return error(error::invalid_operation,
                         "No read_num with basic_default_locale_ref");
        }
    };

    template <typename CharT>
    class basic_locale_ref {
    public:
        using char_type = CharT;
        using string_type = std::basic_string<char_type>;
        using string_view_type = basic_string_view<char_type>;

        basic_locale_ref() = default;
        basic_locale_ref(std::nullptr_t) : basic_locale_ref() {}
        explicit basic_locale_ref(const void* loc);

        constexpr const void* get_ptr() const
        {
            return m_locale;
        }

        bool is_space(char_type ch) const
        {
            if (SCN_LIKELY(is_default())) {
                return detail::is_space(ch);
            }
            return _is_space(ch);
        }
        bool is_digit(char_type ch) const
        {
            if (SCN_LIKELY(is_default())) {
                return detail::is_digit(ch);
            }
            return _is_digit(ch);
        }

        constexpr char_type decimal_point() const
        {
            return m_decimal_point;
        }
        constexpr char_type thousands_separator() const
        {
            return m_thousands_separator;
        }

        constexpr string_view_type truename() const
        {
            return string_view_type(m_truename.data(), m_truename.size());
        }
        constexpr string_view_type falsename() const
        {
            return string_view_type(m_falsename.data(), m_falsename.size());
        }

        CharT widen(char ch) const
        {
            if (SCN_LIKELY(is_default())) {
                return detail::default_widen<CharT>::widen(ch);
            }
            return _widen(ch);
        }
        char narrow(CharT ch, char def) const
        {
            if (SCN_LIKELY(is_default())) {
                return detail::default_narrow<CharT>::narrow(ch, def);
            }
            return _narrow(ch, def);
        }

        template <typename T>
        expected<std::ptrdiff_t> read_num(T& val, const string_type& buf);

        constexpr bool is_default() const noexcept
        {
            return m_locale == nullptr;
        }

    private:
        bool _is_space(char_type) const;
        bool _is_digit(char_type) const;
        CharT _widen(char ch) const;
        char _narrow(char_type ch, char def) const;

        using defaults = detail::locale_defaults<char_type>;

        const void* m_locale{nullptr};
        detail::unique_ptr<detail::truename_falsename_storage<char_type>>
            m_truefalse_storage{nullptr};
        string_view_type m_truename{defaults::truename()};
        string_view_type m_falsename{defaults::falsename()};
        char_type m_decimal_point{defaults::decimal_point()};
        char_type m_thousands_separator{defaults::thousands_separator()};
    };

    SCN_CLANG_POP
    SCN_CLANG_POP_IGNORE_UNDEFINED_TEMPLATE
    SCN_END_NAMESPACE
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_LOCALE_CPP)
#include "locale.cpp"
#endif

#endif  // SCN_DETAIL_LOCALE_H
