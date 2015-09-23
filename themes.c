#include "utils.h"
#include "themes.h"

static char lines_color[64];
static char line_numbers_color[64];
static char pattern_color[64];
static char files_color[64];
static char parsed_pattern_color[64];

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

void read_theme(void)
{
	config_t cfg;
	const char *buffer;

	configuration_init(&cfg);

	if (!config_lookup_string(&cfg, "lines_color", &buffer)) {
		fprintf(stderr, "ngprc: no lines_color string found!\n");
		goto exit_read_theme;
	}
	strcpy(lines_color, buffer);

	if (!config_lookup_string(&cfg, "line_numbers_color", &buffer)) {
		fprintf(stderr, "ngprc: no line_numbers_color string found!\n");
		goto exit_read_theme;
	}
	strcpy(line_numbers_color, buffer);

	if (!config_lookup_string(&cfg, "pattern_color", &buffer)) {
		fprintf(stderr, "ngprc: no pattern_color string found!\n");
		goto exit_read_theme;
	}
	strcpy(pattern_color, buffer);

	if (!config_lookup_string(&cfg, "files_color", &buffer)) {
		fprintf(stderr, "ngprc: no files_color string found!\n");
		goto exit_read_theme;
	}
	strcpy(files_color, buffer);

	if (!config_lookup_string(&cfg, "parsed_pattern_color", &buffer)) {
		fprintf(stderr, "ngprc: no parsed_pattern_color string found!\n");
		goto exit_read_theme;
	}
	strcpy(parsed_pattern_color, buffer);

exit_read_theme:
	config_destroy(&cfg);
}

void apply_theme(void)
{
	read_theme();
	init_pair(1, get_color(lines_color), -1);
	init_pair(2, get_color(line_numbers_color), -1);
	init_pair(3, get_color(parsed_pattern_color), -1);
	init_pair(4, get_color(pattern_color), -1);
	init_pair(5, get_color(files_color), -1);
}
