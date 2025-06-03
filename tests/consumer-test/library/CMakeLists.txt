cmake_minimum_required(VERSION 3.14)

project(scn-consumer-library CXX)

find_package(scn CONFIG REQUIRED)

add_executable(scn-consumer-library main.cpp)
target_link_libraries(scn-consumer-library PRIVATE scn::scn)
