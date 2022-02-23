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
add_fuzzer(types)
add_fuzzer(getline) # use types
add_fuzzer(format)
