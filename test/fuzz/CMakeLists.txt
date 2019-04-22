add_executable(fuzz-test fuzz.cpp)
target_link_libraries(fuzz-test PUBLIC scn scn-ranges scn-fuzzer)
