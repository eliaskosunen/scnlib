function(get_config_definitions defs)
    set(tmp)
    if (SCN_DISABLE_REGEX)
        list(APPEND tmp SCN_DISABLE_REGEX=1)
    else ()
        if (SCN_REGEX_BACKEND STREQUAL "std")
            list(APPEND tmp SCN_REGEX_BACKEND=0)
        elseif (SCN_REGEX_BACKEND STREQUAL "Boost")
            list(APPEND tmp
                    SCN_REGEX_BACKEND=1
                    $<$<BOOL:${SCN_REGEX_BOOST_USE_ICU}>: SCN_REGEX_BOOST_USE_ICU=1>
            )
        elseif (SCN_REGEX_BACKEND STREQUAL "re2")
            list(APPEND tmp SCN_REGEX_BACKEND=2)
        endif ()
    endif ()

    if (SCN_DISABLE_TYPE_SCHAR)
        list(APPEND tmp SCN_DISABLE_TYPE_SCHAR=1)
    endif ()
    if (SCN_DISABLE_TYPE_SHORT)
        list(APPEND tmp SCN_DISABLE_TYPE_SHORT=1)
    endif ()
    if (SCN_DISABLE_TYPE_INT)
        list(APPEND tmp SCN_DISABLE_TYPE_INT=1)
    endif ()
    if (SCN_DISABLE_TYPE_LONG)
        list(APPEND tmp SCN_DISABLE_TYPE_LONG=1)
    endif ()
    if (SCN_DISABLE_TYPE_LONG_LONG)
        list(APPEND tmp SCN_DISABLE_TYPE_LONG_LONG=1)
    endif ()
    if (SCN_DISABLE_TYPE_INT128)
        list(APPEND tmp SCN_DISABLE_TYPE_INT128=1)
    endif ()
    if (SCN_DISABLE_TYPE_UCHAR)
        list(APPEND tmp SCN_DISABLE_TYPE_UCHAR=1)
    endif ()
    if (SCN_DISABLE_TYPE_USHORT)
        list(APPEND tmp SCN_DISABLE_TYPE_USHORT=1)
    endif ()
    if (SCN_DISABLE_TYPE_UINT)
        list(APPEND tmp SCN_DISABLE_TYPE_UINT=1)
    endif ()
    if (SCN_DISABLE_TYPE_ULONG)
        list(APPEND tmp SCN_DISABLE_TYPE_ULONG=1)
    endif ()
    if (SCN_DISABLE_TYPE_ULONG_LONG)
        list(APPEND tmp SCN_DISABLE_TYPE_ULONG_LONG=1)
    endif ()
    if (SCN_DISABLE_TYPE_UINT128)
        list(APPEND tmp SCN_DISABLE_TYPE_UINT128=1)
    endif ()
    if (SCN_DISABLE_TYPE_POINTER)
        list(APPEND tmp SCN_DISABLE_TYPE_POINTER=1)
    endif ()
    if (SCN_DISABLE_TYPE_BOOL)
        list(APPEND tmp SCN_DISABLE_TYPE_BOOL=1)
    endif ()
    if (SCN_DISABLE_TYPE_CHAR)
        list(APPEND tmp SCN_DISABLE_TYPE_CHAR=1)
    endif ()
    if (SCN_DISABLE_TYPE_CHAR32)
        list(APPEND tmp SCN_DISABLE_TYPE_CHAR32=1)
    endif ()
    if (SCN_DISABLE_TYPE_FLOAT)
        list(APPEND tmp SCN_DISABLE_TYPE_FLOAT=1)
    endif ()
    if (SCN_DISABLE_TYPE_DOUBLE)
        list(APPEND tmp SCN_DISABLE_TYPE_DOUBLE=1)
    endif ()
    if (SCN_DISABLE_TYPE_LONG_DOUBLE)
        list(APPEND tmp SCN_DISABLE_TYPE_LONG_DOUBLE=1)
    endif ()
    if (SCN_DISABLE_TYPE_FLOAT16)
        list(APPEND tmp SCN_DISABLE_TYPE_FLOAT16=1)
    endif ()
    if (SCN_DISABLE_TYPE_FLOAT32)
        list(APPEND tmp SCN_DISABLE_TYPE_FLOAT32=1)
    endif ()
    if (SCN_DISABLE_TYPE_FLOAT64)
        list(APPEND tmp SCN_DISABLE_TYPE_FLOAT64=1)
    endif ()
    if (SCN_DISABLE_TYPE_FLOAT128)
        list(APPEND tmp SCN_DISABLE_TYPE_FLOAT128=1)
    endif ()
    if (SCN_DISABLE_TYPE_BFLOAT16)
        list(APPEND tmp SCN_DISABLE_TYPE_BFLOAT16=1)
    endif ()
    if (SCN_DISABLE_TYPE_STRING)
        list(APPEND tmp SCN_DISABLE_TYPE_STRING=1)
    endif ()
    if (SCN_DISABLE_TYPE_STRING_VIEW)
        list(APPEND tmp SCN_DISABLE_TYPE_STRING_VIEW=1)
    endif ()
    if (SCN_DISABLE_TYPE_CUSTOM)
        list(APPEND tmp SCN_DISABLE_TYPE_CUSTOM=1)
    endif ()

    if (SCN_DISABLE_FROM_CHARS)
        list(APPEND tmp SCN_DISABLE_FROM_CHARS=1)
    endif ()
    if (SCN_DISABLE_STRTOD)
        list(APPEND tmp SCN_DISABLE_STRTOD=1)
    endif ()
    if (SCN_DISABLE_IOSTREAM)
        list(APPEND tmp SCN_DISABLE_IOSTREAM=1)
    endif ()
    if (SCN_DISABLE_LOCALE)
        list(APPEND tmp SCN_DISABLE_LOCALE=1)
    endif ()

    set(${defs} ${tmp} PARENT_SCOPE)
