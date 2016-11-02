cmake_minimum_required(VERSION 3.0)

project(ngp)

add_subdirectory(src)
add_subdirectory(tests)

enable_testing()

add_test(UNITTESTS, tests/tests)

if(UNIX AND NOT APPLE)
    install(FILES ngp.1 DESTINATION /usr/share/man/man1)
endif()
