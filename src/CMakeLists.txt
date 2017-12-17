cmake_minimum_required(VERSION 3.0)

set(SRCS
    main.c
    )

set(HEADERS
    list.h
    file.h
    line.h
    entry.h
    display.h
    options.h
    search.h
    theme.h
    utils.h
    configuration.h
    )

add_library(objects STATIC
    list.c
    file.c
    line.c
    entry.c
    display.c
    options.c
    search.c
    search_utils.c
    ngp_search.c
    ag_search.c
    git_search.c
    theme.c
    utils.c
    configuration.c
)

add_executable(ngp ${SRCS} ${HEADERS})

# config
find_package(PkgConfig REQUIRED)

# ncurses
pkg_check_modules(CURSES REQUIRED ncurses)

# libconfig
pkg_check_modules(LIBCONFIG REQUIRED libconfig)
find_library(LIBCONFIG_LIBRARY NAMES config libconfig HINTS ${LIBCONFIG_LIBDIR})

# pcre
pkg_check_modules(LIBPCRE REQUIRED libpcre)
find_library(LIBPCRE_LIBRARY NAMES pcre libpcre HINTS ${LIBPCRE_LIBDIR})

# headers
include_directories(${CURSES_INCLUDE_DIRS} ${LIBCONFIG_INCLUDE_DIRS} ${LIBPCRE_INCLUDE_DIRS})

# libs
target_link_libraries(ngp objects ${CURSES_LIBRARIES} ${LIBPCRE_LIBRARY} ${LIBCONFIG_LIBRARY} pthread)

install(TARGETS ngp DESTINATION bin)
install(FILES ../ngprc DESTINATION etc/ngp)
