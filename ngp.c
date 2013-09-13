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

#define CURSOR_UP 	'k'
#define CURSOR_DOWN 	'j'
#define PAGE_UP		'K'
#define PAGE_DOWN	'J'
#define ENTER	 	'p'
#define QUIT	 	'q'

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
	char pattern[NAME_MAX];
	char options[NAME_MAX];
	char file_type[4];
	char specific_files_list[256][NAME_MAX];
	int specific_files_number;
	char extensions_list[64][NAME_MAX];
	int extensions_number;
	int raw;	
} search_t;

static search_t	search;
static pid_t	pid;

static void ncurses_add_file(const char *file);
static void ncurses_add_line(const char *line, const char* file);

static int is_file(int index)
{
	return search.entries[index].isfile;
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
	for (i = 0; i < search.specific_files_number; i++) {
		if (!strcmp(name + 3, search.specific_files_list[i])) {
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
	if (search.nbentry >= search.size) {
		search.size += 500;
		search.entries = realloc(search.entries, search.size * sizeof(entry_t));
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

	if (*index < search.nbentry) {
		if (!is_file(*index)) {
			if (color == 1) {
				attron(A_REVERSE);
				printl(y, search.entries[*index].data);
				attroff(A_REVERSE);
			} else {
				printl(y, search.entries[*index].data);
			}
		} else {
			attron(A_BOLD);
			if (strcmp(search.directory, "./") == 0)
				printl(y, remove_double_appearance(
					search.entries[*index].data + 3, '/',
					filtered_line));
			else
				printl(y, remove_double_appearance(
					search.entries[*index].data, '/',
					filtered_line));
			attroff(A_BOLD);
		}
	}
}

static int sanitize_filename(char *file)
{
	char out[256];
	char *tok;
	char *buf;

	if ((tok = strtok_r(file, " ", &buf)) != NULL) {
		strncpy(out, tok, 255);
	}

	while((tok = strtok_r(NULL, " ", &buf)) != NULL) {
		strncat(out, "\\ ", 255);
		strncat(out, tok, 255);
	}

	//FIXME: use reentrant
	strncpy(file, out, 255);

	return 0;
}

static int parse_file(const char *file, const char *pattern, char *options)
{
	FILE *f;
	char line[256];
	char command[256];
	char full_line[256];
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

	first = 1;
	line_number = 1;
	while (fgets(line, sizeof(line), f)) {
		if (parser(line, pattern) != NULL) {
			if (first) {
				ncurses_add_file(file);
				first = 0;
			}
			if (line[strlen(line) - 2] == '\r')
				line[strlen(line) - 2] = '\0';
			snprintf(full_line, 256, "%d:%s", line_number, line);
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

	if (search.raw) {
		synchronized(search.data_mutex)
			parse_file(file, pattern, options);
		return;
	}

	if (is_specific_file(file)) {
		synchronized(search.data_mutex)
			parse_file(file, pattern, options);
		return;
	}

	for (i = 0; i < search.extensions_number; i++) {
		if (!strcmp(search.extensions_list[i], file + strlen(file) - strlen(search.extensions_list[i]))) {
			synchronized(search.data_mutex)
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

			if (strchr(file_path, ' ') != NULL)
				sanitize_filename(file_path);

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
	search.entries[search.nbentry].data = new_file;
	search.entries[search.nbentry].isfile = 1;
	search.nbentry++;
}

static void ncurses_add_line(const char *line, const char* file)
{
	char	*new_line;

	check_alloc();
	new_line = malloc(NAME_MAX * sizeof(char));
	strncpy(new_line, line, NAME_MAX);
	search.entries[search.nbentry].data = new_line;
	search.entries[search.nbentry].isfile = 0;
	search.nbentry++;
	if (search.nbentry < LINES)
		display_entries(&search.index, &search.cursor);
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
	if (search.nbentry % LINES == 0)
		max_index = (search.nbentry - LINES);
	else
		max_index = (search.nbentry - (search.nbentry % LINES));

	if (*index == max_index)
		*cursor = (search.nbentry - 1) % LINES;
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

	if (*cursor + *index < search.nbentry - 1) {
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
	synchronized(search.data_mutex) {
		strcpy(line_copy, search.entries[index].data);
		snprintf(command, sizeof(command), editor,
			extract_line_number(line_copy),
			remove_double_appearance(
				search.entries[file_index].data, '/',
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
		clean_search(&search);
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

int main(int argc, char *argv[])
{
	DIR *dp;
	int opt;
	struct dirent *ep;
	int ch;
	int first = 0;
	char command[128];
	const char *editor;
	const char *specific_files;
	const char *extensions;
	char *ptr;
	char *buf;
	config_t cfg;
	pthread_mutex_t *mutex;

	search.index = 0;
	search.cursor = 0;
	search.size = 100;
	search.nbentry = 0;
	search.status = 1;
	search.raw = 0;
	strcpy(search.directory, "./");

	pthread_mutex_init(&search.data_mutex, NULL);

	while ((opt = getopt(argc, argv, "hit:r")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			break;
		case 'i':
			strcpy(search.options, "-i");
			break;
		case 't':
			strncpy(search.file_type, optarg, 3);
			break;
		case 'r':
			search.raw = 1;
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
			strcpy(search.pattern, argv[optind]);
			first = 1;
		} else {
			strcpy(search.directory, argv[optind]);
		}
	}

	pthread_mutex_init(&search.data_mutex, NULL);

	configuration_init(&cfg);
	if (!config_lookup_string(&cfg, "editor", &editor)) {
		fprintf(stderr, "ngprc: no editor string found!\n");
		exit(-1);
	}

	if (!config_lookup_string(&cfg, "files", &specific_files)) {
		fprintf(stderr, "ngprc: no files string found!\n");
		exit(-1);
	}
	
	search.specific_files_number = 0;
	ptr = strtok_r((char *) specific_files, " ", &buf);
	while (ptr != NULL) {
		strcpy(search.specific_files_list[search.specific_files_number],
			ptr);
		search.specific_files_number++;
		ptr = strtok_r(NULL, " ", &buf);
	}

	/* getting files extensions from configuration */
	if (!config_lookup_string(&cfg, "extensions", &extensions)) {
		fprintf(stderr, "ngprc: no extensions string found!\n");
		exit(-1);
	}

	search.extensions_number = 0;
	ptr = strtok_r((char *) extensions, " ", &buf);
	while (ptr != NULL) {
		strcpy(search.extensions_list[search.extensions_number],
			ptr);
		search.extensions_number++;
		ptr = strtok_r(NULL, " ", &buf);
	}

	signal(SIGINT, sig_handler);

	search.entries = (entry_t *) calloc(search.size, sizeof(entry_t));

	if (pthread_create(&pid, NULL, &lookup_thread, &search)) {
		fprintf(stderr, "ngp: cannot create thread");
		clean_search(&search);
		exit(-1);
	}

	ncurses_init();

	synchronized(search.data_mutex)
		display_entries(&search.index, &search.cursor);

	while (ch = getch()) {
		switch(ch) {
		case KEY_RESIZE:
			synchronized(search.data_mutex)
				resize(&search.index, &search.cursor);
			break;
		case CURSOR_DOWN:
		case KEY_DOWN:
			synchronized(search.data_mutex)
				cursor_down(&search.index, &search.cursor);
			break;
		case CURSOR_UP:
		case KEY_UP:
			synchronized(search.data_mutex)
				cursor_up(&search.index, &search.cursor);
			break;
		case KEY_PPAGE:
		case PAGE_UP:
			synchronized(search.data_mutex)
				page_up(&search.index, &search.cursor);
			break;
		case KEY_NPAGE:
		case PAGE_DOWN:
			synchronized(search.data_mutex)
				page_down(&search.index, &search.cursor);
			break;
		case ENTER:
		case '\n':
			ncurses_stop();
			open_entry(search.cursor + search.index, editor,
				search.pattern);
			ncurses_init();
			resize(&search.index, &search.cursor);
			break;
		case QUIT:
			goto quit;
		default:
			break;
		}

		usleep(10000);
		refresh();

		synchronized(search.data_mutex) {
			if (search.status == 0 && search.nbentry == 0) {
				goto quit;
			}
		}
	}

quit:
	ncurses_stop();
	clean_search(&search);
}

