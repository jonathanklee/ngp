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

#include "utils.h"
#include "theme.h"
#include "entry.h"
#include "file.h"
#include "line.h"
#include "list.h"
#include "entry.h"
#include "search.h"
#include "display.h"

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

enum cursor {
    CURSOR_OFF,
    CURSOR_ON
};

/* keep a pointer on search_t & display_t for signal handler ONLY */
struct search_t *global_search;
struct display_t *global_display;

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

void lookup_file(struct search_t *search, const char *file, const char *pattern)
{
    errno = 0;
    pthread_mutex_t *mutex;

    if (is_ignored_file(search, file))
        return;

    if (search->raw_option) {
        lock(search->data_mutex)
            parse_file(search, file, pattern);
        return;
    }

    if (is_specific_file(search, file)) {
        lock(search->data_mutex)
            parse_file(search, file, pattern);
        return;
    }

    if (is_extension_good(search, file)) {
        lock(search->data_mutex)
            parse_file(search, file, pattern);
        return;
    }
}

void lookup_directory(struct search_t *search, const char *dir, const char *pattern)
{
    DIR *dp;

    dp = opendir(dir);
    if (!dp)
        return;

    if (is_ignored_file(search, dir)) {
        closedir(dp);
        return;
    }

    while (1) {
        struct dirent *ep;
        ep = readdir(dp);

        if (!ep)
            break;

        if (!(ep->d_type & DT_DIR)) {
            char file_path[PATH_MAX];
            snprintf(file_path, PATH_MAX, "%s/%s", dir, ep->d_name);

            if (!is_simlink(file_path)) {
                lookup_file(search, file_path, pattern);
            }
        }

        if (ep->d_type & DT_DIR && is_dir_good(ep->d_name)) {
            char path_dir[PATH_MAX] = "";
            snprintf(path_dir, PATH_MAX, "%s/%s", dir, ep->d_name);
            lookup_directory(search, path_dir, pattern);
        }
    }
    closedir(dp);
}

void open_entry(struct search_t *search, int index, const char *editor, const char *pattern)
{
    int i;
    struct entry_t *ptr;
    struct entry_t *file = search->start;

    char command[PATH_MAX];
    char filtered_file_name[PATH_MAX];
    pthread_mutex_t *mutex;

    for (i = 0, ptr = search->start; i < index; i++) {
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
        stop_ncurses(global_display);
        free_search(global_search);
        exit(-1);
    }
}

void *lookup_thread(void *arg)
{
    DIR *dp;

    struct search_t *d = (struct search_t *) arg;
    dp = opendir(d->directory);

    if (!dp) {
        fprintf(stderr, "error: could not open directory \"%s\"\n", d->directory);
        exit(-1);
    }

    lookup_directory(d, d->directory, d->pattern);
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

void read_config(struct search_t *search)
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
        strncpy(search->editor, buffer, LINE_MAX);

    /* only if we don't provide extension as argument */
    if (!search->extension_option) {
        if (!config_lookup_string(&cfg, "files", &specific_files)) {
            fprintf(stderr, "ngprc: no files string found!\n");
            exit(-1);
        }

        search->specific_file = create_list();
        ptr = strtok_r((char *) specific_files, " ", &buf);
        while (ptr != NULL) {
            add_element(&search->specific_file, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }

    if (!search->extension_option) {
        /* getting files extensions from configuration */
        if (!config_lookup_string(&cfg, "extensions", &extensions)) {
            fprintf(stderr, "ngprc: no extensions string found!\n");
            exit(-1);
        }

        search->extension = create_list();
        ptr = strtok_r((char *) extensions, " ", &buf);
        while (ptr != NULL) {
            add_element(&search->extension, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }

    if (!search->ignore_option) {
        /* getting ignored files from configuration */
        if (!config_lookup_string(&cfg, "ignore", &ignore)) {
            fprintf(stderr, "ngprc: no ignore string found!\n");
            exit(-1);
        }

        search->ignore = create_list();
        ptr = strtok_r((char *) ignore, " ", &buf);
        while (ptr != NULL) {
            add_element(&search->ignore, ptr);
            ptr = strtok_r(NULL, " ", &buf);
        }
    }
    config_destroy(&cfg);
}

void parse_args(struct search_t *search, int argc, char *argv[])
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
            search->incase_option = 1;
            break;
        case 't':
            if (!clear_extensions) {
                free_list(&search->extension);
                search->extension_option = 1;
                clear_extensions = 1;
            }
            add_element(&search->extension, optarg);
            break;
        case 'I':
            if (!clear_ignores) {
                free_list(&search->ignore);
                search->ignore_option = 1;
                clear_ignores = 1;
            }
            add_element(&search->ignore, optarg);
            break;
        case 'r':
            search->raw_option = 1;
            break;
        case 'e':
            search->regexp_option = 1;
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
            strcpy(search->pattern, argv[optind]);
            first_argument = 1;
        } else {
            strcpy(search->directory, argv[optind]);
            if (!opendir(search->directory)) {
                fprintf(stderr, "error: could not open directory \"%s\"\n", search->directory);
                exit(-1);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int ch;
    pthread_mutex_t *mutex;
    struct search_t *search;
    struct display_t *display;
    pthread_t pid;

    search = create_search();
    global_search = search;
    pthread_mutex_init(&search->data_mutex, NULL);

    parse_args(search, argc, argv);
    read_config(search);

    display = create_display();
    global_display = display;

    signal(SIGINT, sig_handler);
    if (pthread_create(&pid, NULL, &lookup_thread, search)) {
        fprintf(stderr, "ngp: cannot create thread");
        free_search(search);
        exit(-1);
    }

    lock(search->data_mutex)
        display_results(display, search, LINES);

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
            if (search->nbentry == 0)
                break;
            stop_ncurses(display);
            open_entry(search, display->cursor + display->index,
                       search->editor, search->pattern);
            start_ncurses(display);
            resize_display(display, search, LINES);
            break;
        case QUIT:
            goto quit;
        default:
            break;
        }

        usleep(10000);
        lock(search->data_mutex) {
            display_results(display, search, LINES);
            display_status(search);
            if (search->nbentry != 0 && !display->ncurses_initialized) {
                start_ncurses(display);
                display->ncurses_initialized = 1;
            }
        }

        lock(search->data_mutex) {
            if (search->status == 0 && search->nbentry == 0) {
                goto quit;
            }
        }
    }

quit:
    stop_ncurses(display);
    free_search(search);
    return 0;
}
