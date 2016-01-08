#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ngp], [0.4], [klee.jonathan@gmail.com])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h unistd.h pcre.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MMAP
AC_CHECK_FUNCS([munmap strchr strrchr strstr])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_OUTPUT
