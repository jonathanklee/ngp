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
