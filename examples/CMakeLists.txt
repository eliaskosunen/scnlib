if (NOT SCN_EXAMPLES)
    return()
endif()

add_library(scn_examples_base INTERFACE)
target_link_libraries(scn_examples_base INTERFACE scn_sanitizers)

add_executable(scn_example_1 example_1.cpp)
target_link_libraries(scn_example_1 scn::scn scn_examples_base)

add_executable(scn_example_2 example_2.cpp)
target_link_libraries(scn_example_2 scn::scn scn_examples_base)

add_executable(scn_example_3 example_3.cpp)
target_link_libraries(scn_example_3 scn::scn scn_examples_base)

add_executable(scn_example_4 example_4.cpp)
target_link_libraries(scn_example_4 scn::scn scn_examples_base)
