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

#ifndef CIRCULAR_LIST_H
#define CIRCULAR_LIST_H

#include <stdlib.h>
#include <string.h>

struct circular_list {
    struct list_element *head;
    int size;
    int nb_element;
};

struct list_element {
    struct list_element *next;
    char data[];
};

struct circular_list *create_circular_list(int size);
void add_circular_element(struct circular_list **list, char *element);
void free_circular_list(struct circular_list **list);

#endif
