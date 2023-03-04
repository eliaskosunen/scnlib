function(generate_binarysize_test_files template outdir sources)
    set(binarysize_test_path "${CMAKE_CURRENT_LIST_DIR}/${outdir}/")
    set(binarysize_test_main "${binarysize_test_path}/binarysize_test_main.cpp")
    set(binarysize_test_header "${binarysize_test_path}/binarysize_test_all.h")

    file(WRITE "${binarysize_test_main}" "#include \"binarysize_test_all.h\"\n")
    file(APPEND "${binarysize_test_main}" "int main() {\n")

    set(source_files_list "${binarysize_test_main}" "${binarysize_test_header}")

    foreach (i RANGE 16)
        set(output_file "${binarysize_test_path}/binarysize_test_tmp_${i}.cpp")

        file(READ "${template}" template_contents)
        string(REPLACE "do_scan" "do_scan_${i}" template_contents "${template_contents}")
        string(REPLACE "42" "${i}" template_contents "${template_contents}")
        file(WRITE "${output_file}" "${template_contents}")

        list(APPEND source_files_list "${output_file}")

        file(APPEND "${binarysize_test_main}" "do_scan_${i}();\n")
        file(APPEND "${binarysize_test_header}" "void do_scan_${i}();\n")
    endforeach()

    file(APPEND "${binarysize_test_main}" "return 0;\n}\n")
    set(${sources} "${source_files_list}" PARENT_SCOPE)
endfunction()

add_library(scn_benchmark_binarysize_base INTERFACE)
target_compile_options(scn_benchmark_binarysize_base INTERFACE
        $<$<CXX_COMPILER_ID:GNU>:
        -Wno-unused
        -Wno-unused-but-set-variable
        -Wno-redundant-decls
        -Wno-missing-declarations>
        $<$<CXX_COMPILER_ID:Clang>:
        -Wno-unused-variable
        -Wno-missing-prototypes>
        $<$<CXX_COMPILER_ID:MSVC>:
        /D_CRT_SECURE_NO_WARNINGS>)

# control
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-control.cpp" out/control binarysize_control_sources)
add_executable(scn_benchmark_binarysize_control "${binarysize_control_sources}")
target_link_libraries(scn_benchmark_binarysize_control scn_benchmark_binarysize_base)

# iostream
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-iostream.cpp" out/iostream binarysize_iostream_sources)
add_executable(scn_benchmark_binarysize_iostream "${binarysize_iostream_sources}")
target_link_libraries(scn_benchmark_binarysize_iostream scn_benchmark_binarysize_base)

# scanf
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-scanf.cpp" out/scanf binarysize_scanf_sources)
add_executable(scn_benchmark_binarysize_scanf "${binarysize_scanf_sources}")
target_link_libraries(scn_benchmark_binarysize_scanf scn_benchmark_binarysize_base)

# scnlib
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-scnlib.cpp" out/scnlib binarysize_scnlib_sources)
add_executable(scn_benchmark_binarysize_scnlib "${binarysize_scnlib_sources}")
target_link_libraries(scn_benchmark_binarysize_scnlib scn_internal scn_benchmark_binarysize_base)

add_custom_target(scn_benchmark_binarysize_prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/binarysize_bench.py"
        "${CMAKE_CURRENT_LIST_DIR}/run_binarysize_bench.py"
        "${CMAKE_BINARY_DIR}/benchmark/binarysize"
        COMMENT "Copying binary size benchmark scripts")
