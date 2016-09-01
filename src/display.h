#ifndef DISPLAY_H
#define DISPLAY_H

#include "search.h"

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
