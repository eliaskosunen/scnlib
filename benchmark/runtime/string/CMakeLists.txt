scn_make_runtime_benchmark(scn_string_bench string_bench.cpp)
target_link_libraries(scn_string_bench PRIVATE scn_internal)

# The benchmark reads its input from the working directory
configure_file(lipsum.txt "${CMAKE_CURRENT_BINARY_DIR}/lipsum.txt" COPYONLY)
configure_file(unicode.txt "${CMAKE_CURRENT_BINARY_DIR}/unicode.txt" COPYONLY)
