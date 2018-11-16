cmake_minimum_required(VERSION 3.2.3)

project(scn)

add_subdirectory(include/scn)

add_executable(scn_test main.cpp)
target_link_libraries(scn_test PUBLIC scn)