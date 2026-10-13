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

#include <array>

namespace {
constexpr size_t max_input_bytes = 1024;

template <typename T, typename Source>
void scan_all(Source& source)
{
    constexpr std::array<const char*, 5> format_strings{"{}", "{:c}", "{:.2}",
                                                        "{:>4}", "{:>4c}"};
    for (const auto* format_string : format_strings) {
        auto result = scn::scan<T>(source, scn::runtime_format(format_string));
        (void)result;
    }
}

template <typename Source>
void scan_all_types(Source&& source)
{
    scan_all<char32_t>(source);
    scan_all<wchar_t>(source);
    scan_all<std::wstring>(source);
}

void run(const uint8_t* data, size_t size)
{
    if (size > max_input_bytes) {
        return;
    }

    const auto input =
        std::string_view{reinterpret_cast<const char*>(data), size};
    scan_all_types(input);
    scan_all_types(std::deque<char>(input.begin(), input.end()));
    scan_all_types(scn::ranges::views::to_input(input));
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    run(data, size);
    return 0;
}
