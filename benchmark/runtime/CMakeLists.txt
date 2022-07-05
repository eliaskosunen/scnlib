add_library(scn_benchmark_runtime_common INTERFACE)
target_include_directories(scn_benchmark_runtime_common INTERFACE common)

function(scn_make_runtime_benchmark target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target}
        scn::scn benchmark::benchmark benchmark::benchmark_main scn_benchmark_runtime_common)
endfunction()

add_subdirectory(basic)
add_subdirectory(integer)
add_subdirectory(float)

