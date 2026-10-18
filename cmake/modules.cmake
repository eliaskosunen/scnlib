if(NOT SCN_MODULES)
    return()
endif()

set(SCN_USE_CMAKE_MODULES OFF)
if(
    CMAKE_VERSION VERSION_GREATER_EQUAL 3.28
    AND CMAKE_GENERATOR STREQUAL "Ninja"
)
    set(SCN_USE_CMAKE_MODULES ON)
endif()

# CMake can scan clang-cl sources for modules only since 4.4, and the manual
# module build below only knows the GNU-style clang command line
if(
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
    AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"
    AND (NOT SCN_USE_CMAKE_MODULES OR CMAKE_VERSION VERSION_LESS 4.4)
)
    message(
        FATAL_ERROR
        "SCN_MODULES with clang-cl requires CMake 4.4 or newer and the Ninja generator "
        "(found CMake ${CMAKE_VERSION} and the ${CMAKE_GENERATOR} generator)"
    )
endif()

function(target_uses_extensions target out)
    get_target_property(target_property ${target} CXX_EXTENSIONS)
    if(
        target_property
        AND NOT (target_property STREQUAL "target_property-NOTFOUND")
    )
        set(${out} ON PARENT_SCOPE)
        return()
    endif()

    if(DEFINED CMAKE_CXX_EXTENSIONS)
        set(${out} ${CMAKE_CXX_EXTENSIONS} PARENT_SCOPE)
        return()
    endif()
    if(DEFINED CMAKE_CXX_EXTENSIONS_DEFAULT)
        set(${out} ${CMAKE_CXX_EXTENSIONS_DEFAULT} PARENT_SCOPE)
        return()
    endif()
    set(${out} ON PARENT_SCOPE)
endfunction()

function(add_module_library target)
    set(sources ${ARGN})

    add_library(${target})
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)

    if(SCN_USE_CMAKE_MODULES)
        target_sources(
            ${target}
            PUBLIC FILE_SET scn TYPE CXX_MODULES FILES ${sources}
        )
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PUBLIC -fmodules-ts)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        get_target_property(std ${target} CXX_STANDARD)
        if(NOT std)
            set(std ${CMAKE_CXX_STANDARD})
        endif()
        # Modules require at least C++20, which target_compile_features sets above
        if(NOT std OR std LESS 20)
            set(std 20)
        endif()
        target_uses_extensions(${target} stdext)
        if(stdext)
            set(std_flag "gnu++${std}")
        else()
            set(std_flag "c++${std}")
        endif()

        list(
            TRANSFORM SCN_CONFIG_DEFINITIONS
            PREPEND "-D"
            OUTPUT_VARIABLE definition_flags
        )
        # Importers don't get SCN_DISABLE_EXCEPTIONS_FLAGS, and clang rejects
        # importing a module file built with a different exception setting
        set(private_flags ${SCN_LIBRARY_PRIVATE_FLAGS})
        list(REMOVE_ITEM private_flags ${SCN_DISABLE_EXCEPTIONS_FLAGS})

        set(pcms)
        foreach(src ${sources})
            get_filename_component(pcm_name ${src} NAME_WE)
            set(pcm ${pcm_name}.pcm)

            target_compile_options(
                ${target}
                PUBLIC
                    -fmodule-file=${pcm_name}=${CMAKE_CURRENT_BINARY_DIR}/${pcm}
            )

            set(pcms ${pcms} ${CMAKE_CURRENT_BINARY_DIR}/${pcm})
            add_custom_command(
                OUTPUT ${pcm}
                COMMAND
                    ${CMAKE_CXX_COMPILER} --std=${std_flag} -x c++-module
                    --precompile -o ${pcm} ${CMAKE_CURRENT_SOURCE_DIR}/${src}
                    "-I$<JOIN:$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,;-I>"
                    "${SCN_LIBRARY_PUBLIC_FLAGS}" "${private_flags}"
                    "${definition_flags}"
                COMMAND_EXPAND_LISTS
                DEPENDS ${src}
            )
        endforeach()

        set(sources)
        foreach(pcm ${pcms})
            get_filename_component(pcm_we ${pcm} NAME_WE)
            set(obj ${pcm_we}.o)
            set(sources ${sources} ${CMAKE_CURRENT_BINARY_DIR}/${obj})

            add_custom_command(
                OUTPUT ${obj}
                COMMAND
                    ${CMAKE_CXX_COMPILER}
                    $<TARGET_PROPERTY:${target},COMPILE_OPTIONS> -c -o ${obj}
                    ${pcm}
                COMMAND_EXPAND_LISTS
                DEPENDS ${pcm}
            )
        endforeach()
    endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

    target_sources(${target} PRIVATE ${sources})

    if(MSVC)
        if(CMAKE_GENERATOR STREQUAL "Ninja")
            file(
                RELATIVE_PATH
                BMI_DIR
                "${CMAKE_BINARY_DIR}"
                "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target}.dir"
            )
        else()
            set(BMI_DIR "${CMAKE_CURRENT_BINARY_DIR}")
        endif()
        file(TO_NATIVE_PATH "${BMI_DIR}/${target}.ifc" BMI)

        target_compile_options(
            ${target}
            PRIVATE /interface /ifcOutput ${BMI}
            INTERFACE /reference scn=${BMI}
        )
        set_target_properties(
            ${target}
            PROPERTIES ADDITIONAL_CLEAN_FILES ${BMI}
        )
        set_source_files_properties(${BMI} PROPERTIES GENERATED ON)
    endif(MSVC)
endfunction()
