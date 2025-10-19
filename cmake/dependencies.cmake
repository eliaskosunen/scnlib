include(FetchContent)

set(SCN_DEPENDENCIES_TO_MAKE_AVAILABLE "")

if (CMAKE_FIND_PACKAGE_SORT_ORDER)
    set(OLD_CMAKE_FIND_PACKAGE_SORT_ORDER "${CMAKE_FIND_PACKAGE_SORT_ORDER}")
endif ()
set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)

if (SCN_TESTS)
    # GTest

    if (SCN_USE_EXTERNAL_GTEST)
        if ((NOT TARGET GTest::gtest_main) OR (NOT TARGET GTest::gmock_main))
            find_package(GTest CONFIG REQUIRED)
        else ()
            message(STATUS "GTest targets already defined, not doing find_package(GTest) (SCN_USE_EXTERNAL_GTEST is ON)")
        endif ()

        set(SCN_GTEST_LIBRARIES
                GTest::gtest_main
                GTest::gmock_main
        )
    elseif (TARGET GTest::gtest_main AND TARGET GTest::gmock_main)
        message(WARNING "GTest targets already defined, not doing FetchContent(googletest), even though SCN_USE_EXTERNAL_GTEST is OFF")
        set(SCN_GTEST_LIBRARIES
                GTest::gtest_main
                GTest::gmock_main
        )
    elseif (CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
        # FetchContent_Populate(<name>) deprecated,
        # use MakeAvailable instead

        FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG main
                GIT_SHALLOW TRUE
                SYSTEM
                EXCLUDE_FROM_ALL
        )

        set(gtest_force_shared_crt ON CACHE INTERNAL "")
        set(INSTALL_GTEST OFF CACHE INTERNAL "")
        list(APPEND SCN_DEPENDENCIES_TO_MAKE_AVAILABLE "googletest")
        set(SCN_GTEST_LIBRARIES
                GTest::gtest_main
                GTest::gmock_main
        )
    else ()
        # gtest CMake does some flag overriding we don't want, and it's also quite heavy
        # Do it manually

        FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG main
                GIT_SHALLOW TRUE
        )

        set(gtest_force_shared_crt ON CACHE INTERNAL "")
        set(INSTALL_GTEST OFF CACHE INTERNAL "")

        FetchContent_GetProperties(googletest)
        if (NOT googletest)
            FetchContent_Populate(googletest)
        endif ()

        if (NOT TARGET Threads::Threads)
            find_package(Threads)
        endif ()

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
        if (NOT TARGET benchmark::benchmark)
            find_package(benchmark CONFIG REQUIRED)
        else ()
            message(STATUS "Target benchmark::benchmark already defined, not doing find_package(benchmark) (SCN_USE_EXTERNAL_BENCHMARK is ON)")
        endif ()
    elseif (TARGET benchmark::benchmark)
        message(WARNING "Target benchmark::benchmark already defined, not doing FetchContent(benchmark), even though SCN_USE_EXTERNAL_BENCHMARK is OFF")
    else ()
        set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "Turn off google benchmark tests")
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "Turn off google benchmark install")
        set(BENCHMARK_ENABLE_WERROR OFF CACHE INTERNAL "Turn off -Werror inside google benchmark")
        FetchContent_Declare(
                google-benchmark
                GIT_REPOSITORY https://github.com/google/benchmark.git
                GIT_TAG v1.9.4
                GIT_SHALLOW TRUE
        )
        list(APPEND SCN_DEPENDENCIES_TO_MAKE_AVAILABLE "google-benchmark")
    endif ()
endif ()

# fast_float

if (SCN_DISABLE_FAST_FLOAT)
    message(STATUS "FastFloat disabled, using <charconv> / std::from_chars instead")
    try_compile(
            SCN_CHARCONV_SUCCESS
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/charconv_compile_test.cpp"
            CXX_STANDARD_REQUIRED 17
    )
    if (NOT SCN_CHARCONV_SUCCESS)
        message(FATAL_ERROR
                "FastFloat is disabled, but your stdlib doesn't support std::from_chars with floating-point values")
    endif ()
    set(SCN_FAST_FLOAT_TARGET)
elseif (SCN_USE_EXTERNAL_FAST_FLOAT)
    if (NOT TARGET FastFloat::fast_float)
        find_package(FastFloat CONFIG REQUIRED)
        if (FastFloat_VERSION VERSION_LESS "5.0.0")
            message(FATAL_ERROR "Incompatible version of FastFloat: at least 5.0.0 required, found ${FastFloat_VERSION}")
        endif ()
    else ()
        message(STATUS "Target FastFloat::fast_float already defined, not doing find_package(FastFloat) (SCN_USE_EXTERNAL_FAST_FLOAT is ON)")
    endif ()
    set(SCN_FAST_FLOAT_TARGET FastFloat::fast_float)
