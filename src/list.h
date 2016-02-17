#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <string.h>

struct list {
	struct list *next;
	char data[];
};

struct list* create_list();
void add_element(struct list **list, char *element);
void free_list(struct list **list);

#endif
