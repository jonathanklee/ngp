/* Copyright (C) 2013  Jonathan Klee

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef NGP_H
#define NGP_H

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <menu.h>
#include <signal.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <ctype.h>
#include <pcre.h>

#define NGP_VERSION     "1.4"

#define CURSOR_UP     'k'
#define CURSOR_DOWN     'j'
#define PAGE_UP        'K'
#define PAGE_DOWN    'J'
#define ENTER         'p'
#define QUIT         'q'
#define MARK         'm'

#ifdef LINE_MAX
    #undef LINE_MAX
#endif
#define LINE_MAX    512

#define lock(MUTEX) \
for(mutex = &MUTEX; \
mutex && !pthread_mutex_lock(mutex); \
pthread_mutex_unlock(mutex), mutex = 0)

enum cursor {
    CURSOR_OFF,
    CURSOR_ON
};

struct search_t {
    /* screen */
    int index;
    int cursor;

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
    char options[LINE_MAX];
    struct list *specific_file;
    struct list *extension;
    struct list *ignore;
    int raw_option;
    int regexp_option;
    int extension_option;
    int ignore_option;
    int regexp_is_ok;
};

void ncurses_add_file(struct search_t *search, const char *file);
void ncurses_add_line(struct search_t *search, const char *line, int line_number);
void display_entries(struct search_t *search, int *index, int *cursor);

#endif /* NGP_H */
