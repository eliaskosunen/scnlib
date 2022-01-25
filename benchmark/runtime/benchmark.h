#ifndef SCN_BENCHMARK_H
#define SCN_BENCHMARK_H

#include <scn/scn.h>

SCN_CLANG_PUSH
SCN_CLANG_IGNORE("-Wpadded")
SCN_CLANG_IGNORE("-Wweak-vtables")
SCN_CLANG_IGNORE("-Wglobal-constructors")
SCN_CLANG_IGNORE("-Wused-but-marked-unused")
SCN_CLANG_IGNORE("-Wexit-time-destructors")
SCN_CLANG_IGNORE("-Wshift-sign-overflow")
SCN_CLANG_IGNORE("-Wzero-as-null-pointer-constant")

// -Wsuggest-override requires clang >= 11
#if SCN_CLANG >= SCN_COMPILER(11, 0, 0)
SCN_CLANG_IGNORE("-Wsuggest-override")
#endif

SCN_GCC_PUSH
SCN_GCC_IGNORE("-Wswitch-default")
SCN_GCC_IGNORE("-Wredundant-decls")

#include <benchmark/benchmark.h>

#include <random>

inline std::mt19937_64& get_rng()
{
    static std::random_device rd;
    static std::seed_seq seed{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    static std::mt19937_64 rng(seed);
    return rng;
}

SCN_GCC_POP
SCN_CLANG_POP

#endif  // SCN_BENCHMARK_H
