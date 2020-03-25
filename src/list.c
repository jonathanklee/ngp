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

#include "list.h"

struct list *create_list() {
    return NULL;
}

void add_element(struct list **list, char *element) {
    struct list *new, *pointer;
    int len;

    len = strlen(element) + 1;
    new = calloc(1, sizeof(struct list) + len);
    strncpy(new->data, element, len);

    if (*list) {
        /* list not empty */
        pointer = *list;
        while (pointer->next) pointer = pointer->next;

        pointer->next = new;
    } else {
        /* list empty */
        *list = new;
    }
}

void free_list(struct list **list) {
    struct list *pointer = *list;

    while (*list) {
        pointer = *list;
        *list = (*list)->next;
        free(pointer);
    }
}
