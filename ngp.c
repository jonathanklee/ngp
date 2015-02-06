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
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <ctype.h>

#define NGP_VERSION 	"1.2"

#define CURSOR_UP 	'k'
#define CURSOR_DOWN 	'j'
#define PAGE_UP		'K'
#define PAGE_DOWN	'J'
#define ENTER	 	'p'
#define QUIT	 	'q'
#define MARK	 	'm'

#ifdef LINE_MAX
	#undef LINE_MAX
#endif
#define LINE_MAX	512

#define synchronized(MUTEX) \
for(mutex = &MUTEX; \
mutex && !pthread_mutex_lock(mutex); \
pthread_mutex_unlock(mutex), mutex = 0)

typedef struct s_entry_t {
	struct s_entry_t *next;
	int isfile;
	int len;
	int opened;
	int mark;
	char data[];
} entry_t;

struct list {
	struct list *next;
	char data[];
};

typedef struct s_search_t {
	/* screen */
	int index;
	int cursor;

	/* data */
	entry_t *entries;
	entry_t *start;
	int nbentry;

	/* thread */
	pthread_mutex_t data_mutex;
	int status;

	/* search */
	const char *editor;
	char directory[PATH_MAX];
	char pattern[LINE_MAX];
	char options[LINE_MAX];
	struct list *specific_file;
	struct list *extension;
	struct list *ignore;
	int raw;
	int regexp;
	int regexp_is_ok;
} search_t;

static search_t	mainsearch;
static search_t	*current;
static pthread_t pid;

static void ncurses_add_file(const char *file);
static void ncurses_add_line(const char *line);
static void display_entries(int *index, int *cursor);

static int is_file(int index)
{
	int i;
	entry_t *ptr = current->start;

	for (i = 0; i < index; i++)
		ptr = ptr->next;

	return ptr->isfile;
}

static int is_dir_good(char *dir)
{
	return  *dir != '.' &&
		strcmp(dir, "..") != 0 &&
		strcmp(dir, ".git") != 0 ? 1 : 0;
}

static char * get_file_name(const char * absolute_path)
{
	char *ret;

	if (strrchr(absolute_path + 3, '/') != NULL)
		ret = strrchr(absolute_path + 3, '/') + 1;
	else
		ret = (char *) absolute_path + 3;

	return ret;
}

