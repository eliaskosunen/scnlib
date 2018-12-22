add_executable(tests test.cpp empty.cpp string_view.cpp)
target_link_libraries(tests scn)
target_include_directories(tests SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest)
target_compile_options(tests PRIVATE 
    $<$<CXX_COMPILER_ID:Clang>:
        -Weverything
        -Wpedantic -pedantic-errors
        -fvisibility=hidden -fstrict-aliasing
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-c++98-compat-bind-to-temporary-copy
        -Wno-c++98-compat-local-type-template-args>
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall -Wextra -Wpedantic
        -pedantic-errors
        -fvisibility=hidden -fstrict-aliasing
        -Wconversion -Wsign-conversion
        -Wold-style-cast -Wfloat-equal
        -Wlogical-op -Wundef
        -Wredundant-decls -Wshadow
        -Wwrite-strings
        -Wpointer-arith -Wcast-qual
        -Wformat=2 -Wswitch-default
        -Wmissing-include-dirs -Wcast-align
        -Wswitch-enum -Wnon-virtual-dtor
        -Wctor-dtor-privacy -Wdisabled-optimization
        -Winvalid-pch -Wnoexcept
        -Wmissing-declarations -Woverloaded-virtual
        $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,5.0>>:
        -Wdouble-promotion -Wtrampolines
        -Wzero-as-null-pointer-constant
        -Wuseless-cast -Wvector-operation-performance>
        $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,6.0>>:
        -Wshift-overflow=2 -Wnull-deference
        -Wduplicated-cond>
        $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,7.0>>:
        -Walloc-zero -Walloca
        -Wducplicated-branches>
        $<$<NOT:$<VERSION_LESS:CXX_COMPILER_VERSION,8.0>>:
        -Wcast-align=strict>
        >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /std:c++latest /permissive->)
