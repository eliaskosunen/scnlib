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

#if SCN_REGEX_BACKEND != SCN_REGEX_BACKEND_STD
#error TODO
#endif

namespace scn {
    SCN_BEGIN_NAMESPACE

    template <typename CharT>
    struct basic_regex_matches {
        std::vector<std::optional<std::basic_string<CharT>>> matches;
    };

    SCN_END_NAMESPACE
}  // namespace scn

#endif
