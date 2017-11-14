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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>

#include "list.h"
#include "utils.h"
#include "options.h"

#define NGP_VERSION   "1.4"

static void usage(int status)
{
    FILE *out = status == 0 ? stdout : stderr;
    fprintf(out, "usage: ngp [-h|--help] [-v|--version]\n");
    fprintf(out, "       ngp [--<parser>] [<parser-options>] -- <pattern> [<path>]\n");
    fprintf(out, "       ngp [--<parser>=<parser-options>] <pattern> [<path>]\n");
    fprintf(out, "\n");
    fprintf(out, "options:\n");
    fprintf(out, " -h, --help     display this message\n");
    fprintf(out, " -v, --version  show ngp version\n");
    fprintf(out, "\n");
    fprintf(out, "parser:\n");
    fprintf(out, " --int[=<int-options>]  use ngp's internal search implementation with <int-options>\n");
    fprintf(out, " --ext[=<ext-options>]  use external parser specified in the .ngprc\n");
    fprintf(out, "\n");
    fprintf(out, "int-options:\n");
    fprintf(out, " -i         ignore case distinctions in pattern\n");
    fprintf(out, " -r         raw mode\n");
    fprintf(out, " -t <type>  look into files with specified <type>\n");
    fprintf(out, " -I <name>  ignore file/dir with specified <name>\n");
    fprintf(out, " -e         pattern is a regular expression\n");
    abort();
    exit(status);
}

static void display_version(void)
{
    printf("version %s\n", NGP_VERSION);
    exit(0);
}

static void read_config(struct options_t *options)
{
    const char *specific_files;
    const char *extensions;
    const char *ignore;
    const char *buffer;
    char *ptr;
    char *buf = NULL;
    config_t cfg;

    configuration_init(&cfg);

    if (!config_lookup_string(&cfg, "editor", &buffer)) {
        fprintf(stderr, "ngprc: no editor string found!\n");
        exit(-1);
    }
        strncpy(options->editor, buffer, LINE_MAX);

    if (config_lookup_string(&cfg, "parser_cmd", &buffer)) {
        options->search_type = EXTERNAL_SEARCH;
        strncpy(options->parser_cmd, buffer, LINE_MAX);
    }

    /* only if we don't provide extension as argument */
    if (!options->extension_option) {
        if (!config_lookup_string(&cfg, "files", &specific_files)) {
            fprintf(stderr, "ngprc: no files string found!\n");
            exit(-1);
        }

        options->specific_file = create_list();
        ptr = strtok_r((char *) specific_files, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->specific_file, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }

    if (!options->extension_option) {
        /* getting files extensions from configuration */
        if (!config_lookup_string(&cfg, "extensions", &extensions)) {
            fprintf(stderr, "ngprc: no extensions string found!\n");
            exit(-1);
        }

        options->extension = create_list();
        ptr = strtok_r((char *) extensions, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->extension, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }

    if (!options->ignore_option) {
        /* getting ignored files from configuration */
        if (!config_lookup_string(&cfg, "ignore", &ignore)) {
            fprintf(stderr, "ngprc: no ignore string found!\n");
            exit(-1);
        }

        options->ignore = create_list();
        ptr = strtok_r((char *) ignore, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->ignore, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }
    config_destroy(&cfg);
}

static void parse_ngp_search_args(struct options_t *options, int argc, char *argv[])
{
    opterr = 1;
    optind = 0;

    int opt = 0;
    int clear_extensions = 0;
    int clear_ignores = 0;

    while ((opt = getopt(argc, argv, "eit:rI:")) != -1) {
        switch (opt) {
            case 'i':
                options->incase_option = 1;
                break;
            case 't':
                if (!clear_extensions) {
                    free_list(&options->extension);
                    options->extension_option = 1;
                    clear_extensions = 1;
                }
                add_element(&options->extension, optarg);
                break;
            case 'I':
                if (!clear_ignores) {
                    free_list(&options->ignore);
                    options->ignore_option = 1;
                    clear_ignores = 1;
                }
                add_element(&options->ignore, optarg);
                break;
            case 'r':
                options->raw_option = 1;
                break;
            case 'e':
                options->regexp_option = 1;
                break;
            default:
                usage(-1);
                break;
        }
    }
}

static void parse_args(struct options_t *options, int argc, char *argv[])
{
    static struct option long_options[] = {
        {"help",    no_argument,       0,  'h' },
        {"version", no_argument,       0,  'v' },
        {"int",     optional_argument, 0,  'i' },
        {"ext",     optional_argument, 0,  'e' },
        {0,         0,                 0,   0 }
    };

    int arg_count = argc;
    char **args = calloc(argc, sizeof(*args));
    for (int i = 0; i < argc; ++i) {
        args[i] = argv[i];
    }


    optind = 0;
    opterr = 0;

    int opt = 0;
    while (opt != -1) {
        opt = getopt_long(arg_count, args, "-hv", long_options, NULL);

        int current_index = optind - 1;
                /* printf( "opt= %c %d ARGV[%d] = %s\n", opt, opt, current_index, argv[current_index] ); */
        switch (opt) {
            case 'h':
                usage(0);
                break;
            case 'v':
                display_version();
                break;

            case 'i': {
                if (current_index != 1)
                    usage(-1);

                options->search_type = NGP_SEARCH;
                argv[current_index] = NULL;
            }
            break;
            case 'e': {
                if (current_index != 1)
                    usage(-1);

                if (optarg != NULL) {
                    strcpy(options->parser_options, optarg);
                    opt = -1;
                }

                options->search_type = EXTERNAL_SEARCH;
                argv[current_index] = NULL;
            }
            break;

            case 1:
            {
                if (current_index == 1) {
                    optind--;
                    opt=-1;
                }
            }
            case '?':
            {
                strcat(options->parser_options, argv[current_index]);
                strcat(options->parser_options, " ");
                continue;
            }
            break;
        }
    }

    if (options->search_type == NGP_SEARCH) {

        arg_count = 0;
        for (int i = 0; i < argc; ++i) {
            if (argv[i]) {
                args[arg_count] = argv[i];
                arg_count++;
            }
        }

        parse_ngp_search_args(options, arg_count, args);
    }

    if (arg_count - optind < 1 || arg_count - optind > 2)
        usage(-1);

    int first_argument = 0;
    for ( ; optind < arg_count; optind++) {
        if (!first_argument) {
            strcpy(options->pattern, args[optind]);
            first_argument = 1;
        } else {
            strcpy(options->directory, args[optind]);
        }
    }

    if (!opendir(options->directory)) {
        fprintf(stderr, "error: could not open directory \"%s\"\n", options->directory);
        exit(-1);
    }
}

struct options_t * create_options(int argc, char *argv[])
{
    struct options_t *options = calloc(1, sizeof(*options));

    options->search_type = NGP_SEARCH;
    strcpy(options->directory, ".");

    read_config(options);
    parse_args(options, argc, argv);

    return options;
}
