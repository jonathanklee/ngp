#include "file.h"
#include "utils.h"
#include "theme.h"

struct entry_vtable file_vtable = {
    display_file,
    display_file_with_cursor,
    is_file_selectionable,
    free_file
};

struct entry_t *create_file(struct search_t *search, char *file)
{
    int len = strlen(file) + 1;
    struct file_t *new;

    new = calloc(1, sizeof(struct file_t) + len);
    strncpy(new->entry.data, file, len);
    new->entry.vtable = &file_vtable;
    search->nbentry++;

    if (search->entries) {
        search->entries->next = &new->entry;
    } else {
        search->start = &new->entry;
    }

    return &new->entry;
}

void display_file(struct entry_t *entry, struct search_t *search, int y)
{
    char filtered_line[PATH_MAX];
    char cropped_line[PATH_MAX] = "";
    int crop = COLS;

    /* first clear line */
    move(y, 0);
    clrtoeol();

    attron(A_BOLD);

    if (strcmp(search->directory, "./") == 0)
        remove_double(entry->data + 3, '/', filtered_line);
    else
        remove_double(entry->data , '/', filtered_line);

    strncpy(cropped_line, filtered_line, crop);
    attron(COLOR_PAIR(COLOR_FILE));
    remove_double(cropped_line, '/', filtered_line);
    mvprintw(y, 0, "%s", cropped_line);

    attroff(A_BOLD);
}

void display_file_with_cursor(struct entry_t *entry, struct search_t *search, int y) { }

bool is_file_selectionable(struct entry_t *entry)
{
    return false;
}

void free_file(struct entry_t *entry)
{
    struct file_t *ptr = container_of(entry, struct file_t, entry);
    free(ptr);
}

