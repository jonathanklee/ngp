cmake_minimum_required(VERSION 3.0)

set(SRC_TESTS
    main.c
    )

set(HEADER_TESTS
    minunit.h
    )

add_executable(tests ${SRC_TESTS} ${HEADERS_TESTS})

include_directories(../src)

# config
find_package(PkgConfig REQUIRED)

# ncurses
pkg_check_modules(CURSES REQUIRED ncurses)

# libconfig
pkg_check_modules(LIBCONFIG REQUIRED libconfig>=1.0)
find_library(LIBCONFIG_LIBRARY NAMES config libconfig HINTS ${LIBCONFIG_LIBDIR})

# pcre
pkg_check_modules(LIBPCRE REQUIRED libpcre>=1.0)
find_library(LIBPCRE_LIBRARY NAMES pcre libpcre HINTS ${LIBPCRE_LIBDIR})

# headers
include_directories(${CURSES_INCLUDE_DIRS} ${LIBCONFIG_INCLUDE_DIRS} ${LIBPCRE_INCLUDE_DIRS})

# libs
target_link_libraries(tests objects ${CURSES_LIBRARIES} ${LIBPCRE_LIBRARY} ${LIBCONFIG_LIBRARY})

