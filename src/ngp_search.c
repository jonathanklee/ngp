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
#include "entry.h"
#include "list.h"
#include "file.h"
#include "line.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#define for_lock(MUTEX) \
for(mutex = &MUTEX; mutex && !pthread_mutex_lock(mutex); pthread_mutex_unlock(mutex), mutex = 0)

static int is_simlink(char *file_path)
{
    struct stat filestat;

    lstat(file_path, &filestat);
    return S_ISLNK(filestat.st_mode);
}

static int is_dir_good(char *dir)
{
    return strcmp(dir, ".") != 0 &&
        strcmp(dir, "..") != 0 &&
        strcmp(dir, ".git") != 0 ? 1 : 0;
}

char *get_file_name(const char *absolute_path, char *file_name) {
    char *ret;

    if (strchr(absolute_path, '/') == NULL){
        return (char *)absolute_path;
    }

    strcpy(file_name, absolute_path);

    int len = strlen(file_name);
    if (file_name[len - 1] == '/') {
        file_name[len - 1] = '\0';
    }

    ret = file_name;
    if (strrchr(file_name, '/') != NULL) {
        ret = strrchr(file_name, '/') + 1;
    }

    return ret;
}

static void parse_text(struct search_t *search, const char *file_name, int file_size,
                       const char *text, const char *pattern)
{
    char *end;
    char *endline;
    int first_occurrence;
    int line_number;
    char * (*parser)(struct options_t *, const char *, const char*);
    char *pointer = (char *)text;

    parser = get_parser(search->options);
    first_occurrence = 1;
    line_number = 1;
    end = pointer + file_size;

    while (1) {

        if (pointer == end)
            break;

        endline = memchr(pointer, '\n', end - pointer);
        if (endline == NULL)
            break;

        *endline = '\0';

        char *match_begin = parser(search->options, pointer, pattern);
        if (match_begin != NULL) {
            if (first_occurrence) {
                search->result->entries = create_file(search->result, (char *)file_name);
                first_occurrence = 0;
            }
            range_t match = {0, 0};
            if (search->options->regexp_option) {
                match.begin = strstr(pointer, match_begin) - pointer;
                match.end = match.begin + strlen(match_begin);
            } else {
                match.begin = match_begin - pointer;
                match.end = match.begin + strlen(search->options->pattern);
            }
            search->result->entries = create_line(search->result, pointer, line_number, match);
        }

        *endline = '\n';
        pointer = endline + 1;
        line_number++;
    }

}

static int is_specific_file(struct options_t *options, const char *name)
{
    char *name_begins;
    struct list *pointer = options->specific_file;

    char file_name[FILENAME_MAX];
    while (pointer) {
        name_begins = get_file_name(name, file_name);
        if (!strcmp(name_begins, pointer->data))
            return 1;
        pointer = pointer->next;
    }
    return 0;
}

static int is_ignored_file(struct options_t *options, const char *name)
{
    char *name_begins;
    struct list *pointer = options->ignore;

    char file_name[FILENAME_MAX];
    while (pointer) {
        name_begins = get_file_name(name, file_name);
        if (!strcmp(name_begins, pointer->data))
            return 1;
        pointer = pointer->next;
    }
    return 0;
}

static int is_extension_good(struct options_t *options, const char *file)
{

    struct list *pointer;

    pointer = options->extension;
    while (pointer) {
        if (!strcmp(pointer->data, file + strlen(file) -
            strlen(pointer->data)))
            return 1;
        pointer = pointer->next;
    }
    return 0;
}

static int parse_file(struct search_t *search, const char *file, const char *pattern)
{
    int f;
    char *pointer;
    char *start;
    struct stat sb;
    errno = 0;

    f = open(file, O_RDONLY);
    if (f < 0)
        return -1;

    if (fstat(f, &sb) < 0) {
        close(f);
        return -1;
    }

    pointer = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, f, 0);
    start = pointer;
    if (pointer == MAP_FAILED) {
        close(f);
        return -1;
    }

    close(f);

    parse_text(search, file, sb.st_size, start, pattern);

    if (munmap(start, sb.st_size) < 0)
        return -1;

    return 0;
}

static void lookup_file(struct search_t *search, const char *file, const char *pattern)
{
    errno = 0;
    pthread_mutex_t *mutex;

    if (is_ignored_file(search->options, file) && !search->options->raw_option)
        return;

    if (search->options->raw_option) {
        for_lock(search->data_mutex)
            parse_file(search, file, pattern);
        return;
    }

    if (is_specific_file(search->options, file)) {
        for_lock(search->data_mutex)
            parse_file(search, file, pattern);
        return;
    }

    if (is_extension_good(search->options, file)) {
        for_lock(search->data_mutex)
            parse_file(search, file, pattern);
        return;
    }
}

static void lookup_directory(struct search_t *search, const char *dir, const char *pattern)
{
    DIR *dp;

    if (is_ignored_file(search->options, dir)) {
        return;
    }

    dp = opendir(dir);
    if (!dp)
        return;

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


void do_ngp_search(struct search_t *search)
{
    lookup_directory(search, search->options->directory, search->options->pattern);
}
