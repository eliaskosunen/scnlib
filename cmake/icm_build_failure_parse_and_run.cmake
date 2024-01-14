# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#

# This file is bundled with icm_build_failure_testing.cmake
# It is expected to be in the same directory.

if(DEFINED CFG)
    # optional cfg
    # that way one can run the generated script from the command line as:
    # $ cmake -P script.cmake
    set(cfgArg "--config;${CFG}")
endif()

execute_process(
        COMMAND ${CMAKE_COMMAND} --build . --target @ARG_TARGET@ ${cfgArg}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        RESULT_VARIABLE res
        ERROR_VARIABLE out
        # pipe OUTPUT_VARIABLE as well
        # *some* compilers (MSVC) report errors to the standard output
        OUTPUT_VARIABLE out
)

if(res EQUAL 0)
    # Build command didn't fail. This means the test fails
    message(FATAL_ERROR "Error: Building '@ARG_TARGET@' didn't fail")
endif()

# collect possible errors from source
file(READ "@parsedSourcePath@" sourceText)
string(REGEX MATCHALL "//[ ]*build error:[^\n]+" matchErrors ${sourceText})

# look for collected errors in output
foreach(possibleError ${matchErrors})
    string(REGEX MATCH "//[ ]*build error:[ \t]*(.+)$" _ "${possibleError}")
    set(possibleError "${CMAKE_MATCH_1}")
    string(FIND "${out}" "${possibleError}" pos)
    if(NOT pos EQUAL -1)
        message("Success: output when building '@ARG_TARGET@' contains '${possibleError}'")
        return()
    endif()

    # prepare an output-firendly string in case the test fails
    set(outErrors "${outErrors}\n${possibleError}")
endforeach()

# print execute_process output for debugging purposes
message("${out}")
# print error
message(FATAL_ERROR "Error: Building '@ARG_TARGET@' failed, but output doesn't contain any of the expected errors:${outErrors}")
