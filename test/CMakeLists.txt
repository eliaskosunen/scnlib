message(STATUS "Installing doctest submodule")
execute_process(COMMAND git submodule update --init -- doctest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include(CTest)

add_library(tests-base INTERFACE)
target_include_directories(tests-base SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest)
if (NOT SCN_USE_EXCEPTIONS)
    target_compile_definitions(tests-base INTERFACE -DDOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS=1)
endif()

add_executable(tests
    test.cpp empty.cpp string_view.cpp
    integer.cpp floating.cpp boolean.cpp string.cpp
    result.cpp usertype.cpp istream.cpp buffer.cpp
    small_vector.cpp)
target_link_libraries(tests scn tests-base)
set_private_flags(tests)
add_test(NAME tests COMMAND tests)

if (SCN_HAS_RANGES)
    add_executable(tests-ranges
        ranges.cpp)
    target_link_libraries(tests-ranges scn-ranges tests-base)
    set_private_flags(tests-ranges)
    add_test(NAME ranges COMMAND tests-ranges)
endif ()
