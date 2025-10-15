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
    do_basic_run_for_type<CharT, float>(source, format_string);
    do_basic_run_for_type<CharT, double>(source, format_string);
    do_basic_run_for_type<CharT, long double>(source, format_string);

#if SCN_HAS_STD_F16
    do_basic_run_for_type<CharT, std::float16_t>(source, format_string);
#endif
#if SCN_HAS_STD_F32
    do_basic_run_for_type<CharT, std::float32_t>(source, format_string);
#endif
#if SCN_HAS_STD_F64
    do_basic_run_for_type<CharT, std::float64_t>(source, format_string);
#endif
#if SCN_HAS_STD_F128
    do_basic_run_for_type<CharT, std::float128_t>(source, format_string);
#endif
#if SCN_HAS_STD_BF16
    do_basic_run_for_type<CharT, std::bfloat16_t>(source, format_string);
#endif
}

namespace {
void run(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes || size == 0) {
        return;
    }

    const auto inputs = make_input_views(data, size);

    const auto f =
        format_strings_type<char>{"{}", "{:a}", "{:e}", "{:f}", "{:g}", "{:L}"};
    do_basic_run(inputs.narrow, f);

    const auto wf = format_strings_type<wchar_t>{L"{}",   L"{:a}", L"{:e}",
                                                 L"{:f}", L"{:g}", L"{:L}"};
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
