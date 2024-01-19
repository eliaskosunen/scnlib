# icm_build_failure_testing
#
# SPDX-License-Identifier: MIT
# MIT License:
# Copyright (c) 2022-2023 Borislav Stanimirov
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files(the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and / or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions :
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#           VERSION HISTORY
#
#   1.01 (2023-02-02) Allow target properties for multi-targets funcs
#   1.00 (2022-10-04) Initial release
#
#           NOTES
# This file is bundled with icm_build_failure_parse_and_run.cmake and expects
# it to be in the same directory
include_guard(GLOBAL)

# store current dir to find icm_build_failure_parse_and_run
set(ICM_BUILD_FAILURE_TEST_SCRIPT_DIR "${CMAKE_CURRENT_LIST_DIR}")

# icm_add_build_failure_test
#
# Add a build failure test to a project
#
# Args:
#   * NAME name     - Name of the test (for CTest)
#   * TARGET target - Optional.
#                     Name of the target executable (whose build will fail).
#                     Defaults to '<name>-test' if not provided
#
#   * SOURCES sources - Sources for the executable
#       * PARSE src   - Optional. One of the sources can be marked as PARSE. If
#                       such is present, it will be parsed for expected errors
#
#   * LIBRARIES libs - Optional. Link libraries for the executable
#   * LABELS labels  - Optional. CTest labels for the test
#                      If none are provided, "build-failure" will be added
#   * FOLDER folder  - Optional. MSVC solution folder for the target
#
#   * ERROR_MATCHES mathes - Optional. Strings with which to match the build
#                            output of the executable
#
# Notes:
# * If neither ERROR_MATCHES nor a PARSE source is present, the test will only
#   check that the build of the target fails. This is not recommended
# * If both are present, the PARSE source takes precedence
# * ERROR_MATCHES are a list of strings. The test will check that at least one
#   of provided strings matches the output of the build
# * To list errors in the PARSE source, it must be on a line prepended with:
#   "// build error:". Example:
#
#   // x.cpp:
#   // build error: custom error was triggered
#   static_assert(false, "custom error was triggered")
#
#   If multimple "// build error: lines" are present, at least one of them
#   needs to match for a successful test.
#
# Example:
# icm_add_build_failure_test(
#   NAME mylib-bf-foo-func-first-arg-trivial
#   LIBRARIES mylib
#   SOURCES
#       test-helper.cpp
#       PARSE bf-foo-func-first-arg-trivial.cpp
#   LABELS bf mylib
# )
function(icm_add_build_failure_test)
    cmake_parse_arguments(ARG "" "NAME;TARGET;FOLDER" "SOURCES;LIBRARIES;ERROR_MATCHES;LABELS" ${ARGN})
    if(DEFINED ARG_UNPARSED_ARGUMENTS)
        message(NOTICE "icm_add_build_failure_test called with unknown arguments")
    endif()

    # check sources for a file to parse
    cmake_parse_arguments(ARG "" "PARSE" "" ${ARG_SOURCES})
    set(ARG_SOURCES "${ARG_UNPARSED_ARGUMENTS}")

    if(NOT DEFINED ARG_TARGET)
        set(ARG_TARGET "${ARG_NAME}-test")
    endif()

    # add an executable
    # an object library will be sufficient for compilation errors,
    # but an executable will help with linker errors as well
    add_executable(${ARG_TARGET} ${ARG_SOURCES})
    set_target_properties(${ARG_TARGET} PROPERTIES
            EXCLUDE_FROM_ALL TRUE
            EXCLUDE_FROM_DEFAULT_BUILD TRUE
    )
    if(DEFINED ARG_LIBRARIES)
        target_link_libraries(${ARG_TARGET} PRIVATE ${ARG_LIBRARIES})
    endif()
    if(DEFINED ARG_FOLDER)
        set_target_properties(${ARG_TARGET} PROPERTIES FOLDER ${ARG_FOLDER})
    endif()

    if(DEFINED ARG_PARSE)
        # we find error matches from a parsed file

        # also add the parsed source to the executable's sources
        target_sources(${ARG_TARGET} PRIVATE "${ARG_PARSE}")
        # this var is used in the configured file bellow
        get_filename_component(parsedSourcePath "${ARG_PARSE}" ABSOLUTE)

        # configure in binary dir based on target name
        # it's safe as no two targets can have the same name anyway
        # TODO: when minimal supported version of CMake is 3.18, embed the
        # helper script [=[here]=] and use file(CONFIGURE)
        configure_file(
                "${ICM_BUILD_FAILURE_TEST_SCRIPT_DIR}/icm_build_failure_parse_and_run.cmake"
                "${CMAKE_BINARY_DIR}/${ARG_TARGET}.cmake"
                @ONLY
        )

        add_test(
                NAME ${ARG_NAME}
                # provide the config as a command line arg here
                # we cannot configure the file with a generator expression
                COMMAND ${CMAKE_COMMAND} -DCFG=$<CONFIG> -P ${ARG_TARGET}.cmake
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    else()
        # we look for error matches in arguments
        add_test(
                NAME ${ARG_NAME}
                COMMAND ${CMAKE_COMMAND} --build . --target ${ARG_TARGET} --config $<CONFIG>
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        if(DEFINED ARG_ERROR_MATCHES)
            # matches are provided for all test runs
            set_tests_properties(${ARG_NAME} PROPERTIES
                    PASS_REGULAR_EXPRESSION "${ARG_ERROR_MATCHES}")
        else()
            # no matches are provided so just expect the build to fail
            set_tests_properties(${ARG_NAME} PROPERTIES
                    WILL_FAIL TRUE)
        endif()
    endif()

    if(NOT DEFINED ARG_LABELS)
        # if labels are not provided, still add "build-failure"
        set(ARG_LABELS "build-failure")
    endif()

    set_tests_properties(${ARG_NAME} PROPERTIES LABELS "${ARG_LABELS}")
endfunction()

# icm_add_multiple_build_failure_tests
#
# Add a multiple build failure tests to a project via multiple calls to
# icm_add_build_failure_test
#
# Args:
#   * SOURCES sources - List of sources to add. Each source will lead to a new
#                       test being added
#   * PREFIX prefix   - Optional. Prefix string to add to each test name
#   * PROPERTIES props- Optional. Target properties for all targets
#
#   * LIBRARIES, LABELS, FOLDER, ERROR_MATCHES
#      forwarded to icm_add_build_failure_test
#
# Notes:
# If ERROR_MATCHES is not present, each source is forwarded as PARSE
#
function(icm_add_multiple_build_failure_tests)
    cmake_parse_arguments(ARG "" "PREFIX;FOLDER" "SOURCES;LIBRARIES;ERROR_MATCHES;LABELS;PROPERTIES" ${ARGN})
    if(DEFINED ARG_UNPARSED_ARGUMENTS)
        message(NOTICE "icm_add_multiple_build_failure_tests called with unknown arguments")
    endif()
    foreach(sourceFile ${ARG_SOURCES})
        # replace spaces and path separators with '-'
        string(REGEX REPLACE "[// ]" "-" testName "${sourceFile}")
        # strip extenstion for target name
        get_filename_component(testName ${testName} NAME_WLE)

        if(NOT DEFINED ARG_ERROR_MATCHES)
            set(sourceFile "PARSE;${sourceFile}")
        endif()

        if(DEFINED ARG_PREFIX)
            set(testName ${ARG_PREFIX}-${testName})
        endif()

        # add test and forward args
        icm_add_build_failure_test(
                NAME ${testName}
                SOURCES "${sourceFile}"
                LIBRARIES ${ARG_LIBRARIES}
                LABELS ${ARG_LABELS}
                ERROR_MATCHES "${ARG_ERROR_MATCHES}"
                FOLDER ${ARG_FOLDER}
        )

        if(DEFINED ARG_PROPERTIES)
            set_target_properties(${testName}-test PROPERTIES ${ARG_PROPERTIES})
        endif()
    endforeach()
endfunction()

# icm_glob_build_failure_tests
#
# add multiple sources to icm_add_multiple_build_failure_tests via a
# file(GLOB pattern)
#
# Args:
# * PATTERN pat - GLOB pattern to use
# The rest of the argument are forwarded verbatim to
# icm_add_multiple_build_failure_tests
function(icm_glob_build_failure_tests)
    cmake_parse_arguments(ARG "" "PATTERN" "" ${ARGN})
    file(GLOB srcs
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            LIST_DIRECTORIES OFF
            ${ARG_PATTERN}
    )
    icm_add_multiple_build_failure_tests(
            SOURCES ${srcs}
            ${ARG_UNPARSED_ARGUMENTS}
    )
endfunction()
