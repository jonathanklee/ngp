#ifndef LINE_H
#define LINE_H

#include "entry.h"
#include "utils.h"

struct line_t {
	int line;
	int opened;
	struct entry_t entry;
};

void display_line(struct entry_t *entry, struct search_t *search, int);
void display_line_with_cursor(struct entry_t *entry, struct search_t *search, int y);
void free_line(struct entry_t *entry);
bool is_line_selectionable(struct entry_t *entry);

#endif
