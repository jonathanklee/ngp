cmake_minimum_required(VERSION 3.0)

project(ngp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-stringop-overflow")

add_subdirectory(src)
add_subdirectory(tests)

enable_testing()

add_test(UNITTESTS, tests/tests)

