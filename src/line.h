#ifndef LINE_H
#define LINE_H

#include "entry.h"
#include "utils.h"

struct line_t {
	int line;
	int opened;
	struct entry_t entry;
};

void display_line(struct search_t *search, int y, struct entry_t *entry);
void display_line_with_cursor(struct search_t *search, int y, struct entry_t *entry);
void free_line(struct entry_t *entry);
bool is_line_selectionable(struct entry_t *entry);

#endif
