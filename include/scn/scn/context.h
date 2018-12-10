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

#ifndef SCN_CONTEXT_H
#define SCN_CONTEXT_H

#include "core.h"
#include "locale.h"

namespace scn {
    template <typename Stream>
    class basic_context {
    public:
        using stream_type = Stream;
        using char_type = typename stream_type::char_type;
        using parse_context_type = basic_parse_context<char_type>;
        using locale_type = basic_locale_ref<char_type>;

        template <typename T>
        using value_scanner_type = basic_value_scanner<char_type, T>;

#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
#endif
        basic_context(stream_type& s,
                      basic_string_view<char_type> f,
                      locale_type locale = locale_type())
            : m_stream(std::addressof(s)),
              m_parse_ctx(std::move(f)),
              m_locale(locale)
        {
        }
#if SCN_CLANG
#pragma clang diagnostic pop
#endif

        parse_context_type& parse_context()
        {
            return m_parse_ctx;
        }
        stream_type& stream()
        {
            return *m_stream;
        }
        locale_type locale() const
        {
            return m_locale;
        }

    private:
        stream_type* m_stream;
        parse_context_type m_parse_ctx;
        locale_type m_locale;
    };
}  // namespace scn

#endif  // SCN_CONTEXT_H
