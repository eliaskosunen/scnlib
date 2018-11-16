add_subdirectory(expected-lite/expected-lite)
add_subdirectory(span-lite/span-lite)

add_library(scn INTERFACE)
target_include_directories(scn INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(scn INTERFACE
    expected-lite span-lite)
target_compile_features(scn INTERFACE
    cxx_nullptr)