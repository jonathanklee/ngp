/* Copyright (C) 2013  Jonathan Klee

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <menu.h>
#include <signal.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <regex.h>

#define CURSOR_UP 	'k'
#define CURSOR_DOWN 	'j'
#define PAGE_UP		'K'
#define PAGE_DOWN	'J'
#define ENTER	 	'p'
#define QUIT	 	'q'

#ifdef LINE_MAX
	#undef LINE_MAX
#endif
#define LINE_MAX	256

#define synchronized(MUTEX) \
for(mutex = &MUTEX; \
mutex && !pthread_mutex_lock(mutex); \
pthread_mutex_unlock(mutex), mutex = 0)

typedef struct s_entry_t {
	char *data;
	char isfile:1;
} entry_t;

typedef struct s_search_t {
	/* screen */
	int index;
	int cursor;

	/* data */
	entry_t *entries;
	int nbentry;
	int size;

	/* thread */
	pthread_mutex_t data_mutex;
	int status;

	/* search */
	char directory[PATH_MAX];
	char pattern[LINE_MAX];
	char options[LINE_MAX];
	char file_type[8];
	char specific_files_list[256][LINE_MAX];
	int specific_files_number;
	char extensions_list[64][LINE_MAX];
	int extensions_number;
	int raw;
	int regexp;
	int regexp_is_ok;
} search_t;

static search_t	mainsearch;
static search_t	*current;
static pid_t	pid;

static void ncurses_add_file(const char *file);
static void ncurses_add_line(const char *line, const char* file);
static void display_entries(int *index, int *cursor);

static int is_file(int index)
{
	return current->entries[index].isfile;
}

static int is_dir_good(char *dir)
{
	return  strcmp(dir, ".") != 0 &&
		strcmp(dir, "..") != 0 &&
		strcmp(dir, ".git") != 0 ? 1 : 0;
}

static int is_specific_file(const char *name)
{
	int i;
	char *name_begins;

	for (i = 0; i < current->specific_files_number; i++) {
		name_begins = (strrchr(name + 3, '/') != NULL) ? strrchr(name + 3, '/') + 1 : name + 3;
		if (!strcmp(name_begins, current->specific_files_list[i])) {
			return 1;
		}
	}
	return 0;
}

static char * remove_double_appearance(char *initial, char c, char *final)
{
	int i, j;
	int len = strlen(initial);

	for (i = 0, j = 0; i < len; j++ ) {
		if (initial[i] != c) {
			final[j] = initial[i];
			i++;
		} else {
			final[j] = initial[i];
			if (initial[i + 1] == c) {
				i = i + 2;
			} else {
				i++;
			}
		}
	}
	final[j] = '\0';

	return final;
}

static void usage()
{
	fprintf(stderr, "usage: ngp [options]... pattern [directory]\n\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, " -i : ignore case distinctions in pattern\n");
	fprintf(stderr, " -r : raw mode\n");
	fprintf(stderr, " -e : pattern is a regular expression\n");
	fprintf(stderr, " -t type : look for a file extension only\n");
	exit(-1);
}

static void ncurses_init()
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	start_color();
	use_default_colors();
	init_pair(1, -1, -1);
	init_pair(2, COLOR_YELLOW, -1);
	init_pair(3, COLOR_RED, -1);
	init_pair(4, COLOR_MAGENTA, -1);
	init_pair(5, COLOR_GREEN, -1);
	curs_set(0);
}

static void ncurses_stop()
{
	endwin();
}

static void check_alloc()
{
	if (mainsearch.nbentry >= mainsearch.size) {
		mainsearch.size += 500;
		mainsearch.entries = realloc(mainsearch.entries, mainsearch.size * sizeof(entry_t));
	}
}

static void printl(int *y, char *line)
{
	int size;
	int crop = COLS;
	char cropped_line[PATH_MAX];
	char filtered_line[PATH_MAX];
	char *pos;
	char *buf;
	int length=0;

	strncpy(cropped_line, line, crop);
	cropped_line[COLS] = '\0';

	if (isdigit(cropped_line[0])) {
		pos = strtok_r(cropped_line, ":", &buf);
		attron(COLOR_PAIR(2));
		mvprintw(*y, 0, "%s:", pos);
		length = strlen(pos) + 1;
		attron(COLOR_PAIR(1));
		mvprintw(*y, length, "%s", cropped_line + length);
	} else {
		attron(COLOR_PAIR(5));
		mvprintw(*y, 0, "%s", cropped_line, remove_double_appearance(cropped_line, '/', filtered_line));
	}
}