static int is_specific_file(const char *name)
{
	char *name_begins;
	struct list *pointer = mainsearch.specific_file;

	while (pointer) {
		name_begins = get_file_name(name);
		if (!strcmp(name_begins, pointer->data))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

static int is_ignored_file(const char *name)
{
	char *name_begins;
	struct list *pointer = mainsearch.ignore;

	while (pointer) {
		name_begins = get_file_name(name);
		if (!strcmp(name_begins, pointer->data))
			return 1;
		pointer = pointer->next;
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
			if (initial[i + 1] == c)
				i = i + 2;
			else
				i++;
		}
	}
	final[j] = '\0';

	return final;
}

static void usage(void)
{
	fprintf(stderr, "usage: ngp [options]... pattern [directory]\n\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, " -i : ignore case distinctions in pattern\n");
	fprintf(stderr, " -r : raw mode\n");
	fprintf(stderr, " -t type : look into files that have this extension\n");
	fprintf(stderr, " -I name : ignore file that have this name\n");
	fprintf(stderr, " -e : pattern is a regular expression\n");
	fprintf(stderr, " -v : display version\n");
	exit(-1);
}

static void ncurses_init(void)
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
	init_pair(4, COLOR_CYAN, -1);
	init_pair(5, COLOR_GREEN, -1);
	curs_set(0);
}

static void ncurses_stop(void)
{
	endwin();
}

static entry_t *alloc_word(entry_t *list, int len, int type)
{
	entry_t *new;

	new = calloc(1, sizeof(struct s_entry_t) + len);
	new->len = len;
	new->isfile = type;
	new->next = NULL;
	new->opened = 0;
	new->mark = 0;

	if (list) {
		/* if list not empty, add new element at end */
		list->next = new;
	} else {
		/* if list is emptry,
		set first element as start of list */
		current->start = new;
	}

	return new;
}

static void print_line(int *y, entry_t *entry)
{
	char *pos;
	char *buf = NULL;
	char *pattern;
	char *ptr;
	int length = 0;
	int crop = COLS;
	int counter = 0;
	char cropped_line[PATH_MAX] = "";
	char *line = entry->data;

	strncpy(cropped_line, line, crop);

	/* first clear line */
	move(*y, 0);
	clrtoeol();

	/* display line number */
	pos = strtok_r(cropped_line, ":", &buf);
	attron(COLOR_PAIR(2));
	mvprintw(*y, 0, "%s:", pos);

	/* display rest of line */
	length = strlen(pos) + 1;
	attron(COLOR_PAIR(1));
	mvprintw(*y, length, "%s", cropped_line + length);

	/* highlight pattern */
        if (strstr(current->options, "-i") == NULL)
                pattern = strstr(cropped_line + length, current->pattern);
        else
                pattern = strcasestr(cropped_line + length, current->pattern);

        if (!pattern)
        	return;

	ptr = cropped_line + length;
	move(*y, length);
	while (ptr != pattern) {
		addch(*ptr);
		ptr++;
	}

	/* switch color to red or cyan */
	attron(A_REVERSE);
	if (entry->opened)
		attron(COLOR_PAIR(3));
	else
		attron(COLOR_PAIR(4));

	if (!entry->opened && entry->mark)
		attron(COLOR_PAIR(2));

	length = strlen(current->pattern);

	for (counter = 0; counter < length; counter++, ptr++)
		addch(*ptr);

	attroff(A_REVERSE);
}

static void print_file(int *y, char *line)
{
	char filtered_line[PATH_MAX];
	char cropped_line[PATH_MAX] = "";
	int crop = COLS;

	/* first clear line */
	move(*y, 0);
	clrtoeol();

	strncpy(cropped_line, line, crop);
	attron(COLOR_PAIR(5));
	mvprintw(*y, 0, "%s", cropped_line,
		remove_double_appearance(cropped_line, '/', filtered_line));
}

static void display_entry(int *y, entry_t *ptr, int color)
{
	char filtered_line[PATH_MAX];

	if (!ptr->isfile) {
		if (color == 1) {
			attron(A_REVERSE);
			print_line(y, ptr);
			attroff(A_REVERSE);
		} else {
			print_line(y, ptr);
		}
	} else {
		attron(A_BOLD);
		if (strcmp(current->directory, "./") == 0)
			print_file(y, remove_double_appearance(
				ptr->data + 3, '/',
				filtered_line));
		else
			print_file(y, remove_double_appearance(
				ptr->data, '/',
				filtered_line));
		attroff(A_BOLD);
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
	int f;
	char full_line[LINE_MAX];
	char *p;
	char *start;
	char *end;
	char *endline;
	int first;
	struct stat sb;
	int line_number;
	char * (*parser)(const char *, const char*);
	errno = 0;

	f = open(file, O_RDONLY);
	if (f < 0)
		return -1;

	if (fstat(f, &sb) < 0) {
		close(f);
		return -1;
	}

	p = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, f, 0);
	if (p == MAP_FAILED) {
		close(f);
		return -1;
	}

	close(f);

	if (strstr(options, "-i") == NULL)
		parser = strstr;
	else
		parser = strcasestr;

	if (mainsearch.regexp)
		parser = regex;

	first = 1;
	line_number = 1;
	start = p;
	end = p + sb.st_size;
	while (1) {

		if (p == end)
			break;

		endline = strchr(p, '\n');
		if (endline == NULL)
			break;

		/* replace \n with \0 */
		*endline = '\0';

		if (parser(p, pattern) != NULL) {
			if (first) {
				if (current->nbentry == 0)
					ncurses_init();
				ncurses_add_file(file);
				first = 0;
			}
			if (p[strlen(p) - 2] == '\r')
				p[strlen(p) - 2] = '\0';
			snprintf(full_line, LINE_MAX, "%d:%s", line_number, p);
			ncurses_add_line(full_line);
		}

		/* switch back to \n */
		*endline = '\n';
		p = endline + 1;
		line_number++;
	}

	if (munmap(start, sb.st_size) < 0)
		return -1;

	return 0;
}

static int is_extension_good(const char *file) {

	struct list *pointer;

	pointer = mainsearch.extension;
	while (pointer) {
		if (!strcmp(pointer->data, file + strlen(file) -
			strlen(pointer->data)))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

static void lookup_file(const char *file, const char *pattern, char *options)
{
	errno = 0;
	pthread_mutex_t *mutex;

	if (is_ignored_file(file))
		return;

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

	if (is_extension_good(file)) {
		synchronized(mainsearch.data_mutex)
			parse_file(file, pattern, options);
		return;
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
	char *options)
{
	DIR *dp;

	dp = opendir(dir);
	if (!dp)
		return;

	if (is_ignored_file(dir))
		return;

	while (1) {
		struct dirent *ep;
		ep = readdir(dp);

		if (!ep)
			break;

		if (!(ep->d_type & DT_DIR)) {
			char file_path[PATH_MAX];
			snprintf(file_path, PATH_MAX, "%s/%s", dir,
				ep->d_name);

			if (!is_simlink(file_path)) {
				lookup_file(file_path, pattern, options);
			}
		}

		if (ep->d_type & DT_DIR && is_dir_good(ep->d_name)) {
			char path_dir[PATH_MAX] = "";
			snprintf(path_dir, PATH_MAX, "%s/%s", dir,
				ep->d_name);
			lookup_directory(path_dir, pattern, options);
		}
	}
	closedir(dp);
}

static void display_entries(int *index, int *cursor)
{
	int i = 0;
	entry_t *ptr = current->start;

	for (i = 0; i < *index; i++)
		ptr = ptr->next;

	for (i = 0; i < LINES; i++) {
		if (ptr && *index + i < current->nbentry) {
			if (i == *cursor)
				display_entry(&i, ptr, 1);
			 else
				display_entry(&i, ptr, 0);

			if (ptr->next)
				ptr = ptr->next;
		}
	}
}

static void ncurses_add_file(const char *file)
{
	int len;

	len = strlen(file);
	current->entries = alloc_word(current->entries, len + 1, 1);
	strncpy(current->entries->data, file, len + 1);
	current->nbentry++;
}

static void ncurses_add_line(const char *line)
{
	int len;

	len = strlen(line);
	current->entries = alloc_word(current->entries, len + 1, 0);
	strncpy(current->entries->data, line, len + 1);
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

	if (*cursor > 0)
		*cursor = *cursor - 1;

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

	if (*cursor + *index < current->nbentry - 1)
		*cursor = *cursor + 1;

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
	int i;
	entry_t *ptr;
	entry_t *file = current->start;

	char command[PATH_MAX];
	char filtered_file_name[PATH_MAX];
	char line_copy[PATH_MAX];
	pthread_mutex_t *mutex;

	for (i = 0, ptr = current->start; i < index; i++) {
		ptr = ptr->next;
		if (ptr->isfile)
			file = ptr;
	}

	synchronized(mainsearch.data_mutex) {
		strcpy(line_copy, ptr->data);
		snprintf(command, sizeof(command), editor,
			extract_line_number(line_copy),
			remove_double_appearance(
				file->data, '/',
				filtered_file_name),
			pattern);
	}

	if (system(command) < 0)
		return;

	ptr->opened = 1;
}

static void mark_entry(int index)
{
	int i;
	entry_t *ptr;

	for (i = 0, ptr = current->start; i < index; i++)
		ptr = ptr->next;

	ptr->mark = (ptr->mark + 1) % 2;
}

static void clear_elements(struct list **list)
{
	struct list *pointer = *list;

	while (*list) {
		pointer = *list;
		*list = (*list)->next;
		free(pointer);
	}
}

void clean_search(search_t *search)
{
	entry_t *ptr = search->start;
	entry_t *p;

	while (ptr) {
		p = ptr;
		ptr = ptr->next;
		free(p);
	}

	clear_elements(&mainsearch.extension);
	clear_elements(&mainsearch.specific_file);
	clear_elements(&mainsearch.ignore);
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
	DIR *dp;

	search_t *d = (search_t *) arg;
	dp = opendir(d->directory);

	if (!dp) {
                fprintf(stderr, "error: coult not open directory \"%s\"\n", d->directory);
                exit(-1);
	}

	lookup_directory(d->directory, d->pattern, d->options);
	d->status = 0;
	return (void *) NULL;
}

void init_searchstruct(search_t *searchstruct)
{
	searchstruct->index = 0;
	searchstruct->cursor = 0;
	searchstruct->nbentry = 0;
	searchstruct->status = 1;
	searchstruct->raw = 0;
	searchstruct->entries = NULL;
	searchstruct->start = searchstruct->entries;
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

static void display_version(void)
{
	printf("version %s\n", NGP_VERSION);
}

static void add_element(struct list **list, char *element)
{
	struct list *new, *pointer;
	int len;

	len = strlen(element) + 1;
	new = calloc(1, sizeof(struct list) + len + 1);
	strncpy(new->data, element, len + 1);

	if (*list) {
		/* list not empty */
		pointer = *list;
		while (pointer->next)
			pointer = pointer->next;

		pointer->next = new;
	} else {
		/* list empty */
		*list = new;
	}
}

static void read_config(void)
{
	const char *specific_files;
	const char *extensions;
	const char *ignore;
	char *ptr;
	char *buf = NULL;
	config_t cfg;

	configuration_init(&cfg);

	if (!config_lookup_string(&cfg, "editor", &mainsearch.editor)) {
		fprintf(stderr, "ngprc: no editor string found!\n");
		exit(-1);
	}

	if (!config_lookup_string(&cfg, "files", &specific_files)) {
		fprintf(stderr, "ngprc: no files string found!\n");
		exit(-1);
	}

	ptr = strtok_r((char *) specific_files, " ", &buf);
	while (ptr != NULL) {
		add_element(&mainsearch.specific_file, ptr);
		ptr = strtok_r(NULL, " ", &buf);
	}

	/* getting files extensions from configuration */
	if (!config_lookup_string(&cfg, "extensions", &extensions)) {
		fprintf(stderr, "ngprc: no extensions string found!\n");
		exit(-1);
	}

	ptr = strtok_r((char *) extensions, " ", &buf);
	while (ptr != NULL) {
	        add_element(&mainsearch.extension, ptr);
		ptr = strtok_r(NULL, " ", &buf);
	}

	/* getting ignored files from configuration */
	if (!config_lookup_string(&cfg, "ignore", &ignore)) {
		fprintf(stderr, "ngprc: no ignore string found!\n");
		exit(-1);
	}

	ptr = strtok_r((char *) ignore, " ", &buf);
	while (ptr != NULL) {
	        add_element(&mainsearch.ignore, ptr);
		ptr = strtok_r(NULL, " ", &buf);
	}
}

static void parse_args(int argc, char *argv[])
{
	int opt;
	int clear_extensions = 0;
	int clear_ignores = 0;
	int first = 0;

	while ((opt = getopt(argc, argv, "heit:rI:v")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			break;
		case 'i':
			strcpy(mainsearch.options, "-i");
			break;
		case 't':
			if (!clear_extensions) {
				clear_elements(&mainsearch.extension);
				clear_extensions = 1;
			}
			add_element(&mainsearch.extension, optarg);
			break;
		case 'I':
			if (!clear_ignores) {
				clear_elements(&mainsearch.ignore);
				clear_ignores = 1;
			}
			add_element(&mainsearch.ignore, optarg);
			break;
		case 'r':
			mainsearch.raw = 1;
			break;
		case 'e':
			mainsearch.regexp = 1;
			break;
		case 'v':
			display_version();
			exit(0);
		default:
			exit(-1);
			break;
		}
	}

	if (argc - optind < 1 || argc - optind > 2)
		usage();

	for ( ; optind < argc; optind++) {
		if (!first) {
			strcpy(mainsearch.pattern, argv[optind]);
			first = 1;
		} else {
			strcpy(mainsearch.directory, argv[optind]);
		}
	}
}

int main(int argc, char *argv[])
{
	int ch;
	pthread_mutex_t *mutex;

	current = &mainsearch;
	init_searchstruct(&mainsearch);
	pthread_mutex_init(&mainsearch.data_mutex, NULL);

	read_config();
	parse_args(argc, argv);

	signal(SIGINT, sig_handler);
	if (pthread_create(&pid, NULL, &lookup_thread, &mainsearch)) {
		fprintf(stderr, "ngp: cannot create thread");
		clean_search(&mainsearch);
		exit(-1);
	}

	synchronized(mainsearch.data_mutex)
		display_entries(&mainsearch.index, &mainsearch.cursor);

	while ((ch = getch())) {
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
			open_entry(current->cursor + current->index,
				mainsearch.editor, current->pattern);
			ncurses_init();
			resize(&current->index, &current->cursor);
			break;
		case QUIT:
			goto quit;
		case MARK:
			mark_entry(current->cursor + current->index);
			break;
		default:
			break;
		}

		usleep(10000);
		synchronized(mainsearch.data_mutex) {
			display_entries(&mainsearch.index, &mainsearch.cursor);
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
	return 0;
}
