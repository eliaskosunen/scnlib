cmake_minimum_required(VERSION 3.16)

project(
        scn
        VERSION 4.0.1
        DESCRIPTION "scanf for modern C++"
        HOMEPAGE_URL "https://scnlib.dev"
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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

include(cmake/util.cmake)
include(cmake/options.cmake)
include(cmake/buildflags.cmake)
include(cmake/dependencies.cmake)
include(cmake/sanitizers.cmake)

set(SCN_PUBLIC_HEADERS
        include/scn/fwd.h
        include/scn/scan.h
        include/scn/ranges.h
        include/scn/regex.h
        include/scn/istream.h
        include/scn/xchar.h
)
set(SCN_PRIVATE_HEADERS
        src/scn/impl.h
)
set(SCN_PRIVATE_SOURCES
        src/scn/impl.cpp
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
        ${SCN_FAST_FLOAT_TARGET}
        ${SCN_REGEX_BACKEND_TARGET}
)
set_library_flags(scn)

set_property(TARGET scn PROPERTY SOVERSION 4)
add_library(scn::scn ALIAS scn)

add_library(scn_internal INTERFACE)
target_link_libraries(scn_internal INTERFACE
        scn::scn
        ${SCN_FAST_FLOAT_TARGET}
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