static int display_entry(int *y, int *index, int color)
{
	char filtered_line[PATH_MAX];

	if (*index < current->nbentry) {
		if (!is_file(*index)) {
			if (color == 1) {
				attron(A_REVERSE);
				printl(y, current->entries[*index].data);
				attroff(A_REVERSE);
			} else {
				printl(y, current->entries[*index].data);
			}
		} else {
			attron(A_BOLD);
			if (strcmp(current->directory, "./") == 0)
				printl(y, remove_double_appearance(
					current->entries[*index].data + 3, '/',
					filtered_line));
			else
				printl(y, remove_double_appearance(
					current->entries[*index].data, '/',
					filtered_line));
			attroff(A_BOLD);
		}
	}
}

static char * regex(const char *line, const char *pattern)
{
	int ret;
	regex_t reg;

	ret = regcomp(&reg, pattern, REG_NEWLINE);
	if (ret) {
		regfree(&reg);
		return "1";
	}

        mainsearch.regexp_is_ok = 1;
	ret = regexec(&reg, line, 0, NULL, 0);
	if (!ret) {
		regfree(&reg);
		return "1";
	} else {
		regfree(&reg);
		return NULL;
	}
}

static int parse_file(const char *file, const char *pattern, char *options)
{
	FILE *f;
	char line[LINE_MAX];
	char full_line[LINE_MAX];
	int first;
	int line_number;
	char * (*parser)(const char *, const char*);
	errno = 0;

	f = fopen(file, "r");
	if (f == NULL)
		return -1;

	if (strstr(options, "-i") == NULL)
		parser = strstr;
	else
		parser = strcasestr;

	if (mainsearch.regexp)
		parser = regex;

	first = 1;
	line_number = 1;
	while (fgets(line, sizeof(line), f)) {
		if (parser(line, pattern) != NULL) {
			if (first) {
				if (current->nbentry == 0)
					ncurses_init();
				ncurses_add_file(file);
				first = 0;
			}
			if (line[strlen(line) - 2] == '\r')
				line[strlen(line) - 2] = '\0';
			snprintf(full_line, LINE_MAX, "%d:%s", line_number, line);
			ncurses_add_line(full_line, file);
		}
		line_number++;
	}
	fclose(f);
	return 0;
}

static void lookup_file(const char *file, const char *pattern, char *options)
{
	int i;
	int nb_regex;
	errno = 0;
	pthread_mutex_t *mutex;

	if (mainsearch.raw) {
		synchronized(mainsearch.data_mutex)
			parse_file(file, pattern, options);
		return;
	}

	if (is_specific_file(file)) {
		synchronized(mainsearch.data_mutex)
			parse_file(file, pattern, options);
		return;
	}

	for (i = 0; i < mainsearch.extensions_number; i++) {
		if (!strcmp(mainsearch.extensions_list[i], file + strlen(file) - strlen(mainsearch.extensions_list[i]))) {
			synchronized(mainsearch.data_mutex)
				parse_file(file, pattern, options);
			break;
		}
	}
}

static char * extract_line_number(char *line)
{
	char *token;
	char *buffer;
	token = strtok_r(line, " :", &buffer);
	return token;
}

static int is_simlink(char *file_path)
{
	struct stat filestat;

	lstat(file_path, &filestat);
	return S_ISLNK(filestat.st_mode);
}

static void lookup_directory(const char *dir, const char *pattern,
	char *options, char *file_type)
{
	DIR *dp;

	dp = opendir(dir);
	if (!dp) {
		return;
	}

	while (1) {
		struct dirent *ep;
		ep = readdir(dp);

		if (!ep) {
			break;
		}

		if (!(ep->d_type & DT_DIR) && is_dir_good(ep->d_name)) {
			char file_path[PATH_MAX];
			snprintf(file_path, PATH_MAX, "%s/%s", dir,
				ep->d_name);

			if (!is_simlink(file_path)) {
				if (file_type != NULL) {
					if (!strcmp(file_type, ep->d_name + strlen(ep->d_name) - strlen(file_type) ))
						lookup_file(file_path, pattern, options);
				} else {
					lookup_file(file_path, pattern, options);
				}
			}
		}

		if (ep->d_type & DT_DIR && is_dir_good(ep->d_name)) {
			char path_dir[PATH_MAX] = "";
			snprintf(path_dir, PATH_MAX, "%s/%s", dir,
				ep->d_name);
			lookup_directory(path_dir, pattern, options, file_type);
		}
	}
	closedir(dp);
}

static void display_entries(int *index, int *cursor)
{
	int i = 0;
	int ptr = 0;

	for (i = 0; i < LINES; i++) {
		ptr = *index + i;
		if (i == *cursor) {
			display_entry(&i, &ptr, 1);
		} else {
			display_entry(&i, &ptr, 0);
		}
	}
}

