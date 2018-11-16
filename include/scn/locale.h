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
    struct basic_locale {
        basic_string_view<CharT> space;
        basic_string_view<CharT> thousand_sep;
        basic_string_view<CharT> decimal_sep;
        basic_string_view<CharT> true_str;
        basic_string_view<CharT> false_str;
    };

    template <typename CharT>
    basic_locale<CharT> classic_locale();
    template <>
    inline basic_locale<char> classic_locale()
    {
        static basic_locale<char> locale{" \r\n\t\v", " ,", ".", "true",
                                         "false"};
        return locale;
    }
    template <>
    inline basic_locale<wchar_t> classic_locale()
    {
        static basic_locale<wchar_t> locale{L" \r\n\t\v", L" ,", L".", L"true",
                                            L"false"};
        return locale;
    }
}  // namespace scn

#endif  // SCN_LOCALE_H