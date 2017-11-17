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

#ifndef SEARCH_UTILS_H
#define SEARCH_UTILS_H

#include "search.h"

typedef struct {
    const char *default_arguments;
    int (*match_file)(struct result_t *result, const char *output);
    int (*match_line)(struct result_t *result, const char *output);
    int (*match_blank_line)(struct result_t *result, const char *output);
} external_parser_t;

int validate_file(const char *path);
const char* apply_regex(const char *output, const char *expr);
void popen_search(struct search_t *search, external_parser_t *parser);

#endif
