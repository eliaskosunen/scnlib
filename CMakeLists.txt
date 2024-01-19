cmake_minimum_required(VERSION 3.16)

project(
        scn
        VERSION 2.0.0
        DESCRIPTION "scanf for modern C++"
        HOMEPAGE_URL "https://v2.scnlib.dev"
        LANGUAGES CXX
)

option(SCN_DISABLE_TOP_PROJECT OFF)

if (CMAKE_VERSION VERSION_LESS 3.21)
    get_property(not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
    if (NOT not_top)
        set(PROJECT_IS_TOP_LEVEL ON)
    endif ()
endif ()
if (PROJECT_IS_TOP_LEVEL AND NOT SCN_DISABLE_TOP_PROJECT)
    set(SCN_IS_TOP_PROJECT ON)
else ()
    set(SCN_IS_TOP_PROJECT OFF)
endif ()

if (SCN_IS_TOP_PROJECT AND NOT CMAKE_BUILD_TYPE)
    message(WARNING "No CMAKE_BUILD_TYPE set, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(cmake/util.cmake)
include(cmake/options.cmake)
include(cmake/buildflags.cmake)
include(cmake/dependencies.cmake)
include(cmake/sanitizers.cmake)

set(SCN_PUBLIC_HEADERS
        include/scn/all.h
        include/scn/fwd.h
        include/scn/scan.h
        include/scn/ranges.h
        include/scn/regex.h
        include/scn/istream.h
        include/scn/xchar.h

        include/scn/detail/config.h

        include/scn/detail/args.h
        include/scn/detail/context.h
        include/scn/detail/error.h
        include/scn/detail/format_string.h
        include/scn/detail/format_string_parser.h
        include/scn/detail/input_map.h
        include/scn/detail/istream_scanner.h
        include/scn/detail/locale_ref.h
        include/scn/detail/parse_context.h
        include/scn/detail/pp.h
        include/scn/detail/pp_detect.h
        include/scn/detail/regex.h
        include/scn/detail/scanner_range.h
        include/scn/detail/ranges.h
        include/scn/detail/scan.h
        include/scn/detail/scan_buffer.h
        include/scn/detail/scanner.h
        include/scn/detail/scanner_builtin.h
        include/scn/detail/unicode.h
        include/scn/detail/visitor.h
        include/scn/detail/vscan.h
        include/scn/detail/xchar.h

        include/scn/util/algorithm.h
        include/scn/util/expected.h
        include/scn/util/memory.h
        include/scn/util/meta.h
        include/scn/util/span.h
        include/scn/util/string_view.h
)
set(SCN_PRIVATE_HEADERS
        src/scn/impl/algorithms/common.h
        src/scn/impl/algorithms/contiguous_range_factory.h
        src/scn/impl/algorithms/eof_check.h
        src/scn/impl/algorithms/find_whitespace.h
        src/scn/impl/algorithms/read.h
        src/scn/impl/algorithms/take_width_view.h

        src/scn/impl/reader/reader.h
        src/scn/impl/reader/common.h
        src/scn/impl/reader/numeric_reader.h

        src/scn/impl/reader/code_unit_and_point_reader.h
        src/scn/impl/reader/bool_reader.h
        src/scn/impl/reader/float_reader.h
        src/scn/impl/reader/integer_reader.h
        src/scn/impl/reader/pointer_reader.h
        src/scn/impl/reader/string_reader.h

        src/scn/impl/unicode/unicode.h

        src/scn/impl/util/ascii_ctype.h
        src/scn/impl/util/bits.h
        src/scn/impl/util/function_ref.h
        src/scn/impl/util/internal_error.h
        src/scn/impl/util/text_width.h

        src/scn/impl/locale.h
)
set(SCN_PRIVATE_SOURCES
        src/scn/scan.cpp
        src/scn/scan_buffer.cpp
        src/scn/scanner.cpp
        src/scn/vscan.cpp

        src/scn/impl/locale.cpp

        src/scn/impl/algorithms/find_whitespace.cpp

        src/scn/impl/reader/reader.cpp
        src/scn/impl/reader/float_reader.cpp
        src/scn/impl/reader/integer_reader.cpp
)
set(SCN_PRIVATE_SOURCES_WITH_IOSTREAMS
)

add_library(scn
        "${SCN_PUBLIC_HEADERS}"
        "${SCN_PRIVATE_HEADERS}"
        "${SCN_PRIVATE_SOURCES}"
)
target_include_directories(scn
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_link_libraries(scn PRIVATE
        simdutf::simdutf
        FastFloat::fast_float
        ${SCN_REGEX_BACKEND_TARGET}
)
set_library_flags(scn)

set_property(TARGET scn PROPERTY SOVERSION 2)
add_library(scn::scn ALIAS scn)

add_library(scn_internal INTERFACE)
target_link_libraries(scn_internal INTERFACE
        scn::scn
        simdutf::simdutf
        FastFloat::fast_float
        ${SCN_REGEX_BACKEND_TARGET}
)
target_include_directories(scn_internal
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
set_interface_flags(scn_internal)

add_subdirectory(scripts)

add_subdirectory(benchmark)
add_subdirectory(docs)
add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)

include(cmake/install.cmake)
