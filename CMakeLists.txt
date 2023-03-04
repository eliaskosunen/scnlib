cmake_minimum_required(VERSION 3.14)
project(scn VERSION 2.0.0 LANGUAGES CXX)

option(SCN_DISABLE_TOP_PROJECT OFF)

if (CMAKE_VERSION VERSION_LESS 3.21)
    get_property(not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
    if (NOT not_top)
        set(PROJECT_IS_TOP_LEVEL ON)
    endif()
endif()
if (PROJECT_IS_TOP_LEVEL AND NOT SCN_DISABLE_TOP_PROJECT)
    set(SCN_IS_TOP_PROJECT ON)
else()
    set(SCN_IS_TOP_PROJECT OFF)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(cmake/util.cmake)
include(cmake/options.cmake)
include(cmake/buildflags.cmake)
include(cmake/dependencies.cmake)
include(cmake/sanitizers.cmake)

set(SCN_PUBLIC_HEADERS
        include/scn/fwd.h
        include/scn/scan.h
        include/scn/ranges.h
        include/scn/istream.h

        include/scn/detail/config.h

        include/scn/detail/args.h
        include/scn/detail/caching_view.h
        include/scn/detail/context.h
        include/scn/detail/erased_range.h
        include/scn/detail/error.h
        include/scn/detail/format_string.h
        include/scn/detail/format_string_parser.h
        include/scn/detail/input_map.h
        include/scn/detail/istream_range.h
        include/scn/detail/istream_scanner.h
        include/scn/detail/locale_ref.h
        include/scn/detail/parse_context.h
        include/scn/detail/pp.h
        include/scn/detail/pp_detect.h
        include/scn/detail/scanner_range.h
        include/scn/detail/ranges.h
        include/scn/detail/scan.h
        include/scn/detail/scanner.h
        include/scn/detail/scanner_builtin.h
        include/scn/detail/unicode.h
        include/scn/detail/visitor.h
        include/scn/detail/vscan.h

        include/scn/util/algorithm.h
        include/scn/util/expected.h
        include/scn/util/memory.h
        include/scn/util/meta.h
        include/scn/util/optional.h
        include/scn/util/span.h
        include/scn/util/string_view.h
        include/scn/util/unique_ptr.h
        )
set(SCN_PRIVATE_HEADERS
        src/scn/impl/algorithms/common.h
        src/scn/impl/algorithms/eof_check.h
        src/scn/impl/algorithms/find_whitespace.h
        src/scn/impl/algorithms/read_code_point.h
        src/scn/impl/algorithms/read_copying.h
        src/scn/impl/algorithms/read_localized.h
        src/scn/impl/algorithms/read_nocopy.h

        src/scn/impl/reader/reader.h
        src/scn/impl/reader/common.h

        src/scn/impl/reader/code_unit_and_point_reader.h
        src/scn/impl/reader/bool_reader.h
        src/scn/impl/reader/pointer_reader.h

        src/scn/impl/reader/integer/reader.h
        src/scn/impl/reader/integer/source_reader.h
        src/scn/impl/reader/integer/classic_value_reader.h
        src/scn/impl/reader/integer/localized_value_reader.h

        src/scn/impl/reader/float/reader.h
        src/scn/impl/reader/float/value_reader.h

        src/scn/impl/reader/string/reader.h
        src/scn/impl/reader/string/character_reader.h
        src/scn/impl/reader/string/character_set_reader.h
        src/scn/impl/reader/string/word_reader.h

        src/scn/impl/unicode/unicode.h

        src/scn/impl/util/ascii_ctype.h
        src/scn/impl/util/bits.h

        src/scn/impl/locale.h
        )
set(SCN_PRIVATE_SOURCES
        src/scn/scan.cpp
        src/scn/istream_range.cpp
        src/scn/istream_scanner.cpp
        src/scn/scanner.cpp
        src/scn/vscan.cpp

        src/scn/impl/locale.cpp

        src/scn/impl/algorithms/find_whitespace.cpp

        src/scn/impl/reader/common.cpp

        src/scn/impl/reader/bool_reader.cpp

        src/scn/impl/reader/float/value_reader.cpp

        src/scn/impl/reader/integer/source_reader.cpp
        src/scn/impl/reader/integer/classic_value_reader.cpp
        src/scn/impl/reader/integer/localized_value_reader.cpp

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
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
        )
set_library_flags(scn)

target_link_libraries(scn
        PRIVATE
            simdutf::simdutf
            fast_float
        )

set_property(TARGET scn PROPERTY SOVERSION 2)

add_library(scn::scn ALIAS scn)

add_library(scn_internal INTERFACE)
target_link_libraries(scn_internal INTERFACE scn::scn)
set_interface_flags(scn_internal)

add_subdirectory(scripts)

add_subdirectory(benchmark)
add_subdirectory(docs)
add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)

include(cmake/install.cmake)
