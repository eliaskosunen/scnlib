add_executable(fuzz-test fuzz.cpp)

target_link_libraries(fuzz-test PUBLIC scn)
if (SCN_BUILD_FUZZING)
    target_link_libraries(fuzz-test PUBLIC scn-fuzzer)
endif()