static void ncurses_add_file(const char *file)
{
	char	*new_file;

	check_alloc();
	new_file = malloc(PATH_MAX * sizeof(char));
	strncpy(new_file, file, PATH_MAX);
	current->entries[current->nbentry].data = new_file;
	current->entries[current->nbentry].isfile = 1;
	current->nbentry++;
}

static void ncurses_add_line(const char *line, const char* file)
{
	char	*new_line;

	check_alloc();
	new_line = malloc(LINE_MAX * sizeof(char));
	strncpy(new_line, line, LINE_MAX);
	current->entries[current->nbentry].data = new_line;
	current->entries[current->nbentry].isfile = 0;
	current->nbentry++;
	if (current->nbentry <= LINES)
		display_entries(&current->index, &current->cursor);
}

static void resize(int *index, int *cursor)
{
	/* right now this is a bit trivial,
	 * but we may do more complex moving around
	 * when the window is resized */
	clear();
	display_entries(index, cursor);
	refresh();
}

static void page_up(int *index, int *cursor)
{
	clear();
	refresh();
	if (*index == 0)
		*cursor = 0;
	else
		*cursor = LINES - 1;
	*index -= LINES;
	*index = (*index < 0 ? 0 : *index);

	if (is_file(*index + *cursor) && *index != 0)
		*cursor -= 1;

	display_entries(index, cursor);
}

static void page_down(int *index, int *cursor)
{
	int max_index;

	if (current->nbentry == 0)
		return;

	if (current->nbentry % LINES == 0)
		max_index = (current->nbentry - LINES);
	else
		max_index = (current->nbentry - (current->nbentry % LINES));

	if (*index == max_index)
		*cursor = (current->nbentry - 1) % LINES;
	else
		*cursor = 0;

	clear();
	refresh();
	*index += LINES;
	*index = (*index > max_index ? max_index : *index);

	if (is_file(*index + *cursor))
		*cursor += 1;
	display_entries(index, cursor);
}

static void cursor_up(int *index, int *cursor)
{
	if (*cursor == 0) {
		page_up(index, cursor);
		return;
	}

	if (*cursor > 0) {
		*cursor = *cursor - 1;
	}

	if (is_file(*index + *cursor))
		*cursor = *cursor - 1;

	if (*cursor < 0) {
		page_up(index, cursor);
		return;
	}

	display_entries(index, cursor);
}

static void cursor_down(int *index, int *cursor)
{
	if (*cursor == (LINES - 1)) {
		page_down(index, cursor);
		return;
	}

	if (*cursor + *index < current->nbentry - 1) {
		*cursor = *cursor + 1;
	}

	if (is_file(*index + *cursor))
		*cursor = *cursor + 1;

	if (*cursor > (LINES - 1)) {
		page_down(index, cursor);
		return;
	}

	display_entries(index, cursor);
}

int find_file(int index)
{
	while (!is_file(index))
		index--;

	return index;
}

static void open_entry(int index, const char *editor, const char *pattern)
{
	char command[PATH_MAX];
	char filtered_file_name[PATH_MAX];
	char line_copy[PATH_MAX];
	int file_index;
	pthread_mutex_t *mutex;

	file_index = find_file(index);
	synchronized(mainsearch.data_mutex) {
		strcpy(line_copy, current->entries[index].data);
		snprintf(command, sizeof(command), editor,
			extract_line_number(line_copy),
			remove_double_appearance(
				current->entries[file_index].data, '/',
				filtered_file_name),
			pattern);
	}
	system(command);
}

void clean_search(search_t *search)
{
	int i;

	for (i=0; i<search->nbentry; i++) {
		free(search->entries[i].data);
	}
	free(search->entries);
}

static void sig_handler(int signo)
{
	if (signo == SIGINT) {
		ncurses_stop();
		clean_search(&mainsearch);
		exit(-1);
	}
}

static void configuration_init(config_t *cfg)
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

void * lookup_thread(void *arg)
{
	search_t *d = (search_t *) arg;

	lookup_directory(d->directory, d->pattern, d->options, d->file_type);
	d->status = 0;
}

void init_searchstruct(search_t *searchstruct)
{
	searchstruct->index = 0;
	searchstruct->cursor = 0;
	searchstruct->size = 100;
	searchstruct->nbentry = 0;
	searchstruct->status = 1;
	searchstruct->raw = 0;
	strcpy(searchstruct->directory, "./");
}

void display_status(void)
{
	char *rollingwheel[] = {
		".  ", ".  ", ".  ", ".  ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		" . ", " . ", " . ", " . ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		"  .", "  .", "  .", "  .",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		"   ", "   ", "   ", "   ",
		};
	static int i = 0;

	attron(COLOR_PAIR(1));
	if (mainsearch.status)
		mvaddstr(0, COLS - 3, rollingwheel[++i%60]);
	else
		mvaddstr(0, COLS - 5, "Done.");
}

