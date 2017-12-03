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
    fprintf(out, " --nat[=<nat-options>]       use ngp's native search implementation with <int-options>\n");
    fprintf(out, " --ag[=<ag-options>]         use ag aka sliver searcher as parser\n");
    fprintf(out, " --git[=<git-grep-options>]  use git-grep as parser (works only within GIT repositories)\n");
    fprintf(out, "\n");
    fprintf(out, "nat-options:\n");
    fprintf(out, " -i         ignore case distinctions in pattern\n");
    fprintf(out, " -r         raw mode\n");
    fprintf(out, " -t <type>  look into files with specified <type>\n");
    fprintf(out, " -I <name>  ignore file/dir with specified <name>\n");
    fprintf(out, " -e         pattern is a regular expression\n");
    exit(status);
}

static void display_version(void)
{
    fprintf(stdout, "version %s\n", NGP_VERSION);
    exit(0);
}

#ifndef read_config /* ignore for testing */
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

    if (config_lookup_string(&cfg, "default_parser", &buffer)) {
        if (!strncmp(buffer, "ag", 2))
            options->search_type = AG_SEARCH;
        else if (!strncmp(buffer, "git", 3))
            options->search_type = GIT_SEARCH;
        else
            options->search_type = NGP_SEARCH;
    }

    if (config_lookup_string(&cfg, "ag_cmd", &buffer)) {
        strncpy(options->parser_cmd[AG_SEARCH], buffer, LINE_MAX);
    } else {
        fprintf(stderr, "ngprc: no ag_cmd string found!\n");
        exit(-1);
    }

    if (config_lookup_string(&cfg, "git_cmd", &buffer)) {
        strncpy(options->parser_cmd[GIT_SEARCH], buffer, LINE_MAX);
    } else {
        fprintf(stderr, "ngprc: no git_cmd string found!\n");
        exit(-1);
    }

    if (config_lookup_string(&cfg, "files", &specific_files)) {
        options->specific_file = create_list();
        ptr = strtok_r((char *) specific_files, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->specific_file, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    } else {
        fprintf(stderr, "ngprc: no files string found!\n");
        exit(-1);
    }

    /* getting files extensions from configuration */
    if (config_lookup_string(&cfg, "extensions", &extensions)) {
        options->extension = create_list();
        ptr = strtok_r((char *) extensions, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->extension, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    } else {
        fprintf(stderr, "ngprc: no extensions string found!\n");
        exit(-1);
    }

    /* getting ignored files from configuration */
    if (config_lookup_string(&cfg, "ignore", &ignore)) {
        options->ignore = create_list();
        ptr = strtok_r((char *) ignore, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->ignore, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    } else {
        fprintf(stderr, "ngprc: no ignore string found!\n");
        exit(-1);
    }

    config_destroy(&cfg);
}
#endif

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
                    free_list(&options->specific_file);
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
                free_options(options);
                free(argv);
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
        {"nat",     optional_argument, 0,  'n' },
        {"ag",      optional_argument, 0,  'a' },
        {"git",     optional_argument, 0,  'g' },
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
        switch (opt) {
            case 'h':
                free_options(options);
                free(args);
                usage(0);
                break;
            case 'v':
                free_options(options);
                free(args);
                display_version();
                break;

            case 'n': {
                if (current_index != 1)
                    goto error;

                options->search_type = NGP_SEARCH;

                if (optarg != NULL) {
                    strcpy(options->parser_options, optarg);
                    opt = -1;
                }

                argv[current_index] = NULL;
            }
            break;
            case 'a': {
                if (current_index != 1)
                    goto error;

                options->search_type = AG_SEARCH;

                if (optarg != NULL) {
                    strcpy(options->parser_options, optarg);
                    opt = -1;
                }

                argv[current_index] = NULL;
            }
            break;
            case 'g': {
                if (current_index != 1)
                    goto error;

                options->search_type = GIT_SEARCH;

                if (optarg != NULL) {
                    strcpy(options->parser_options, optarg);
                    opt = -1;
                }

                argv[current_index] = NULL;
            }
            break;

            case 1:
            {
                if (current_index == 1) {
                    optind--;
                    opt=-1;
                    continue;
                }
            }
            case '?':
            {
                if (options->search_type == NGP_SEARCH)
                    continue;

                if (strlen(options->parser_options) > 0)
                    strcat(options->parser_options, " ");
                strcat(options->parser_options, argv[current_index]);
                continue;
            }
            break;
        }
    }

    if (options->search_type == NGP_SEARCH) {

        /* delete NULL pointer from args */
        arg_count = 0;
        for (int i = 0; i < argc; ++i) {
            if (argv[i])
                args[arg_count++] = argv[i];
        }

        if (strlen(options->parser_options) > 1) {

            /* count args in parser_options */
            int new_argc = arg_count;
            char *arg = options->parser_options;
            for (int i = 0; i < strlen(arg); ++i) {
                if (arg[i] == ' ')
                    new_argc++;
            }
            new_argc++;

             /* create new args */
            char **new_args = calloc(new_argc, sizeof(*new_args));
            new_args[0] = argv[0];

            /* "copy" args from parser_options to new args */
            new_argc = 1;
            while ((arg = strtok(arg, " ")) != NULL) {
                new_args[new_argc++] = arg;
                arg += strlen(arg) + 1;
            }

            /* append remaineing args to new args */
            for (int i = 1; i < arg_count; ++i) {
                new_args[new_argc++] = args[i];
            }

            /* set args to new args */
            free(args);
            arg_count = new_argc;
            args = new_args;
        }

        parse_ngp_search_args(options, arg_count, args);
    }

    if (arg_count - optind < 1 || arg_count - optind > 2)
        goto error;

    int first_argument = 0;
    for ( ; optind < arg_count; optind++) {
        if (!first_argument) {
            strcpy(options->pattern, args[optind]);
            first_argument = 1;
        } else {
            strcpy(options->directory, args[optind]);
        }
    }

    free(args);

    DIR* dirp = opendir(options->directory);
    if (!dirp) {
        fprintf(stderr, "error: could not open directory \"%s\"\n", options->directory);
        free_options(options);
        exit(-1);
    }
    closedir(dirp);

    return;

error:
    free_options(options);
    free(args);
    usage(-1);
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

void free_options(struct options_t* options)
{
   if (!options) {
       return;
   }

   if (options->extension) {
       free_list(&options->extension);
   }

   if (options->ignore) {
       free_list(&options->ignore);
   }

   if (options->specific_file) {
       free_list(&options->specific_file);
   }

    /* free pcre stuffs if needed */
    if (options->pcre_compiled)
        pcre_free((void *) options->pcre_compiled);

    if (options->pcre_extra)
        pcre_free((void *) options->pcre_extra);

    free(options);
}
