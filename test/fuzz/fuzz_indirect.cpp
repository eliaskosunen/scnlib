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

#include "fuzz.h"

template <typename Source>
void run(Source data)
{
    using char_type = typename Source::value_type;

    auto& source_indirect = populate_indirect(data);
    basic_run_for_source<char_type>(source_indirect);
    reset_indirect(SCN_MOVE(source_indirect));
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return 0;
    }

    scn::string_view sv;
    scn::wstring_view wsv1, wsv2;
    populate_views(data, size, sv, wsv1, wsv2);

    run(sv);
    run(wsv1);
    run(wsv2);

    return 0;
}
