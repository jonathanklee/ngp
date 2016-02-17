#include "list.h"

struct list* create_list()
{
	return NULL;
}

void add_element(struct list **list, char *element)
{
	struct list *new, *pointer;
	int len;

	len = strlen(element) + 1;
	new = calloc(1, sizeof(struct list) + len);
	strncpy(new->data, element, len);

	if (*list) {
		/* list not empty */
		pointer = *list;
		while (pointer->next)
			pointer = pointer->next;

		pointer->next = new;
	} else {
		/* list empty */
		*list = new;
	}
}

void free_list(struct list **list)
{
	struct list *pointer = *list;

	while (*list) {
		pointer = *list;
		*list = (*list)->next;
		free(pointer);
	}
}

