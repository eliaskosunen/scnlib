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

namespace scn_fuzz {
    template <typename CharT, typename Source>
    void do_basic_run_for_source(Source& source,
                                 format_strings_view<CharT> format_strings)
    {
        do_basic_run_for_type<CharT, bool>(source, format_strings);
    }
}  // namespace scn_fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size > scn_fuzz::globals::max_input_bytes || size == 0) {
        return 0;
    }

    scn::string_view sv;
    scn::wstring_view wsv1, wsv2;
    scn_fuzz::populate_views(data, size, sv, wsv1, wsv2);

    auto f = scn_fuzz::get_format_strings<char>("{}", "{:s}", "{:i}", "{:L}");
    scn_fuzz::do_basic_run(sv, f);

    auto wf =
        scn_fuzz::get_format_strings<wchar_t>(L"{}", L"{:s}", L"{:i}", L"{:L}");
    scn_fuzz::do_basic_run(wsv1, wf);
    scn_fuzz::do_basic_run(wsv2, wf);

    return 0;
}
