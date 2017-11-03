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

#include "external_search.h"
#include "../utils.h"
#include "../entry.h"
#include "../list.h"
#include "../file.h"
#include "../line.h"

#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

struct search_t * create_external_search()
{
    return NULL;
}


void do_external_search(struct search_t *search)
{
    char command[PATH_MAX] = {'\0'};
    snprintf(command, sizeof(command),
        search->options->parser_cmd,
        search->options->pattern,
        search->options->directory);

    /* open the command for reading. */
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        printf("Failed to run command: %s \n", command);
        exit(1);
    }

    /* parse the output a line at a time. */
    char output[PATH_MAX] = {'\0'};
    while (fgets(output, sizeof(output)-1, fp) != NULL) {

        /* empty line */
        size_t line_length = strlen(output) - 1;
        if (line_length == 0) {
            continue;
        }

        /* only '--' */
        char* match = regex(search->options, output, "^--$");
        search->options->pcre_compiled = 0;
        if (match) {
            search->result->entries = create_empty_line(search->result);
            pcre_free_substring(match);
            continue;
        }

        /* file names */
        match = regex(search->options, output, "^(?!(\\d+?[-:=])|(--)).+$");
        search->options->pcre_compiled = 0;
        if (match) {
            search->result->entries = create_file(search->result, match);
            pcre_free_substring(match);
            continue;
        }

        /* line number */
        size_t line_number = 0;
        match = regex(search->options, output, "^\\d+?(?=[-:=])");
        search->options->pcre_compiled = 0;
        if (match) {
            line_number = atoi(match);
            pcre_free_substring(match);
        }

        /* line content */
        if (line_number > 0) {
            match = regex(search->options, output, "(?<=\\d[-=]).*$");
            search->options->pcre_compiled = 0;
            if (match) {
                search->result->entries = create_unselectable_line(search->result, match, line_number);
                /* search->result->entries = create_line(search->result, match, line_number); */
                pcre_free_substring(match);
                continue;
            }

            match = regex(search->options, output, "(?<=\\d[:]).*$");
            search->options->pcre_compiled = 0;
            if (match) {
                search->result->entries = create_line(search->result, match, line_number);
                pcre_free_substring(match);
                continue;
            }
        }

        fprintf(stderr, "error: unmatched output line:\n%s\n", output);
        exit(-1);
    }

    /* close */
    pclose(fp);
}


void free_external_search(struct search_t *search)
{
}

