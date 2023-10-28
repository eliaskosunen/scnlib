cmake_minimum_required(VERSION 3.14)

project(scn-consumer CXX)

find_package(scn CONFIG REQUIRED)

add_executable(scn-consumer main.cpp)
target_link_libraries(scn-consumer PRIVATE scn::scn)
