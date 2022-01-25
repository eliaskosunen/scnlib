add_executable(bench-word
        repeated.cpp bench_word.h main.cpp)
target_link_libraries(bench-word PRIVATE scn benchmark)
set_private_flags(bench-word)
target_compile_features(bench-word PRIVATE cxx_std_17)
target_compile_options(bench-word PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:
        -Wno-global-constructors
        -Wno-exit-time-destructors>)
