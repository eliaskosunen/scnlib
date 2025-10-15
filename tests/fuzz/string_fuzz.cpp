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
void do_basic_run_for_source(Source&& source,
                             std::basic_string_view<CharT> format_string)
{
    do_basic_run_for_type<CharT, std::basic_string<CharT>>(source,
                                                           format_string);

    if constexpr (scn::ranges::contiguous_range<Source>) {
        do_basic_run_for_type<CharT, std::basic_string_view<CharT>>(
            source, format_string);
    }
}

namespace {
void run(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return;
    }

    const auto inputs = make_input_views(data, size);

    const auto f = format_strings_type<char>{"{}",     "{:L}",   "{:s}",
                                             "{:64c}", "{:64U}", "{:[A-Za-z]}"};
    do_basic_run(inputs.narrow, f);

    const auto wf = format_strings_type<wchar_t>{
        L"{}", L"{:L}", L"{:s}", L"{:64c}", L"{:64U}", L"{:[A-Za-z]}"};
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
