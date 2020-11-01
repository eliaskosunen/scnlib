make_test(file file.cpp)

add_custom_target(test-file-prepare ALL
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_LIST_DIR}/testfile.txt"
        "${CMAKE_BINARY_DIR}/test/file")
