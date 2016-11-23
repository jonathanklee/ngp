/* Copyright (c) 2013-2016  Jonathan Klee

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

static void hilight_pattern(struct entry_t *entry, char *line, struct search_t *search, int y)
{
    int length;
    int counter;
    char *ptr;
    char *regexp_matched_string = NULL;
    char *pattern = NULL;
    char * (*parser)(struct search_t *, const char *, const char*);
    struct line_t *container = container_of(entry, struct line_t, entry);
    char buffer[32];

    if (search->regexp_option) {
        regexp_matched_string = regex(search, line, search->pattern);
        if (!regexp_matched_string)
            return;

        pattern = strstr(line, regexp_matched_string);
        goto start_printing;
    }

    parser = get_parser(search);
    pattern = parser(search, line, search->pattern);

start_printing:

    if (!pattern)
           return;

    ptr = (char *)line;
    length = get_integer_as_string(container->line, buffer);
    move(y, length);
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

void display_line(struct entry_t *entry, struct search_t *search, int y)
{
    int length = 0;
    char cropped_line[PATH_MAX] = "";
    char *line = entry->data;

    /* first clear line */
    move(y, 0);
    clrtoeol();

    /* display line number */
    attron(COLOR_PAIR(COLOR_LINE_NUMBER));
    struct line_t *container = container_of(entry, struct line_t, entry);
    mvprintw(y, 0, "%d:", container->line);

    /* display rest of line */
    char line_begin[16];
    sprintf(line_begin, "%d", container->line);
    length = strlen(line_begin) + 1;
    attron(COLOR_PAIR(COLOR_LINE));
    strncpy(cropped_line, line, COLS - length);
    mvprintw(y, length, "%s", cropped_line);

    hilight_pattern(entry, cropped_line, search, y);
}

void display_line_with_cursor(struct entry_t *entry, struct search_t *search, int y)
{
    attron(A_REVERSE);
    display_line(entry, search, y);
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