endfunction()

function(get_gcc_warning_flags flags)
    set(tmp
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
    )

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.0)
        list(APPEND tmp
                -Wdouble-promotion -Wtrampolines
                -Wzero-as-null-pointer-constant
                -Wuseless-cast -Wvector-operation-performance
        )
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 6.0)
        list(APPEND tmp
                -Wshift-overflow=2 -Wnull-dereference
                -Wduplicated-cond
        )
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7.0)
        list(APPEND tmp
                -Walloca
                -Wduplicated-branches
        )
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
        list(APPEND tmp
                -Wcast-align=strict
        )
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0)
        list(APPEND tmp
                -fconcepts-diagnostics-depth=99
                -Wmismatched-tags -Wredundant-tags
        )
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
        list(APPEND tmp
                -Wundef
        )
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0)
        list(APPEND tmp
                #-Wnrvo
                -fdiagnostics-all-candidates
        )
    endif ()

    set(${flags} ${tmp} PARENT_SCOPE)
endfunction()

function(get_clang_warning_flags flags)
    set(tmp
            -ftemplate-backtrace-limit=0
            -Weverything
            -Wno-padded
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++98-compat-bind-to-temporary-copy
            -Wno-c++98-compat-local-type-template-args
            -Qunused-arguments -fcolor-diagnostics)

    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        list(APPEND tmp -Wno-unsafe-buffer-usage)
    endif ()
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 17.0)
        list(APPEND tmp -fsafe-buffer-usage-suggestions)
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

function(get_warning_flags flags)
    if (SCN_CXX_FRONTEND STREQUAL "MSVC")
        get_msvc_warning_flags(flag_list)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_gcc_warning_flags(flag_list)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        get_clang_warning_flags(flag_list)
    endif ()
    set(${flags} ${flag_list} PARENT_SCOPE)
endfunction()

function(get_werror_flags flags)
    if (SCN_CXX_FRONTEND STREQUAL "GNU")
        set(${flags} -Werror PARENT_SCOPE)
    elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(${flags} /WX PARENT_SCOPE)
    else ()
        set(${flags} PARENT_SCOPE)
    endif ()
endfunction()

function(get_suppress_warnings_flags flags)
    if (SCN_CXX_FRONTEND STREQUAL "GNU")
        set(${flags} -w PARENT_SCOPE)
    elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(${flags} /w PARENT_SCOPE)
    else ()
        set(${flags} PARENT_SCOPE)
    endif ()
endfunction()

function(get_disable_exceptions_flags flags)
    if (SCN_CXX_FRONTEND STREQUAL "GNU")
        set(${flags} -fno-exceptions PARENT_SCOPE)
    elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(${flags} /EHs-c- PARENT_SCOPE)
    else ()
        set(${flags} PARENT_SCOPE)
    endif ()
endfunction()

function(get_disable_rtti_flags flags)
    if (SCN_CXX_FRONTEND STREQUAL "GNU")
        set(${flags} -fno-rtti PARENT_SCOPE)
    elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(${flags} /GR- PARENT_SCOPE)
    else ()
        set(${flags} PARENT_SCOPE)
    endif ()
endfunction()

function(get_coverage_flags flags)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(${flags} -O0 -g --coverage PARENT_SCOPE)
    else ()
        set(${flags} PARENT_SCOPE)
    endif ()
endfunction()

