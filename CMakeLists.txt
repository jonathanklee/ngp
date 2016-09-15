cmake_minimum_required(VERSION 3.0)

project(ngp)

add_subdirectory(src)
add_subdirectory(tests)

enable_testing()

add_test(UNITTESTS, tests/tests)

