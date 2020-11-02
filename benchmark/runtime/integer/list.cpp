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

#include "bench_int.h"

static void scan_int_list_scn(benchmark::State& state)
{
    const auto n = static_cast<size_t>(state.range(0));
    auto data = stringified_integer_list<int>(n);
    std::vector<int> read;
    read.reserve(n);

    for (auto _ : state) {
        auto result = scn::make_result(data);
        while (true) {
            int i{};
            result = scn::scan(result.range(), "{}", i);
            if (!result) {
                if (result.error() != scn::error::end_of_range) {
                    state.SkipWithError("Benchmark errored");
                }
                break;
            }
            read.push_back(i);
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * n * sizeof(int)));
}
BENCHMARK(scan_int_list_scn)->Arg(16)->Arg(64)->Arg(256);

static void scan_int_list_scn_default(benchmark::State& state)
{
    const auto n = static_cast<size_t>(state.range(0));
    auto data = stringified_integer_list<int>(n);
    std::vector<int> read;
    read.reserve(n);

    for (auto _ : state) {
        auto result = scn::make_result(data);
        while (true) {
            int i{};
            result = scn::scan_default(result.range(), i);
            if (!result) {
                if (result.error() != scn::error::end_of_range) {
                    state.SkipWithError("Benchmark errored");
                }
                break;
            }
            read.push_back(i);
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * n * sizeof(int)));
}
BENCHMARK(scan_int_list_scn_default)->Arg(16)->Arg(64)->Arg(256);

static void scan_int_list_scn_value(benchmark::State& state)
{
    const auto n = static_cast<size_t>(state.range(0));
    auto data = stringified_integer_list<int>(n);
    std::vector<int> read;
    read.reserve(n);

    for (auto _ : state) {
        auto result = scn::make_result<scn::expected<int>>(data);
        while (true) {
            result = scn::scan_value<int>(result.range());
            if (!result) {
                if (result.error() != scn::error::end_of_range) {
                    state.SkipWithError("Benchmark errored");
                }
                break;
            }
            read.push_back(result.value());
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * n * sizeof(int)));
}
BENCHMARK(scan_int_list_scn_value)->Arg(16)->Arg(64)->Arg(256);

static void scan_int_list_scn_list(benchmark::State& state)
{
    const auto n = static_cast<size_t>(state.range(0));
    auto data = stringified_integer_list<int>(n);
    std::vector<int> read;
    read.reserve(n);

    for (auto _ : state) {
        auto result = scn::scan_list(data, read);
        if (!result) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * n * sizeof(int)));
}
BENCHMARK(scan_int_list_scn_list)->Arg(16)->Arg(64)->Arg(256);

static void scan_int_list_sstream(benchmark::State& state)
{
    const auto n = static_cast<size_t>(state.range(0));
    auto data = stringified_integer_list<int>(n);
    std::vector<int> read;
    read.reserve(n);

    for (auto _ : state) {
        std::istringstream iss{data};
        int i;
        while (iss >> i) {
            read.push_back(i);
        }
        if (iss.fail() && !iss.eof()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * n * sizeof(int)));
}
BENCHMARK(scan_int_list_sstream)->Arg(16)->Arg(64)->Arg(256);

static void scan_int_list_scanf(benchmark::State& state)
{
    const auto n = static_cast<size_t>(state.range(0));
    auto data = stringified_integer_list<int>(n);
    std::vector<int> read;
    read.reserve(n);

    for (auto _ : state) {
        auto ptr = &data[0];
        while (true) {
            int i;
            auto ret = scanf_integral_n(ptr, i);
            if (ret != 1) {
                if (ret != EOF) {
                    state.SkipWithError("Benchmark errored");
                }
                break;
            }
            read.push_back(i);
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * n * sizeof(int)));
}
BENCHMARK(scan_int_list_scanf)->Arg(16)->Arg(64)->Arg(256);