function(get_disable_msvc_secure_definitions flags)
    if (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(${flags}
                _CRT_SECURE_NO_WARNINGS
                _SCL_SECURE_NO_WARNINGS
                PARENT_SCOPE)
    else ()
        set(${flags} PARENT_SCOPE)
    endif ()
endfunction()

function(get_bigobj_flags flags)
    if (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(${flags} /bigobj PARENT_SCOPE)
    elseif (MINGW)
        set(${flags} -Wa,-mbig-obj PARENT_SCOPE)
    else ()
        set(${flags} PARENT_SCOPE)
    endif ()
endfunction()

function(get_ci_flags flags)
    # In CI, disable warnings about deprecated stuff on MSVC,
    # because seemingly the pragmas can't disable it all
    # (we still want to test deprecated features)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
            CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # clang-cl
        set(${flags} -Wno-error=deprecated-declarations PARENT_SCOPE)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(${flags} /wd4996 PARENT_SCOPE)
    endif ()
endfunction()

function(get_interface_flags prefix)
    set(flags_list)
    set(definitions_list)

    if (SCN_PEDANTIC)
        get_warning_flags(warning_flags)
        list(APPEND flags_list ${warning_flags})
    endif ()
    if (SCN_WERROR)
        get_werror_flags(werror_flags)
        list(APPEND flags_list ${werror_flags})
    endif ()

    get_bigobj_flags(bigobj_flags)
    list(APPEND flags_list ${bigobj_flags})

    if (SCN_CI)
        get_ci_flags(ci_flags)
        list(APPEND flags_list ${ci_flags})
    endif ()

    get_config_definitions(config_defs)
    get_disable_msvc_secure_definitions(msvc_sec_defs)
    list(APPEND definitions_list ${config_defs} ${msvc_sec_defs})

    set(${prefix}_flags ${flags_list} PARENT_SCOPE)
    set(${prefix}_definitions ${definitions_list} PARENT_SCOPE)
endfunction()

function(set_interface_flags target)
    get_interface_flags(${target})
    target_compile_options(${target} INTERFACE ${${target}_flags})
    target_compile_definitions(${target} INTERFACE ${${target}_definitions})
    target_compile_features(${target} INTERFACE cxx_std_17)
endfunction()

function(get_library_flags prefix)
    set(list_public_flags)
    set(list_private_flags)
    set(list_public_definitions)
    set(list_private_definitions)
    set(list_public_link_flags)

    if (SCN_PEDANTIC)
        get_warning_flags(warning_flags)
        list(APPEND list_private_flags ${warning_flags})
    endif ()
    if (SCN_WERROR)
        get_werror_flags(werror_flags)
        list(APPEND list_private_flags ${werror_flags})
    endif ()
    if (SCN_USE_32BIT)
        list(APPEND list_public_flags -m32)
        list(APPEND list_public_link_flags -m32)
    endif ()
    if (SCN_USE_NATIVE_ARCH)
        list(APPEND list_private_flags -march=native)
    elseif (SCN_USE_HASWELL_ARCH)
        list(APPEND list_private_flags -march=haswell)
    endif ()
    if (NOT SCN_USE_EXCEPTIONS)
        get_disable_exceptions_flags(noexceptions_flags)
        list(APPEND list_private_flags ${noexceptions_flag})
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND SCN_CXX_FRONTEND STREQUAL "MSVC")
        # clang-cl requires explicitly enabling exceptions
        list(APPEND list_public_flags /EHsc)
    endif ()
    if (NOT SCN_USE_RTTI)
        get_disable_rtti_flags(nortti_flags)
        list(APPEND list_private_flags ${nortti_flags})
    endif ()
    if (SCN_COVERAGE)
        get_coverage_flags(coverage_flags)
        list(APPEND list_public_flags ${coverage_flags})
        list(APPEND list_public_link_flags --coverage)
    endif ()
    if (SCN_DISABLE_FAST_FLOAT)
        list(APPEND list_public_definitions SCN_DISABLE_FAST_FLOAT=1)
    endif ()

    get_config_definitions(config_defs)
    list(APPEND list_public_definitions ${config_defs})

    get_disable_msvc_secure_definitions(msvc_sec_defs)
    list(APPEND list_private_definitions ${msvc_sec_defs})

    get_bigobj_flags(bigobj_flags)
    list(APPEND list_private_flags ${bigobj_flags})

    if (SCN_CI)
        get_ci_flags(ci_flags)
        list(APPEND list_public_flags ${ci_flags})
    endif ()

    set(${prefix}_public_flags ${list_public_flags} PARENT_SCOPE)
    set(${prefix}_private_flags ${list_private_flags} PARENT_SCOPE)
    set(${prefix}_public_definitions ${list_public_definitions} PARENT_SCOPE)
    set(${prefix}_private_definitions ${list_private_definitions} PARENT_SCOPE)
    set(${prefix}_public_link_flags ${list_public_link_flags} PARENT_SCOPE)
endfunction()

function(set_library_flags target)
    get_library_flags(${target})
    target_compile_options(${target} PUBLIC ${${target}_public_flags} PRIVATE ${${target}_private_flags})
    target_compile_definitions(${target} PUBLIC ${${target}_public_definitions} PRIVATE ${${target}_private_definitions})
    target_link_options(${target} PUBLIC ${${target}_public_link_flags})
    target_compile_features(${target} PUBLIC cxx_std_17)
endfunction()
