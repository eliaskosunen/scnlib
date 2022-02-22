add_library(scn-sanitizers INTERFACE)
set(SCN_SANITIZERS_COMPILE_FLAGS "")
set(SCN_SANITIZERS_LINK_FLAGS "")
if (SCN_USE_ASAN)
    if (CMAKE_VERSION VERSION_LESS 3.13)
        message(FATAL_ERROR "SCN_USE_ASAN requires CMake >= 3.13 (for target_link_options)")
    endif ()
    list(APPEND SCN_SANITIZERS_COMPILE_FLAGS "address")
    list(APPEND SCN_SANITIZERS_LINK_FLAGS "address")
    target_compile_options(scn-sanitizers INTERFACE
            -g -fno-omit-frame-pointer)
endif ()
if (SCN_USE_UBSAN)
    if (CMAKE_VERSION VERSION_LESS 3.13)
        message(FATAL_ERROR "SCN_USE_UBSAN requires CMake >= 3.13 (for target_link_options)")
    endif ()
    list(APPEND SCN_SANITIZERS_COMPILE_FLAGS "undefined,implicit-conversion,integer,nullability")
    list(APPEND SCN_SANITIZERS_LINK_FLAGS "undefined,implicit-conversion,integer,nullability")
    target_compile_options(scn-sanitizers INTERFACE
            -g -fno-omit-frame-pointer)
endif ()
if (SCN_USE_MSAN)
    if (CMAKE_VERSION VERSION_LESS 3.13)
        message(FATAL_ERROR "SCN_USE_MSAN requires CMake >= 3.13 (for target_link_options)")
    endif ()
    list(APPEND SCN_SANITIZERS_COMPILE_FLAGS "memory")
    list(APPEND SCN_SANITIZERS_LINK_FLAGS "memory")
    target_compile_options(scn-sanitizers INTERFACE
            -g -fno-omit-frame-pointer)
endif ()

string(REPLACE ";" "," SCN_SANITIZERS_COMPILE_FLAGS_JOINED "${SCN_SANITIZERS_COMPILE_FLAGS}")
string(REPLACE ";" "," SCN_SANITIZERS_LINK_FLAGS_JOINED "${SCN_SANITIZERS_LINK_FLAGS}")
list(LENGTH SCN_SANITIZERS_COMPILE_FLAGS sanitizers_compiler_list_length)
if (sanitizers_compiler_list_length GREATER 0)
    target_compile_options(scn-sanitizers INTERFACE
            "-fsanitize=${SCN_SANITIZERS_COMPILE_FLAGS_JOINED}")
endif ()
list(LENGTH SCN_SANITIZERS_LINK_FLAGS sanitizers_link_list_length)
if (sanitizers_link_list_length GREATER 0)
    target_link_options(scn-sanitizers INTERFACE
            "-fsanitize=${SCN_SANITIZERS_LINK_FLAGS_JOINED}")
endif ()

if (SCN_BUILD_FUZZING)
    set(SCN_FUZZ_LDFLAGS "" CACHE STRING "LDFLAGS for fuzz targets")


    add_library(scn-fuzzer INTERFACE)
    if (SCN_FUZZ_LDFLAGS)
        target_link_libraries(scn-fuzzer INTERFACE ${SCN_FUZZ_LDFLAGS})
    else ()
        list(APPEND SCN_SANITIZERS_COMPILE_FLAGS "fuzzer")
        list(APPEND SCN_SANITIZERS_LINK_FLAGS "fuzzer")
        string(REPLACE ";" "," SCN_SANITIZERS_COMPILE_FLAGS_JOINED "${SCN_SANITIZERS_COMPILE_FLAGS}")
        string(REPLACE ";" "," SCN_SANITIZERS_LINK_FLAGS_JOINED "${SCN_SANITIZERS_LINK_FLAGS}")

        target_compile_options(scn-fuzzer INTERFACE
                "-fsanitize=${SCN_SANITIZERS_COMPILE_FLAGS_JOINED}")
        target_link_libraries(scn-fuzzer INTERFACE
                "-fsanitize=${SCN_SANITIZERS_LINK_FLAGS_JOINED}")
    endif ()
endif ()
