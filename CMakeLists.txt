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

if (SCN_IS_TOP_PROJECT)
    if (NOT DEFINED CMAKE_BUILD_TYPE)
        message(WARNING "No CMAKE_BUILD_TYPE set, defaulting to Release")
        set(CMAKE_BUILD_TYPE Release CACHE STRING "")
    endif ()
    if (NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET)
        message(STATUS "No CMAKE_CXX_VISIBILITY_PRESET set, defaulting to hidden")
        set(CMAKE_CXX_VISIBILITY_PRESET hidden CACHE STRING "Preset for the export of private symbols")
        set_property(CACHE CMAKE_CXX_VISIBILITY_PRESET PROPERTY STRINGS hidden default)
    endif ()
    if (NOT DEFINED CMAKE_VISIBILITY_INLINES_HIDDEN)
        message(STATUS "No CMAKE_VISIBILITY_INLINES_HIDDEN set, defaulting to ON")
        set(CMAKE_VISIBILITY_INLINES_HIDDEN ON CACHE BOOL
                "Whether to add a compiler flag to hide symbols of inline functions")
    endif ()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

include(cmake/util.cmake)
include(cmake/options.cmake)
include(cmake/buildflags.cmake)
include(cmake/dependencies.cmake)
include(cmake/sanitizers.cmake)

set(SCN_SYSTEM_HEADERS_ATTRIBUTE "")
if (SCN_HEADERS_AS_SYSTEM)
    set(SCN_SYSTEM_HEADERS_ATTRIBUTE "SYSTEM")
endif ()

set(SCN_PUBLIC_HEADERS
        include/scn/fwd.h
        include/scn/macros.h
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
set(SCN_MODULE_SOURCES
        src/scn/scn.cppm
)

add_library(
        scn
        ${SCN_PUBLIC_HEADERS}
        ${SCN_PRIVATE_HEADERS}
        ${SCN_PRIVATE_SOURCES}
)
target_include_directories(scn
        ${SCN_SYSTEM_HEADERS_ATTRIBUTE}
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

set_property(TARGET scn PROPERTY SOVERSION 4.0.1)
add_library(scn::scn ALIAS scn)

if (SCN_MODULES)
    if (CMAKE_VERSION VERSION_LESS 3.28 OR NOT (CMAKE_GENERATOR STREQUAL "Ninja"))
        message(FATAL_ERROR "SCN_MODULES requires CMake >= 3.28 and Ninja (for target_sources(CXX_MODULES) )")
    endif ()

    add_library(scn_module)
    target_sources(scn_module PUBLIC FILE_SET scn TYPE CXX_MODULES FILES ${SCN_MODULE_SOURCES})
    target_link_libraries(scn_module PUBLIC scn::scn)
    target_compile_features(scn_module PUBLIC cxx_std_20)
    set_target_properties(scn_module PROPERTIES CXX_SCAN_FOR_MODULES ON)

    add_library(scn::module ALIAS scn_module)
endif (SCN_MODULES)

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
