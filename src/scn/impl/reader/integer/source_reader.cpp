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

#include <scn/impl/reader/integer/source_reader.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        template <typename CharT>
        int_localized_source_reader<CharT>::int_localized_source_reader(
            detail::locale_ref loc,
            int base,
            bool allow_minus_sign)
            : m_locale{loc}
        {
            auto f = localized_number_formatting_options<char_type>{loc};
            m_thsep = f.thousands_sep;

            switch (base) {
                case 8:
                    m_digits = "-+01234567";
                    break;
                case 10:
                    m_digits = "-+0123456789";
                    break;
                default:
                    m_digits = "-+0123456789abcdefxABCDEFX";
                    break;
            }
            if (!allow_minus_sign) {
                m_digits.remove_prefix(1);
            }
        }

        template int_localized_source_reader<char>::int_localized_source_reader(
            detail::locale_ref,
            int,
            bool);
        template int_localized_source_reader<
            wchar_t>::int_localized_source_reader(detail::locale_ref,
                                                  int,
                                                  bool);
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
