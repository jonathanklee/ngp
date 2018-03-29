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

#include "entry.h"
#include "utils.h"
#include "list.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CONFIG_DIR     "ngp"
#define CONFIG_FILE    "ngprc"

int is_selectable(struct search_t *search, int index)
{
    int i;
    struct entry_t *ptr = search->result->start;

    for (i = 0; i < index; i++)
        ptr = ptr->next;

    return is_entry_selectable(ptr);
}

char *regex(struct options_t *options, const char *line, const char *pattern)
{
    int ret;
    const char *pcre_error;
    int pcre_error_offset;
    int substring_vector[30];
    const char *matched_string;

    /* check if regexp has already been compiled */
    if (!options->pcre_compiled) {
        options->pcre_compiled = pcre_compile(pattern, 0, &pcre_error,
            &pcre_error_offset, NULL);
        if (!options->pcre_compiled)
            return NULL;

        options->pcre_extra =
            pcre_study(options->pcre_compiled, 0, &pcre_error);
        if (pcre_error)
            return NULL;
    }

    ret = pcre_exec(options->pcre_compiled, options->pcre_extra, line,
        strlen(line), 0, 0, substring_vector, 30);

    if (ret < 0)
        return NULL;

    pcre_get_substring(line, substring_vector, ret, 0, &matched_string);

    return (char *) matched_string;
}

void *get_parser(struct options_t *options)
{
    char * (*parser)(struct options_t *, const char *, const char*);

    if (!options->incase_option)
        parser = strstr_wrapper;
    else
        parser = strcasestr_wrapper;

    if (options->regexp_option)
        parser = regex;

    return parser;
}

char *strstr_wrapper(struct options_t *options, const char *line, const char *pattern)
{
    return strstr(line, pattern);
}

char *strcasestr_wrapper(struct options_t *options, const char *line, const char *pattern)
{
    return strcasestr(line, pattern);
}
