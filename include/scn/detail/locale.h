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

#ifndef SCN_DETAIL_LOCALE_H
#define SCN_DETAIL_LOCALE_H

#include "result.h"
#include "string_view.h"

namespace scn {
#if SCN_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winline"
#endif
#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
    template <typename CharT>
    class basic_locale_ref {
    public:
        using char_type = CharT;
        using string_type = std::basic_string<char_type>;
        using string_view_type = basic_string_view<char_type>;
        using iterator = typename string_view_type::iterator;

        basic_locale_ref();
        explicit basic_locale_ref(const void* loc);

        const void* get_ptr() const
        {
            return m_locale;
        }

        bool is_space(char_type) const;
        bool is_digit(char_type) const;

        char_type decimal_point() const;
        char_type thousands_separator() const;

        string_view_type truename() const;
        string_view_type falsename() const;

        CharT widen(char ch) const;
        char narrow(CharT ch, char def) const;

        template <typename T>
        result<size_t> read_num(T& val, string_type buf);

        bool is_default() const
        {
            return m_locale == nullptr;
        }

    private:
        const void* m_locale{nullptr};
        string_type m_truename;
        string_type m_falsename;
        char_type m_decimal_point;
        char_type m_thousands_separator;
    };

#if SCN_CLANG
#pragma clang diagnostic pop
#endif
#if SCN_GCC
#pragma GCC diagnostic pop
#endif
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_LOCALE_CPP)
#include "locale.cpp"
#endif

#endif  // SCN_DETAIL_LOCALE_H
