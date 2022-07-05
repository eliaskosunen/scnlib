function(generate_binarysize_test_files template outdir sources)
    set(binarysize_test_path "${CMAKE_CURRENT_LIST_DIR}/${outdir}/")
    set(binarysize_test_main "${binarysize_test_path}/binarysize_test_main.cpp")
    set(binarysize_test_header "${binarysize_test_path}/binarysize_test_all.h")

    file(WRITE "${binarysize_test_main}" "#include \"binarysize_test_all.h\"\n")
    file(APPEND "${binarysize_test_main}" "int main() {\n")

    set(source_files_list "${binarysize_test_main}" "${binarysize_test_header}")

    foreach (i RANGE 16)
        set(output_file "${binarysize_test_path}/binarysize_test_tmp_${i}.cpp")
        execute_process(COMMAND
                "${CMAKE_CURRENT_LIST_DIR}/process-binarysize-source.sh"
                "${template}" "${i}" "${output_file}")
        list(APPEND source_files_list "${output_file}")

        file(APPEND "${binarysize_test_main}" "do_scan_${i}();\n")
        file(APPEND "${binarysize_test_header}" "void do_scan_${i}();\n")
    endforeach()

    file(APPEND "${binarysize_test_main}" "return 0;\n}\n")
    set(${sources} "${source_files_list}" PARENT_SCOPE)
endfunction()

# control
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-control.cpp" out/control binarysize_control_sources)
add_executable(scn_benchmark_binarysize_control "${binarysize_control_sources}")

# iostream
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-iostream.cpp" out/iostream binarysize_iostream_sources)
add_executable(scn_benchmark_binarysize_iostream "${binarysize_iostream_sources}")

# scanf
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-scanf.cpp" out/scanf binarysize_scanf_sources)
add_executable(scn_benchmark_binarysize_scanf "${binarysize_scanf_sources}")

# scnlib
generate_binarysize_test_files("${CMAKE_CURRENT_LIST_DIR}/template-scnlib.cpp" out/scnlib binarysize_scnlib_sources)
add_executable(scn_benchmark_binarysize_scnlib "${binarysize_scnlib_sources}")
target_link_libraries(scn_benchmark_binarysize_scnlib scn::scn)

add_custom_target(scn_benchmark_binarysize_prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/binarysize_bench.py"
        "${CMAKE_CURRENT_LIST_DIR}/run_binarysize_bench.py"
        "${CMAKE_BINARY_DIR}/benchmark/binarysize"
        COMMENT "Copying binary size benchmark scripts")
