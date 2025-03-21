include(icm_build_failure_testing)

set(buildfail_sources
        brace_as_fill_character.cpp
        charset_empty.cpp
        charset_reversed_range.cpp
        charset_unterminated.cpp
        integer_with_string_presentation.cpp
        invalid_unicode_in_format_string.cpp
        letters_in_argument_id.cpp
        locale_flag_with_locale_disabled.cpp
        locale_flag_with_string.cpp
        negative_argument_id.cpp
        regex_disabled.cpp
        string_view_non_contiguous_source.cpp
        unterminated_argument_id.cpp
        unterminated_format_specifier.cpp
        usertype_non_contiguous_source.cpp
)
if (NOT SCN_DISABLE_REGEX)
    list(APPEND buildfail_sources
            regex_empty.cpp
            regex_flag_invalid.cpp
            regex_flag_multiple.cpp
            regex_matches_non_contiguous_source.cpp
            regex_no_presentation.cpp
            regex_non_contiguous_source.cpp
            regex_unterminated.cpp
            regex_wide_strings.cpp
    )
endif ()

if ((SCN_CXX_FRONTEND STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.0) OR SCN_CXX_FRONTEND STREQUAL "Clang")
    # gcc pre-9 has weird error messages
    icm_add_multiple_build_failure_tests(
            SOURCES ${buildfail_sources}
            PREFIX scn-compilefail
            LIBRARIES scn
    )
else ()
    # We expect the error messages to contain the call to on_error("..."),
    # which contains the library error message.
    # Seemingly, only gcc and clang do that.
    message(STATUS "Compiler frontend not gcc or clang -- build failure tests don't check the compiler output")
    foreach (source ${buildfail_sources})
        get_filename_component(source_name ${source} NAME_WE)
        icm_add_build_failure_test(
                NAME scn-compilefail-${source_name}
                LIBRARIES scn
                SOURCES ${source}
        )
    endforeach ()
endif ()