elseif (TARGET FastFloat::fast_float)
    message(WARNING "Target FastFloat::fast_float already defined, not doing FetchContent(fast_float), even though SCN_USE_EXTERNAL_FAST_FLOAT is OFF")
elseif (CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
    FetchContent_Declare(
            fast_float
            GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
            GIT_TAG v8.0.2
            GIT_SHALLOW TRUE
            SYSTEM
            EXCLUDE_FROM_ALL
    )

    set(FASTFLOAT_INSTALL OFF CACHE INTERNAL "")
    list(APPEND SCN_DEPENDENCIES_TO_MAKE_AVAILABLE "fast_float")
    set(SCN_FAST_FLOAT_TARGET FastFloat::fast_float)
else ()
    FetchContent_Declare(
            fast_float
            GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
            GIT_TAG v8.0.2
            GIT_SHALLOW TRUE
    )

    cmake_policy(SET CMP0077 NEW)
    set(FASTFLOAT_INSTALL OFF CACHE INTERNAL "")

    FetchContent_GetProperties(fast_float)
    if (NOT fast_float_POPULATED)
        FetchContent_Populate(fast_float)

        add_subdirectory(${fast_float_SOURCE_DIR} ${fast_float_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif ()

    set(SCN_FAST_FLOAT_TARGET FastFloat::fast_float)
endif ()

# std::regex

if (SCN_REGEX_BACKEND STREQUAL "std")
    if (NOT SCN_USE_EXTERNAL_REGEX_BACKEND)
        message(FATAL_ERROR "SCN_USE_EXTERNAL_REGEX_BACKEND=OFF isn't supported when SCN_REGEX_BACKEND is std")
    endif ()
    if (SCN_REGEX_BOOST_USE_ICU)
        message(FATAL_ERROR "SCN_REGEX_BOOST_USE_ICU isn't supported when SCN_REGEX_BACKEND is std")
    endif ()

    set(SCN_REGEX_BACKEND_TARGET)
endif ()

# Boost.Regex

if (SCN_REGEX_BACKEND STREQUAL "Boost")
    if (NOT SCN_USE_EXTERNAL_REGEX_BACKEND)
        message(FATAL_ERROR "SCN_USE_EXTERNAL_REGEX_BACKEND=OFF isn't supported when SCN_REGEX_BACKEND is Boost")
    endif ()

    if (NOT TARGET Boost::regex)
        cmake_policy(PUSH)
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
            # CMake v3.30 has deprecated find_package(Boost) (using a built-in FindBoost.cmake),
            # instead preferring find_package(Boost CONFIG),
            # which would use the BoostConfig.cmake file shipped with Boost.
            # However, that file appeared in Boost v1.70, which was released in 2019,
            # and we want to support Boost versions older than that.
            # For that reason, we're explicitly opting-in to not using that,
            # while it's still possible.
            cmake_policy(SET CMP0167 OLD)
        endif ()
        find_package(Boost REQUIRED COMPONENTS regex)
        cmake_policy(POP)
    else ()
        message(STATUS "Target Boost::regex already defined, not doing find_package(Boost COMPONENTS regex)")
    endif ()

    if (NOT SCN_REGEX_BOOST_USE_ICU)
        set(SCN_REGEX_BACKEND_TARGET Boost::regex)
    else ()
        if (TARGET Boost::regex_icu)
            set(SCN_REGEX_BACKEND_TARGET Boost::regex_icu)
        else ()
            # Boost::regex_icu not defined, do it manually
            if ((NOT TARGET ICU::data) OR (NOT TARGET ICU::i18n) OR (NOT TARGET ICU::uc))
                find_package(ICU REQUIRED COMPONENTS data i18n uc)
            else ()
                message(STATUS "ICU targets already defined, not doing find_package(ICU)")
            endif ()

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

    if (NOT TARGET re2::re2)
        find_package(re2 REQUIRED)
        if (re2_VERSION VERSION_LESS 11.0.0)
            message(FATAL_ERROR "Incompatible version of re2: at least 11.0.0 required, found ${re2_VERSION}")
        endif ()
    else ()
        message(STATUS "Target re2::re2 already defined, not doing find_package(re2)")
    endif ()

    set(SCN_REGEX_BACKEND_TARGET re2::re2)
endif ()

# make available

FetchContent_MakeAvailable(
        ${SCN_DEPENDENCIES_TO_MAKE_AVAILABLE}
)

if (OLD_CMAKE_FIND_PACKAGE_SORT_ORDER)
    set(CMAKE_FIND_PACKAGE_SORT_ORDER "${OLD_CMAKE_FIND_PACKAGE_SORT_ORDER}")
else ()
    set(CMAKE_FIND_PACKAGE_SORT_ORDER NONE) # default value
endif ()
