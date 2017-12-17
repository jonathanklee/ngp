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
#include "theme.h"

static int get_color(const char *color)
{
    if (!strcmp(color, "yellow")) {
        return COLOR_YELLOW;
    } else if (!strcmp(color, "red")) {
        return COLOR_RED;
    } else if (!strcmp(color, "cyan")) {
        return COLOR_CYAN;
    } else if (!strcmp(color, "green")) {
        return COLOR_GREEN;
    } else if (!strcmp(color, "black")) {
        return COLOR_BLACK;
    } else if (!strcmp(color, "blue")) {
        return COLOR_BLUE;
    } else if (!strcmp(color, "white")) {
        return COLOR_WHITE;
    } else if (!strcmp(color, "magenta")) {
        return COLOR_MAGENTA;
    }
    return -1;
}

struct theme_t *read_theme(struct configuration_t *config)
{
    const char *buffer;
    char line_color[64];
    char line_number_color[64];
    char highlight_color[64];
    char file_color[64];
    char opened_line_color[64];
    struct theme_t *new;

    new = calloc(1, sizeof(struct theme_t));
    load_configuration(config);
    config_t cfg = config->config;

    if (!config_lookup_string(&cfg, "line_color", &buffer)) {
        fprintf(stderr, "ngprc: no line_color string found!\n");
        goto exit_read_theme;
    }
    strcpy(line_color, buffer);

    if (!config_lookup_string(&cfg, "line_number_color", &buffer)) {
        fprintf(stderr, "ngprc: no line_number_color string found!\n");
        goto exit_read_theme;
    }
    strcpy(line_number_color, buffer);

    if (!config_lookup_string(&cfg, "highlight_color", &buffer)) {
        fprintf(stderr, "ngprc: no highlight_color string found!\n");
        goto exit_read_theme;
    }
    strcpy(highlight_color, buffer);

    if (!config_lookup_string(&cfg, "file_color", &buffer)) {
        fprintf(stderr, "ngprc: no file_color string found!\n");
        goto exit_read_theme;
    }
    strcpy(file_color, buffer);

    if (!config_lookup_string(&cfg, "opened_line_color", &buffer)) {
        fprintf(stderr, "ngprc: no opened_line_color string found!\n");
        goto exit_read_theme;
    }
    strcpy(opened_line_color, buffer);

    new->line_color = get_color(line_color);
    new->line_number_color = get_color(line_number_color);
    new->opened_line_color = get_color(opened_line_color);
    new->highlight_color = get_color(highlight_color);
    new->file_color = get_color(file_color);

    return new;

exit_read_theme:
    config_destroy(&cfg);
    destroy_theme(new);
    return NULL;
}

void apply_theme(struct theme_t *theme)
{
    init_pair(COLOR_LINE, theme->line_color, -1);
    init_pair(COLOR_LINE_NUMBER, theme->line_number_color, -1);
    init_pair(COLOR_OPENED_LINE, theme->opened_line_color, -1);
    init_pair(COLOR_HIGHLIGHT, theme->highlight_color, -1);
    init_pair(COLOR_FILE, theme->file_color, -1);
}

void destroy_theme(struct theme_t *theme)
{
    free(theme);
}

