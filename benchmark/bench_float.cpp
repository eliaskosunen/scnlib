// Copyright 2017-2018 Elias Kosunen
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

template <typename Float>
static void scanfloat_scn(benchmark::State& state)
{
    auto data = generate_float_data<Float>(static_cast<size_t>(state.range(0)));
    auto stream = scn::make_stream(data);
    Float f{};
    for (auto _ : state) {
        auto e = scn::scan(stream, "{}", f);

        benchmark::DoNotOptimize(f);
        if (!e) {
            if (e == scn::error::end_of_stream) {
                state.PauseTiming();
                data = generate_float_data<Float>(
                    static_cast<size_t>(state.range(0)));
                stream = scn::make_stream(data);
                state.ResumeTiming();
            }
            else {
                state.SkipWithError("Benchmark errored");
                break;
            }
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
// BENCHMARK_TEMPLATE(scanfloat_scn, float)->Arg(2 << 15);
// BENCHMARK_TEMPLATE(scanfloat_scn, double)->Arg(2 << 15);

template <typename Float>
static void scanfloat_sstream(benchmark::State& state)
{
    auto data = generate_float_data<Float>(static_cast<size_t>(state.range(0)));
    auto stream = std::istringstream(data);
    stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    Float f{};
    for (auto _ : state) {
        stream >> f;

        benchmark::DoNotOptimize(f);
        if (stream.eof()) {
            state.PauseTiming();
            data =
                generate_float_data<Float>(static_cast<size_t>(state.range(0)));
            stream = std::istringstream(data);
            state.ResumeTiming();
            continue;
        }
        if (stream.fail()) {
            state.SkipWithError("Benchmark errored");
            break;
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations() * sizeof(Float)));
}
// BENCHMARK_TEMPLATE(scanfloat_sstream, float)->Arg(2 << 15);
// BENCHMARK_TEMPLATE(scanfloat_sstream, double)->Arg(2 << 15);

#if SCN_CLANG
#pragma clang diagnostic pop
#endif
