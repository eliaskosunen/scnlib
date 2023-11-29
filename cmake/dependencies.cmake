include(FetchContent)

set(SCN_OPTIONAL_DEPENDENCIES "")

if (SCN_TESTS)
    # GTest

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG main
            GIT_SHALLOW TRUE
    )

    # gtest CMake does some flag overriding we don't want, and it's also quite heavy
    # Do it manually

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_GetProperties(googletest)
    if (NOT googletest)
        FetchContent_Populate(googletest)
    endif ()

    find_package(Threads)

    add_library(scn_gtest
            "${googletest_SOURCE_DIR}/googletest/src/gtest-all.cc"
            "${googletest_SOURCE_DIR}/googlemock/src/gmock-all.cc"
    )
    target_include_directories(scn_gtest SYSTEM
            PUBLIC
            "${googletest_SOURCE_DIR}/googletest/include"
            "${googletest_SOURCE_DIR}/googlemock/include"
            PRIVATE
            "${googletest_SOURCE_DIR}/googletest"
            "${googletest_SOURCE_DIR}/googlemock"
    )
    target_link_libraries(scn_gtest PRIVATE Threads::Threads)
    target_compile_features(scn_gtest PUBLIC cxx_std_14)
    target_compile_options(scn_gtest PRIVATE $<$<CXX_COMPILER_ID:GNU>: -Wno-psabi>)
endif ()

if (SCN_BENCHMARKS)
    # Google Benchmark

    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "Turn off google benchmark tests")
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "Turn off google benchmark install")
    FetchContent_Declare(
            google-benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG main
            GIT_SHALLOW TRUE
    )
    list(APPEND SCN_OPTIONAL_DEPENDENCIES "google-benchmark")
endif ()

# simdutf

# simdutf CMake includes tests if BUILD_TESTING is globally ON
# we don't want to include tests of dependencies, so we need to do some manual work

if (SCN_USE_EXTERNAL_SIMDUTF)
    find_package(simdutf 4.0.5 CONFIG REQUIRED)
else ()
    FetchContent_Declare(
            simdutf
            GIT_REPOSITORY https://github.com/simdutf/simdutf.git
            GIT_TAG v4.0.5
            GIT_SHALLOW TRUE
    )

    set(SIMDUTF_BENCHMARKS_BEFORE_SIMDUTF ${SIMDUTF_BENCHMARKS})
    set(BUILD_TESTING_BEFORE_SIMDUTF ${BUILD_TESTING})

    set(SIMDUTF_BENCHMARKS OFF)
    set(BUILD_TESTING OFF)

    FetchContent_GetProperties(simdutf)
    if (NOT simdutf_POPULATED)
        FetchContent_Populate(simdutf)

        add_subdirectory(${simdutf_SOURCE_DIR} ${simdutf_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif ()

    set(SIMDUTF_BENCHMARKS ${SIMDUTF_BENCHMARKS_BEFORE_SIMDUTF})
    set(BUILD_TESTING ${BUILD_TESTING_BEFORE_SIMDUTF})
endif ()

# fast_float

if (SCN_USE_EXTERNAL_FAST_FLOAT)
    find_package(FastFloat 5.3.0 CONFIG REQUIRED)
else ()
    FetchContent_Declare(
            fast_float
            GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
            GIT_TAG v5.3.0
            GIT_SHALLOW TRUE
    )

    cmake_policy(SET CMP0077 NEW)
    set(FASTFLOAT_INSTALL OFF CACHE INTERNAL "")

    FetchContent_GetProperties(fast_float)
    if (NOT fast_float_POPULATED)
        FetchContent_Populate(fast_float)

        add_subdirectory(${fast_float_SOURCE_DIR} ${fast_float_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif ()
endif ()

# Boost.Regex

if (SCN_REGEX_BACKEND STREQUAL "Boost")
    if (NOT SCN_USE_EXTERNAL_REGEX_BACKEND)
        message(FATAL_ERROR "SCN_USE_EXTERNAL_REGEX_BACKEND=OFF is not supported when SCN_REGEX_BACKEND is Boost")
    endif ()

    find_package(Boost REQUIRED COMPONENTS regex)
    set(SCN_REGEX_BACKEND_TARGET Boost::regex)
endif ()

# re2

if (SCN_REGEX_BACKEND STREQUAL "re2")
    if (NOT SCN_USE_EXTERNAL_REGEX_BACKEND)
        message(FATAL_ERROR "SCN_USE_EXTERNAL_REGEX_BACKEND=OFF is not supported when SCN_REGEX_BACKEND is re2")
    endif ()

    find_package(re2 11.0.0 REQUIRED)
    set(SCN_REGEX_BACKEND_TARGET re2::re2)
endif ()

# ctre

if (SCN_REGEX_BACKEND STREQUAL "ctre")
    if (SCN_USE_EXTERNAL_REGEX_BACKEND)
        find_package(ctre REQUIRED)
    else ()
        FetchContent_Declare(
                ctre
                GIT_REPOSITORY  https://github.com/hanickadot/compile-time-regular-expressions.git
                GIT_TAG v3.8.1
                GIT_SHALLOW TRUE
        )

        list(APPEND SCN_OPTIONAL_DEPENDENCIES "ctre")
    endif ()
    set(SCN_REGEX_BACKEND_TARGET ctre::ctre)
endif ()

# make available

FetchContent_MakeAvailable(
        ${SCN_OPTIONAL_DEPENDENCIES}
)
