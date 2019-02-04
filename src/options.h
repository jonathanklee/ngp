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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "search.h"
#include "configuration.h"

#ifdef LINE_MAX
    #undef LINE_MAX
#endif
#define LINE_MAX    512

typedef enum {
    NGP_SEARCH = 0,
    AG_SEARCH,
    RG_SEARCH,
    GIT_SEARCH,

    NUM_SEARCHES
} SearchType;

struct options_t {
    const pcre *pcre_compiled;
    const pcre_extra *pcre_extra;
    char editor[LINE_MAX];
    char directory[PATH_MAX];
    char pattern[LINE_MAX];
    struct list *specific_file;
    struct list *extension;
    struct list *ignore;
    int raw_option;
    int regexp_option;
    int extension_option;
    int incase_option;
    int ignore_option;
    int regexp_is_ok;

    SearchType search_type;
    char parser_cmd[NUM_SEARCHES][LINE_MAX];
    char parser_options[LINE_MAX];
};

struct options_t * create_options(struct configuration_t *config, int argc, char *argv[]);
void free_options(struct options_t* options);
#endif
