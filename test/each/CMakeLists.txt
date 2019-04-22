add_executable(tests-each-integer
    integer_each.cpp)
target_link_libraries(tests-each-integer PRIVATE scn tests-base)
set_private_flags(tests-each-integer)
add_test(NAME each-integer COMMAND tests-each-integer)

#add_executable(tests-each-float
#    float_each.cpp)
#target_link_libraries(tests-each-float PRIVATE scn tests-base)
#set_private_flags(tests-each-float)
#add_test(NAME each-float COMMAND tests-each-float)

add_executable(tests-each-char
    char_each.cpp)
target_link_libraries(tests-each-char PRIVATE scn tests-base)
set_private_flags(tests-each-char)
add_test(NAME each-char COMMAND tests-each-char)

