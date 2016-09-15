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
    search.h
    theme.h
    utils.h
    )

add_library(objects STATIC
    list.c
    file.c
    line.c
    entry.c
    display.c
    search.c
    theme.c
    utils.c
)

add_executable(ngp ${SRCS} ${HEADERS})

# ncurses
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
target_link_libraries(ngp objects ${CURSES_LIBRARIES} ${LIBPCRE_LIBRARY} ${LIBCONFIG_LIBRARY} pthread)

install(TARGETS ngp DESTINATION bin)
install(FILES ../ngprc DESTINATION /etc)
