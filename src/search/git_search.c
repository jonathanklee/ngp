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
#include <string.h>

#include "../file.h"
#include "../line.h"

#include "search_utils.h"

/*
 * command: git grep -n --heading -C 1 -p --color=always "git_"
 *
 *[36m--[m
 *src/search/search.c
 *28[36m=[mvoid do_ag_search(struct search_t *search);
 *29[36m:[mvoid do_[1;31mgit_[msearch(struct search_t *search);
 *30[36m-[m
 *[36m--[m
 *47[36m=[mvoid do_search(struct search_t *search)
 *[36m--[m
 *56[36m-[m        case GIT_SEARCH:
 *57[36m:[m            return do_[1;31mgit_[msearch(search);
 *58[36m-[m
 */

static int match_blank_line(struct result_t *result, const char *output)
{
    /* empty line */
    size_t line_length = strlen(output);
    if (line_length == 0) {
        result->entries = create_blank_line(result);
        return 1;
    }

    /* only colored '--' */
    const char* match = apply_regex(output, "^(\\033\\[36m)(--)(\\033\\[m)$");
    if (!match)
        return 0;

    result->entries = create_blank_line(result);
    pcre_free_substring(match);

    return 1;
}

static int match_file(struct result_t *result, const char *output)
{
    const char* match = apply_regex(output, "^([^\\033]+)$");
    if (!match)
        return 0;

    if(!validate_file(match))
        return 1;

    result->entries = create_file(result, (char*)match);
    pcre_free_substring(match);

    return 1;
}

static int match_line(struct result_t *result, const char *output)
{
    /* match line number */
    const char *match = apply_regex(output, "^\\d+");
    if (!match)
        return 0;

    size_t line_number = atoi(match);
    pcre_free_substring(match);

    if (line_number == 0)
        return 0;

    /* match context lines ('-' or '=' after line number) */
    match = apply_regex(output, "(?<=([-=]\\033\\[m)).*$");
    if (match) {
        result->entries = create_unselectable_line(result, (char*)match, line_number);
        pcre_free_substring(match);
        return 1;
    }


    range_t highlight = {0, 0};
    char line[4096] = {'\0'};

    /* match from line number until match */
    match = apply_regex(output, "(?<=([:]\\033\\[m))[^\\033]*");
    if (!match)
        return 0;

    strcat(line, match);
    pcre_free_substring(match);

    /* match the highlighted match */
    match = apply_regex(output, "(?<=(\\033\\[1;31m))[^\\033]*");
    if (!match)
        return 0;

    highlight.begin = strlen(line);
    highlight.end = highlight.begin + strlen(match);
    strcat(line, match);
    pcre_free_substring(match);

    /* match rest of line */
    match = apply_regex(output, "(?<=(\\033\\[m))[^\\033]*$");
    if (!match)
        return 0;

    strcat(line, match);
    pcre_free_substring(match);

    result->entries = create_line(result, line, line_number, highlight);

    return 1;
}

void do_git_search(struct search_t *search)
{
    char default_arguments[] = "-n --heading --color=always ";
    external_parser_t git_grep = {
        default_arguments,
        match_file,
        match_line,
        match_blank_line
    };

    popen_search(search, &git_grep);
}
