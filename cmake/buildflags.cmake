function(get_config_flags flags)
    if (SCN_DISABLE_REGEX)
        set(regex_flag -DSCN_DISABLE_REGEX=1)
    else ()
        if (SCN_REGEX_BACKEND STREQUAL "std")
            set(regex_flag -DSCN_REGEX_BACKEND=0)
        elseif (SCN_REGEX_BACKEND STREQUAL "Boost")
            set(regex_flag
                    -DSCN_REGEX_BACKEND=1
                    $<$<BOOL:${SCN_REGEX_BOOST_USE_ICU}>: -DSCN_REGEX_BOOST_USE_ICU=1>
            )
        elseif (SCN_REGEX_BACKEND STREQUAL "re2")
            set(regex_flag -DSCN_REGEX_BACKEND=2)
        endif ()
    endif ()
    set(${flags}
            $<$<BOOL:${SCN_DISABLE_TYPE_SCHAR}>: -DSCN_DISABLE_TYPE_SCHAR=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_SHORT}>: -DSCN_DISABLE_TYPE_SHORT=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_INT}>: -DSCN_DISABLE_TYPE_INT=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_LONG}>: -DSCN_DISABLE_TYPE_LONG=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_LONG_LONG}>: -DSCN_DISABLE_TYPE_LONG_LONG=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_INT128}>: -DSCN_DISABLE_TYPE_INT128=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_UCHAR}>: -DSCN_DISABLE_TYPE_UCHAR=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_USHORT}>: -DSCN_DISABLE_TYPE_USHORT=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_UINT}>: -DSCN_DISABLE_TYPE_UINT=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_ULONG}>: -DSCN_DISABLE_TYPE_ULONG=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_ULONG_LONG}>: -DSCN_DISABLE_TYPE_ULONG_LONG=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_UINT128}>: -DSCN_DISABLE_TYPE_UINT128=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_POINTER}>: -DSCN_DISABLE_TYPE_POINTER=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_BOOL}>: -DSCN_DISABLE_TYPE_BOOL=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_CHAR}>: -DSCN_DISABLE_TYPE_CHAR=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_CHAR32}>: -DSCN_DISABLE_TYPE_CHAR32=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_FLOAT}>: -DSCN_DISABLE_TYPE_FLOAT=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_DOUBLE}>: -DSCN_DISABLE_TYPE_DOUBLE=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_LONG_DOUBLE}>:-DSCN_DISABLE_TYPE_LONG_DOUBLE=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_FLOAT16}>:-DSCN_DISABLE_TYPE_FLOAT16=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_FLOAT32}>:-DSCN_DISABLE_TYPE_FLOAT32=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_FLOAT64}>:-DSCN_DISABLE_TYPE_FLOAT64=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_FLOAT128}>:-DSCN_DISABLE_TYPE_FLOAT128=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_BFLOAT16}>:-DSCN_DISABLE_TYPE_BFLOAT16=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_STRING}>: -DSCN_DISABLE_TYPE_STRING=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_STRING_VIEW}>:-DSCN_DISABLE_TYPE_STRING_VIEW=1>
            $<$<BOOL:${SCN_DISABLE_TYPE_CUSTOM}>: -DSCN_DISABLE_TYPE_CUSTOM=1>

            $<$<BOOL:${SCN_DISABLE_FROM_CHARS}>: -DSCN_DISABLE_FROM_CHARS=1>
            $<$<BOOL:${SCN_DISABLE_STRTOD}>: -DSCN_DISABLE_STRTOD=1>

            $<$<BOOL:${SCN_DISABLE_IOSTREAM}>: -DSCN_DISABLE_IOSTREAM=1>
            $<$<BOOL:${SCN_DISABLE_LOCALE}>: -DSCN_DISABLE_LOCALE=1>

            ${regex_flag}
            PARENT_SCOPE
    )
endfunction()

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
        get_gcc_warning_flags(flag_list)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            get_msvc_warning_flags(flag_list)
        else ()
            get_clang_warning_flags(flag_list)
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        get_msvc_warning_flags(flag_list)
    endif ()
    set(${flags} ${flag_list} PARENT_SCOPE)
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

function(get_disable_msvc_secure_definitions defs)
    set(${defs}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS>
            PARENT_SCOPE)
endfunction()

function(get_bigobj_flags flags)
    set(${flags}
            $<$<STREQUAL:${SCN_CXX_FRONTEND},MSVC>:
            /bigobj>
            $<$<BOOL:${MINGW}>:-Wa,-mbig-obj>
            PARENT_SCOPE
    )
endfunction()

function(get_interface_flags prefix)
    set(flags_list)
    set(definitions_list)

    if (SCN_PEDANTIC)
        get_warning_flags(warning_flags)
        set(flags_list ${flags_list} ${warning_flags})
    endif ()
    if (SCN_WERROR)
        get_werror_flags(werror_flags)
        set(flags_list ${flags_list} ${werror_flags})
    endif ()

    get_config_flags(config_flags)
    get_bigobj_flags(bigobj_flags)
    set(flags_list ${flags_list} ${config_flags} ${bigobj_flags})

    get_disable_msvc_secure_definitions(msvc_sec_defs)
    set(definitions_list ${definitions_list} ${msvc_sec_defs})

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
        set(list_private_flags ${list_private_flags} ${warning_flags})
    endif ()
    if (SCN_WERROR)
        get_werror_flags(werror_flags)
        set(list_private_flags ${list_private_flags} ${werror_flags})
    endif ()
    if (SCN_USE_32BIT)
        set(list_public_flags ${list_public_flags} -m32)
        set(list_public_link_flags ${list_public_link_flags} -m32)
    endif ()
    if (SCN_USE_NATIVE_ARCH)
        set(list_private_flags ${list_private_flags} -march=native)
    elseif (SCN_USE_HASWELL_ARCH)
        set(list_private_flags ${list_private_flags} -march=haswell)
    endif ()
    if (NOT SCN_USE_EXCEPTIONS)
        get_disable_exceptions_flags(noexceptions_flags)
        set(list_private_flags ${list_private_flags} ${noexceptions_flag})
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND SCN_CXX_FRONTEND STREQUAL "MSVC")
        # clang-cl requires explicitly enabling exceptions
        set(list_public_flags ${list_public_flags} /EHsc)
    endif ()
    if (NOT SCN_USE_RTTI)
        get_disable_rtti_flags(nortti_flags)
        set(list_private_flags ${list_private_flags} ${nortti_flags})
    endif ()
    if (SCN_COVERAGE)
        get_coverage_flags(coverage_flags)
        set(list_public_flags ${list_public_flags} ${coverage_flags})
        set(list_public_link_flags ${list_public_link_flags} --coverage)
    endif ()
    if (SCN_DISABLE_FAST_FLOAT)
        set(list_public_definitions ${list_public_definitions} -DSCN_DISABLE_FAST_FLOAT=1)
    endif ()

    get_config_flags(config_flags)
    set(list_public_definitions ${list_public_definitions} ${config_flags})

    get_disable_msvc_secure_definitions(msvc_sec_defs)
    set(list_private_definitions ${list_private_definitions} ${msvc_sec_defs})

    get_bigobj_flags(bigobj_flags)
    set(list_private_flags ${list_private_flags} ${bigobj_flags})

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
