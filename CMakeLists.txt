cmake_minimum_required(VERSION 3.5)

project(scn CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(MASTER_PROJECT ON)
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

option(SCN_TESTS "Generate tests target" ${MASTER_PROJECT})
option(SCN_EXAMPLES "Generate examples target" ${MASTER_PROJECT})
option(SCN_BENCHMARKS "Generate benchmark target" ${MASTER_PROJECT})
option(SCN_DOCS "Generate documentation target" ${MASTER_PROJECT})
option(SCN_INSTALL "Generate install target" ${MASTER_PROJECT})
option(SCN_PEDANTIC "Use stricter warnings" ${MASTER_PROJECT})

option(SCN_COVERAGE "Enable coverage reporting" OFF)

option(SCN_WERROR "Halt compilation in case of a warning" OFF)

option(SCN_USE_32BIT "Compile as 32-bit (gcc or clang only)" OFF)
option(SCN_USE_EXCEPTIONS "Compile with exception support (disabling will cause test failures)" ON)
option(SCN_USE_RTTI "Compile with RTTI (run-time type information) support" ON)
option(SCN_USE_NATIVE_ARCH "Add -march=native to build flags (gcc or clang only)" OFF)

option(SCN_USE_ASAN "Compile with AddressSanitizer (clang only)" OFF)
option(SCN_USE_UBSAN "Compile with UndefinedBehaviorSanitizer (clang only)" OFF)
option(SCN_USE_MSAN "Compile with MemorySanitizer (clang only)" OFF)

option(SCN_BUILD_FUZZING "Build fuzzing tests" OFF)
option(SCN_BUILD_LOCALIZED_TESTS "Build scan_localized tests (requires locales en_US.UTF-8 and fi_FI.UTF-8)" OFF)
option(SCN_BUILD_BLOAT "Generate bloat test target" OFF)
option(SCN_BUILD_BUILDTIME "Generate build time test target" OFF)

option(SCN_USE_BUNDLED_FAST_FLOAT "Use lemire/fast_float bundled with scnlib" ON)

option(SCN_DISABLE_TYPE_SCHAR "Disable scanning of signed char" OFF)
option(SCN_DISABLE_TYPE_SHORT "Disable scanning of short" OFF)
option(SCN_DISABLE_TYPE_INT "Disable scanning of int" OFF)
option(SCN_DISABLE_TYPE_LONG "Disable scanning of long" OFF)
option(SCN_DISABLE_TYPE_LONG_LONG "Disable scanning of long long" OFF)
option(SCN_DISABLE_TYPE_UCHAR "Disable scanning of unsigned char" OFF)
option(SCN_DISABLE_TYPE_USHORT "Disable scanning of unsigned short" OFF)
option(SCN_DISABLE_TYPE_UINT "Disable scanning of unsigned int" OFF)
option(SCN_DISABLE_TYPE_ULONG "Disable scanning of unsigned long" OFF)
option(SCN_DISABLE_TYPE_ULONG_LONG "Disable scanning of unsigned long long" OFF)
option(SCN_DISABLE_TYPE_BOOL "Disable scanning of bool" OFF)
option(SCN_DISABLE_TYPE_CHAR "Disable scanning of char" OFF)
option(SCN_DISABLE_TYPE_CODE_POINT "Disable scanning of code_point" OFF)
option(SCN_DISABLE_TYPE_FLOAT "Disable scanning of float" OFF)
option(SCN_DISABLE_TYPE_DOUBLE "Disable scanning of double" OFF)
option(SCN_DISABLE_TYPE_LONG_DOUBLE "Disable scanning of long double" OFF)
option(SCN_DISABLE_TYPE_BUFFER "Disable scanning of span" OFF)
option(SCN_DISABLE_TYPE_STRING "Disable scanning of std::string" OFF)
option(SCN_DISABLE_TYPE_STRING_VIEW "Disable scanning of string_view" OFF)
option(SCN_DISABLE_TYPE_CUSTOM "Disable scanning of user types" OFF)

option(SCN_DISABLE_FROM_CHARS "Disallow falling back on std::from_chars when scanning floating-point values" OFF)
option(SCN_DISABLE_STRTOD "Disallow falling back on std::strtod when scanning floating-point values" OFF)
option(SCN_DISABLE_LOCALE "Disable all localization" OFF)

file(READ include/scn/detail/config.h config_h)
if (NOT config_h MATCHES "SCN_VERSION SCN_COMPILER\\(([0-9]+), ([0-9]+), ([0-9]+)\\)")
    message(FATAL_ERROR "Cannot get SCN_VERSION from config.h")
endif ()
set(SCN_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
message(STATUS "scn version: ${SCN_VERSION}")

if (NOT (${CMAKE_VERSION} VERSION_LESS "3.9.0"))
    # IPO/LTO
    cmake_policy(SET CMP0069 NEW)
endif ()

if (NOT SCN_USE_BUNDLED_FAST_FLOAT)
    find_package(FastFloat REQUIRED)
endif ()

include(sanitizers)
include(flags)

message(STATUS "SCN_PEDANTIC: ${SCN_PEDANTIC}")
message(STATUS "SCN_WERROR: ${SCN_WERROR}")

add_subdirectory(src)

set(SCN_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/vscan.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/locale.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/reader_float.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/reader_int.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/file.cpp)

function(generate_library_target target_name)
    add_library(${target_name})
    target_sources(${target_name}
            PRIVATE ${SCN_SOURCES})
    target_include_directories(${target_name} PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_include_directories(${target_name} PRIVATE
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
    target_compile_definitions(${target_name} PUBLIC
            -DSCN_HEADER_ONLY=0)
    target_compile_options(${target_name} PUBLIC
            $<$<CXX_COMPILER_ID:MSVC>: /bigobj>)
    target_compile_features(${target_name} PUBLIC cxx_std_11)
    set_private_flags(${target_name})

    if (SCN_USE_BUNDLED_FAST_FLOAT)
        target_include_directories(${target_name} PRIVATE
                $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/deps/fast_float/single_include>)
    else ()
        target_link_libraries(${target_name} INTERFACE
                FastFloat::fast_float)
    endif ()
endfunction()
function(generate_header_only_target target_name)
    add_library(${target_name} INTERFACE)
    target_include_directories(${target_name} INTERFACE
            "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    target_compile_definitions(${target_name} INTERFACE
            -DSCN_HEADER_ONLY=1)
    target_compile_features(${target_name} INTERFACE cxx_std_11)

    if (SCN_USE_BUNDLED_FAST_FLOAT)
        target_include_directories(${target_name} INTERFACE
                $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/deps/fast_float/single_include>)
    else ()
        target_link_libraries(${target_name} INTERFACE
                FastFloat::fast_float)
    endif ()
endfunction()

generate_library_target(scn)
generate_header_only_target(scn-header-only)

set(SCN_EXPORT_TARGETS_LIST
        scn scn-header-only)
add_library(scn::scn ALIAS scn)
add_library(scn::scn-header-only ALIAS scn-header-only)

set_property(TARGET scn PROPERTY SOVERSION 0)

add_subdirectory(scripts)

if (SCN_TESTS)
    enable_testing()
endif ()
add_subdirectory(test)

if (SCN_EXAMPLES)
    add_subdirectory(examples)
endif ()
if (SCN_BENCHMARKS)
    add_subdirectory(benchmark)
endif ()
if (SCN_DOCS)
    add_subdirectory(docs)
endif ()

if (SCN_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/scnConfigVersion.cmake"
            VERSION ${SCN_VERSION}
            COMPATIBILITY SameMinorVersion)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/scnConfig.cmake"
            @ONLY)

    install(DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/include/"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT scnlib_Development)

    install(DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/src/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/scn/detail"
            COMPONENT scnlib_Development)

    install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
            "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/scn
            COMPONENT scnlib_Development)

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/scnConfigVersion.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/scnConfig.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/scn"
            COMPONENT scnlib_Development)

    export(TARGETS ${SCN_EXPORT_TARGETS_LIST}
            NAMESPACE scn
            FILE "${CMAKE_CURRENT_BINARY_DIR}/scnTargets.cmake")

    export(PACKAGE scn)

    install(TARGETS ${SCN_EXPORT_TARGETS_LIST}
            COMPONENT scnlib_Development
            EXPORT scnTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT scnTargets
            COMPONENT scnlib_Development
            NAMESPACE scn::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/scn")
endif ()
