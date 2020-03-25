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

#ifndef ENTRY_H
#define ENTRY_H

#include <stddef.h>

#include "search.h"

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr)-offsetof(type, member)))

typedef enum { FILE_ENTRY, LINE_ENTRY } entry_type_t;

struct entry_t {
    struct entry_vtable *vtable;
    struct entry_t *next;
    char data[];
};

struct entry_vtable {
    void (*display)(struct entry_t *, struct search_t *, int, int);
    int (*is_selectable)(struct entry_t *);
    void (*free)(struct entry_t *);
    void *(*get_type)(struct entry_t *, entry_type_t);
};

void display_entry(struct entry_t *entry, struct search_t *search, int y,
                   int is_cursor_on_entry);

void free_entry(struct entry_t *entry);

int is_entry_selectable(struct entry_t *entry);

void *get_type(struct entry_t *entry, entry_type_t type);

#endif