int main(int argc, char *argv[])
{
	DIR *dp;
	int opt;
	struct dirent *ep;
	int ch;
	int first = 0;
	const char *editor;
	const char *specific_files;
	const char *extensions;
	char *ptr;
	char *buf;
	config_t cfg;
	pthread_mutex_t *mutex;

	current = &mainsearch;
	init_searchstruct(&mainsearch);
	pthread_mutex_init(&mainsearch.data_mutex, NULL);

	while ((opt = getopt(argc, argv, "heit:r")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			break;
		case 'i':
			strcpy(mainsearch.options, "-i");
			break;
		case 't':
			strncpy(mainsearch.file_type, optarg, 8);
			break;
		case 'r':
			mainsearch.raw = 1;
			break;
		case 'e':
			mainsearch.regexp = 1;
			break;
		default:
			exit(-1);
			break;
		}
	}

	if (argc - optind < 1 || argc - optind > 2) {
		usage();
	}

	for ( ; optind < argc; optind++) {
		if (!first) {
			strcpy(mainsearch.pattern, argv[optind]);
			first = 1;
		} else {
			strcpy(mainsearch.directory, argv[optind]);
		}
	}

	configuration_init(&cfg);
	if (!config_lookup_string(&cfg, "editor", &editor)) {
		fprintf(stderr, "ngprc: no editor string found!\n");
		exit(-1);
	}

	if (!config_lookup_string(&cfg, "files", &specific_files)) {
		fprintf(stderr, "ngprc: no files string found!\n");
		exit(-1);
	}

	mainsearch.specific_files_number = 0;
	ptr = strtok_r((char *) specific_files, " ", &buf);
	while (ptr != NULL) {
		strcpy(mainsearch.specific_files_list[mainsearch.specific_files_number],
			ptr);
		mainsearch.specific_files_number++;
		ptr = strtok_r(NULL, " ", &buf);
	}

	/* getting files extensions from configuration */
	if (!config_lookup_string(&cfg, "extensions", &extensions)) {
		fprintf(stderr, "ngprc: no extensions string found!\n");
		exit(-1);
	}

	mainsearch.extensions_number = 0;
	ptr = strtok_r((char *) extensions, " ", &buf);
	while (ptr != NULL) {
		strcpy(mainsearch.extensions_list[mainsearch.extensions_number],
			ptr);
		mainsearch.extensions_number++;
		ptr = strtok_r(NULL, " ", &buf);
	}

	signal(SIGINT, sig_handler);

	mainsearch.entries = (entry_t *) calloc(mainsearch.size, sizeof(entry_t));

	if (pthread_create(&pid, NULL, &lookup_thread, &mainsearch)) {
		fprintf(stderr, "ngp: cannot create thread");
		clean_search(&mainsearch);
		exit(-1);
	}

	synchronized(mainsearch.data_mutex)
		display_entries(&mainsearch.index, &mainsearch.cursor);

	while (ch = getch()) {
		switch(ch) {
		case KEY_RESIZE:
			synchronized(mainsearch.data_mutex)
				resize(&current->index, &current->cursor);
			break;
		case CURSOR_DOWN:
		case KEY_DOWN:
			synchronized(mainsearch.data_mutex)
				cursor_down(&current->index, &current->cursor);
			break;
		case CURSOR_UP:
		case KEY_UP:
			synchronized(mainsearch.data_mutex)
				cursor_up(&current->index, &current->cursor);
			break;
		case KEY_PPAGE:
		case PAGE_UP:
			synchronized(mainsearch.data_mutex)
				page_up(&current->index, &current->cursor);
			break;
		case KEY_NPAGE:
		case PAGE_DOWN:
			synchronized(mainsearch.data_mutex)
				page_down(&current->index, &current->cursor);
			break;
		case ENTER:
		case '\n':
			if (mainsearch.nbentry == 0)
				break;
			ncurses_stop();
			open_entry(current->cursor + current->index, editor,
				current->pattern);
			ncurses_init();
			resize(&current->index, &current->cursor);
			break;
		case QUIT:
			goto quit;
		default:
			break;
		}

		usleep(10000);
		refresh();
		synchronized(mainsearch.data_mutex) {
			display_status();
		}

		synchronized(mainsearch.data_mutex) {
			if (mainsearch.status == 0 && mainsearch.nbentry == 0) {
				goto quit;
			}
		}
	}

quit:
	ncurses_stop();
	clean_search(&mainsearch);
}

