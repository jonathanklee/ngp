#include "display.h"
#include "theme.h"
#include "search.h"
#include "entry.h"
#include "utils.h"
#include <ncurses.h>
#include <stdlib.h>

struct display_t * create_display()
{
    struct display_t *display;
    display = calloc(1, sizeof(struct display_t));

    display->cursor = 1;
    display->index = 0;
    display->ncurses_initialized = 0;

    return display;
}

void start_ncurses(struct display_t *display)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    curs_set(0);

    struct theme_t *theme;
    theme = read_theme();
    apply_theme(theme);
    destroy_theme(theme);
}

void stop_ncurses(struct display_t *display)
{
    endwin();
}

void display_results(struct display_t *display, struct search_t *search)
{
    int i = 0;
    struct entry_t *ptr = search->start;

    for (i = 0; i < display->index; i++)
        ptr = ptr->next;

    for (i = 0; i < LINES; i++) {
        if (ptr && display->index + i < search->nbentry) {
            if (i == display->cursor)
                display_entry_with_cursor(search, &i, ptr);
             else
                display_entry(search, &i, ptr);

            if (ptr->next)
                ptr = ptr->next;
        }
    }
}

void move_page_up(struct display_t *display, struct search_t *search)
{
    clear();
    refresh();
    if (display->index == 0)
        display->cursor = 1;
    else
        display->cursor = LINES - 1;
    display->index -= LINES;
    display->index = (display->index < 0 ? 0 : display->index);

    if (!is_selectionable(search, display->index + display->cursor))
        display->cursor -= 1;

    display_results(display, search);
}

void move_page_down(struct display_t *display, struct search_t *search)
{
    int max_index;

    if (search->nbentry == 0)
        return;

    if (search->nbentry % LINES == 0)
        max_index = (search->nbentry - LINES);
    else
        max_index = (search->nbentry - (search->nbentry % LINES));

    if (display->index == max_index)
        display->cursor = (search->nbentry - 1) % LINES;
    else
        display->cursor = 0;

    clear();
    refresh();
    display->index += LINES;
    display->index = (display->index > max_index ? max_index : display->index);

    if (!is_selectionable(search, display->index + display->cursor))
        display->cursor += 1;
    display_results(display, search);
}

void move_cursor_up(struct display_t *display, struct search_t *search)
{
    /* when cursor is on the first page and on the 2nd line,
       do not move the cursor up */
    if (display->index == 0 && display->cursor == 1)
        return;

    if (display->cursor == 0) {
        move_page_up(display, search);
        return;
    }

    if (display->cursor > 0)
        display->cursor = display->cursor - 1;

    if (!is_selectionable(search, display->index + display->cursor))
        display->cursor = display->cursor - 1;

    if (display->cursor < 0) {
        move_page_up(display, search);
        return;
    }

    display_results(display, search);
}

void move_cursor_down(struct display_t *display, struct search_t *search)
{
    if (display->cursor == (LINES - 1)) {
        move_page_down(display, search);
        return;
    }

    if (display->cursor + display->index < search->nbentry - 1)
        display->cursor = display->cursor + 1;

    if (!is_selectionable(search, display->index + display->cursor))
        display->cursor = display->cursor + 1;

    if (display->cursor > (LINES - 1)) {
        move_page_down(display, search);
        return;
    }

    display_results(display, search);
}

void resize_display(struct display_t *display, struct search_t *search)
{
    /* right now this is a bit trivial,
     * but we may do more complex moving around
     * when the window is resized */
    clear();
    display_results(display, search);
    refresh();
}

void free_display(struct display_t *display)
{
    free(display);
}
