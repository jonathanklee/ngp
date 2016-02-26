#include "entry.h"
#include "string.h"
#include "utils.h"
#include "ngp.h"
#include "theme.h"
#include "file.h"
#include "line.h"

void display_entry(struct search_t *search, int *y, struct entry_t *entry)
{
	entry->vtable->display(search, y, entry);
}

void display_entry_with_cursor(struct search_t *search, int *y, struct entry_t *entry)
{
	entry->vtable->display_with_cursor(search, y, entry);
}

void free_entry(struct entry_t *entry)
{
	entry->vtable->free(entry);
}

bool is_entry_selectionable(struct entry_t *entry)
{
	return entry->vtable->is_selectionable(entry);
}


