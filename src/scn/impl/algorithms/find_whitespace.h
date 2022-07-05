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

#include <scn/util/string_view.h>

namespace scn {
    SCN_BEGIN_NAMESPACE

    namespace impl {
        std::string_view::iterator find_classic_space_narrow_fast(
            std::string_view source);

        std::string_view::iterator find_classic_nonspace_narrow_fast(
            std::string_view source);

        std::string_view::iterator find_nondecimal_digit_narrow_fast(
            std::string_view source);
    }  // namespace impl

    SCN_END_NAMESPACE
}  // namespace scn
