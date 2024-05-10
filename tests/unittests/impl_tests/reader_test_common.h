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

#include "../test_common.h"

#include <scn/impl.h>

template <bool Localized,
          typename CharT,
          typename ValueT,
          template <class>
          class Reader>
class reader_wrapper {
public:
    using char_type = CharT;
    using value_type = ValueT;
    using reader_type = Reader<char_type>;

    static constexpr bool is_localized = Localized;

    constexpr reader_wrapper() = default;

    auto read_default(std::basic_string_view<CharT> source, ValueT& value)
    {
        return m_reader.read_default(source, value, {});
    }

    auto read_specs(std::basic_string_view<CharT> source,
                    const scn::detail::format_specs& specs,
                    ValueT& value)
    {
        auto specs_copy = specs;
        specs_copy.localized = is_localized;

        return m_reader.read_specs(source, specs_copy, value, {});
    }

    auto read_specs_with_locale(std::basic_string_view<CharT> source,
                                const scn::detail::format_specs& specs,
                                ValueT& value,
                                scn::detail::locale_ref loc)
    {
        auto specs_copy = specs;
        specs_copy.localized = is_localized;

        return m_reader.read_specs(source, specs_copy, value, loc);
    }

private:
    reader_type m_reader{};
};
