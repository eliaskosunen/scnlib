include(CheckCXXCompilerFlag)

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

# Appends the flags given after `out_var` to it, if the compiler accepts them.
# -Wno-X is checked as -WX, because GCC accepts -Wno-X for any X.
function(scn_append_supported_flags out_var)
    set(result ${${out_var}})
    foreach (flag IN LISTS ARGN)
        string(REGEX REPLACE "^-Wno-" "-W" checked_flag "${flag}")
        string(MAKE_C_IDENTIFIER "SCN_HAS_FLAG${checked_flag}" flag_variable)
        check_cxx_compiler_flag(${checked_flag} ${flag_variable})
        if (${flag_variable})
            list(APPEND result ${flag})
        endif ()
    endforeach ()
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

# Options passed on to the code as a definition (OPTION=1), when ON
set(SCN_CONFIG_OPTIONS
        SCN_DISABLE_TYPE_SCHAR
        SCN_DISABLE_TYPE_SHORT
        SCN_DISABLE_TYPE_INT
        SCN_DISABLE_TYPE_LONG
        SCN_DISABLE_TYPE_LONG_LONG
        SCN_DISABLE_TYPE_INT128
        SCN_DISABLE_TYPE_UCHAR
        SCN_DISABLE_TYPE_USHORT
        SCN_DISABLE_TYPE_UINT
        SCN_DISABLE_TYPE_ULONG
        SCN_DISABLE_TYPE_ULONG_LONG
        SCN_DISABLE_TYPE_UINT128
        SCN_DISABLE_TYPE_POINTER
        SCN_DISABLE_TYPE_BOOL
        SCN_DISABLE_TYPE_CHAR
        SCN_DISABLE_TYPE_CHAR32
        SCN_DISABLE_TYPE_FLOAT
        SCN_DISABLE_TYPE_DOUBLE
        SCN_DISABLE_TYPE_LONG_DOUBLE
        SCN_DISABLE_TYPE_FLOAT16
        SCN_DISABLE_TYPE_FLOAT32
        SCN_DISABLE_TYPE_FLOAT64
        SCN_DISABLE_TYPE_FLOAT128
        SCN_DISABLE_TYPE_BFLOAT16
        SCN_DISABLE_TYPE_STRING
        SCN_DISABLE_TYPE_STRING_VIEW
        SCN_DISABLE_TYPE_CUSTOM
        SCN_DISABLE_FAST_FLOAT
        SCN_DISABLE_EKNAN
        SCN_DISABLE_FROM_CHARS
        SCN_DISABLE_STRTOD
        SCN_DISABLE_IOSTREAM
        SCN_DISABLE_LOCALE
        SCN_DISABLE_CHRONO
)

set(SCN_CONFIG_DEFINITIONS)
foreach (option IN LISTS SCN_CONFIG_OPTIONS)
    if (${option})
        list(APPEND SCN_CONFIG_DEFINITIONS ${option}=1)
    endif ()
endforeach ()

if (SCN_DISABLE_REGEX)
    list(APPEND SCN_CONFIG_DEFINITIONS SCN_DISABLE_REGEX=1)
elseif (SCN_REGEX_BACKEND STREQUAL "Boost")
    list(APPEND SCN_CONFIG_DEFINITIONS SCN_REGEX_BACKEND=1)
    if (SCN_REGEX_BOOST_USE_ICU)
        list(APPEND SCN_CONFIG_DEFINITIONS SCN_REGEX_BOOST_USE_ICU=1)
    endif ()
elseif (SCN_REGEX_BACKEND STREQUAL "re2")
    list(APPEND SCN_CONFIG_DEFINITIONS SCN_REGEX_BACKEND=2)
endif ()

if (NOT SCN_USE_EXCEPTIONS)
    list(APPEND SCN_CONFIG_DEFINITIONS SCN_USE_EXCEPTIONS=0)
endif ()

