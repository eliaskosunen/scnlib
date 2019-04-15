add_executable(example-1 example-1.cpp)
target_link_libraries(example-1 PRIVATE scn)
set_private_flags(example-1)

add_executable(example-2 example-2.cpp)
target_link_libraries(example-2 PRIVATE scn)
set_private_flags(example-2)

add_executable(example-3 example-3.cpp)
target_link_libraries(example-3 PRIVATE scn)
set_private_flags(example-3)
