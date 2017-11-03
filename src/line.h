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

#ifndef LINE_H
#define LINE_H

#include "entry.h"
#include "utils.h"

struct line_t {
	int line;
	int opened;
	int is_selectable;
	struct entry_t entry;
};

struct entry_t *create_line(struct result_t *result, char *line, int line_number);
struct entry_t *create_unselectable_line(struct result_t *result, char *line, int line_number);
struct entry_t *create_blank_line(struct result_t *result);
void display_line(struct entry_t *entry, struct search_t *search, int y, int is_cursor_on_entry);
void free_line(struct entry_t *entry);
int is_line_selectable(struct entry_t *entry);

#endif
