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

#include "entry.h"
#include "list.h"
#include "options.h"
#include "search.h"


void do_ngp_search(struct search_t *search);
void do_ag_search(struct search_t *search);
void do_git_search(struct search_t *search);

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

        case AG_SEARCH:
            return do_ag_search(search);

        case GIT_SEARCH:
            return do_git_search(search);

        default:
            break;
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
    free(search->result);

   if (search->options) {
       free_options(search->options);
   }

   free(search);
}
