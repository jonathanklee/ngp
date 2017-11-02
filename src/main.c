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

#include "search/search.h"

#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>

#define _GNU_SOURCE
#define NGP_VERSION   "1.4"

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

void usage(void)
{
    fprintf(stderr, "usage: ngp [options]... pattern [directory]\n\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, " -i : ignore case distinctions in pattern\n");
    fprintf(stderr, " -r : raw mode\n");
    fprintf(stderr, " -t type : look into files with specified extension\n");
    fprintf(stderr, " -I name : ignore file/dir with specified name\n");
    fprintf(stderr, " -e : pattern is a regular expression\n");
    fprintf(stderr, " -v : display version\n");
    exit(-1);
}

void open_entry(struct search_t *search, int index, const char *editor, const char *pattern)
{
    int i;
    struct entry_t *ptr;
    struct entry_t *file = search->result->start;

    char command[PATH_MAX];
    char filtered_file_name[PATH_MAX];
    pthread_mutex_t *mutex;

    for (i = 0, ptr = search->result->start; i < index; i++) {
        ptr = ptr->next;
        if (!is_entry_selectionable(ptr))
            file = ptr;
    }

    struct line_t *line = container_of(ptr, struct line_t, entry);

    lock(search->data_mutex) {
        remove_double(file->data, '/', filtered_file_name);
        snprintf(command, sizeof(command), editor,
            pattern,
            line->line,
            filtered_file_name);
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

void display_version(void)
{
    printf("version %s\n", NGP_VERSION);
}

void read_config(struct options_t *options)
{
    const char *specific_files;
    const char *extensions;
    const char *ignore;
    const char *buffer;
    char *ptr;
    char *buf = NULL;
    config_t cfg;

    configuration_init(&cfg);

    if (!config_lookup_string(&cfg, "editor", &buffer)) {
        fprintf(stderr, "ngprc: no editor string found!\n");
        exit(-1);
    }
        strncpy(options->editor, buffer, LINE_MAX);

    if (config_lookup_string(&cfg, "parser_cmd", &buffer)) {
        options->search_type = EXTERNAL_SEARCH;
        strncpy(options->parser_cmd, buffer, LINE_MAX);
    }

    /* only if we don't provide extension as argument */
    if (!options->extension_option) {
        if (!config_lookup_string(&cfg, "files", &specific_files)) {
            fprintf(stderr, "ngprc: no files string found!\n");
            exit(-1);
        }

        options->specific_file = create_list();
        ptr = strtok_r((char *) specific_files, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->specific_file, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }

    if (!options->extension_option) {
        /* getting files extensions from configuration */
        if (!config_lookup_string(&cfg, "extensions", &extensions)) {
            fprintf(stderr, "ngprc: no extensions string found!\n");
            exit(-1);
        }

        options->extension = create_list();
        ptr = strtok_r((char *) extensions, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->extension, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }

    if (!options->ignore_option) {
        /* getting ignored files from configuration */
        if (!config_lookup_string(&cfg, "ignore", &ignore)) {
            fprintf(stderr, "ngprc: no ignore string found!\n");
            exit(-1);
        }

        options->ignore = create_list();
        ptr = strtok_r((char *) ignore, " ", &buf);
        while (ptr != NULL) {
            add_element(&options->ignore, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }
    config_destroy(&cfg);
}

void parse_args(struct options_t *options, int argc, char *argv[])
{
    int opt;
    int clear_extensions = 0;
    int clear_ignores = 0;
    int first_argument = 0;

    while ((opt = getopt(argc, argv, "heit:rI:v")) != -1) {
        switch (opt) {
        case 'h':
            usage();
            break;
        case 'i':
            options->incase_option = 1;
            break;
        case 't':
            if (!clear_extensions) {
                free_list(&options->extension);
                options->extension_option = 1;
                clear_extensions = 1;
            }
            add_element(&options->extension, optarg);
            break;
        case 'I':
            if (!clear_ignores) {
                free_list(&options->ignore);
                options->ignore_option = 1;
                clear_ignores = 1;
            }
            add_element(&options->ignore, optarg);
            break;
        case 'r':
            options->raw_option = 1;
            break;
        case 'e':
            options->regexp_option = 1;
            break;
        case 'v':
            display_version();
            exit(0);
        default:
            exit(-1);
            break;
        }
    }

    if (argc - optind < 1 || argc - optind > 2)
        usage();

    for ( ; optind < argc; optind++) {
        if (!first_argument) {
            strcpy(options->pattern, argv[optind]);
            first_argument = 1;
        } else {
            strcpy(options->directory, argv[optind]);
        }
    }

    if (!opendir(options->directory)) {
        fprintf(stderr, "error: could not open directory \"%s\"\n", options->directory);
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    struct options_t *options = create_options();
    parse_args(options, argc, argv);
    read_config(options);

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
