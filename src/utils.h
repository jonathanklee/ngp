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

#ifndef UTILS_H
#define UTILS_H

#include <libconfig.h>

#include "search.h"

typedef char *(*parser_t)(struct options_t *, const char *, const char *);

int is_selectable(struct search_t *search, int index);
char *regex(struct options_t *options, const char *line, const char *pattern);
void *from_options_to_parser(struct options_t *options);
char *strstr_wrapper(struct options_t *options, const char *line,
                     const char *pattern);
char *strcasestr_wrapper(struct options_t *options, const char *line,
                         const char *pattern);

#endif /* UTILS_H */
