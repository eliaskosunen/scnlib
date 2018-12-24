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

add_subdirectory(include/scn/span-lite/span-lite)

set(SCN_HEADERS
	include/scn/istream.h include/scn/scn.h
	include/scn/scn/args.h include/scn/scn/config.h
	include/scn/scn/context.h include/scn/scn/core.h
	include/scn/scn/locale.h include/scn/scn/stream.h
	include/scn/scn/string_view.h include/scn/scn/types.h
	include/scn/scn/util.h
	include/scn/span-lite/span.h)

if (SCN_HEADER_ONLY)
    add_library(scn INTERFACE ${SCN_HEADERS})
    target_include_directories(scn INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src>"
        "$<INSTALL_INTERFACE:include;src>")
    target_link_libraries(scn INTERFACE
        span-lite)
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
    target_link_libraries(scn PUBLIC
        span-lite)
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

