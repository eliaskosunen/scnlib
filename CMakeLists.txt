cmake_minimum_required(VERSION 3.4)

project(scn)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(SCN_TESTS "Build scnlib tests" ON)
    option(SCN_EXAMPLES "Build scnlib examples" ON)
    option(SCN_BENCHMARKS "Build scnlib benchmarks" ON)
else ()
    option(SCN_TESTS "Build scnlib tests" OFF)
    option(SCN_EXAMPLES "Build scnlib examples" OFF)
    option(SCN_BENCHMARKS "Build scnlib benchmarks" OFF)
endif ()

option(SCN_HEADER_ONLY "Build scnlib as a header-only library" OFF)

set(SCN_HEADERS
	include/scn/istream.h include/scn/scn.h
	include/scn/detail/args.h include/scn/detail/config.h
	include/scn/detail/context.h include/scn/detail/core.h
	include/scn/detail/locale.h include/scn/detail/stream.h
	include/scn/detail/span.h include/scn/detail/string_view.h
	include/scn/detail/types.h include/scn/detail/util.h)

if (SCN_HEADER_ONLY)
    add_library(scn INTERFACE ${SCN_HEADERS})
    target_include_directories(scn INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src>"
        "$<INSTALL_INTERFACE:include;src>")
    target_compile_features(scn INTERFACE
        cxx_nullptr)
    target_compile_definitions(scn INTERFACE
        -DSCN_HEADER_ONLY=1)
else ()
    add_library(scn ${SCN_HEADERS}
        src/locale.cpp)
    target_include_directories(scn PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(scn PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:src>)
    target_compile_features(scn PUBLIC
        cxx_nullptr)
    target_compile_definitions(scn PUBLIC
        -DSCN_HEADER_ONLY=0)
endif ()

if (SCN_TESTS)
    add_subdirectory(test)
endif ()
if (SCN_EXAMPLES)
    add_subdirectory(examples)
endif ()
if (SCN_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()

