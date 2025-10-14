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

#include <scn/chrono.h>

namespace scn::fuzz {
template <typename CharT, typename Source>
void do_basic_run_for_source(Source&& source,
                             const format_strings_type<CharT>& format_strings)
{
    do_basic_run_for_type<CharT, std::tm>(source, format_strings);
    do_basic_run_for_type<CharT, tm_with_tz>(source, format_strings);
    do_basic_run_for_type<CharT, datetime_components>(source, format_strings);
}

namespace {
void run(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return;
    }

    const auto inputs = make_input_views(data, size);

    const auto f = format_strings_type<char>{
        "{:%T}", "{:%R}", "{:%D}", "{:%F}", "{:%Y-%m-%dT%H:%M:%S%z}",
        "{:%a}", "{:%b}"};
    do_basic_run(inputs.narrow, f);

    const auto wf = format_strings_type<wchar_t>{
        L"{:%T}", L"{:%R}", L"{:%D}", L"{:%F}", L"{:%Y-%m-%dT%H:%M:%S%z}",
        L"{:%a}", L"{:%b}"};
    do_basic_run(inputs.wide_copied, wf);
    do_basic_run(inputs.wide_reinterpreted, wf);
    if (!inputs.wide_transcoded.empty()) {
        do_basic_run(inputs.wide_transcoded, wf);
    }
}
}  // namespace
}  // namespace scn::fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    scn::fuzz::run(data, size);
    return 0;
}
