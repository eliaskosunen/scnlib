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

#ifndef SCN_DETAIL_TUPLE_RETURN_H
#define SCN_DETAIL_TUPLE_RETURN_H

#include "scan.h"

#include <tuple>

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename Stream, typename... Args>
    std::tuple<result<int>, Args...> scan_return(
        Stream& s,
        basic_string_view<typename Stream::char_type> f)
    {
        std::tuple<Args...> values;
        auto scanfn = [&s, &f](auto&... a) { return scan(s, f, a...); };
        auto ret = std::apply(scanfn, values);
        return std::tuple_cat(std::tuple<result<int>>{std::move(ret)},
                              std::move(values));
    }

    SCN_END_NAMESPACE
}  // namespace scn

#endif
