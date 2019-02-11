cmake_minimum_required(VERSION 3.4)

project(scn)

set(MASTER_PROJECT OFF)
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(MASTER_PROJECT ON)
endif ()

option(SCN_TESTS "Generate tests target" ${MASTER_PROJECT})
option(SCN_EXAMPLES "Generate examples target" ${MASTER_PROJECT})
option(SCN_BENCHMARKS "Generate benchmark target" ${MASTER_PROJECT})
option(SCN_DOCS "Generate documentation target" ${MASTER_PROJECT})
option(SCN_INSTALL "Generate install target" ${MASTER_PROJECT})
option(SCN_PEDANTIC "Use stricter warnings" ${MASTER_PROJECT})

option(SCN_WERROR "Halt compilation in case of a warning" OFF)

file(READ include/scn/detail/config.h config_h)
if (NOT config_h MATCHES "SCN_VERSION SCN_COMPILER\\(([0-9]+), ([0-9]+), ([0-9]+)\\)")
    message(FATAL_ERROR "Cannot get SCN_VERSION from config.h")
endif ()
set(SCN_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
message(STATUS "scn version: ${SCN_VERSION}")

function(set_headers VAR)
    set(headers ${${VAR}})
    foreach (header ${ARGN})
        set(headers ${headers} "${PROJECT_SOURCE_DIR}/include/scn/${header}")
    endforeach ()
    set(${VAR} ${headers} PARENT_SCOPE)
endfunction ()

set_headers(SCN_HEADERS
	istream.h scn.h
	detail/args.h detail/config.h
	detail/context.h detail/core.h
	detail/locale.h detail/stream.h
	detail/span.h detail/string_view.h
	detail/types.h detail/util.h)

add_library(scn-base INTERFACE)
target_compile_features(scn-base INTERFACE cxx_std_11)
if (SCN_PEDANTIC)
    target_compile_options(scn-base INTERFACE
        $<$<CXX_COMPILER_ID:Clang>:
            -Weverything
            -Wpedantic -pedantic-errors
            -fvisibility=hidden -fstrict-aliasing
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++98-compat-bind-to-temporary-copy
            -Wno-c++98-compat-local-type-template-args>
        $<$<CXX_COMPILER_ID:GNU>:
            -Wall -Wextra -Wpedantic
            -pedantic-errors
            -fvisibility=hidden -fstrict-aliasing
            -Wconversion -Wsign-conversion
            -Wold-style-cast -Wfloat-equal
            -Wlogical-op -Wundef
            -Wredundant-decls -Wshadow
            -Wwrite-strings
            -Wpointer-arith -Wcast-qual
            -Wformat=2 -Wswitch-default
            -Wmissing-include-dirs -Wcast-align
            -Wswitch-enum -Wnon-virtual-dtor
            -Wctor-dtor-privacy -Wdisabled-optimization
            -Winvalid-pch -Wnoexcept
            -Wmissing-declarations -Woverloaded-virtual
            $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,5.0>>:
            -Wdouble-promotion -Wtrampolines
            -Wzero-as-null-pointer-constant
            -Wuseless-cast -Wvector-operation-performance>
            $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,6.0>>:
            -Wshift-overflow=2 -Wnull-dereference
            -Wduplicated-cond>
            $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,7.0>>:
            -Walloc-zero -Walloca
            -Wduplicated-branches>
            $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,8.0>>:
            -Wcast-align=strict>
            >
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /std:c++latest /permissive->)
endif ()
if (SCN_WERROR)
    target_compile_options(scn-base INTERFACE
        $<$<CXX_COMPILER_ID:Clang>:
            -Werror>
        $<$<CXX_COMPILER_ID:GNU>:
            -Werror>
        $<$<CXX_COMPILER_ID:MSVC>:
            /WX)
endif()

message(STATUS "SCN_PEDANTIC: ${SCN_PEDANTIC}")
message(STATUS "SCN_WERROR: ${SCN_WERROR}")

function (generate_library_target target_name)
    add_library(${target_name} ${SCN_HEADERS} src/locale.cpp)
    target_include_directories(${target_name} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(${target_name} PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
    target_link_libraries(${target_name} PRIVATE scn-base)
    target_compile_definitions(${target_name} PUBLIC
        -DSCN_HEADER_ONLY=0)
endfunction ()
function (generate_header_only_target target_name)
    add_library(${target_name} INTERFACE)
    target_include_directories(${target_name} INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src>"
        "$<INSTALL_INTERFACE:include>")
    target_compile_definitions(${target_name} INTERFACE
        -DSCN_HEADER_ONLY=1)
endfunction ()

generate_library_target(scn)
generate_header_only_target(scn-header-only)
set(SCN_EXPORT_TARGETS_LIST scn scn-base scn-header-only)
add_library(scn::scn-header-only ALIAS scn-header-only)
add_library(scn::scn ALIAS scn)

if (SCN_TESTS)
    add_subdirectory(test)
endif ()
if (SCN_EXAMPLES)
    add_subdirectory(examples)
endif ()
if (SCN_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()
if (SCN_DOCS)
    add_subdirectory(doc)
endif ()

if (SCN_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfigVersion.cmake"
        VERSION ${SCN_VERSION}
        COMPATIBILITY ExactVersion)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfig.cmake"
        @ONLY)

    install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/src/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/scn/detail")

    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/scn)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/scnConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/scn")

    export(TARGETS ${SCN_EXPORT_TARGETS_LIST}
        NAMESPACE scn
        FILE "${CMAKE_CURRENT_BINARY_DIR}/scnTargets.cmake")

    install(TARGETS ${SCN_EXPORT_TARGETS_LIST}
        EXPORT scnTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT scnTargets
        NAMESPACE scn::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/scn")
endif ()
