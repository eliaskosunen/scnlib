function (generate_bloat_test_files template outdir sources)
    set(bloat_test_main "${CMAKE_CURRENT_LIST_DIR}/${outdir}/bloat_test_main.cpp")
    set(bloat_test_header "${CMAKE_CURRENT_LIST_DIR}/${outdir}/bloat_test_all.h")

    file(WRITE ${bloat_test_main} "#include \"bloat_test_all.h\"\n")
    file(APPEND ${bloat_test_main} "int main() {\n")

    set(source_files_list ${bloat_test_main} ${bloat_test_header})

    foreach (i RANGE 24)
        set(output_file "${CMAKE_CURRENT_LIST_DIR}/${outdir}/bloat_test_tmp_${i}.cpp")
        execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/process-bloat-source.sh ${template} ${i} ${output_file})
        list(APPEND source_files_list ${output_file})

        file(APPEND ${bloat_test_main} "do_scan${i}();\n")
        file(APPEND ${bloat_test_header} "void do_scan${i}();\n")
    endforeach()

    file(APPEND ${bloat_test_main} "return 0;\n}")
    set(${sources} ${source_files_list} PARENT_SCOPE)
endfunction()

# control
generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/control-template.cpp" out/control bloat-control-sources)
add_executable(bloat-control ${bloat-control-sources})

# iostream
generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/iostream-template.cpp" out/iostream bloat-iostream-sources)
add_executable(bloat-iostream ${bloat-iostream-sources})

# scanf
generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scanf-template.cpp" out/scanf bloat-scanf-sources)
add_executable(bloat-scanf ${bloat-scanf-sources})

# scnlib
generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scnlib-template.cpp" out/scnlib bloat-scnlib-sources)
add_executable(bloat-scnlib ${bloat-scnlib-sources})
target_link_libraries(bloat-scnlib scn)

# scnlib (header-only)
add_executable(bloat-scnlib-header-only ${bloat-scnlib-sources})
target_link_libraries(bloat-scnlib-header-only scn-header-only)

# scnlib (erased)
generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scnlib-erased-template.cpp" out/scnlib-erased bloat-scnlib-erased-sources)
add_executable(bloat-erased-scnlib ${bloat-scnlib-erased-sources})
target_link_libraries(bloat-erased-scnlib scn)

# scnlib (erased, header-only)
add_executable(bloat-scnlib-erased-header-only ${bloat-scnlib-erased-sources})
target_link_libraries(bloat-scnlib-erased-header-only scn-header-only)

if (SCN_HAS_RANGES)
    # scnlib + range-v3
    generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scnlib-ranges-template.cpp" out/scnlib-ranges bloat-scnlib-ranges-sources)
    add_executable(bloat-scnlib-ranges ${bloat-scnlib-ranges-sources})
    target_link_libraries(bloat-scnlib-ranges scn-ranges)

    # scnlib + range-v3 (header-only)
    add_executable(bloat-scnlib-ranges-header-only ${bloat-scnlib-ranges-sources})
    target_link_libraries(bloat-scnlib-ranges-header-only scn-ranges-header-only)
endif ()

add_custom_target(bloat-prepare ALL
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_LIST_DIR}/bloat-test.py" "${CMAKE_CURRENT_LIST_DIR}/run-bloat-tests.py"
    "${CMAKE_BINARY_DIR}/benchmark/bloat"
    COMMENT "Copying bloat test scripts")
