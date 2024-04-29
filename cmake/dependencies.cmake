include(FetchContent)

set(SCN_OPTIONAL_DEPENDENCIES "")

set(OLD_CMAKE_FIND_PACKAGE_SORT_ORDER "${CMAKE_FIND_PACKAGE_SORT_ORDER}")
set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)

if (SCN_TESTS)
    # GTest

    if (SCN_USE_EXTERNAL_GTEST)
        find_package(GTest CONFIG REQUIRED)
        set(SCN_GTEST_LIBRARIES
            GTest::gtest_main
            GTest::gmock_main
        )
    else ()
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
        target_compile_features(scn_gtest PUBLIC cxx_std_17)
        target_compile_options(scn_gtest PRIVATE $<$<CXX_COMPILER_ID:GNU>: -Wno-psabi>)
        set(SCN_GTEST_LIBRARIES scn_gtest)
    endif ()
endif ()

if (SCN_BENCHMARKS)
    # Google Benchmark

    if (SCN_USE_EXTERNAL_BENCHMARK)
        find_package(benchmark CONFIG REQUIRED)
    else ()
        set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "Turn off google benchmark tests")
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "Turn off google benchmark install")
        FetchContent_Declare(
                google-benchmark
                GIT_REPOSITORY https://github.com/google/benchmark.git
                GIT_TAG v1.8.3
                GIT_SHALLOW TRUE
        )
        list(APPEND SCN_OPTIONAL_DEPENDENCIES "google-benchmark")
    endif ()
endif ()

# fast_float

if (SCN_USE_EXTERNAL_FAST_FLOAT)
    find_package(FastFloat CONFIG REQUIRED)
    if (FastFloat_VERSION VERSION_LESS 5.0.0)
        message(FATAL_ERROR "Incompatible version of FastFloat: at least 5.0.0 required, found ${FastFloat_VERSION}")
    endif ()
else ()
    FetchContent_Declare(
            fast_float
            GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
            GIT_TAG v6.1.1
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

# std::regex

if (SCN_REGEX_BACKEND STREQUAL "std")
    if (NOT SCN_USE_EXTERNAL_REGEX_BACKEND)
        message(FATAL_ERROR "SCN_USE_EXTERNAL_REGEX_BACKEND=OFF isn't supported when SCN_REGEX_BACKEND is std")
    endif ()
    if (SCN_REGEX_BOOST_USE_ICU)
        message(FATAL_ERROR "SCN_REGEX_BOOST_USE_ICU isn't supported when SCN_REGEX_BACKEND is std")
    endif ()
endif ()

# Boost.Regex

if (SCN_REGEX_BACKEND STREQUAL "Boost")
    if (NOT SCN_USE_EXTERNAL_REGEX_BACKEND)
        message(FATAL_ERROR "SCN_USE_EXTERNAL_REGEX_BACKEND=OFF isn't supported when SCN_REGEX_BACKEND is Boost")
    endif ()

    find_package(Boost REQUIRED COMPONENTS regex)
    if (NOT SCN_REGEX_BOOST_USE_ICU)
        set(SCN_REGEX_BACKEND_TARGET Boost::regex)
    else ()
        if (TARGET Boost::regex_icu)
            set(SCN_REGEX_BACKEND_TARGET Boost::regex_icu)
        else ()
            # Boost::regex_icu not defined, do it manually
            find_package(ICU REQUIRED COMPONENTS data i18n uc)
            set(SCN_REGEX_BACKEND_TARGET
                    Boost::regex ICU::data ICU::i18n ICU::uc
            )
        endif ()
    endif ()
endif ()

# re2

if (SCN_REGEX_BACKEND STREQUAL "re2")
    if (NOT SCN_USE_EXTERNAL_REGEX_BACKEND)
        message(FATAL_ERROR "SCN_USE_EXTERNAL_REGEX_BACKEND=OFF isn't supported when SCN_REGEX_BACKEND is re2")
    endif ()
    if (SCN_REGEX_BOOST_USE_ICU)
        message(FATAL_ERROR "SCN_REGEX_BOOST_USE_ICU isn't supported when SCN_REGEX_BACKEND is re2")
    endif ()

    find_package(re2 REQUIRED)
    if (re2_VERSION VERSION_LESS 11.0.0)
        message(FATAL_ERROR "Incompatible version of re2: at least 11.0.0 required, found ${re2_VERSION}")
    endif ()
    set(SCN_REGEX_BACKEND_TARGET re2::re2)
endif ()

# make available

FetchContent_MakeAvailable(
        ${SCN_OPTIONAL_DEPENDENCIES}
)

set(CMAKE_FIND_PACKAGE_SORT_ORDER "${OLD_CMAKE_FIND_PACKAGE_SORT_ORDER}")
