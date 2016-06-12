#include "line.h"
#include "theme.h"

struct entry_vtable line_vtable = {
    display_line,
    display_line_with_cursor,
    is_line_selectionable,
    free_line
};

struct entry_t *create_line(struct search_t *search, char *line, int line_number)
{
    int len = strlen(line) + 1;
    struct line_t *new;

    new = calloc(1, sizeof(struct line_t) + len);
    strncpy(new->entry.data, line, len);
    new->opened = 0;
    new->line = line_number;
    search->nbentry++;

    new->entry.vtable = &line_vtable;

    if (search->entries) {
        search->entries->next = &new->entry;
    } else {
        search->start = &new->entry;
    }

    return &new->entry;
}

void display_line(struct search_t *search, int *y, struct entry_t *entry)
{
    char *pattern = NULL;
    char *ptr;
    char *regexp_matched_string = NULL;
    char * (*parser)(struct search_t *, const char *, const char*);
    int length = 0;
    int crop = COLS;
    int counter = 0;
    char cropped_line[PATH_MAX] = "";
    char *line = entry->data;


    /* first clear line */
    move(*y, 0);
    clrtoeol();

    /* display line number */
    attron(COLOR_PAIR(COLOR_LINE_NUMBER));
    struct line_t *container = container_of(entry, struct line_t, entry);
    mvprintw(*y, 0, "%d:", container->line);

    /* display rest of line */
    char line_begin[16];
    sprintf(line_begin, "%d", container->line);
    length = strlen(line_begin) + 1;
    attron(COLOR_PAIR(COLOR_LINE));
    strncpy(cropped_line, line, crop - length);
    mvprintw(*y, length, "%s", cropped_line);

    /* highlight pattern */
    if (search->regexp_option) {
        regexp_matched_string = regex(search, cropped_line, search->pattern);
        if (!regexp_matched_string)
            return;

        pattern = strstr(cropped_line, regexp_matched_string);
        goto start_printing;
    }

    parser = get_parser(search);
    pattern = parser(search, cropped_line, search->pattern);

start_printing:

    if (!pattern)
           return;

    ptr = cropped_line;
    move(*y, length);
    while (ptr != pattern) {
        addch(*ptr);
        ptr++;
    }

    /* switch color to cyan */
    attron(A_REVERSE);

    if (container->opened)
       attron(COLOR_PAIR(COLOR_OPENED_LINE));
    else
       attron(COLOR_PAIR(COLOR_HIGHLIGHT));

    if (search->regexp_option) {
        length = strlen(regexp_matched_string);
        pcre_free_substring(regexp_matched_string);
    } else {
        length = strlen(search->pattern);
    }

    for (counter = 0; counter < length; counter++, ptr++)
        addch(*ptr);

    attroff(A_REVERSE);
}

void display_line_with_cursor(struct search_t *search, int *y, struct entry_t *entry)
{
    attron(A_REVERSE);
    display_line(search, y, entry);
    attroff(A_REVERSE);
}

bool is_line_selectionable(struct entry_t *entry)
{
    return true;
}

void free_line(struct entry_t *entry)
{
    struct line_t *ptr = container_of(entry, struct line_t, entry);
    free(ptr);
}


