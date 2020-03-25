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

#include "entry.h"

#include <sys/stat.h>

#include "file.h"
#include "line.h"
#include "string.h"
#include "theme.h"
#include "utils.h"

void display_entry(struct entry_t *entry, struct search_t *search, int y,
                   int is_cursor_on_entry) {
    entry->vtable->display(entry, search, y, is_cursor_on_entry);
}

void free_entry(struct entry_t *entry) { entry->vtable->free(entry); }

int is_entry_selectable(struct entry_t *entry) {
    return entry->vtable->is_selectable(entry);
}

void *get_type(struct entry_t *entry, entry_type_t type) {
    return entry->vtable->get_type(entry, type);
}
