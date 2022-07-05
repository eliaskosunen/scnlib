if (NOT SCN_TESTS)
    return()
endif()

add_library(scn_tests_base INTERFACE)
target_link_libraries(scn_tests_base INTERFACE scn_sanitizers scn_internal)
target_compile_options(scn_tests_base INTERFACE
    $<$<CXX_COMPILER_ID:Clang>:
        -Wno-unused-variable
        -Wno-used-but-marked-unused
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-weak-vtables
        >)


add_executable(scn_tests
        main.cpp

        args_test.cpp
        caching_view_test.cpp
        char_test.cpp
        context_test.cpp
        custom_type_test.cpp
        erased_range_test.cpp
        error_test.cpp
        format_string_test.cpp
        format_string_parser_test.cpp
        integer_test.cpp
        input_map_test.cpp
        istream_range_test.cpp
        istream_scanner_test.cpp
        ranges_test.cpp
        result_test.cpp
        scan_test.cpp
        source_test.cpp
        string_test.cpp
        string_view_test.cpp
)
target_link_libraries(scn_tests gtest_main gmock_main scn_tests_base)
add_test(NAME scn_tests COMMAND scn_tests)

add_executable(scn_impl_tests
        main.cpp

        impl_tests/character_set_reader_test.cpp
        impl_tests/copy_test.cpp
        impl_tests/float_reader_test.cpp
        impl_tests/int_source_reader_test.cpp
        impl_tests/read_code_point_test.cpp
        impl_tests/read_copying_test.cpp
        impl_tests/read_localized_test.cpp
        impl_tests/read_nocopy_test.cpp
        impl_tests/utf8_test.cpp
        impl_tests/whitespace_skip_test.cpp

        impl_tests/int_reader_test.h
        impl_tests/int_reader_test_narrow_classic.cpp
        impl_tests/int_reader_test_narrow_localized.cpp
        impl_tests/int_reader_test_wide_classic.cpp
        impl_tests/int_reader_test_wide_localized.cpp
)
target_link_libraries(scn_impl_tests
        gtest_main gmock_main scn_tests_base
        simdutf::simdutf fast_float)
target_include_directories(scn_impl_tests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src")
add_test(NAME scn_impl_tests COMMAND scn_impl_tests)

