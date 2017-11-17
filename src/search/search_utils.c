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
#include <sys/stat.h>

#include "../utils.h"
#include "search_utils.h"


int validate_file(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0 &&
        S_ISREG(st.st_mode) == 1)
        return 1;

    return 0;
}

const char* apply_regex(const char *output, const char *expr)
{
    struct options_t *options = calloc(1, sizeof(*options));
    char* match = regex(options, output, expr);
    free( options );
    return match;
}

void popen_search(struct search_t *search, external_parser_t *parser)
{
    char parser_args[PATH_MAX] = {'\0'};
    strcat(parser_args, parser->default_arguments);
    strcat(parser_args, search->options->parser_options);

    char command[PATH_MAX] = {'\0'};
    snprintf(command, sizeof(command),
        search->options->parser_cmd[search->options->search_type],
        parser_args,
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

        char* eol = &line[strlen(line)-1];
        if (*eol == '\n')
            *eol = '\0';

        if (parser->match_blank_line(search->result, line))
            continue;

        if (parser->match_file(search->result, line))
            continue;

        if (parser->match_line(search->result, line))
            continue;

        fprintf(stderr, "error: unmatched output line:\n\t\"%s\"\n", line);
        abort();
        exit(-1);
    }

    free( line );

    /* close */
    pclose(fp);
}
