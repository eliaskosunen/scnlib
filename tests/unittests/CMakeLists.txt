if (NOT SCN_TESTS)
    return()
endif ()

add_library(scn_tests_base INTERFACE)
target_link_libraries(scn_tests_base INTERFACE scn_sanitizers scn_internal)
target_compile_options(scn_tests_base INTERFACE
        $<$<CXX_COMPILER_ID:GNU>:
        -Wno-unused>
        $<$<CXX_COMPILER_ID:Clang>:
        -Wno-unused-variable
        -Wno-used-but-marked-unused
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-weak-vtables>
)

add_executable(scn_tests
        main.cpp

        align_and_fill_test.cpp
        args_test.cpp
        buffer_test.cpp
        char_test.cpp
        chrono_test.cpp
        context_test.cpp
        custom_type_test.cpp
        error_test.cpp
        float_test.cpp
        format_string_test.cpp
        format_string_parser_test.cpp
        integer_test.cpp
        input_map_test.cpp
        istream_scanner_test.cpp
        memory_test.cpp
        ranges_test.cpp
        regex_test.cpp
        result_test.cpp
        scan_test.cpp
        source_test.cpp
        standalone_fwd_include_test.cpp
        standalone_scan_include_test.cpp
        string_test.cpp
        string_view_test.cpp
        unicode_test.cpp
)
target_link_libraries(scn_tests ${SCN_GTEST_LIBRARIES} scn_tests_base)
add_test(NAME scn_tests COMMAND scn_tests)

add_executable(scn_impl_tests
        main.cpp

        impl_tests/bits_test.cpp
        impl_tests/contiguous_range_factory_test.cpp
        impl_tests/find_fast_test.cpp
        impl_tests/function_ref_test.cpp
        impl_tests/read_algorithms_test.cpp
        impl_tests/text_width_test.cpp
        impl_tests/transcode_test.cpp
        impl_tests/whitespace_skip_test.cpp

        impl_tests/reader_test_common.h
        impl_tests/bool_reader_test.cpp
        impl_tests/float_reader_test.cpp
        impl_tests/string_reader_test.cpp

        impl_tests/integer_reader_test.h
        impl_tests/integer_reader_test.impl_narrow_classic.cpp
        impl_tests/integer_reader_test.impl_narrow_localized.cpp
        impl_tests/integer_reader_test.impl_wide_classic.cpp
        impl_tests/integer_reader_test.impl_wide_localized.cpp
)
target_link_libraries(scn_impl_tests PRIVATE
        ${SCN_GTEST_LIBRARIES} scn_tests_base scn_internal)
# pragma ignores seem to not always work for overflowing literals
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties(
            impl_tests/float_reader_test.cpp
            PROPERTIES COMPILE_OPTIONS
            -Wno-overflow
    )
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(
            impl_tests/float_reader_test.cpp
            PROPERTIES COMPILE_OPTIONS
            -Wno-literal-range
    )
endif ()
add_test(NAME scn_impl_tests COMMAND scn_impl_tests)

if (SCN_MODULES)
    add_executable(scn_module_tests module_test.cpp)
    target_link_libraries(scn_module_tests PRIVATE scn_module INTERFACE scn_sanitizers)

    target_uses_extensions(scn_module use_stdext)
    set_target_properties(
            scn_module_tests PROPERTIES
            CXX_SCAN_FOR_MODULES ON
            CXX_EXTENSIONS ${use_stdext}
    )

    add_test(NAME scn_module_tests COMMAND scn_module_tests)
endif ()

if (SCN_TESTS_LOCALIZED)
    add_executable(scn_localized_tests
            main.cpp

            localized_tests/localized_chrono_test.cpp
    )
    target_link_libraries(scn_localized_tests ${SCN_GTEST_LIBRARIES} scn_tests_base)
    add_test(NAME scn_localized_tests COMMAND scn_localized_tests)
endif ()

add_executable(scn_stdin_test
        main.cpp

        stdin_test.cpp
)
target_link_libraries(scn_stdin_test ${SCN_GTEST_LIBRARIES} scn_tests_base)

add_custom_target(scn_stdin_test_prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/stdin_test_runner.py"
        "${CMAKE_CURRENT_LIST_DIR}/stdin_test_input.txt"
        "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Copying stdin test runner"
)
add_custom_target(scn_examples_test_prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/examples_test_runner.py"
        "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Copying examples test runner"
)

find_package(Python3 COMPONENTS Interpreter)
if (Python3_FOUND)
    add_test(NAME scn_stdin_test COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/stdin_test_runner.py")
    set_tests_properties(scn_stdin_test PROPERTIES DEPENDS scn_stdin_test_prepare)

    if (SCN_EXAMPLES)
        get_target_property(examples_bin_dir scn_example_1 BINARY_DIR)
        add_test(NAME scn_examples_test COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/examples_test_runner.py" "${examples_bin_dir}")
        set_tests_properties(scn_examples_test PROPERTIES DEPENDS scn_examples_test_prepare)
    else()
        message(STATUS "scn_examples_test disabled, because SCN_EXAMPLES is OFF")
    endif()
else ()
    message(WARNING "python3 not found, scn_stdin_test and scn_examples_test not added to CTest")
endif ()

add_subdirectory(compilefail_tests)
