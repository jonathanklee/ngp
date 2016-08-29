#ifndef SEARCH_H
#define SEARCH_H

#include <pthread.h>
#include <pcre.h>
#include <limits.h>

#ifdef LINE_MAX
    #undef LINE_MAX
#endif
#define LINE_MAX    512

struct search_t {

    /* data */
    struct entry_t *entries;
    struct entry_t *start;
    int nbentry;

    /* thread */
    pthread_mutex_t data_mutex;
    int status;

    /* search */
    const pcre *pcre_compiled;
    const pcre_extra *pcre_extra;
    char editor[LINE_MAX];
    char directory[PATH_MAX];
    char pattern[LINE_MAX];
    char options[LINE_MAX];
    struct list *specific_file;
    struct list *extension;
    struct list *ignore;
    int raw_option;
    int regexp_option;
    int extension_option;
    int ignore_option;
    int regexp_is_ok;
};

struct search_t * create_search();
int parse_file(struct search_t *search, const char *file, const char *pattern);
void parse_text(struct search_t *search, const char *file_name, int file_size,
                const char *text, const char *pattern);
void free_search(struct search_t *search);

#endif
