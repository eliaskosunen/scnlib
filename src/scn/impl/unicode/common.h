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

#pragma once

#include <scn/detail/unicode.h>
#include <scn/util/meta.h>

#include <cstdint>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wdocumentation")
SCN_CLANG_IGNORE("-Wnewline-eof")
SCN_CLANG_IGNORE("-Wextra-semi")

#include <simdutf.h>

SCN_CLANG_POP

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        enum class encoding : char { utf8, utf16, utf32, other };

        template <typename CharT>
        constexpr encoding get_encoding()
        {
            SCN_GCC_PUSH
            SCN_GCC_IGNORE("-Wswitch-default")
            switch (sizeof(CharT)) {
                case 1:
                    return encoding::utf8;
                case 2:
                    return encoding::utf16;
                case 4:
                    return encoding::utf32;
            }
            return encoding::other;
            SCN_GCC_POP  // -Wswitch-default
        }

        template <typename CharT>
        auto encoding_char_type_impl()
        {
            constexpr auto enc = get_encoding<CharT>();
            if constexpr (enc == encoding::utf8) {
                return char{};
            }
            else if constexpr (enc == encoding::utf16) {
                return char16_t{};
            }
            else if constexpr (enc == encoding::utf32) {
                return char32_t{};
            }
            else {
                static_assert(detail::dependent_false<CharT>::type);
            }
        }
        template <typename CharT>
        using char_type_for_encoding =
            decltype(encoding_char_type_impl<CharT>());

        template <typename CharT>
        auto string_view_to_encoding(std::basic_string_view<CharT> input)
        {
            using char_type = char_type_for_encoding<CharT>;
            return std::basic_string_view<char_type>{
                reinterpret_cast<const char_type*>(input.data()), input.size()};
        }
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
