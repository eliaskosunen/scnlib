if (NOT SCN_FUZZING)
    return()
endif ()

add_library(scn_fuzz_base INTERFACE)
target_link_libraries(scn_fuzz_base INTERFACE scn scn_fuzzer scn_internal)

function(add_fuzzer name)
    add_executable(scn_fuzz_${name}
            fuzz.h
            ${name}_fuzz.cpp)
    target_link_libraries(scn_fuzz_${name} PRIVATE scn scn_internal)
    if (SCN_FUZZING_LDFLAGS)
        target_link_options(scn_fuzz_${name} PRIVATE ${SCN_FUZZING_LDFLAGS})
    else ()
        target_link_libraries(scn_fuzz_${name} PRIVATE scn_fuzzer)
    endif ()
endfunction()

add_fuzzer(int)
add_fuzzer(float)
add_fuzzer(string)
add_fuzzer(format)
add_fuzzer(chrono)
add_fuzzer(string_impl)

add_custom_target(scn_fuzz_prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/run-fuzz.sh"
        "${CMAKE_BINARY_DIR}/tests/fuzz"
        COMMENT "Copying fuzzer scripts")
