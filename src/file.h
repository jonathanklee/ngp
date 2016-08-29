#ifndef FILE_H
#define FILE_H

#include "entry.h"

struct file_t {
    struct entry_t entry;
};

struct entry_t *create_file(struct search_t *search, char *file);
void display_file(struct search_t *search, int *y, struct entry_t *entry);
void display_file_with_cursor(struct search_t *search, int *y, struct entry_t *entry);
void free_file(struct entry_t *entry);
bool is_file_selectionable(struct entry_t *entry);

#endif
