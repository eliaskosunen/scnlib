scn_make_runtime_benchmark(scn_string_bench string_bench.cpp)
target_link_libraries(scn_string_bench PRIVATE scn_internal)

add_custom_target(scn_string_bench_prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/lipsum.txt"
        "${CMAKE_CURRENT_LIST_DIR}/unicode.txt"
        "${CMAKE_BINARY_DIR}/benchmark/runtime/string"
        COMMENT "Copying string benchmark input files")
