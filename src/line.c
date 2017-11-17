/* Copyright (c) 2013 Jonathan Klee

This file is part of ngp.

ngp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ngp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ngp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "line.h"
#include "theme.h"

static void *get_line(struct entry_t *entry, entry_type_t type);

struct entry_vtable line_vtable = {
    display_line,
    is_line_selectable,
    free_line,
    get_line
};

struct entry_t *create_line(struct result_t *result, char *line, int line_number, range_t match)
{
    int len = strlen(line) + 1;
    struct line_t *new;

    new = calloc(1, sizeof(struct line_t) + len);
    strncpy(new->entry.data, line, len);
    new->opened = 0;
    new->is_selectable = 1;
    new->line = line_number;
    new->highlight.begin = match.begin;
    new->highlight.end = match.end;
    result->nbentry++;

    new->entry.vtable = &line_vtable;

    if (result->entries) {
        result->entries->next = &new->entry;
    } else {
        result->start = &new->entry;
    }

    return &new->entry;
}

struct entry_t *create_unselectable_line(struct result_t *result, char *line, int line_number)
{
    range_t no_match = {0, 0};
    struct entry_t *entry = create_line(result, line, line_number, no_match);
    struct line_t *new = container_of(entry, struct line_t, entry);
    new->is_selectable = 0;

    return entry;
}

struct entry_t *create_blank_line(struct result_t *result)
{
    return create_unselectable_line(result, "", 0);
}


static int get_integer_as_string(int integer, char *string)
{
    sprintf(string, "%d", integer);
    return strlen(string) + 1;
}

static void hilight_pattern(struct line_t *container, char *line, int y)
{
    char *ptr = (char *)line;
    char buffer[32];
    int length = get_integer_as_string(container->line, buffer);
    move(y, length);

    /* while (ptr != pattern) { */
    while (container->highlight.begin > (ptr - line)) {
        addch(*ptr);
        ptr++;
    }

    /* switch color to cyan */
    attron(A_REVERSE);

    if (container->opened)
       attron(COLOR_PAIR(COLOR_OPENED_LINE));
    else
       attron(COLOR_PAIR(COLOR_HIGHLIGHT));

    length = container->highlight.end - container->highlight.begin;
    for (int counter = 0; counter < length; counter++, ptr++)
        addch(*ptr);

    attroff(A_REVERSE);
}

void display_line(struct entry_t *entry, struct search_t *search, int y, int is_cursor_on_entry)
{
    int length = 0;
    char cropped_line[PATH_MAX] = "";
    char *line = entry->data;

    if (is_cursor_on_entry)
        attron(A_REVERSE);

    /* first clear line */
    move(y, 0);
    clrtoeol();

    /* display blank line */
    struct line_t *container = container_of(entry, struct line_t, entry);
    if (container->line == 0) {
        for (int i = 0; i < COLS; ++i)
            cropped_line[i] = '-';

        attron(A_BOLD);
        attron(COLOR_PAIR(COLOR_FILE));
        mvprintw(y, 0, "%s", cropped_line);
        attroff(COLOR_PAIR(COLOR_FILE));
        attroff(A_BOLD);
        return;
    }

    /* display line number */
    attron(COLOR_PAIR(COLOR_LINE_NUMBER));
    mvprintw(y, 0, "%d:", container->line);

    /* display rest of line */
    char line_begin[16];
    sprintf(line_begin, "%d", container->line);
    length = strlen(line_begin) + 1;
    attron(COLOR_PAIR(COLOR_LINE));
    strncpy(cropped_line, line, COLS - length);
    mvprintw(y, length, "%s", cropped_line);

    hilight_pattern(container, cropped_line, y);

    if (is_cursor_on_entry)
        attroff(A_REVERSE);
}

int is_line_selectable(struct entry_t *entry)
{
    struct line_t *line = container_of(entry, struct line_t, entry);
    return line->is_selectable;
}

void free_line(struct entry_t *entry)
{
    struct line_t *ptr = container_of(entry, struct line_t, entry);
    free(ptr);
}

static void *get_line(struct entry_t *entry, entry_type_t type)
{
    if (type == LINE_ENTRY)
        return container_of(entry, struct line_t, entry);

    return NULL;
}
