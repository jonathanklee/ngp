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

#ifndef DISPLAY_H
#define DISPLAY_H

#include "search/search.h"

struct display_t {
    int index;
    int cursor;
    int ncurses_initialized;
};

struct display_t * create_display();
void start_ncurses(struct display_t *display);
void stop_ncurses(struct display_t *display);
void display_results(struct display_t *display, struct search_t *search, int terminal_line_nb);
void resize_display(struct display_t *display, struct search_t *search, int terminal_line_nb);
void move_cursor_up_and_refresh(struct display_t *display, struct search_t *search);
void move_cursor_down_and_refresh(struct display_t *display, struct search_t *search);
void move_page_up_and_refresh(struct display_t *display, struct search_t *search);
void move_page_down_and_refresh(struct display_t *display, struct search_t *search);
void move_cursor_up(struct display_t *display, struct search_t *search, int terminal_line_nb);
void move_cursor_down(struct display_t *display, struct search_t *search, int terminal_line_nb);
void move_page_up(struct display_t *display, struct search_t *search, int terminal_line_nb);
void move_page_down(struct display_t *display, struct search_t *search, int terminal_line_nb);
void free_display(struct display_t *display);

#endif
