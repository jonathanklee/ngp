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

#include "search.h"
#include "utils.h"
#include "entry.h"
#include "list.h"
#include "file.h"
#include "line.h"

#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

struct search_t * create_search()
{
    struct search_t *search;
    search = calloc(1, sizeof(struct search_t));

    search->nbentry = 0;
    search->status = 1;
    search->raw_option = 0;
    search->regexp_option = 0;
    search->incase_option = 0;
    search->entries = NULL;
    search->start = search->entries;
    strcpy(search->directory, ".");

    return search;
}

int parse_file(struct search_t *search, const char *file, const char *pattern)
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

void parse_text(struct search_t *search, const char *file_name, int file_size,
                const char *text, const char *pattern)
{
    char *end;
    char *endline;
    int first_occurrence;
    int line_number;
    char * (*parser)(struct search_t *, const char *, const char*);
    char *pointer = (char *)text;

    parser = get_parser(search);
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

        if (parser(search, pointer, pattern) != NULL) {
            if (first_occurrence) {
                search->entries = create_file(search, (char *)file_name);
                first_occurrence = 0;
            }
            search->entries = create_line(search, pointer, line_number);
        }

        *endline = '\n';
        pointer = endline + 1;
        line_number++;
    }

}

int is_specific_file(struct search_t *search, const char *name)
{
    char *name_begins;
    struct list *pointer = search->specific_file;

    while (pointer) {
        name_begins = get_file_name(name);
        if (!strcmp(name_begins, pointer->data))
            return 1;
        pointer = pointer->next;
    }
    return 0;
}

int is_ignored_file(struct search_t *search, const char *name)
{
    char *name_begins;
    struct list *pointer = search->ignore;

    while (pointer) {
        name_begins = get_file_name(name);
        if (!strcmp(name_begins, pointer->data))
            return 1;
        pointer = pointer->next;
    }
    return 0;
}

int is_extension_good(struct search_t *search, const char *file)
{

    struct list *pointer;

    pointer = search->extension;
    while (pointer) {
        if (!strcmp(pointer->data, file + strlen(file) -
            strlen(pointer->data)))
            return 1;
        pointer = pointer->next;
    }
    return 0;
}

void free_search(struct search_t *search)
{
    struct entry_t *ptr = search->start;
    struct entry_t *p;

    while (ptr) {
        p = ptr;
        ptr = ptr->next;
        free_entry(p);
    }

    free_list(&search->extension);
    free_list(&search->specific_file);
    free_list(&search->ignore);

    /* free pcre stuffs if needed */
    if (search->pcre_compiled)
        pcre_free((void *) search->pcre_compiled);

    if (search->pcre_extra)
        pcre_free((void *) search->pcre_extra);
}
