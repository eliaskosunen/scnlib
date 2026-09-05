if (NOT DEFINED CROSS_TRIPLE)
    message(FATAL_ERROR "CROSS_TRIPLE must be defined (e.g., -DCROSS_TRIPLE=aarch64-linux-gnu)")
endif()

if (NOT DEFINED CROSS_SYSTEM_PROCESSOR)
    message(FATAL_ERROR "CROSS_SYSTEM_PROCESSOR must be defined (e.g., -DCROSS_SYSTEM_PROCESSOR=aarch64)")
endif()

# Cache these variables so they're available in try_compile() invocations
set(CROSS_TRIPLE "${CROSS_TRIPLE}" CACHE STRING "GCC cross-compiler triple")
set(CROSS_SYSTEM_PROCESSOR "${CROSS_SYSTEM_PROCESSOR}" CACHE STRING "Target processor architecture")

# Tell CMake to pass these variables to try_compile()
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CROSS_TRIPLE CROSS_SYSTEM_PROCESSOR)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "${CROSS_SYSTEM_PROCESSOR}")

set(CMAKE_C_COMPILER "${CROSS_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${CROSS_TRIPLE}-g++")

# Don't search for programs in the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Only search for libraries, includes, and packages in the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
