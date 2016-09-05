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


