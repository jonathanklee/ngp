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

#include "../entry.h"
#include "../list.h"
#include "search.h"
#include "ngp_search.h"
#include "external_search.h"

#include <string.h>

struct search_t * create_search( struct options_t *options )
{
    struct result_t* result = calloc(1, sizeof(*result));
    result->entries = NULL;
    result->start = result->entries;
    result->nbentry = 0;

    struct search_t *search = calloc(1, sizeof(*search));
    search->result = result;
    search->status = 1;

    search->options = options;

    return search;
}

void do_search(struct search_t *search)
{
    switch (search->options->search_type) {
        case NGP_SEARCH:
            return do_ngp_search(search);

        case EXTERNAL_SEARCH:
            return do_external_search(search);
    }

    exit(-1);
}

void free_search(struct search_t *search)
{
    struct entry_t *ptr = search->result->start;
    struct entry_t *p;

    while (ptr) {
        p = ptr;
        ptr = ptr->next;
        free_entry(p);
    }

    free_list(&search->options->extension);
    free_list(&search->options->specific_file);
    free_list(&search->options->ignore);

    /* free pcre stuffs if needed */
    if (search->options->pcre_compiled)
        pcre_free((void *) search->options->pcre_compiled);

    if (search->options->pcre_extra)
        pcre_free((void *) search->options->pcre_extra);
}

struct options_t * create_options()
{
    struct options_t *options = calloc(1, sizeof(*options));

    options->search_type = NGP_SEARCH;
    strcpy(options->directory, ".");

    return options;
}
