option(SCN_USE_IOSTREAMS "Use iostreams" ON)

option(SCN_CI "Enable CI preset" OFF)
if (SCN_IS_TOP_PROJECT)
    set(SCN_ENABLE_EXTRAS ON)
elseif(SCN_CI)
    set(SCN_ENABLE_EXTRAS ON)
else()
    set(SCN_ENABLE_EXTRAS OFF)
endif()

option(SCN_TESTS "Enable tests" ${SCN_ENABLE_EXTRAS})
option(SCN_DOCS "Enable docs target" ${SCN_ENABLE_EXTRAS})
option(SCN_EXAMPLES "Enable examples target" ${SCN_ENABLE_EXTRAS})
option(SCN_INSTALL "Enable install target" ON)

option(SCN_BENCHMARKS "Enable runtime benchmarks" ${SCN_ENABLE_EXTRAS})
option(SCN_BENCHMARKS_BUILDTIME "Enable buildtime benchmarks" ${SCN_ENABLE_EXTRAS})
option(SCN_BENCHMARKS_BINARYSIZE "Enable binary size benchmarks" ${SCN_ENABLE_EXTRAS})

option(SCN_FUZZING "Enable fuzz tests (clang only)" OFF)
option(SCN_COVERAGE "Enable coverage reporting" OFF)

option(SCN_PEDANTIC "Enable pedantic compilation flags" ${SCN_ENABLE_EXTRAS})
option(SCN_WERROR "Halt compilation in case of a warning" ${SCN_ENABLE_EXTRAS})

option(SCN_USE_32BIT "Compile as 32-bit (gcc or clang only)" OFF)
option(SCN_USE_EXCEPTIONS "Compile with exception support (disabling will cause test failures)" ON)
option(SCN_USE_RTTI "Compile with RTTI (run-time type information) support" ON)

option(SCN_USE_NATIVE_ARCH "Add -march=native to build flags (gcc or clang only)" OFF)
option(SCN_USE_HASWELL_ARCH "Add -march=haswell to build flags (gcc or clang only)" OFF)

option(SCN_USE_ASAN "Compile with AddressSanitizer (clang only)" OFF)
option(SCN_USE_UBSAN "Compile with UndefinedBehaviorSanitizer (clang only)" OFF)
option(SCN_USE_MSAN "Compile with MemorySanitizer (clang only)" OFF)
