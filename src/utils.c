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

#define _GNU_SOURCE

#include "entry.h"
#include "utils.h"
#include "list.h"

#include <string.h>
#include <sys/stat.h>

int is_selectionable(struct search_t *search, int index)
{
    int i;
    struct entry_t *ptr = search->result->start;

    for (i = 0; i < index; i++)
        ptr = ptr->next;

    return is_entry_selectionable(ptr);
}

int is_dir_good(char *dir)
{
    return strcmp(dir, ".") != 0 &&
        strcmp(dir, "..") != 0 &&
        strcmp(dir, ".git") != 0 ? 1 : 0;
}

char *get_file_name(const char * absolute_path)
{
    char *ret;

    if (strrchr(absolute_path + 3, '/') != NULL)
        ret = strrchr(absolute_path + 3, '/') + 1;
    else
        ret = (char *) absolute_path + 3;

    return ret;
}

char *remove_double(char *initial, char c, char *final)
{
    int i, j;
    int len = strlen(initial);

    for (i = 0, j = 0; i < len; j++ ) {
        if (initial[i] != c) {
            final[j] = initial[i];
            i++;
        } else {
            final[j] = initial[i];
            if (initial[i + 1] == c)
                i = i + 2;
            else
                i++;
        }
    }
    final[j] = '\0';

    return final;
}

char *extract_line_number(char *line)
{
    char *token;
    char *buffer;
    token = strtok_r(line, " :", &buffer);
    return token;
}

int is_simlink(char *file_path)
{
    struct stat filestat;

    lstat(file_path, &filestat);
    return S_ISLNK(filestat.st_mode);
}

void configuration_init(config_t *cfg)
{
    char *user_name;
    char user_ngprc[PATH_MAX];

    config_init(cfg);

    user_name = getenv("USER");
    snprintf(user_ngprc, PATH_MAX, "/home/%s/%s",
        user_name, ".ngprc");

    if (config_read_file(cfg, user_ngprc))
        return;

    if (!config_read_file(cfg, "/etc/ngprc")) {
        fprintf(stderr, "error in /etc/ngprc\n");
        fprintf(stderr, "Could be that the configuration file has not been found\n");
        config_destroy(cfg);
        exit(1);
    }
}

char *regex(struct options_t *options, const char *line, const char *pattern)
{
    int ret;
    const char *pcre_error;
    int pcre_error_offset;
    int substring_vector[30];
    const char *matched_string;

    /* check if regexp has already been compiled */
    if (!options->pcre_compiled) {
        options->pcre_compiled = pcre_compile(pattern, 0, &pcre_error,
            &pcre_error_offset, NULL);
        if (!options->pcre_compiled)
            return NULL;

        options->pcre_extra =
            pcre_study(options->pcre_compiled, 0, &pcre_error);
        if (!options->pcre_extra)
            return NULL;
    }

    ret = pcre_exec(options->pcre_compiled, options->pcre_extra, line,
        strlen(line), 0, 0, substring_vector, 30);

    if (ret < 0)
        return NULL;

    pcre_get_substring(line, substring_vector, ret, 0, &matched_string);

    return (char *) matched_string;
}

void *get_parser(struct options_t *options)
{
    char * (*parser)(struct options_t *, const char *, const char*);

    if (!options->incase_option)
        parser = strstr_wrapper;
    else
        parser = strcasestr_wrapper;

    if (options->regexp_option)
        parser = regex;

    return parser;
}

char *strstr_wrapper(struct options_t *options, const char *line, const char *pattern)
{
    return strstr(line, pattern);
}

char *strcasestr_wrapper(struct options_t *options, const char *line, const char *pattern)
{
    return strcasestr(line, pattern);
}

int get_integer_as_string(int integer, char *string)
{
    sprintf(string, "%d", integer);
    return strlen(string) + 1;
}

