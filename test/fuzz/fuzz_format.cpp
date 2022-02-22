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

template <typename T, typename Source>
void run_for_type(Source source)
{
    T value{};
    auto result = scn::scan(source, source, value);
    result = scn::scan_localized(global_locale, source, source, value);
}
template <typename Source>
void run(Source source)
{
    using char_type = typename Source::value_type;

    run_for_type<char_type>(source);
    run_for_type<scn::code_point>(source);
    run_for_type<int>(source);
    run_for_type<unsigned>(source);
    run_for_type<double>(source);
    run_for_type<bool>(source);
    run_for_type<std::basic_string<char_type>>(source);
    run_for_type<scn::basic_string_view<char_type>>(source);
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
