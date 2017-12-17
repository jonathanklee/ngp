/* Copyright (c) 2013 Jonathan Klee

This file is part of ngp.

ngp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ngp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ngp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include "minunit.h"

static jmp_buf buf;
static int success = 0;

/* change exit to longjmp */
static void test_exit(int status)
{
    success = !status;
    longjmp(buf, 1);
}
#define exit test_exit

/* redirect stdout/stderr to /dev/null */
#undef stdout
#undef stderr
#define stdout fopen("/dev/null", "w")
#define stderr stdout

/* ignore config for the tests */
#define read_config(configuration, options) \
    options->search_type = NGP_SEARCH

/* silence getopt error output */
int opterr = 0;
int dummy;
#define opterr dummy

#include "../src/options.c"


/*
 * Tests for parsing command line args
 */

static char * test_show_help()
{
    {
        success = 0;

        char *argv[] = {"ngp", "-h"};
        int argc = sizeof(argv) / sizeof(*argv);
        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }
        mu_assert_verbose(success == 1);
        free_options(options);
    }
    {
        success = 0;

        char *argv[] = {"ngp", "--help"};
        int argc = sizeof(argv) / sizeof(*argv);
        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }
        mu_assert_verbose(success == 1);
        free_options(options);
    }

    return 0;
}

static char * test_get_version()
{
    {
        success = 0;

        char *argv[] = {"ngp", "-v"};
        int argc = sizeof(argv) / sizeof(*argv);
        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }
        mu_assert_verbose(success == 1);
        free_options(options);
    }
    {
        success = 0;

        char *argv[] = {"ngp", "--version"};
        int argc = sizeof(argv) / sizeof(*argv);
        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }
        mu_assert_verbose(success == 1);
        free_options(options);
    }

    return 0;
}

static char * test_simple_pattern()
{
    success = 42;

    char *argv[] = {"ngp", "pattern"};
    int argc = sizeof(argv) / sizeof(*argv);

    struct configuration_t* config = NULL;
    struct options_t *options = NULL;
    if (!setjmp(buf)) {
        options = create_options(config, argc, argv);
    }

    mu_assert_verbose(success == 42);
    mu_assert_verbose(!strcmp("pattern", options->pattern));

    free_options(options);

    return 0;
}

static char * test_pattern_and_path()
{
    success = 42;

    char *argv[] = {"ngp", "pattern", ".."};
    int argc = sizeof(argv) / sizeof(*argv);

    struct configuration_t* config = NULL;
    struct options_t *options = NULL;
    if (!setjmp(buf)) {
        options = create_options(config, argc, argv);
    }

    mu_assert_verbose(success == 42);
    mu_assert_verbose(!strcmp("pattern", options->pattern));
    mu_assert_verbose(!strcmp("..", options->directory));

    free_options(options);

    return 0;
}

static char * test_parser_and_pattern()
{
    success = 42;

    {
        char *argv[] = {"ngp", "--nat", "--", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == NGP_SEARCH);
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "--ag", "--", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == AG_SEARCH);
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "--git", "--", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == GIT_SEARCH);
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }

    return 0;
}

static char * test_wrong_parser_option()
{
    {
        success = 42;
        char *argv[] = {"ngp", "--parser", "--", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }
    {
        success = 42;
        char *argv[] = {"ngp", "-i", "--nat", "--", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }
    return 0;
}

static char * test_parser_and_pattern_and_path()
{
    {
        success = 42;

        char *argv[] = {"ngp", "--nat", "--", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == NGP_SEARCH);
        mu_assert_verbose(!strcmp("pattern", options->pattern));
        mu_assert_verbose(!strcmp("..", options->directory));

        free_options(options);
    }
    {
        success = 42;

        char *argv[] = {"ngp", "--ag", "--", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == AG_SEARCH);
        mu_assert_verbose(!strcmp("pattern", options->pattern));
        mu_assert_verbose(!strcmp("..", options->directory));

        free_options(options);
    }
    {
        success = 42;

        char *argv[] = {"ngp", "--git", "--", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == GIT_SEARCH);
        mu_assert_verbose(!strcmp("pattern", options->pattern));
        mu_assert_verbose(!strcmp("..", options->directory));

        free_options(options);
    }

    return 0;
}

static char * test_ngp_search_options()
{
    success = 42;

    {
        char *argv[] = {"ngp", "-i", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->incase_option == 1);
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "-r", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->raw_option == 1);
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "-t", ".c", "-t", ".h", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->extension_option == 1);
        mu_assert_verbose(!strcmp(".c", options->extension->data));
        mu_assert_verbose(!strcmp(".h", options->extension->next->data));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "-I", "Makefile", "-I", "build", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->ignore_option == 1);
        mu_assert_verbose(!strcmp("Makefile", options->ignore->data));
        mu_assert_verbose(!strcmp("build", options->ignore->next->data));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "-e", "^pattern$"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->regexp_option == 1);
        mu_assert_verbose(!strcmp("^pattern$", options->pattern));

        free_options(options);
    }

    return 0;
}

