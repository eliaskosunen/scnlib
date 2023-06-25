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

#include "benchmark_common.h"

#include "int_bench.h"

#if SCN_HAS_INTEGER_CHARCONV
#include <charconv>
#endif

template <typename Int>
static void scan_int_repeated_scn(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        auto result = scn::scan<Int>(s.view(), "{}");

        if (!result) {
            if (result.error() == scn::scan_error::end_of_range) {
                s.reset();
            }
            else {
                state.SkipWithError("Scan error");
                break;
            }
        }
        else {
            s.push(result->value());
            s.it = scn::detail::to_address(result->begin());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scn, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn, unsigned);

template <typename Int>
static void scan_int_repeated_scn_value(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        auto result = scn::scan_value<Int>(s.view());

        if (!result) {
            if (result.error() == scn::scan_error::end_of_range) {
                s.reset();
            }
            else {
                state.SkipWithError("Scan error");
                break;
            }
        }
        else {
            s.push(result->value());
            s.it = scn::detail::to_address(result->begin());
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scn_value, unsigned);

template <typename Int>
static void scan_int_repeated_sstream(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};
    std::istringstream stream{s.source};

    for (auto _ : state) {
        Int i{};
        stream >> i;

        if (stream.eof()) {
            stream = std::istringstream(s.source);
            s.values.clear();
        }
        else if (stream.fail()) {
            state.SkipWithError("Scan error");
            break;
        }
        else {
            s.push(i);
        }
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, int);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_sstream, unsigned);

template <typename Int>
static void scan_int_repeated_scanf(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        Int i{};

        auto ret = sscanf_integral_n(s.it, i);
        if (ret != 1) {
            if (ret == EOF) {
                s.reset();
                continue;
            }

            state.SkipWithError("Scan error");
            break;
        }
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, int);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_scanf, unsigned);

#if SCN_HAS_INTEGER_CHARCONV

template <typename Int>
static void scan_int_repeated_charconv(benchmark::State& state)
{
    repeated_state<Int> s{get_integer_string<Int>()};

    for (auto _ : state) {
        Int i{};

        for (; std::isspace(*s.it) != 0; ++s.it) {}
        if (s.it == s.source_end_addr()) {
            s.reset();
        }

        auto ret = std::from_chars(s.view().begin(), s.view().end(), i);
        if (ret.ec != std::errc{}) {
            state.SkipWithError("Scan error");
            break;
        }
        s.it = ret.ptr;
        s.push(i);
    }
    state.SetBytesProcessed(s.get_bytes_processed(state));
}
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, int);
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, long long);
BENCHMARK_TEMPLATE(scan_int_repeated_charconv, unsigned);

#endif  // SCN_HAS_INTEGER_CHARCONV
