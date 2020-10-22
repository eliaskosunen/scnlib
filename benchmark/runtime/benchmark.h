#ifndef SCN_BENCHMARK_H
#define SCN_BENCHMARK_H

#include <scn/scn.h>

SCN_CLANG_PUSH
    SCN_CLANG_IGNORE("-Wpadded")
SCN_CLANG_IGNORE("-Wweak-vtables")
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wused-but-marked-unused")
SCN_CLANG_IGNORE("-Wexit-time-destructors")

SCN_GCC_PUSH
    SCN_GCC_IGNORE("-Wswitch-default")
SCN_GCC_IGNORE("-Wredundant-decls")

#include <benchmark/benchmark.h>

#include <random>

inline std::mt19937_64& get_rng() {
    static std::random_device rd;
    static std::seed_seq seed{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    static std::mt19937_64 rng(seed);
    return rng;
}

SCN_GCC_POP
SCN_CLANG_POP

#endif  // SCN_BENCHMARK_H
