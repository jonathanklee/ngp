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

#include "utils.h"
#include "theme.h"
#include "entry.h"
#include "file.h"
#include "line.h"
#include "list.h"
#include "entry.h"
#include "display.h"
#include "options.h"

#include "search/search.h"

#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>

#define _GNU_SOURCE

#define CURSOR_UP     'k'
#define CURSOR_DOWN   'j'
#define PAGE_UP       'K'
#define PAGE_DOWN     'J'
#define ENTER         'p'
#define QUIT          'q'
#define MARK          'm'
#define CTRL_D         4
#define CTRL_U        21

#define lock(MUTEX) \
for(mutex = &MUTEX; \
mutex && !pthread_mutex_lock(mutex); \
pthread_mutex_unlock(mutex), mutex = 0)

/* keep a pointer on search_t & display_t for signal handler ONLY */
static struct search_t *global_search;
static struct display_t *global_display;
static pthread_t pid;

void open_entry(struct search_t *search, int index, const char *editor, const char *pattern)
{
    int i;
    struct entry_t *ptr;
    struct entry_t *file = search->result->start;

    char command[PATH_MAX];
    pthread_mutex_t *mutex;

    for (i = 0, ptr = search->result->start; i < index; i++) {
        ptr = ptr->next;
        struct file_t *cast_result = get_type(ptr, FILE_ENTRY);
        if (cast_result)
            file = &cast_result->entry;
    }

    struct line_t *line = container_of(ptr, struct line_t, entry);

    lock(search->data_mutex) {
        snprintf(command, sizeof(command), editor,
            pattern,
            line->line,
            file->data);
    }

    if (system(command) < 0)
        return;

    line->opened = 1;
}

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        pthread_cancel(pid);
        pthread_join(pid, NULL);
        stop_ncurses(global_display);
        free_search(global_search);
        exit(-1);
    }
}

void *lookup_thread(void *arg)
{
    DIR *dp;

    struct search_t *d = (struct search_t *) arg;
    dp = opendir(d->options->directory);

    if (!dp) {
        fprintf(stderr, "error: could not open directory \"%s\"\n", d->options->directory);
        exit(-1);
    }

    do_search(d);

    d->status = 0;
    closedir(dp);
    return (void *) NULL;
}

void display_status(struct search_t *search)
{
    char *rollingwheel[] = {
        ".  ", ".  ", ".  ", ".  ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        " . ", " . ", " . ", " . ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        "  .", "  .", "  .", "  .",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        "   ", "   ", "   ", "   ",
        };
    static int i = 0;

    attron(COLOR_PAIR(COLOR_FILE));
    if (search->status)
        mvaddstr(0, COLS - 3, rollingwheel[++i%60]);
    else
        mvaddstr(0, COLS - 5, "");
}

int main(int argc, char *argv[])
{
    struct options_t *options = create_options(argc, argv);
    struct search_t *search = create_search(options);
    global_search = search;
    pthread_mutex_init(&search->data_mutex, NULL);

    struct display_t *display;
    display = create_display();
    global_display = display;

    signal(SIGINT, sig_handler);
    if (pthread_create(&pid, NULL, lookup_thread, search)) {
        fprintf(stderr, "ngp: cannot create thread");
        free_search(search);
        exit(-1);
    }

    pthread_mutex_t *mutex;
    lock(search->data_mutex)
        display_results(display, search, LINES);

    int ch;
    while ((ch = getch())) {
        switch(ch) {
        case KEY_RESIZE:
            lock(search->data_mutex)
                resize_display(display, search, LINES);
            break;
        case CURSOR_DOWN:
        case KEY_DOWN:
            lock(search->data_mutex)
                move_cursor_down_and_refresh(display, search);
            break;
        case CURSOR_UP:
        case KEY_UP:
            lock(search->data_mutex)
                move_cursor_up_and_refresh(display, search);
            break;
        case KEY_PPAGE:
        case PAGE_UP:
        case CTRL_U:
            lock(search->data_mutex)
                move_page_up_and_refresh(display, search);
            break;
        case KEY_NPAGE:
        case PAGE_DOWN:
        case CTRL_D:
            lock(search->data_mutex)
                move_page_down_and_refresh(display, search);
            break;
        case ENTER:
        case '\n':
            if (search->result->nbentry == 0)
                break;
            stop_ncurses(display);
            open_entry(search, display->cursor + display->index,
                       search->options->editor, search->options->pattern);
            start_ncurses(display);
            resize_display(display, search, LINES);
            break;
        case QUIT:
            goto quit;
        default:
            break;
        }

        // disable no-delay mode after search was finished
        if (search->status == 0) {
            nodelay(stdscr, FALSE);
        } else {
            usleep(10000);
        }

        lock(search->data_mutex) {
            display_results(display, search, LINES);
            display_status(search);
            if (search->result->nbentry != 0 && !display->ncurses_initialized) {
                start_ncurses(display);
                display->ncurses_initialized = 1;
            }
        }

        lock(search->data_mutex) {
            if (search->status == 0 && search->result->nbentry == 0) {
                goto quit;
            }
        }
    }

quit:
    pthread_cancel(pid);
    pthread_join(pid, NULL);
    stop_ncurses(display);
    free_search(search);
    return 0;
}
