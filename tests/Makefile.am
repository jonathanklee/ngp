TESTS = unit_tests
check_PROGRAMS = unit_tests
unit_tests_SOURCES = main.c
unit_tests_CFLAGS = -I../src/
unit_tests_LDADD = $(top_builddir)/src/ngp-search.o \
                  $(top_builddir)/src/ngp-utils.o \
                  $(top_builddir)/src/ngp-line.o \
                  $(top_builddir)/src/ngp-file.o \
                  $(top_builddir)/src/ngp-list.o \
                  $(top_builddir)/src/ngp-entry.o \
                  $(top_builddir)/src/ngp-display.o \
                  $(top_builddir)/src/ngp-theme.o
