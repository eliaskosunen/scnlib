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

#include <scn/impl/locale.h>

#include <locale>

namespace scn {
    SCN_BEGIN_NAMESPACE

    // locale_ref

    namespace detail {
        template <typename Locale>
        locale_ref::locale_ref(const Locale& loc) : m_locale(&loc)
        {
            static_assert(std::is_same_v<Locale, std::locale>);
        }

        template <typename Locale>
        Locale locale_ref::get() const
        {
            static_assert(std::is_same_v<Locale, std::locale>);
            return m_locale ? *static_cast<const std::locale*>(m_locale)
                            : std::locale{};
        }

        template locale_ref::locale_ref(const std::locale&);
        template auto locale_ref::get() const -> std::locale;
    }  // namespace detail

    SCN_END_NAMESPACE
}  // namespace scn
