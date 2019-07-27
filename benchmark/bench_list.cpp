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
    for (auto _ : state) {
        state.PauseTiming();
        auto data =
            generate_list_data<int>(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();

        std::vector<int> read;
        read.reserve(static_cast<size_t>(state.range(0)));
        auto stream = scn::make_stream(data);
        while (true) {
            scn::string_view str;
            auto err = scn::getline(stream, str, ',');
            if (!err) {
                if (err == scn::error::end_of_stream) {
                    break;
                }
                state.SkipWithError("");
                break;
            }

            auto strstream = scn::make_stream(str);
            auto tmp = scn::get_value<int>(strstream);
            if (!tmp) {
                state.SkipWithError("");
                break;
            }
            read.push_back(tmp.value());
        }
    }
}
BENCHMARK(scanlist_scn)->Arg(16)->Arg(64)->Arg(256);

static void scanlist_scn_alt(benchmark::State& state)
{
    for (auto _ : state) {
        state.PauseTiming();
        auto data =
            generate_list_data<int>(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();

        std::vector<int> read;
        read.reserve(static_cast<size_t>(state.range(0)));
        auto stream = scn::make_stream(data);
        while (true) {
            auto d = scn::get_value<int>(stream);
            if (!d) {
                state.SkipWithError(d.error().msg());
                break;
            }
            read.push_back(d.value());

            auto ch = scn::get_value<char>(stream);
            if (!ch) {
                if (ch.error() == scn::error::end_of_stream) {
                    break;
                }
                state.SkipWithError(ch.error().msg());
                break;
            }
            if (ch.value() != ',') {
                state.SkipWithError("");
                break;
            }
        }
    }
}
BENCHMARK(scanlist_scn_alt)->Arg(16)->Arg(64)->Arg(256);

static void scanlist_scn_list(benchmark::State& state)
{
    for (auto _ : state) {
        state.PauseTiming();
        auto data =
            generate_list_data<int>(static_cast<size_t>(state.range(0)));
        state.ResumeTiming();

        std::vector<int> read;
        read.reserve(static_cast<size_t>(state.range(0)));
        auto stream = scn::make_stream(data);
        auto ret = scn::scan(stream, "{:,}", scn::temp(scn::make_list(read))());
        if (!ret) {
            state.SkipWithError(ret.error().msg());
            break;
        }
    }
}
BENCHMARK(scanlist_scn_list)->Arg(16)->Arg(64)->Arg(256);

SCN_CLANG_POP
