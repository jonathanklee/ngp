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
#include "entry.h"
#include "search.h"
#include <stdbool.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

struct entry_t {
    struct entry_vtable *vtable;
    struct entry_t *next;
    char data[];
};

struct entry_vtable {
    void (*display)(struct entry_t *, struct search_t *, int, bool);
    bool (*is_selectionable)(struct entry_t *);
    void (*free)(struct entry_t *);
};

void display_entry(struct entry_t *entry, struct search_t *search, int y, bool is_cursor_on_entry);

void free_entry(struct entry_t *entry);

bool is_entry_selectionable(struct entry_t *entry);

#endif
