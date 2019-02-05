message(STATUS "Installing doctest submodule")
execute_process(COMMAND git submodule update --init -- doctest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tests
    test.cpp empty.cpp string_view.cpp
    integer.cpp floating.cpp boolean.cpp string.cpp
    result.cpp usertype.cpp istream.cpp buffer.cpp)
target_link_libraries(tests scn)
target_include_directories(tests SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest)
