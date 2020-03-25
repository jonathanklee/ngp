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

#ifndef SEARCH_H
#define SEARCH_H

#include <limits.h>
#include <pcre.h>
#include <pthread.h>

#include "options.h"

struct result_t {
    struct entry_t *entries;
    struct entry_t *start;
    int nbentry;
};

struct search_t {

    struct result_t *result;

    /* thread */
    pthread_mutex_t data_mutex;
    int status;

    struct options_t *options;
};

struct search_t *create_search(struct options_t *options);
void do_search(struct search_t *search);
void free_search(struct search_t *search);

#endif
