find_package(benchmark QUIET)

if (NOT benchmark_FOUND)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "")
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE INTERNAL "")

    add_subdirectory(google-benchmark EXCLUDE_FROM_ALL)
endif ()

if (SCN_USE_32BIT)
    target_compile_options(benchmark PUBLIC -m32)
    set_target_properties(benchmark PROPERTIES LINK_FLAGS -m32)
endif ()

add_subdirectory(runtime)

if (SCN_BUILD_BUILDTIME)
    add_subdirectory(buildtime)
endif ()

if (SCN_BUILD_BLOAT)
    add_subdirectory(bloat)
endif ()
