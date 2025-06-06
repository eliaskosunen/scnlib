cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
project(scn-consumer-module CXX)

find_package(scn CONFIG REQUIRED)

add_executable(scn-consumer-module main.cpp)
target_link_libraries(scn-consumer-module PRIVATE scn::scn_module)
