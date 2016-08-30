/* Copyright (C) 2013  Jonathan Klee

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "utils.h"
#include "theme.h"
#include "entry.h"
#include "file.h"
#include "line.h"
#include "list.h"
#include "entry.h"
#include "search.h"

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

#define lock(MUTEX) \
for(mutex = &MUTEX; \
mutex && !pthread_mutex_lock(mutex); \
pthread_mutex_unlock(mutex), mutex = 0)

enum cursor {
    CURSOR_OFF,
    CURSOR_ON
};

/* keep a pointer on search_t for signal handler ONLY */
struct search_t *global_search;

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

void ncurses_init(void)
{
    struct theme_t *theme;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    theme = read_theme();
    apply_theme(theme);
    destroy_theme(theme);
    curs_set(0);
}

void ncurses_stop(void)
{
    endwin();
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

void display_entries(struct search_t *search, int *index, int *cursor)
{
    int i = 0;
    struct entry_t *ptr = search->start;

    for (i = 0; i < *index; i++)
        ptr = ptr->next;

    for (i = 0; i < LINES; i++) {
        if (ptr && *index + i < search->nbentry) {
            if (i == *cursor)
                display_entry_with_cursor(search, &i, ptr);
             else
                display_entry(search, &i, ptr);

            if (ptr->next)
                ptr = ptr->next;
        }
    }
}

void ncurses_add_line(struct search_t *search, const char *line, int line_number)
{
    search->entries = create_line(search, (char *)line, line_number);
    //if (search->nbentry <= LINES)
        //display_entries(search, &index, &cursor);
}

void resize(struct search_t *search, int *index, int *cursor)
{
    /* right now this is a bit trivial,
     * but we may do more complex moving around
     * when the window is resized */
    clear();
    display_entries(search, index, cursor);
    refresh();
}

void page_up(struct search_t *search, int *index, int *cursor)
{
    clear();
    refresh();
    if (*index == 0)
        *cursor = 1;
    else
        *cursor = LINES - 1;
    *index -= LINES;
    *index = (*index < 0 ? 0 : *index);

    if (!is_selectionable(search, *index + *cursor))
        *cursor -= 1;

    display_entries(search, index, cursor);
}

void page_down(struct search_t *search, int *index, int *cursor)
{
    int max_index;

    if (search->nbentry == 0)
        return;

    if (search->nbentry % LINES == 0)
        max_index = (search->nbentry - LINES);
    else
        max_index = (search->nbentry - (search->nbentry % LINES));

    if (*index == max_index)
        *cursor = (search->nbentry - 1) % LINES;
    else
        *cursor = 0;

    clear();
    refresh();
    *index += LINES;
    *index = (*index > max_index ? max_index : *index);

    if (!is_selectionable(search, *index + *cursor))
        *cursor += 1;
    display_entries(search, index, cursor);
}

void cursor_up(struct search_t *search, int *index, int *cursor)
{
    /* when cursor is on the first page and on the 2nd line,
       do not move the cursor up */
    if (*index == 0 && *cursor == 1)
        return;

    if (*cursor == 0) {
        page_up(search, index, cursor);
        return;
    }

    if (*cursor > 0)
        *cursor = *cursor - 1;

    if (!is_selectionable(search, *index + *cursor))
        *cursor = *cursor - 1;

    if (*cursor < 0) {
        page_up(search, index, cursor);
        return;
    }

    display_entries(search, index, cursor);
}

void cursor_down(struct search_t *search, int *index, int *cursor)
{
    if (*cursor == (LINES - 1)) {
        page_down(search, index, cursor);
        return;
    }

    if (*cursor + *index < search->nbentry - 1)
        *cursor = *cursor + 1;

    if (!is_selectionable(search, *index + *cursor))
        *cursor = *cursor + 1;

    if (*cursor > (LINES - 1)) {
        page_down(search, index, cursor);
        return;
    }

    display_entries(search, index, cursor);
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
        ncurses_stop();
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

    while ((opt = getopt(argc, argv, "heit:rI:v")) != -1) {
        switch (opt) {
        case 'h':
            usage();
            break;
        case 'i':
            strcpy(search->options, "-i");
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
        if (optind == 1) {
            strcpy(search->pattern, argv[optind]);
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
    static struct search_t *search;
    pthread_t pid;

    int is_ncurses_init;

    global_search = search;
    search = create_search();
    pthread_mutex_init(&search->data_mutex, NULL);

    parse_args(search, argc, argv);
    read_config(search);

    int index;
    int cursor;

    index = 0;
    cursor = 1;
    is_ncurses_init = 0;

    signal(SIGINT, sig_handler);
    if (pthread_create(&pid, NULL, &lookup_thread, search)) {
        fprintf(stderr, "ngp: cannot create thread");
        free_search(search);
        exit(-1);
    }

    lock(search->data_mutex)
        display_entries(search, &index, &cursor);

    while ((ch = getch())) {
        switch(ch) {
        case KEY_RESIZE:
            lock(search->data_mutex)
                resize(search, &index, &cursor);
            break;
        case CURSOR_DOWN:
        case KEY_DOWN:
            lock(search->data_mutex)
                cursor_down(search, &index, &cursor);
            break;
        case CURSOR_UP:
        case KEY_UP:
            lock(search->data_mutex)
                cursor_up(search, &index, &cursor);
            break;
        case KEY_PPAGE:
        case PAGE_UP:
            lock(search->data_mutex)
                page_up(search, &index, &cursor);
            break;
        case KEY_NPAGE:
        case PAGE_DOWN:
            lock(search->data_mutex)
                page_down(search, &index, &cursor);
            break;
        case ENTER:
        case '\n':
            if (search->nbentry == 0)
                break;
            ncurses_stop();
            open_entry(search, cursor + index,
                       search->editor, search->pattern);
            ncurses_init();
            resize(search, &index, &cursor);
            break;
        case QUIT:
            goto quit;
        default:
            break;
        }

        usleep(10000);
        lock(search->data_mutex) {
            display_entries(search, &index, &cursor);
            display_status(search);
            if (search->nbentry != 0 && !is_ncurses_init) {
                ncurses_init();
                is_ncurses_init = 1;
            }
        }

        lock(search->data_mutex) {
            if (search->status == 0 && search->nbentry == 0) {
                goto quit;
            }
        }
    }

quit:
    ncurses_stop();
    free_search(search);
    return 0;
}