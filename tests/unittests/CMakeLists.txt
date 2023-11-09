if (NOT SCN_TESTS)
    return()
endif()

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
        -Wno-weak-vtables
        >)


add_executable(scn_tests
        main.cpp

        align_and_fill_test.cpp
        args_test.cpp
        caching_view_test.cpp
        char_test.cpp
        context_test.cpp
        custom_type_test.cpp
        erased_range_test.cpp
        error_test.cpp
        float_test.cpp
        format_string_test.cpp
        format_string_parser_test.cpp
        integer_test.cpp
        input_map_test.cpp
        istream_range_test.cpp
        istream_scanner_test.cpp
        memory_test.cpp
        ranges_test.cpp
        result_test.cpp
        scan_test.cpp
        source_test.cpp
        string_test.cpp
        string_view_test.cpp
        unicode_test.cpp
)
target_link_libraries(scn_tests scn_gtest scn_tests_base)
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
        scn_gtest scn_tests_base scn_internal)
add_test(NAME scn_impl_tests COMMAND scn_impl_tests)
