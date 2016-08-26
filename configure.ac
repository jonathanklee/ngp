#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ngp], [0.4], [klee.jonathan@gmail.com])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADER(libconfig.h,, AC_MSG_ERROR(oops no libconfig.h ?))
AC_CHECK_HEADER(regex.h,, AC_MSG_ERROR(oops no regex.h ?))
AC_CHECK_HEADER(pcre.h,, AC_MSG_ERROR(oops no pcre.h ?))
AC_CHECK_HEADER(check.h,, AC_MSG_ERROR(oops no check.h ?))
AC_CHECK_LIB(pthread, pthread_create, ,AC_MSG_ERROR(oops no libpthread ?))
AC_CHECK_LIB(ncurses, initscr, ,AC_MSG_ERROR(oops no libncurses ?))
AC_CHECK_LIB(pcre, pcre_compile, ,AC_MSG_ERROR(oops no libpcre ?))
AC_CHECK_LIB(config, config_lookup_string, ,AC_MSG_ERROR(oops no libconfig ?))

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MMAP

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_OUTPUT
