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

#include "file.h"
#include "line.h"

#include "search_utils.h"

/*
 * command: ag -H --color -C 1 "ag_"
 *
 *[0m[35msrc/search.c[0m
 *[0m[32m28[0m-void do_ag_search(struct search_t *search);
 *[0m[32m29[0m:void do_[0m[1m[31mrg_[0msearch(struct search_t *search);
 *[0m[32m30[0m-void do_git_search(struct search_t *search);
 *--
 *[0m[32m57[0m-        case RG_SEARCH:
 *[0m[32m58[0m:            return do_[0m[1m[31mrg_[0msearch(search);
 *[0m[32m59[0m-
 *
 */

static int match_blank_line(struct result_t *result, const char *output)
{
    /* empty line */
    size_t line_length = strlen(output);
    if (line_length == 0) {
        return 1;
    }

    /* only '--' */
    const char* match = apply_regex(output, "^(--)$");
    if (!match)
        return 0;

    result->entries = create_blank_line(result);
    pcre_free_substring(match);

    return 1;
}

static int match_file(struct result_t *result, const char *output)
{
    const char *match = apply_regex(output, "(?<=(\\[35m))[^\\033]*");
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
    const char *match = apply_regex(output, "(?<=(\\[32m))[^\\033]*");
    if (!match) {
        return 0;
    }

    size_t line_number = atoi(match);
    pcre_free_substring(match);

    if (line_number == 0)
        return 0;

    /* match context lines ('-' after line number) */
    match = apply_regex(output, "(?<=(\\[[-])).*$");
    if (match) {
        result->entries = create_unselectable_line(result, (char*)match, line_number);
        pcre_free_substring(match);
        return 1;
    }


    range_t highlight = {0, 0};
    size_t line_length = 1024;
    char *line = calloc(line_length, sizeof(*line));

    /* match from line number until match */
    match = apply_regex(output, "(?<=(\\[0m[:]))[^\\033]*");
    if (!match)
        return 0;

    resize_string(&line, &line_length, strlen(match));
    strcat(line, match);
    pcre_free_substring(match);

    /* match the highlighted match */
    match = apply_regex(output, "(?<=(\\[31m))[^\\033]*");
    if (!match)
        return 0;

    highlight.begin = strlen(line);
    highlight.end = highlight.begin + strlen(match);

    resize_string(&line, &line_length, strlen(match));
    strcat(line, match);
    pcre_free_substring(match);

    /* match rest of line */
    match = apply_regex(output, "(?<=(\\[0m))[^\\033]*$");
    if (!match)
        return 0;

    resize_string(&line, &line_length, strlen(match));
    strcat(line, match);
    pcre_free_substring(match);

    result->entries = create_line(result, line, line_number, highlight);

    return 1;
}

void do_rg_search(struct search_t *search)
{
    char default_arguments[] = "-H --color always -n --heading ";
    external_parser_t ag = {
        default_arguments,
        match_file,
        match_line,
        match_blank_line
    };

    popen_search(search, &ag);
}
