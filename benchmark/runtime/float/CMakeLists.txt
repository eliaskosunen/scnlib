add_executable(bench-float
        single.cpp repeated.cpp list.cpp bench_float.h main.cpp)
target_link_libraries(bench-float PRIVATE scn benchmark)
set_private_flags(bench-float)
target_compile_features(bench-float PRIVATE cxx_std_17)
target_compile_options(bench-float PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:
        -Wno-global-constructors
        -Wno-exit-time-destructors>)
