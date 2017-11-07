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
#include <sys/stat.h>
#include <dirent.h>

struct search_t * create_external_search()
{
    return NULL;
}

static int validate_file(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0 &&
        S_ISREG(st.st_mode) == 1)
        return 1;


    FILE* stream = fopen("/tmp/ngp.log", "w");
    fprintf(stream, "parsing error: not a valid file:\n%s\n", path);
    fflush( stream );
    pclose(stream);
    /* exit(1); */
    return 0;
}

static const char* apply_regex(const char *output, const char *expr)
{
    struct options_t *options = calloc(1, sizeof(*options));
    char* match = regex(options, output, expr);
    free( options );
    return match;
}

static int match_blank_line(struct result_t *result, const char *output)
{
    /* empty line */
    size_t line_length = strlen(output) - 1;
    if (line_length == 0) {
        result->entries = create_blank_line(result);
        return 1;
    }

    /* only '--' */
    const char* match = apply_regex(output, "^--$");
    if (!match)
        return 0;

    result->entries = create_blank_line(result);
    pcre_free_substring(match);

    return 1;
}

static int match_file(struct result_t *result, const char *output)
{
    const char* match = apply_regex(output, "^(?!(\\d+?[-:=])|(--)).+$");
    if (!match)
        return 0;

    if(!validate_file(match))
        return 1;

    result->entries = create_file(result, (char*)match);
    pcre_free_substring(match);

    return 1;
}

static int match_line(struct result_t *result, const char *output)
{
    const char *match = apply_regex(output, "^\\d+?(?=[-:=])");
    if (!match)
        return 0;

    size_t line_number = atoi(match);
    pcre_free_substring(match);

    if (line_number == 0)
        return 0;

    match = apply_regex(output, "(?<=\\d[-=]).*$");
    if (match) {
        result->entries = create_unselectable_line(result, (char*)match, line_number);
        pcre_free_substring(match);
        return 1;
    }

    match = apply_regex(output, "(?<=\\d[:]).*$");
    if (match) {
        result->entries = create_line(result, (char*)match, line_number);
        pcre_free_substring(match);
        return 1;
    }

    return 0;
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
    size_t read_size;
    char* line;
    while (getline(&line, &read_size, fp) != -1) {

        if (match_blank_line(search->result, line))
            continue;

        if (match_file(search->result, line))
            continue;

        if (match_line(search->result, line))
            continue;

        fprintf(stderr, "error: unmatched output line:\n\t%s\n", line);
        exit(-1);
    }

    free( line );

    /* close */
    pclose(fp);
}


void free_external_search(struct search_t *search)
{
}

