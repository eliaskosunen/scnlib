add_executable(tests-ranges
    ranges.cpp)
target_link_libraries(tests-ranges PRIVATE scn-ranges tests-base)
set_private_flags(tests-ranges)
add_test(NAME ranges COMMAND tests-ranges)
