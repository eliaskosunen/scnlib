add_library(fuzz-base INTERFACE)
target_link_libraries(fuzz-base INTERFACE scn)
if (SCN_BUILD_FUZZING)
    target_link_libraries(fuzz-base INTERFACE scn-fuzzer)
endif ()

function(add_fuzzer name)
    add_executable(fuzz-${name} fuzz_${name}.cpp)
    target_link_libraries(fuzz-${name} PUBLIC fuzz-base)
endfunction()

add_fuzzer(roundtrip) # use int
add_fuzzer(int)
add_fuzzer(float)
add_fuzzer(bool)
add_fuzzer(char)
add_fuzzer(string)
add_fuzzer(getline) # use string
add_fuzzer(format)

add_custom_target(fuzz-prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/run-fuzz.sh"
        "${CMAKE_BINARY_DIR}/test/fuzz"
        COMMENT "Copying fuzz scripts")
