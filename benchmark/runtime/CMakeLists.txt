add_library(scn_benchmark_runtime_common INTERFACE)
target_include_directories(scn_benchmark_runtime_common INTERFACE common)
target_compile_options(scn_benchmark_runtime_common INTERFACE
        $<$<CXX_COMPILER_ID:GNU>:
        -Wno-unused
        -Wno-redundant-decls>
        $<$<CXX_COMPILER_ID:Clang>:
        -Wno-unused-variable
        -Wno-used-but-marked-unused
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-weak-vtables
        >)

function(scn_make_runtime_benchmark target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE
        scn_internal benchmark::benchmark benchmark::benchmark_main scn_benchmark_runtime_common)
endfunction()

add_subdirectory(basic)
add_subdirectory(integer)
add_subdirectory(float)

