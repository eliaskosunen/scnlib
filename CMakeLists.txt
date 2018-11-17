cmake_minimum_required(VERSION 3.4)

project(scn)

option(SCN_HEADER_ONLY "Build scnlib as a header-only library" OFF)

add_subdirectory(include/scn/expected-lite/expected-lite)
add_subdirectory(include/scn/span-lite/span-lite)

if (SCN_HEADER_ONLY)
    add_library(scn INTERFACE)
    target_include_directories(scn INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src>"
        "$<INSTALL_INTERFACE:include;src>")
    target_link_libraries(scn INTERFACE
        expected-lite span-lite)
    target_compile_features(scn INTERFACE
        cxx_nullptr)
    target_compile_definitions(scn INTERFACE
        -DSCN_HEADER_ONLY=1)
else ()
    add_library(scn
        src/locale.cpp src/util.cpp)
    target_include_directories(scn PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(scn PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:src>)
    target_link_libraries(scn PUBLIC
        expected-lite span-lite)
    target_compile_features(scn PUBLIC
        cxx_nullptr)
    target_compile_definitions(scn PUBLIC
        -DSCN_HEADER_ONLY=0)
endif ()

add_subdirectory(test)