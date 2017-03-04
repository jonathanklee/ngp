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

#include <pthread.h>
#include <pcre.h>
#include <limits.h>

#ifdef LINE_MAX
    #undef LINE_MAX
#endif
#define LINE_MAX    512

struct search_t {

    /* data */
    struct entry_t *entries;
    struct entry_t *start;
    int nbentry;

    /* thread */
    pthread_mutex_t data_mutex;
    int status;

    /* search */
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
};

struct search_t * create_search();
int parse_file(struct search_t *search, const char *file, const char *pattern);
void parse_text(struct search_t *search, const char *file_name, int file_size,
                const char *text, const char *pattern);
int is_specific_file(struct search_t *search, const char *name);
int is_extension_good(struct search_t *search, const char *file);
int is_ignored_file(struct search_t *search, const char *name);
void free_search(struct search_t *search);

#endif
