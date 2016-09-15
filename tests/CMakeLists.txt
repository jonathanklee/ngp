cmake_minimum_required(VERSION 3.0)

set(SRC_TESTS
    main.c
    )

set(HEADER_TESTS
    minunit.h
    )

add_executable(tests ${SRC_TESTS} ${HEADERS_TESTS})

include_directories(../src)

find_package(Curses REQUIRED)

# config
find_path(LIBCONFIG_INCLUDE_DIR libconfig.h)
find_library(LIBCONFIG_LIBRARY libconfig.so)

# pcre
find_path(LIBPCRE_INCLUDE_DIR libpcre.h)
find_library(LIBPCRE_LIBRARY libpcre.so)

# headers
include_directories(${CURSES_INCLUDE_DIRS})

# libs
target_link_libraries(tests ${CURSES_LIBRARIES} ${LIBPCRE_LIBRARY} ${LIBCONFIG_LIBRARY} objects)

