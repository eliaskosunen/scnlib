if (SCN_COVERAGE)
    add_custom_target(scn_coverage_script_prepare ALL
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_LIST_DIR}/coverage.sh"
            "${CMAKE_BINARY_DIR}"
            COMMENT "Copying coverage script")
endif ()
