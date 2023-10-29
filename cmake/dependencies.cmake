include(FetchContent)

set(SCN_OPTIONAL_DEPENDENCIES "")

if (SCN_TESTS)
    # GTest

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY  https://github.com/google/googletest.git
            GIT_TAG         main
            GIT_SHALLOW     TRUE
    )

    # gtest CMake does some flag overriding we don't want, and it's also quite heavy
    # Do it manually

    FetchContent_GetProperties(googletest)
    if(NOT googletest)
        FetchContent_Populate(googletest)
    endif()

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
endif()

if (SCN_BENCHMARKS)
    # Google Benchmark

    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "Turn off google benchmark tests")
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "Turn off google benchmark install")
    FetchContent_Declare(
            google-benchmark
            GIT_REPOSITORY  https://github.com/google/benchmark.git
            GIT_TAG         main
            GIT_SHALLOW     TRUE
    )
    list(APPEND SCN_OPTIONAL_DEPENDENCIES "google-benchmark")
endif()

# simdutf

FetchContent_Declare(
        simdutf
        GIT_REPOSITORY  https://github.com/simdutf/simdutf.git
        GIT_TAG         v4.0.3
        GIT_SHALLOW     TRUE
)

# simdutf CMake includes tests if BUILD_TESTING is globally ON
# we don't want to include tests of dependencies, so we need to do some manual work

set(SIMDUTF_BENCHMARKS_BEFORE_SIMDUTF ${SIMDUTF_BENCHMARKS})
set(BUILD_TESTING_BEFORE_SIMDUTF ${BUILD_TESTING})

set(SIMDUTF_BENCHMARKS OFF)
set(BUILD_TESTING OFF)

FetchContent_GetProperties(simdutf)
if(NOT simdutf_POPULATED)
    FetchContent_Populate(simdutf)

    add_subdirectory(${simdutf_SOURCE_DIR} ${simdutf_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(SIMDUTF_BENCHMARKS ${SIMDUTF_BENCHMARKS_BEFORE_SIMDUTF})
set(BUILD_TESTING ${BUILD_TESTING_BEFORE_SIMDUTF})

# fast_float

set(FASTFLOAT_INSTALL_BEFORE_INCLUDE ${FASTFLOAT_INSTALL})
set(FASTFLOAT_INSTALL OFF)
FetchContent_Declare(
        fast_float
        GIT_REPOSITORY  https://github.com/fastfloat/fast_float.git
        GIT_TAG         v5.2.0
        GIT_SHALLOW     TRUE
)

# make available

FetchContent_MakeAvailable(
        fast_float
        ${SCN_OPTIONAL_DEPENDENCIES}
)
set(FASTFLOAT_INSTALL ${FASTFLOAT_INSTALL_BEFORE_INCLUDE})
