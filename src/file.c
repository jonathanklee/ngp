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

#include "file.h"

#include "theme.h"
#include "utils.h"

static void *get_file(struct entry_t *entry, entry_type_t type);

struct entry_vtable file_vtable = {display_file, is_file_selectable, free_file,
                                   get_file};

static char *remove_double(char *initial, char c, char *final) {
    int i, j;
    int len = strlen(initial);

    for (i = 0, j = 0; i < len; j++) {
        if (initial[i] != c) {
            final[j] = initial[i];
            i++;
        } else {
            final[j] = initial[i];
            if (initial[i + 1] == c)
                i = i + 2;
            else
                i++;
        }
    }
    final[j] = '\0';

    return final;
}

static void format_path(char *src, char *dest) {
    if (strncmp(src, "./", 2) == 0)
        remove_double(src + 2, '/', dest);
    else
        remove_double(src, '/', dest);
}

struct entry_t *create_file(struct result_t *result, char *file) {
    int len = strlen(file) + 1;
    struct file_t *new;

    new = calloc(1, sizeof(struct file_t) + len);

    format_path(file, new->entry.data);
    new->entry.vtable = &file_vtable;
    result->nbentry++;

    if (result->entries) {
        result->entries->next = &new->entry;
    } else {
        result->start = &new->entry;
    }

    return &new->entry;
}

void display_file(struct entry_t *entry, struct search_t *search, int y,
                  int is_cursor_on_entry) {
    int crop = COLS;
    char cropped_line[PATH_MAX] = "";
    strncpy(cropped_line, entry->data, crop);

    /* first clear line */
    move(y, 0);
    clrtoeol();

    attron(A_BOLD);

    attron(COLOR_PAIR(COLOR_FILE));
    mvprintw(y, 0, "%s", cropped_line);

    attroff(A_BOLD);
}

int is_file_selectable(struct entry_t *entry) { return false; }

void free_file(struct entry_t *entry) {
    struct file_t *ptr = container_of(entry, struct file_t, entry);
    free(ptr);
}

static void *get_file(struct entry_t *entry, entry_type_t type) {
    if (type == FILE_ENTRY) return container_of(entry, struct file_t, entry);

    return NULL;
}
