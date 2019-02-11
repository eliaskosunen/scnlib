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

#if SCN_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wunused-template"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

static void scanbuffer_scn(benchmark::State& state)
{
    auto size = static_cast<size_t>(state.range(0));
    auto data = generate_buffer(size * 16);
    auto stream = scn::make_stream(data);
    std::vector<char> buf(size);
    auto span = scn::make_span(buf);

    for (auto _ : state) {
        auto e = scn::scan(stream, "{}", span);

        benchmark::DoNotOptimize(span);
        benchmark::DoNotOptimize(buf);
        if (!e) {
            if (e == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_data<char>(size * 16);
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
}
//BENCHMARK(scanbuffer_scn)->Arg(256)->Arg(1024)->Arg(4096);

static void scanbuffer_sstream(benchmark::State& state)
{
    auto size = static_cast<size_t>(state.range(0));
    auto data = generate_buffer(size * 16);
    auto stream = std::istringstream(data);
    std::vector<char> buf(size);

    for (auto _ : state) {
        stream.read(buf.data(), static_cast<std::streamsize>(size));

        benchmark::DoNotOptimize(buf);
        if (stream.eof()) {
            state.PauseTiming();
            data = generate_data<char>(size * 16);
            stream = std::istringstream(data);
            state.ResumeTiming();
            continue;
        }
        if (stream.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
}
//BENCHMARK(scanbuffer_sstream)->Arg(256)->Arg(1024)->Arg(4096);

#if SCN_CLANG
#pragma clang diagnostic pop
#endif
