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

find_program (BASH bash)
function (generate_bloat_test target)
    add_test(NAME ${target} COMMAND ${BASH} ${CMAKE_CURRENT_LIST_DIR}/bloat-test.sh ${target})
endfunction ()

generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/control_template.cpp" out/control bloat-control-sources)
add_executable(bloat-control ${bloat-control-sources})
generate_bloat_test(bloat-control)

generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/iostream_template.cpp" out/iostream bloat-iostream-sources)
add_executable(bloat-iostream ${bloat-iostream-sources})
generate_bloat_test(bloat-iostream)

generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scanf_template.cpp" out/scanf bloat-scanf-sources)
add_executable(bloat-scanf ${bloat-scanf-sources})
generate_bloat_test(bloat-scanf)

generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scanf_string_template.cpp" out/scanf-string bloat-scanf-string-sources)
add_executable(bloat-scanf-string ${bloat-scanf-string-sources})
generate_bloat_test(bloat-scanf-string)

generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scnlib_template.cpp" out/scnlib bloat-scnlib-sources)
add_executable(bloat-scnlib ${bloat-scnlib-sources})
target_link_libraries(bloat-scnlib scn)
generate_bloat_test(bloat-scnlib)

add_executable(bloat-scnlib-header-only ${bloat-scnlib-sources})
target_link_libraries(bloat-scnlib-header-only scn-header-only)
generate_bloat_test(bloat-scnlib-header-only)

if (SCN_HAS_RANGES)
    generate_bloat_test_files("${CMAKE_CURRENT_LIST_DIR}/scnlib_ranges_template.cpp" out/scnlib-ranges bloat-scnlib-ranges-sources)
    add_executable(bloat-scnlib-ranges ${bloat-scnlib-ranges-sources})
    target_link_libraries(bloat-scnlib-ranges scn-ranges)
    generate_bloat_test(bloat-scnlib-ranges)

    add_executable(bloat-scnlib-ranges-header-only ${bloat-scnlib-ranges-sources})
    target_link_libraries(bloat-scnlib-ranges-header-only scn-ranges-header-only)
    generate_bloat_test(bloat-scnlib-ranges-header-only)
endif ()
