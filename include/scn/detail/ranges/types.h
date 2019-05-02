// Copyright 2017-2019 Elias Kosunen
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

#ifndef SCN_DETAIL_RANGES_TYPES_H
#define SCN_DETAIL_RANGES_TYPES_H

#include "stream.h"

#include "../types.h"

namespace scn {
    namespace ranges {
        SCN_BEGIN_NAMESPACE

        template <typename Range,
                  typename Traits,
                  typename Allocator,
                  typename Iterator = SCN_RANGES_NS::iterator_t<Range>,
                  typename CharT = SCN_RANGES_NS::value_type_t<Iterator>>
        expected<Iterator> getline(
            Range& r,
            std::basic_string<CharT, Traits, Allocator>& str,
            CharT until)
        {
            str.clear();

            auto s = detail::make_underlying_stream(r);
            auto res = read_into_if(s, std::back_inserter(str),
                                    predicates::until<CharT>{until}, true);
            if (!res) {
                return res.error();
            }
            str.erase(res.value());
            return {SCN_RANGES_NS::begin(r) +
                    static_cast<std::ptrdiff_t>(s.chars_read())};
        }
        template <typename Range,
                  typename Traits,
                  typename Allocator,
                  typename CharT = SCN_RANGES_NS::value_type_t<
                      SCN_RANGES_NS::iterator_t<Range>>>
        auto getline(Range& r, std::basic_string<CharT, Traits, Allocator>& str)
        {
            return getline(r, str, ::scn::detail::ascii_widen<CharT>('\n'));
        }

        SCN_END_NAMESPACE
    }  // namespace ranges
}  // namespace scn

#endif  // SCN_DETAIL_RANGES_TYPES_H
