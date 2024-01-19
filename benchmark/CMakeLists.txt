if (SCN_BENCHMARKS)
    add_subdirectory(runtime)
endif()

if (SCN_BENCHMARKS_BINARYSIZE)
    add_subdirectory(binarysize)
endif()

if (SCN_BENCHMARKS_BUILDTIME)
    add_subdirectory(buildtime)
endif()
