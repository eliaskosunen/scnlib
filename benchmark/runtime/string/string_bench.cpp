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

#include <scn/xchar.h>

#include "benchmark_common.h"
#include "string_bench.h"

template <typename CharT>
constexpr auto bench_format_string()
{
    if constexpr (sizeof(CharT) == 1) {
        return scn::runtime("{}");
    }
    else {
        return scn::runtime(L"{}");
    }
}

template <typename SourceCharT, typename DestStringT, typename Tag>
static void bench_string_scn(benchmark::State& state)
{
    auto input = get_benchmark_input<SourceCharT, Tag>();
    auto subr = scn::ranges::subrange{input};
    for (auto _ : state) {
        if (auto result = scn::scan<DestStringT>(
                subr, bench_format_string<SourceCharT>())) {
            benchmark::DoNotOptimize(result->value());
            subr = result->range();
        }
        else if (result.error() == scn::scan_error::end_of_range) {
            subr = scn::ranges::subrange{input};
        }
        else {
            state.SkipWithError("Failed scan");
            break;
        }
    }
}

BENCHMARK(bench_string_scn<char, std::string_view, lipsum_tag>);
BENCHMARK(bench_string_scn<char, std::string_view, unicode_tag>);
BENCHMARK(bench_string_scn<char, std::string, lipsum_tag>);
BENCHMARK(bench_string_scn<char, std::string, unicode_tag>);
BENCHMARK(bench_string_scn<char, std::wstring, lipsum_tag>);
BENCHMARK(bench_string_scn<char, std::wstring, unicode_tag>);
BENCHMARK(bench_string_scn<wchar_t, std::string, lipsum_tag>);
BENCHMARK(bench_string_scn<wchar_t, std::string, unicode_tag>);
BENCHMARK(bench_string_scn<wchar_t, std::wstring, lipsum_tag>);
BENCHMARK(bench_string_scn<wchar_t, std::wstring, unicode_tag>);
BENCHMARK(bench_string_scn<wchar_t, std::wstring_view, lipsum_tag>);
BENCHMARK(bench_string_scn<wchar_t, std::wstring_view, unicode_tag>);

template <typename CharT, typename Tag>
static void bench_string_sstream(benchmark::State& state)
{
    auto input = get_benchmark_input<CharT, Tag>();
    std::basic_istringstream<CharT> iss{input};
    for (auto _ : state) {
        std::basic_string<CharT> val{};
        if (!(iss >> val)) {
            if (iss.eof()) {
                iss = std::basic_istringstream<CharT>{input};
                continue;
            }
            state.SkipWithError("Failed scan");
            break;
        }
        benchmark::DoNotOptimize(SCN_MOVE(val));
    }
}

BENCHMARK(bench_string_sstream<char, lipsum_tag>);
BENCHMARK(bench_string_sstream<char, unicode_tag>);
BENCHMARK(bench_string_sstream<wchar_t, lipsum_tag>);
BENCHMARK(bench_string_sstream<wchar_t, unicode_tag>);

BENCHMARK_MAIN();
