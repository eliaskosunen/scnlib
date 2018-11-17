set(DOCTEST_WITH_TESTS OFF)
set(DOCTEST_NO_INSTALL ON)
add_subdirectory(doctest EXCLUDE_FROM_ALL)

add_executable(tests test.cpp)
target_link_libraries(tests PUBLIC scn doctest)