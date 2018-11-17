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

#ifndef SCN_LOCALE_H
#define SCN_LOCALE_H

#include "string_view.h"

namespace scn {
    template <typename CharT>
    class locale_ref {
    public:
        using char_type = CharT;
        using string_view_type = basic_string_view<char_type>;

        locale_ref() = default;
        locale_ref(const void* loc) : m_locale(loc) {}

        const void* get_ptr() const {
            return m_locale;
        }

        bool is_space(char_type) const;

        char_type decimal_point() const;
        char_type thousands_separator() const;

        string_view_type truename() const;
        string_view_type falsename() const;

    private:
        const void* m_locale{nullptr};

        friend class locale;
    };
}  // namespace scn

#if defined(SCN_HEADER_ONLY) && SCN_HEADER_ONLY && !defined(SCN_LOCALE_CPP)
#include "locale.cpp"
#endif

#endif  // SCN_LOCALE_H