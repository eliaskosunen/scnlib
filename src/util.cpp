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

#define SCN_UTIL_CPP

#include <scn/scn/util.h>

namespace scn {
    namespace detail {
        SCN_FUNC bool is_digit(basic_locale_ref<char> loc,
                               char c,
                               int base,
                               bool localized)
        {
            switch (base) {
                case 10:
                    return localized
                               ? loc.is_digit(c)
                               : std::isdigit(static_cast<unsigned char>(c)) !=
                                     0;
                case 16:
                    return std::isxdigit(static_cast<unsigned char>(c)) != 0;
                case 2:
                    return c == '0' || c == '1';
                case 8:
                    return c >= '0' && c <= '7';
                default:
                    assert(false && "Invalid base");
            }
            return false;
        }
        SCN_FUNC bool is_digit(basic_locale_ref<wchar_t> loc,
                               wchar_t c,
                               int base,
                               bool localized)
        {
            switch (base) {
                case 10:
                    return localized ? loc.is_digit(c) : std::iswdigit(c) != 0;
                case 16:
                    return std::iswxdigit(c) != 0;
                case 2:
                    return c == L'0' || c == L'1';
                case 8:
                    return c >= L'0' && c <= L'7';
                default:
                    assert(false && "Invalid base");
            }
            return false;
        }
    }  // namespace detail
}  // namespace scn