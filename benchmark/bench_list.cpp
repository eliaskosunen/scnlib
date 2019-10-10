// Copyright 2017-2019 Elias Kosunen
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

#include "benchmark.h"

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wunused-template")
SCN_CLANG_IGNORE("-Wexit-time-destructors")

template <typename Int>
std::string generate_list_data(size_t n)
{
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<Int> dist(0, std::numeric_limits<Int>::max());

    std::ostringstream oss;
    for (size_t i = 0; i < n - 1; ++i) {
        oss << dist(rng) << ", ";
    }
    oss << dist(rng);
    return oss.str();
}

static void scanlist_scn(benchmark::State& state)
{
    auto data = generate_list_data<int>(static_cast<size_t>(state.range(0)));
    auto range = scn::make_view(data);
    std::vector<int> read;
    read.reserve(static_cast<size_t>(state.range(0)));
    for (auto _ : state) {
        scn::string_view str;
        auto err = scn::getline(range, str, ',');
        if (!err) {
            if (err.error() == scn::error::end_of_range) {
                state.PauseTiming();
                data = generate_list_data<int>(
                    static_cast<size_t>(state.range(0)));
                range = scn::make_view(data);
                read.clear();
                read.reserve(static_cast<size_t>(state.range(0)));
                state.ResumeTiming();
                continue;
            }
            state.SkipWithError("Benchmark errored");
            break;
        }

        int value{};
        auto tmp = scn::scan(scn::make_view(str), scn::default_tag, value);
        if (!tmp) {
            state.SkipWithError("");
            break;
        }
        read.push_back(value);
    }
}
BENCHMARK(scanlist_scn)->Arg(16)->Arg(64)->Arg(256);

static void scanlist_scn_alt(benchmark::State& state)
{
    auto data = generate_list_data<int>(static_cast<size_t>(state.range(0)));
    auto range = scn::make_view(data);
    std::vector<int> read;
    read.reserve(static_cast<size_t>(state.range(0)));
    for (auto _ : state) {
        int value{};
        auto d = scn::scan(range, scn::default_tag, value);
        if (!d) {
            state.SkipWithError(d.error().msg());
            break;
        }
        read.push_back(value);

        char ch{};
        d = scn::scan(range, scn::default_tag, ch);
        if (!d) {
            if (d.error() == scn::error::end_of_range) {
                state.PauseTiming();
                data = generate_list_data<int>(
                    static_cast<size_t>(state.range(0)));
                range = scn::make_view(data);
                read.clear();
                read.reserve(static_cast<size_t>(state.range(0)));
                state.ResumeTiming();
                continue;
            }
            state.SkipWithError(d.error().msg());
            break;
        }
        if (ch != ',') {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
}
BENCHMARK(scanlist_scn_alt)->Arg(16)->Arg(64)->Arg(256);

static void scanlist_scn_list(benchmark::State& state)
{
    auto data = generate_list_data<int>(static_cast<size_t>(state.range(0)));
    auto range = scn::make_view(data);
    std::vector<int> read;
    read.reserve(static_cast<size_t>(state.range(0)));
    for (auto _ : state) {
        auto ret = scn::scan_list(range, read, ',');
        if (!ret) {
            if (ret.error() == scn::error::end_of_range) {
                state.PauseTiming();
                data = generate_list_data<int>(
                    static_cast<size_t>(state.range(0)));
                range = scn::make_view(data);
                read.clear();
                read.reserve(static_cast<size_t>(state.range(0)));
                state.ResumeTiming();
                continue;
            }
            state.SkipWithError(ret.error().msg());
            break;
        }
    }
}
BENCHMARK(scanlist_scn_list)->Arg(16)->Arg(64)->Arg(256);

SCN_CLANG_POP
