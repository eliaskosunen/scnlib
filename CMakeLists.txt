cmake_minimum_required(VERSION 3.4)

project(scn CXX)

set(MASTER_PROJECT OFF)
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(MASTER_PROJECT ON)
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

option(SCN_USE_CMCSTL2 "Use cmcstl2 instead of range-v3" OFF)

option(SCN_PREDEFINE_VSCAN_OVERLOADS "Define additional overloads of scn::vscan in a source file (increases compilation time, decreases application object file size)" OFF)

option(SCN_TESTS "Generate tests target" ${MASTER_PROJECT})
option(SCN_EXAMPLES "Generate examples target" ${MASTER_PROJECT})
option(SCN_BENCHMARKS "Generate benchmark target" ${MASTER_PROJECT})
option(SCN_DOCS "Generate documentation target" ${MASTER_PROJECT})
option(SCN_INSTALL "Generate install target" ${MASTER_PROJECT})
option(SCN_PEDANTIC "Use stricter warnings" ${MASTER_PROJECT})
option(SCN_RANGES "Generate scn-ranges and scn-ranges-header-only targets (requires range-v3)" ON)

option(SCN_COVERAGE "Enable coverage reporting" OFF)
option(SCN_BLOAT "Generate bloat test target" OFF)

option(SCN_WERROR "Halt compilation in case of a warning" OFF)

option(SCN_USE_32BIT "Compile as 32-bit (gcc or clang only)" OFF)
option(SCN_USE_EXCEPTIONS "Compile with exception support (disabling will cause test failures)" ON)
option(SCN_USE_RTTI "Compile with RTTI (run-time type information) support" ON)
option(SCN_USE_NATIVE_ARCH "Add -march=native to build flags (gcc or clang only)" OFF)

option(SCN_USE_ASAN "Compile with AddressSanitizer (clang only)" OFF)
option(SCN_USE_UBSAN "Compile with UndefinedBehaviorSanitizer (clang only)" OFF)
option(SCN_USE_MSAN "Compile with MemorySanitizer (clang only)" OFF)

option(SCN_BUILD_FUZZING "Build fuzzing tests" OFF)
option(SCN_BUILD_LOCALE_TESTS "Build localized tests, needs en_US.utf8 and fi_FI.utf8 locales" OFF)

file(READ include/scn/detail/config.h config_h)
if (NOT config_h MATCHES "SCN_VERSION SCN_COMPILER\\(([0-9]+), ([0-9]+), ([0-9]+)\\)")
    message(FATAL_ERROR "Cannot get SCN_VERSION from config.h")
endif ()
set(SCN_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
message(STATUS "scn version: ${SCN_VERSION}")

include(sanitizers)
include(flags)

message(STATUS "SCN_PEDANTIC: ${SCN_PEDANTIC}")
message(STATUS "SCN_WERROR: ${SCN_WERROR}")

function (generate_library_target target_name)
    add_library(${target_name}
        src/istream.cpp src/stream.cpp src/vscan.cpp src/locale.cpp src/visitor.cpp)
    target_include_directories(${target_name} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(${target_name} PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
    target_compile_definitions(${target_name} PUBLIC
        -DSCN_HEADER_ONLY=0
        -DSCN_PREDEFINE_VSCAN_OVERLOADS=$<BOOL:${SCN_PREDEFINE_VSCAN_OVERLOADS}>)
    target_compile_options(${target_name} PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj>)

    target_compile_features(${target_name} PUBLIC cxx_std_11)
    set_private_flags(${target_name})
endfunction ()
function (generate_header_only_target target_name)
    add_library(${target_name} INTERFACE)
    target_include_directories(${target_name} INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src>"
        "$<INSTALL_INTERFACE:include>")
    target_compile_definitions(${target_name} INTERFACE
        -DSCN_HEADER_ONLY=1
        -DSCN_PREDEFINE_VSCAN_OVERLOADS=$<BOOL:${SCN_PREDEFINE_VSCAN_OVERLOADS}>)
    target_compile_features(${target_name} INTERFACE cxx_std_11)
endfunction ()

generate_library_target(scn)
generate_header_only_target(scn-header-only)

if (SCN_RANGES)
    if (SCN_USE_CMCSTL2)
        find_package(cmcstl2 QUIET CONFIG)
        if (cmcstl2_FOUND)
            set(SCN_HAS_RANGES ON)
        else ()
            message(STATUS "cmcstl2 not found")
            set(SCN_HAS_RANGES OFF)
        endif ()
    else ()
        find_package(range-v3 QUIET CONFIG)
        if (range-v3_FOUND)
            set(SCN_HAS_RANGES ON)
        else ()
            message(STATUS "range-v3 not found")
            set(SCN_HAS_RANGES OFF)
        endif ()
    endif ()
else ()
    set(SCN_HAS_RANGES OFF)
endif ()

if (SCN_HAS_RANGES)
    add_library(scn-ranges
        src/ranges/vscan.cpp)
    target_link_libraries(scn-ranges PUBLIC scn)

    add_library(scn-ranges-header-only INTERFACE)
    target_link_libraries(scn-ranges-header-only INTERFACE scn-header-only)

    if (SCN_USE_CMCSTL2)
        target_link_libraries(scn-ranges PUBLIC scn stl2)
        target_link_libraries(scn-ranges-header-only INTERFACE scn-header-only stl2)

        target_compile_features(scn-ranges PUBLIC cxx_std_17)
        target_compile_features(scn-ranges-header-only INTERFACE cxx_std_17)

        target_compile_options(scn-ranges PUBLIC
            $<$<CXX_COMPILER_ID:GNU>: -fconcepts>
            $<$<CXX_COMPILER_ID:Clang>: -Xclang -fconcepts-ts>)
        target_compile_options(scn-ranges-header-only INTERFACE
            $<$<CXX_COMPILER_ID:GNU>: -fconcepts>
            $<$<CXX_COMPILER_ID:Clang>: -Xclang -fconcepts-ts>)

        target_compile_definitions(scn-ranges PUBLIC -DSCN_RANGES_USE_CMCSTL2=1)
        target_compile_definitions(scn-ranges-header-only INTERFACE -DSCN_RANGES_USE_CMCSTL2=1)
    else ()
        target_link_libraries(scn-ranges PUBLIC range-v3)
        target_link_libraries(scn-ranges-header-only INTERFACE range-v3)

        target_compile_features(scn-ranges PUBLIC cxx_std_14)
        target_compile_features(scn-ranges-header-only INTERFACE cxx_std_14)

        target_compile_definitions(scn-ranges PUBLIC -DSCN_RANGES_USE_RANGEV3=1)
        target_compile_definitions(scn-ranges-header-only INTERFACE -DSCN_RANGES_USE_RANGEV3=1)
    endif ()
endif ()


set(SCN_EXPORT_TARGETS_LIST scn scn-header-only)
add_library(scn::scn ALIAS scn)
add_library(scn::scn-header-only ALIAS scn-header-only)

if (SCN_HAS_RANGES)
    set(SCN_EXPORT_TARGETS_LIST ${SCN_EXPORT_TARGETS_LIST} scn-ranges scn-ranges-header-only)
    add_library(scn::scn-ranges ALIAS scn-ranges)
    add_library(scn::scn-ranges-header-only ALIAS scn-ranges-header-only)
endif()

if (SCN_TESTS)
    enable_testing()
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
