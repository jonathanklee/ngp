AC_INIT([ngp], [0.1], [klee.jonathan@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CHECK_HEADERS(libconfig.h,,AC_MSG_ERROR(oops no libconfig.h ?))
AC_CHECK_HEADERS(regex.h,,AC_MSG_ERROR(oops no regex.h ?))
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(ncurses, initscr)
AC_CHECK_LIB(config, config_read_file)
AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT
