message(STATUS "Installing doctest submodule")
execute_process(COMMAND git submodule update --init -- doctest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include(CTest)

add_library(tests-base INTERFACE)
target_include_directories(tests-base SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/doctest/doctest)
if (NOT SCN_USE_EXCEPTIONS)
    target_compile_definitions(tests-base INTERFACE -DDOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS=1)
endif()
target_link_libraries(tests-base INTERFACE scn-sanitizers)

function (make_test test source)
    add_executable(test-${test} ${source})
    target_link_libraries(test-${test} PRIVATE scn tests-base)
    set_private_flags(test-${test})
    add_test(NAME ${test} COMMAND test-${test})
endfunction()

make_test(test test.cpp)
make_test(empty empty.cpp)

make_test(util util.cpp)
make_test(small-vector small_vector.cpp)
make_test(string-view string_view.cpp)
make_test(until-space until_space.cpp)
make_test(locale locale.cpp)
make_test(stream stream.cpp)

make_test(result result.cpp)
make_test(istream istream.cpp)
make_test(tuple-return tuple_return.cpp)

make_test(char char.cpp)
make_test(integer integer.cpp)
make_test(float floating.cpp)
make_test(string string.cpp)
make_test(buffer buffer.cpp)
make_test(usertype usertype.cpp)

if (SCN_BUILD_LOCALE_TESTS)
    make_test(locale_scan locale_scan.cpp)
endif ()

add_subdirectory(each)

if (SCN_HAS_RANGES)
    add_subdirectory(ranges)
endif ()

if (SCN_BUILD_FUZZING)
    add_subdirectory(fuzz)
endif ()
