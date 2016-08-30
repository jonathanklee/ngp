TESTS = check_ngp
check_PROGRAMS = check_ngp
check_ngp_SOURCES = search.c
check_ngp_CFLAGS = -I../src/
check_ngp_LDADD = $(top_builddir)/src/ngp-search.o \
                  $(top_builddir)/src/ngp-utils.o \
                  $(top_builddir)/src/ngp-line.o \
                  $(top_builddir)/src/ngp-file.o \
                  $(top_builddir)/src/ngp-list.o \
                  $(top_builddir)/src/ngp-entry.o
