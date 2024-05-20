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

#include <random>

#include <scn/scan.h>

inline std::mt19937_64& get_rng()
{
    static std::random_device rd;
    static std::seed_seq seed{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    static std::mt19937_64 rng(seed);
    return rng;
}

// Stolen from ascii_ctype.h
inline bool is_classic_ascii_space(char ch)
{
    static constexpr std::array<bool, 256> lookup = {
        {false, false, false, false, false, false, false, false, false, true,
         true,  true,  true,  true,  false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, true,  false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false, false, false,
         false, false, false, false, false, false}};

    return lookup[static_cast<size_t>(static_cast<unsigned char>(ch))];
}

template <typename T>
struct single_state {
    single_state(const std::vector<std::string>& src) : source(src), it(source.begin())
    {
    }

    void reset_if_necessary()
    {
        if (it == source.end()) {
            it = source.begin();
        }
    }

    void push(T i)
    {
        benchmark::DoNotOptimize(i);
    }

    static int64_t get_bytes_processed(benchmark::State& state)
    {
        return state.iterations() * static_cast<int64_t>(sizeof(T));
    }

    const std::vector<std::string>& source;
    typename std::vector<std::string>::const_iterator it;
};

template <typename T>
struct repeated_state {
    repeated_state(const std::string& src) : source(src), it(source.data()) {}

    void reset()
    {
        it = source.data();
    }

    auto source_end_addr() const
    {
        return scn::detail::to_address(source.end());
    }

    void skip_classic_ascii_space()
    {
        for (; is_classic_ascii_space(*it); ++it) {}
        if (it == source_end_addr()) {
            reset();
        }
    }

    auto view() const
    {
        return scn::ranges::subrange{it, source_end_addr()};
    }

    void push(T i)
    {
        benchmark::DoNotOptimize(i);
    }

    static int64_t get_bytes_processed(benchmark::State& state)
    {
        return state.iterations() * static_cast<int64_t>(sizeof(T));
    }

    const std::string& source;
    const char* it;
};
