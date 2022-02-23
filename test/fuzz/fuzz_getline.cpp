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

template <typename CharT, typename Source>
void run_getline_and_ignore(const Source& source)
{
    auto result = scn::make_result(source);
    std::basic_string<CharT> str;
    while (true) {
        result = scn::getline(result.range(), str);
        if (!result) {
            break;
        }
    }

    if (source.size() < 4) {
        return;
    }
    CharT until = scn_fuzz::unwrap_expected(source[source.size() / 2]);
    scn::ignore_until(source, until);
}

template <typename Source>
void run(Source data)
{
    using char_type = typename Source::value_type;

    auto source_sv = data;
    run_getline_and_ignore<char_type>(source_sv);

    auto& source_str = scn_fuzz::populate_string(source_sv);
    run_getline_and_ignore<char_type>(source_str);

    auto& source_deque = scn_fuzz::populate_deque(source_sv);
    run_getline_and_ignore<char_type>(source_deque);

    auto& source_indirect = scn_fuzz::populate_indirect(source_sv);
    run_getline_and_ignore<char_type>(source_indirect);
    scn_fuzz::reset_indirect(SCN_MOVE(source_indirect));
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > scn_fuzz::globals::max_input_bytes || size == 0) {
        return 0;
    }

    scn::string_view sv;
    scn::wstring_view wsv1, wsv2;
    scn_fuzz::populate_views(data, size, sv, wsv1, wsv2);

    run(sv);
    run(wsv1);
    run(wsv2);

    return 0;
}
