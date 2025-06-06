if (NOT SCN_MODULES)
    return()
endif ()

set(SCN_USE_CMAKE_MODULES OFF)
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.28 AND CMAKE_GENERATOR STREQUAL "Ninja")
    set(SCN_USE_CMAKE_MODULES ON)
endif ()

function (target_uses_extensions target out)
    get_target_property(target_property ${target} CXX_EXTENSIONS)
    if (target_property AND NOT (target_property STREQUAL "target_property-NOTFOUND"))
        set(${out} ON PARENT_SCOPE)
        return()
    endif ()

    if (DEFINED CMAKE_CXX_EXTENSIONS)
        set(${out} ${CMAKE_CXX_EXTENSIONS} PARENT_SCOPE)
        return()
    endif ()
    if (DEFINED CMAKE_CXX_EXTENSIONS_DEFAULT)
        set(${out} ${CMAKE_CXX_EXTENSIONS_DEFAULT} PARENT_SCOPE)
        return()
    endif ()
    set(${out} ON PARENT_SCOPE)
endfunction ()

function (add_module_library target)
    set(sources ${ARGN})

    add_library(${target})
    target_compile_features(${target} PUBLIC cxx_std_20)
    set_target_properties(
            ${target} PROPERTIES
            LINKER_LANGUAGE CXX
    )

    if (SCN_USE_CMAKE_MODULES)
        target_sources(${target} PUBLIC FILE_SET scn TYPE CXX_MODULES FILES ${sources})
        return()
    endif ()

    if (CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(${target} PUBLIC -fmodules-ts)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        get_target_property(std ${target} CXX_STANDARD)
        target_uses_extensions(${target} stdext)
        if (stdext)
            set(std_flag "gnu++${std}")
        else ()
            set(std_flag "c++${std}")
        endif ()

        set(pcms)
        foreach (src ${sources})
            get_filename_component(pcm_name ${src} NAME_WE)
            set(pcm ${pcm_name}.pcm)

            get_library_flags(private_flags public_flags public_definitions public_link_flags)
            target_compile_options(${target} PUBLIC -fmodule-file=${pcm_name}=${CMAKE_CURRENT_BINARY_DIR}/${pcm})
            target_link_options(${target} PUBLIC ${public_link_flags})

            set(pcms ${pcms} ${CMAKE_CURRENT_BINARY_DIR}/${pcm})
            add_custom_command(
                    OUTPUT ${pcm}
                    COMMAND ${CMAKE_CXX_COMPILER}
                            --std=${std_flag} -x c++-module --precompile
                            -o ${pcm} ${CMAKE_CURRENT_SOURCE_DIR}/${src}
                            "-I$<JOIN:$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,;-I>"
                            "${public_flags}" "${private_flags}" "${public_definitions}"
                    COMMAND_EXPAND_LISTS
                    DEPENDS ${src}
            )
        endforeach ()

        set(sources)
        foreach (pcm ${pcms})
            get_filename_component(pcm_we ${pcm} NAME_WE)
            set(obj ${pcm_we}.o)
            set(sources ${sources} ${CMAKE_CURRENT_BINARY_DIR}/${obj})

            add_custom_command(
                    OUTPUT ${obj}
                    COMMAND ${CMAKE_CXX_COMPILER} $<TARGET_PROPERTY:${target},COMPILE_OPTIONS>
                            -c -o ${obj} ${pcm}
                    COMMAND_EXPAND_LISTS
                    DEPENDS ${pcm}
            )
        endforeach ()
    endif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

    target_sources(${target} PRIVATE ${sources})

    if (MSVC)
        if (CMAKE_GENERATOR STREQUAL "Ninja")
            file(RELATIVE_PATH BMI_DIR "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target}.dir")
        else ()
            set(BMI_DIR "${CMAKE_CURRENT_BINARY_DIR}")
        endif ()
        file(TO_NATIVE_PATH "${BMI_DIR}/${target}.ifc" BMI)

        target_compile_options(
                ${target}
                PRIVATE /interface /ifcOutput ${BMI}
                INTERFACE /reference scn=${BMI}
        )
        set_target_properties(${target} PROPERTIES ADDITIONAL_CLEAN_FILES ${BMI})
        set_source_files_properties(${BMI} PROPERTIES GENERATED ON)
    endif (MSVC)
endfunction ()