static char * test_wrong_ngp_search_options()
{
    {
        success = 42;
        char *argv[] = {"ngp", "-C", "3", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }
    {
        success = 42;
        char *argv[] = {"ngp", "-I", "pattern"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }
    {
        success = 42;
        char *argv[] = {"ngp", "-i", "Makefile", "-e" "^pattern"};
        int argc = sizeof(argv) / sizeof(*argv);
        struct configuration_t* config = NULL;
        struct options_t *options = NULL;

        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }

    return 0;
}

static char * test_parser_and_search_options()
{
    success = 42;

    {
        char *argv[] = {"ngp", "--nat", "-i", "-r", "-t", ".c", "-I", "Makefile", "--", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == NGP_SEARCH);
        mu_assert_verbose(options->incase_option == 1);
        mu_assert_verbose(options->raw_option == 1);
        mu_assert_verbose(options->extension_option == 1);
        mu_assert_verbose(!strcmp(".c", options->extension->data));
        mu_assert_verbose(options->ignore_option == 1);
        mu_assert_verbose(!strcmp("Makefile", options->ignore->data));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "--nat=-i -r -t .c -I Makefile", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == NGP_SEARCH);
        mu_assert_verbose(options->incase_option == 1);
        mu_assert_verbose(options->raw_option == 1);
        mu_assert_verbose(options->extension_option == 1);
        mu_assert_verbose(!strcmp(".c", options->extension->data));
        mu_assert_verbose(options->ignore_option == 1);
        mu_assert_verbose(!strcmp("Makefile", options->ignore->data));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "--ag", "-i", "-r", "-t", ".c", "-I", "Makefile", "--", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == AG_SEARCH);
        mu_assert_verbose(!strcmp("-i -r -t .c -I Makefile", options->parser_options));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "--ag=-i -r -t .c -I Makefile", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == AG_SEARCH);
        mu_assert_verbose(!strcmp("-i -r -t .c -I Makefile", options->parser_options));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "--git", "-i", "-r", "-t", ".c", "-I", "Makefile", "--", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == GIT_SEARCH);
        mu_assert_verbose(!strcmp("-i -r -t .c -I Makefile", options->parser_options));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }
    {
        char *argv[] = {"ngp", "--git=-i -r -t .c -I Makefile", "pattern", ".."};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 42);
        mu_assert_verbose(options->search_type == GIT_SEARCH);
        mu_assert_verbose(!strcmp("-i -r -t .c -I Makefile", options->parser_options));
        mu_assert_verbose(!strcmp("pattern", options->pattern));

        free_options(options);
    }

    return 0;
}

static char * test_missing_pattern()
{
    {
        success = 42;
        char *argv[] = {"ngp", "-i"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }
    {
        success = 42;
        char *argv[] = {"ngp", "--git", "--"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }
    {
        success = 42;
        char *argv[] = {"ngp", "--ag=-C 2"};
        int argc = sizeof(argv) / sizeof(*argv);

        struct configuration_t* config = NULL;
        struct options_t *options = NULL;
        if (!setjmp(buf)) {
            options = create_options(config, argc, argv);
        }

        mu_assert_verbose(success == 0);

        free_options(options);
    }

    return 0;
}

static char * test_invalid_path()
{
    success = 42;
    char *argv[] = {"ngp", "pattern", "/some/invalid/dir"};
    int argc = sizeof(argv) / sizeof(*argv);

    struct configuration_t* config = NULL;
    struct options_t *options = NULL;
    if (!setjmp(buf)) {
        options = create_options(config, argc, argv);
    }

    mu_assert_verbose(success == 0);

    free_options(options);

    return 0;
}


char * command_line_arg_tests()
{
    mu_run_test(test_show_help);
    mu_run_test(test_get_version);
    mu_run_test(test_simple_pattern);
    mu_run_test(test_pattern_and_path);
    mu_run_test(test_parser_and_pattern);
    mu_run_test(test_wrong_parser_option);
    mu_run_test(test_parser_and_pattern_and_path);
    mu_run_test(test_ngp_search_options);
    mu_run_test(test_wrong_ngp_search_options);
    mu_run_test(test_parser_and_search_options);
    mu_run_test(test_missing_pattern);
    mu_run_test(test_invalid_path);

    return 0;
}

