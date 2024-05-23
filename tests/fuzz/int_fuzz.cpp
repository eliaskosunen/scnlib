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

namespace scn::fuzz {
template <typename CharT, typename Source>
void do_basic_run_for_source(Source& source,
                             const format_strings_type<CharT>& format_strings)
{
    do_basic_run_for_type<CharT, signed char>(source, format_strings);
    do_basic_run_for_type<CharT, unsigned char>(source, format_strings);
    do_basic_run_for_type<CharT, int>(source, format_strings);
    do_basic_run_for_type<CharT, unsigned>(source, format_strings);
    do_basic_run_for_type<CharT, long long>(source, format_strings);
    do_basic_run_for_type<CharT, unsigned long long>(source, format_strings);
}

namespace {
void run(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return;
    }

    auto [sv, wsv_reinterpret, wsv_transcode] = make_input_views(data, size);

    const auto& f =
        get_format_strings<char>("{}", "{:i}", "{:d}", "{:L}", "{:x}", "{:b}",
                                 "{:o}", "{:B36}", "{:'}", "{:'L}");
    do_basic_run(sv, f);

    const auto& wf = get_format_strings<wchar_t>(
        L"{}", L"{:i}", L"{:d}", L"{:L}", L"{:x}", L"{:b}", L"{:o}", L"{:B36}",
        L"{:'}", L"{:'L}");
    do_basic_run(wsv_reinterpret, wf);
    if (!wsv_transcode.empty()) {
        do_basic_run(wsv_transcode, wf);
    }
}
}  // namespace
}  // namespace scn::fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    scn::fuzz::run(data, size);
    return 0;
}
