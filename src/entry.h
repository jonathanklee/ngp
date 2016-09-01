#ifndef ENTRY_H
#define ENTRY_H

#include <stddef.h>
#include "entry.h"
#include "search.h"
#include <stdbool.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

struct entry_t {
    struct entry_vtable *vtable;
    struct entry_t *next;
    char data[];
};

struct entry_vtable {
    void (*display)(struct search_t *, int, struct entry_t *);
    void (*display_with_cursor)(struct search_t *, int, struct entry_t *);
    bool (*is_selectionable)(struct entry_t *);
    void (*free)(struct entry_t *);
};

void display_entry(struct search_t *search, int y, struct entry_t *entry);

void display_entry_with_cursor(struct search_t *search, int y, struct entry_t *entry);

void free_entry(struct entry_t *entry);

struct entry_t *create_line(struct search_t *search, char *line, int line_numer);

bool is_entry_selectionable(struct entry_t *entry);

#endif
