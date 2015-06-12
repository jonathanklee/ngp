AC_INIT([ngp], [0.1], [klee.jonathan@gmail.com])
AM_INIT_AUTOMAKE
AC_PROG_CC
CFLAGS="$CFLAGS -Wall -Werror"
AC_CHECK_HEADERS(libconfig.h,,AC_MSG_ERROR(oops no libconfig.h ?))
AC_CHECK_HEADERS(regex.h,,AC_MSG_ERROR(oops no regex.h ?))
AC_CHECK_HEADERS(pcre.h,,AC_MSG_ERROR(oops no pcre.h ?))
AC_CHECK_LIB(pthread, pthread_create,,AC_MSG_ERROR(oops no lib pthread ?))
AC_CHECK_LIB(ncurses, initscr,,AC_MSG_ERROR(oops no lib ncurses ?))
AC_CHECK_LIB(config, config_read_file,,AC_MSG_ERROR(oops no lib config ?))
AC_CHECK_LIB(pcre, pcre_compile,,AC_MSG_ERROR(oops no lib pcre ?))
AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT
