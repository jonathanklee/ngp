#include "ngp.h"

extern search_t	*current;

int is_file(int index)
{
	int i;
	entry_t *ptr = current->start;

	for (i = 0; i < index; i++)
		ptr = ptr->next;

	return ptr->isfile;
}
