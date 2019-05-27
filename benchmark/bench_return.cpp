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

#include <scn/tuple_return.h>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wunused-template")
SCN_CLANG_IGNORE("-Wunused-function")
SCN_CLANG_IGNORE("-Wexit-time-destructors")

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wunused-function")

static void return_ref(benchmark::State& state)
{
    auto data = generate_data<char>(4096);
    auto stream = scn::make_stream(data);

    for (auto _ : state) {
        char c{};
        auto e = scn::scan(stream, "{}", c);

        if (!e) {
            if (e.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<char>(4096);
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(return_ref);

static void return_tuple_tie(benchmark::State& state)
{
    auto data = generate_data<char>(4096);
    auto stream = scn::make_stream(data);

    for (auto _ : state) {
        scn::result<int> r{0};
        char c{};
        std::tie(r, c) = scn::scan<char>(stream, "{}");

        if (!r) {
            if (r.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<char>(4096);
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(return_tuple_tie);

#if defined(__cpp_structured_bindings) && __cpp_structured_bindings >= 201606
static void return_tuple(benchmark::State& state)
{
    auto data = generate_data<char>(4096);
    auto stream = scn::make_stream(data);

    for (auto _ : state) {
        auto [r, c] = scn::scan<char>(stream, "{}");

        if (!r) {
            if (r.error() == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<char>(4096);
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(return_tuple);
#endif  // structured bindings
