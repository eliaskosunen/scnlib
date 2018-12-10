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

#include <scn/expected-lite/expected.h>
#include <scn/scn/core.h>
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

    template <>
    basic_locale_ref<char>::basic_locale_ref()
        : m_locale(nullptr), m_truename("true"), m_falsename("false")
    {
    }
    template <>
    basic_locale_ref<wchar_t>::basic_locale_ref()
        : m_locale(nullptr), m_truename(L"true"), m_falsename(L"false")
    {
    }
    template <typename CharT>
    basic_locale_ref<CharT>::basic_locale_ref(const void* loc)
        : m_locale(loc),
          m_truename(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .truename()),
          m_falsename(
              std::use_facet<std::numpunct<CharT>>(detail::get_locale(*this))
                  .falsename())
    {
    }

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
        return string_view_type(m_truename.data(), m_truename.size());
    }
    template <typename CharT>
    typename basic_locale_ref<CharT>::string_view_type
    basic_locale_ref<CharT>::falsename() const
    {
        return string_view_type(m_falsename.data(), m_falsename.size());
    }

    template class basic_locale_ref<char>;
    template class basic_locale_ref<wchar_t>;
}  // namespace scn
