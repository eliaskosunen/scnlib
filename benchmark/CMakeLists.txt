set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(google-benchmark EXCLUDE_FROM_ALL)

add_executable(bench bench.cpp)
target_link_libraries(bench scn benchmark)
