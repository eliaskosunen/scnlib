add_custom_target(buildtime-prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/empty.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/cstdio.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/iostream.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/scnlib.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/run-buildtime-tests.sh"
        "${CMAKE_BINARY_DIR}/benchmark/buildtime"
        COMMENT "Copying build time benchmark scripts")
