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

#if !SCN_DISABLE_REGEX

#include <scn/detail/scanner.h>

#include <optional>
#include <vector>

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_STD
#define SCN_REGEX_SUPPORTS_NAMED_CAPTURES 0
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_BOOST
#define SCN_REGEX_SUPPORTS_NAMED_CAPTURES 0
#elif SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
#define SCN_REGEX_SUPPORTS_NAMED_CAPTURES 1
#else
#error TODO
#endif

#if SCN_REGEX_BACKEND == SCN_REGEX_BACKEND_RE2
#define SCN_REGEX_SUPPORTS_WIDE_STRINGS 0
#else
#define SCN_REGEX_SUPPORTS_WIDE_STRINGS 1
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename CharT>
    struct basic_regex_matches {
        class match {
        public:
            using char_type = CharT;

            match(std::basic_string_view<CharT> str) : m_str(str) {}

#if SCN_REGEX_SUPPORTS_NAMED_CAPTURES
            match(std::basic_string_view<CharT> str,
                  std::basic_string<CharT> name)
                : m_str(str), m_name(name)
            {
            }
#endif

            std::basic_string_view<CharT> get() const
            {
                return m_str;
            }

            auto operator*() const
            {
                return m_str;
            }
            auto operator->() const
            {
                return &m_str;
            }

#if SCN_REGEX_SUPPORTS_NAMED_CAPTURES
            std::optional<std::basic_string_view<CharT>> name() const
            {
                return m_name;
            }
#endif

        private:
            std::basic_string_view<CharT> m_str;

#if SCN_REGEX_SUPPORTS_NAMED_CAPTURES
            std::optional<std::basic_string<CharT>> m_name;
#endif
        };

        std::vector<std::optional<match>> matches;
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif
