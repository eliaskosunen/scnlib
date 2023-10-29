function(get_gcc_warning_flags flags)
    set(${flags}
            -ftemplate-backtrace-limit=0
            -Wall -Wextra -Wpedantic
            -pedantic-errors
            -Wconversion -Wsign-conversion
            -Wold-style-cast -Wfloat-equal
            -Wlogical-op
            -Wredundant-decls -Wshadow
            -Wwrite-strings
            -Wpointer-arith -Wcast-qual
            -Wformat=2 -Wswitch-default
            -Wmissing-include-dirs -Wcast-align
            -Wswitch-enum -Wnon-virtual-dtor
            -Wctor-dtor-privacy -Wdisabled-optimization
            -Winvalid-pch -Wnoexcept
            -Wmissing-declarations -Woverloaded-virtual
            -Wno-psabi
            $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,5.0>:
            -Wdouble-promotion -Wtrampolines
            -Wzero-as-null-pointer-constant
            -Wuseless-cast -Wvector-operation-performance>
            $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,6.0>:
            -Wshift-overflow=2 -Wnull-dereference
            -Wduplicated-cond>
            $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,7.0>:
            -Walloc-zero -Walloca
            -Wduplicated-branches>
            $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,8.0>:
            -Wcast-align=strict>
            $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,9.0>:
            -Wmismatched-tags -Wredundant-tags>
            $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,10.0>:
            -fconcepts-diagnostics-depth=99>
            $<$<VERSION_GREATER_EQUAL:CXX_COMPILER_VERSION,13.0>:
            -Wundef>
            PARENT_SCOPE
    )
endfunction()

function(get_clang_warning_flags flags)
    set(tmp
            -Weverything
            -Wno-padded
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++98-compat-bind-to-temporary-copy
            -Wno-c++98-compat-local-type-template-args
            -Qunused-arguments -fcolor-diagnostics)

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        set(tmp ${tmp} -Wno-unsafe-buffer-usage)
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 17.0)
        set(tmp ${tmp} -fsafe-buffer-usage-suggestions)
    endif ()

    set(${flags} ${tmp} PARENT_SCOPE)
endfunction()

function(get_msvc_warning_flags flags)
    set(${flags}
            /W3
            /wd4324 # padding
            /permissive-
            PARENT_SCOPE)
endfunction()

function(get_warning_flags flags)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_gcc_warning_flags(flags)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            get_msvc_warning_flags(flags)
        else ()
            get_clang_warning_flags(flags)
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        get_msvc_warning_flags(flags)
    endif ()
endfunction()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(SCN_CXX_FRONTEND "GNU")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(SCN_CXX_FRONTEND "MSVC")
    else ()
        set(SCN_CXX_FRONTEND "GNU")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(SCN_CXX_FRONTEND "MSVC")
else ()
    set(SCN_CXX_FRONTEND "Other")
endif ()

function(get_werror_flags flags)
    set(${flags}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},GNU>:
            -Werror>
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            /WX>

            PARENT_SCOPE)
endfunction()

function(get_suppress_warnings_flags flags)
    set(${flags}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},GNU>:
            -w>
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            /w>

            PARENT_SCOPE)
endfunction()

function(get_disable_exceptions_flags flags)
    set(${flags}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},GNU>:
            -fno-exceptions>
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            /EHs-c->

            PARENT_SCOPE)
endfunction()

function(get_disable_rtti_flags flags)
    set(${flags}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},GNU>:
            -fno-rtti>
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            /GR->

            PARENT_SCOPE)
endfunction()

function(get_coverage_flags flags)
    set(${flags}
            $<$<CXX_COMPILER_ID:GNU>:
            -O0
            -g
            --coverage>
            PARENT_SCOPE)
endfunction()

function(disable_msvc_secure_flags target scope)
    target_compile_definitions(${target} ${scope}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS>)
endfunction()

function(set_bigobj_flags target scope)
    target_compile_options(${target} ${scope}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            /bigobj>)

    if (MINGW)
        target_compile_options(${target} ${scope} -Wa,-mbig-obj)
    endif ()
endfunction()

function(set_interface_flags target)
    if (SCN_PEDANTIC)
        get_warning_flags(warning_flags)
        target_compile_options(${target} INTERFACE ${warning_flags})
    endif ()
    if (SCN_WERROR)
        get_werror_flags(werror_flags)
        target_compile_options(${target} INTERFACE ${werror_flags})
    endif ()
    if (SCN_COVERAGE)
        get_coverage_flags(coverage_flags)
        target_compile_options(${target} INTERFACE ${coverage_flags})
        if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
            target_link_options(${target} INTERFACE --coverage)
        else ()
            target_link_libraries(${target} INTERFACE --coverage)
        endif ()
    endif ()
    disable_msvc_secure_flags(${target} INTERFACE)
    set_bigobj_flags(${target} INTERFACE)
    target_compile_features(${target} INTERFACE cxx_std_11)
endfunction()

function(set_private_flags target)
    if (SCN_PEDANTIC)
        get_warning_flags(warning_flags)
        target_compile_options(${target} PRIVATE ${warning_flags})
    endif ()
    if (SCN_WERROR)
        get_werror_flags(werror_flags)
        target_compile_options(${target} PRIVATE ${werror_flags})
    endif ()
    if (SCN_USE_32BIT)
        target_compile_options(${target} PUBLIC -m32)
        set_target_properties(${target} PROPERTIES LINK_FLAGS -m32)
    endif ()
    if (SCN_USE_NATIVE_ARCH)
        target_compile_options(${target} PRIVATE -march=native)
    endif ()
    if (NOT SCN_USE_EXCEPTIONS)
        get_disable_exceptions_flags(noexceptions_flags)
        target_compile_options(${target} PRIVATE ${noexceptions_flags})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND SCN_CXX_FRONTEND STREQUAL "MSVC")
        # clang-cl requires explicitly enabling exceptions
        target_compile_options(${target} PUBLIC /EHsc)
    endif ()
    if (NOT SCN_USE_RTTI)
        get_disable_rtti_flags(nortti_flags)
        target_compile_options(${target} PRIVATE ${nortti_flags})
    endif ()
    if (SCN_COVERAGE)
        get_coverage_flags(coverage_flags)
        target_compile_options(${target} PUBLIC ${coverage_flags})
        if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
            target_link_options(${target} PUBLIC --coverage)
        else ()
            target_link_libraries(${target} PUBLIC --coverage)
        endif ()
    endif ()

    disable_msvc_secure_flags(${target} PRIVATE)
    set_bigobj_flags(${target} PRIVATE)

    target_compile_features(${target} PUBLIC cxx_std_11)
endfunction()
