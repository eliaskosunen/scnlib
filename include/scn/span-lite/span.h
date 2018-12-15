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

#ifndef SCN_SPAN_LITE_H
#define SCN_SPAN_LITE_H

#include "../scn/config.h"

#if SCN_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif
#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

#define span_FEATURE_MAKE_SPAN_TO_STD 99
#include "nonstd/span.hpp"

#if SCN_CLANG
#pragma clang diagnostic pop
#endif
#if SCN_GCC
#pragma GCC diagnostic pop
#endif

namespace scn {
    using nonstd::make_span;
    using nonstd::span;

    namespace detail {
        template <typename T, typename U>
        inline bool contains(T val, U s)
        {
            using std::begin;
            using std::end;
            return std::find(begin(s), end(s), val) != end(s);
        }
    }  // namespace detail
}  // namespace scn

#endif  // SCN_SPAN_LITE_H
