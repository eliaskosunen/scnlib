add_executable(bench-int
        single.cpp repeated.cpp list.cpp bench_int.h main.cpp)
target_link_libraries(bench-int PRIVATE scn benchmark)
set_private_flags(bench-int)
target_compile_features(bench-int PRIVATE cxx_std_17)
target_compile_options(bench-int PRIVATE
        $<$<CXX_COMPILER_ID:Clang>:
        -Wno-global-constructors
        -Wno-exit-time-destructors>)
