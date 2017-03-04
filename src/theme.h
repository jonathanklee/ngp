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

#ifndef THEMES_H
#define THEMES_H

#include <libconfig.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

struct theme_t {
    int line_color;
    int file_color;
    int line_number_color;
    int highlight_color;
    int opened_line_color;

};

enum color_t {
    NO_COLOR,
    COLOR_LINE,
    COLOR_LINE_NUMBER,
    COLOR_OPENED_LINE,
    COLOR_HIGHLIGHT,
    COLOR_FILE
};

struct theme_t *read_theme();
void apply_theme(struct theme_t *theme);
void destroy_theme(struct theme_t *theme);

#endif /* THEMES_H */
