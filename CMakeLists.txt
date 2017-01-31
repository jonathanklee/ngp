cmake_minimum_required(VERSION 3.0)

project(ngp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

add_subdirectory(src)
add_subdirectory(tests)

enable_testing()

add_test(UNITTESTS, tests/tests)

if(UNIX AND NOT APPLE)
    install(FILES ngp.1 DESTINATION /usr/share/man/man1)
endif()