set(SCN_MSVC_SECURE_DEFINITIONS)
if (SCN_CXX_FRONTEND STREQUAL "MSVC")
    set(SCN_MSVC_SECURE_DEFINITIONS
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS
    )
endif ()

set(SCN_WARNING_FLAGS)
if (SCN_PEDANTIC)
    if (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(SCN_WARNING_FLAGS
                /W3
                /wd4324 # padding
                /permissive-
        )
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        scn_append_supported_flags(SCN_WARNING_FLAGS
                -ftemplate-backtrace-limit=0
                -fconcepts-diagnostics-depth=99
                -fdiagnostics-all-candidates
                -Wall -Wextra -Wpedantic
                -Wconversion -Wsign-conversion
                -Wold-style-cast -Wfloat-equal -Wdouble-promotion
                -Wuseless-cast -Wzero-as-null-pointer-constant
                -Wlogical-op -Wduplicated-cond -Wduplicated-branches
                -Wredundant-decls -Wshadow
                -Wwrite-strings
                -Wpointer-arith -Wcast-qual -Wcast-align -Wcast-align=strict
                -Wformat=2 -Wswitch-default -Wswitch-enum
                -Wmissing-include-dirs
                -Wnon-virtual-dtor -Wctor-dtor-privacy
                -Wdisabled-optimization -Winvalid-pch -Wnoexcept
                -Wmissing-declarations -Woverloaded-virtual
                -Wmismatched-tags -Wredundant-tags
                -Wshift-overflow=2 -Wnull-dereference
                -Wtrampolines -Walloca -Wvector-operation-performance
                -Wno-psabi
        )
        # Before GCC 13, #pragma GCC diagnostic can't disable -Wundef in C++
        # (GCC PR53431), which impl.h needs to do
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
            list(APPEND SCN_WARNING_FLAGS -Wundef)
        endif ()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        scn_append_supported_flags(SCN_WARNING_FLAGS
                -ftemplate-backtrace-limit=0
                -fsafe-buffer-usage-suggestions
                -Qunused-arguments -fcolor-diagnostics
                -Weverything
                -Wno-padded
                -Wno-c++98-compat
                -Wno-c++98-compat-pedantic
                -Wno-c++98-compat-bind-to-temporary-copy
                -Wno-c++98-compat-local-type-template-args
                -Wno-unsafe-buffer-usage
                -Wno-nrvo
                # Clang 23: suggestions to add [[clang::lifetimebound]], and
                # false positives for using a reference to a string after
                # modifying it
                -Wno-lifetime-safety-suggestions
                -Wno-lifetime-safety-invalidation
        )
    endif ()
endif ()

set(SCN_WERROR_FLAGS)
if (SCN_WERROR)
    if (SCN_CXX_FRONTEND STREQUAL "GNU")
        set(SCN_WERROR_FLAGS -Werror -pedantic-errors)
    elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
        set(SCN_WERROR_FLAGS /WX)
    endif ()
endif ()

set(SCN_BIGOBJ_FLAGS)
if (SCN_CXX_FRONTEND STREQUAL "MSVC")
    set(SCN_BIGOBJ_FLAGS /bigobj)
elseif (MINGW)
    set(SCN_BIGOBJ_FLAGS -Wa,-mbig-obj)
endif ()

# In CI, disable warnings about deprecated stuff on MSVC,
# because seemingly the pragmas can't disable it all
# (we still want to test deprecated features)
set(SCN_CI_FLAGS)
if (SCN_CI)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(SCN_CI_FLAGS /wd4996)
    elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
        # clang-cl
        set(SCN_CI_FLAGS -Wno-error=deprecated-declarations)
    endif ()
endif ()

set(SCN_DISABLE_EXCEPTIONS_FLAGS)
set(SCN_DISABLE_EXCEPTIONS_DEFINITIONS)
if (SCN_CXX_FRONTEND STREQUAL "GNU")
    set(SCN_DISABLE_EXCEPTIONS_FLAGS -fno-exceptions)
elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
    set(SCN_DISABLE_EXCEPTIONS_FLAGS /EHs-c-)
    # Without it, the MSVC STL still uses exceptions, warning about it (C4530)
    set(SCN_DISABLE_EXCEPTIONS_DEFINITIONS _HAS_EXCEPTIONS=0)
endif ()

# Flags for the library target (scn), also used for the module (scn_module)
set(SCN_LIBRARY_PUBLIC_FLAGS ${SCN_CI_FLAGS})
set(SCN_LIBRARY_PRIVATE_FLAGS
        ${SCN_WARNING_FLAGS}
        ${SCN_WERROR_FLAGS}
        ${SCN_BIGOBJ_FLAGS}
)
set(SCN_LIBRARY_LINK_FLAGS)

if (SCN_USE_32BIT)
    list(APPEND SCN_LIBRARY_PUBLIC_FLAGS -m32)
    list(APPEND SCN_LIBRARY_LINK_FLAGS -m32)
endif ()

if (SCN_USE_NATIVE_ARCH)
    list(APPEND SCN_LIBRARY_PRIVATE_FLAGS -march=native)
elseif (SCN_USE_HASWELL_ARCH)
    list(APPEND SCN_LIBRARY_PRIVATE_FLAGS -march=haswell)
endif ()

if (NOT SCN_USE_EXCEPTIONS)
    list(APPEND SCN_LIBRARY_PRIVATE_FLAGS ${SCN_DISABLE_EXCEPTIONS_FLAGS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND SCN_CXX_FRONTEND STREQUAL "MSVC")
    # clang-cl requires explicitly enabling exceptions
    list(APPEND SCN_LIBRARY_PUBLIC_FLAGS /EHsc)
endif ()

if (NOT SCN_USE_RTTI)
    if (SCN_CXX_FRONTEND STREQUAL "GNU")
        list(APPEND SCN_LIBRARY_PRIVATE_FLAGS -fno-rtti)
    elseif (SCN_CXX_FRONTEND STREQUAL "MSVC")
        list(APPEND SCN_LIBRARY_PRIVATE_FLAGS /GR-)
    endif ()
endif ()

if (SCN_COVERAGE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND SCN_LIBRARY_PUBLIC_FLAGS -O0 -g --coverage)
    list(APPEND SCN_LIBRARY_LINK_FLAGS --coverage)
endif ()

function(set_library_flags target)
    target_compile_options(${target}
            PUBLIC ${SCN_LIBRARY_PUBLIC_FLAGS}
            PRIVATE ${SCN_LIBRARY_PRIVATE_FLAGS})
    target_compile_definitions(${target}
            PUBLIC ${SCN_CONFIG_DEFINITIONS}
            PRIVATE ${SCN_MSVC_SECURE_DEFINITIONS})
    if (NOT SCN_USE_EXCEPTIONS)
        target_compile_definitions(${target} PRIVATE ${SCN_DISABLE_EXCEPTIONS_DEFINITIONS})
    endif ()
    target_link_options(${target} PUBLIC ${SCN_LIBRARY_LINK_FLAGS})
    target_compile_features(${target} PUBLIC cxx_std_17)
endfunction()

# For targets using the library internals, like the tests
function(set_interface_flags target)
    target_compile_options(${target} INTERFACE
            ${SCN_WARNING_FLAGS}
            ${SCN_WERROR_FLAGS}
            ${SCN_BIGOBJ_FLAGS}
            ${SCN_CI_FLAGS})
    if (NOT SCN_USE_EXCEPTIONS)
        target_compile_options(${target} INTERFACE ${SCN_DISABLE_EXCEPTIONS_FLAGS})
        target_compile_definitions(${target} INTERFACE ${SCN_DISABLE_EXCEPTIONS_DEFINITIONS})
    endif ()
    target_compile_definitions(${target} INTERFACE
            ${SCN_CONFIG_DEFINITIONS}
            ${SCN_MSVC_SECURE_DEFINITIONS})
    target_compile_features(${target} INTERFACE cxx_std_17)
endfunction()
