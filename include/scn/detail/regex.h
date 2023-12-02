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

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename CharT>
    class basic_regex_match {
    public:
        using char_type = CharT;

        basic_regex_match(std::basic_string_view<CharT> str) : m_str(str) {}

#if SCN_REGEX_SUPPORTS_NAMED_CAPTURES
        basic_regex_match(std::basic_string_view<CharT> str,
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

    template <typename CharT>
    class basic_regex_matches
        : private std::vector<std::optional<basic_regex_match<CharT>>> {
        using base = std::vector<std::optional<basic_regex_match<CharT>>>;

    public:
        using match_type = basic_regex_match<CharT>;
        using typename base::const_iterator;
        using typename base::const_reverse_iterator;
        using typename base::iterator;
        using typename base::pointer;
        using typename base::reference;
        using typename base::reverse_iterator;
        using typename base::size_type;
        using typename base::value_type;

        using base::base;

        using base::emplace;
        using base::emplace_back;
        using base::insert;
        using base::push_back;

        using base::reserve;
        using base::resize;

        using base::at;
        using base::operator[];

        using base::begin;
        using base::end;
        using base::rbegin;
        using base::rend;

        using base::data;
        using base::size;

        using base::swap;
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif
