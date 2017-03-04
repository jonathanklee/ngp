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
#include "string.h"
#include "utils.h"
#include "theme.h"
#include "file.h"
#include "line.h"

#include <sys/stat.h>

void display_entry(struct entry_t *entry, struct search_t *search, int y)
{
    entry->vtable->display(entry, search, y);
}

void display_entry_with_cursor(struct entry_t *entry, struct search_t *search, int y)
{
    entry->vtable->display_with_cursor(entry, search, y);
}

void free_entry(struct entry_t *entry)
{
    entry->vtable->free(entry);
}

bool is_entry_selectionable(struct entry_t *entry)
{
    return entry->vtable->is_selectionable(entry);
}


