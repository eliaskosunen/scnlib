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

#define SCN_LOCALE_CPP

#include <scn/scn/locale.h>

#include <locale>

namespace scn {
    namespace detail {
        template <typename CharT>
        std::locale get_locale(basic_locale_ref<CharT> ref)
        {
            if (ref.get_ptr()) {
                return *static_cast<const std::locale*>(ref.get_ptr());
            }
            return std::locale();
        }
    }  // namespace detail

    template <typename CharT>
    bool basic_locale_ref<CharT>::is_space(CharT ch) const
    {
        return std::isspace(ch, detail::get_locale(*this));
    }
    template <typename CharT>
    bool basic_locale_ref<CharT>::is_digit(CharT ch) const
    {
        return std::isdigit(ch, detail::get_locale(*this));
    }
    template <typename CharT>
    CharT basic_locale_ref<CharT>::decimal_point() const
    {
        return std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
            .decimal_point();
    }
    template <typename CharT>
    CharT basic_locale_ref<CharT>::thousands_separator() const
    {
        return std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
            .thousands_sep();
    }
    template <typename CharT>
    typename basic_locale_ref<CharT>::string_view_type
    basic_locale_ref<CharT>::truename() const
    {
        static auto str =
            std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                .truename();
        return string_view_type(str.data(), str.size());
    }
    template <typename CharT>
    typename basic_locale_ref<CharT>::string_view_type
    basic_locale_ref<CharT>::falsename() const
    {
        static auto str =
            std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                .falsename();
        return string_view_type(str.data(), str.size());
    }

    template class basic_locale_ref<char>;
    template class basic_locale_ref<wchar_t>;
}  // namespace scn