message(STATUS "Installing Google benchmark submodule")
execute_process(COMMAND git submodule update --init -- google-benchmark
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "")
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE INTERNAL "")
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE INTERNAL "")
add_subdirectory(google-benchmark EXCLUDE_FROM_ALL)

add_executable(bench
    bench.cpp benchmark.h
    bench_int.cpp bench_float.cpp bench_word.cpp bench_char.cpp bench_buffer.cpp)
target_link_libraries(bench scn benchmark)
set_private_flags(tests)
