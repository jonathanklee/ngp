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

#include "ngp.h"
#include "utils.h"
#include "theme.h"
#include "entry.h"
#include "file.h"
#include "line.h"
#include "list.h"

/* keep a pointer on search_t for signal handler ONLY */
struct search_t *global_search;

void usage(void)
{
	fprintf(stderr, "usage: ngp [options]... pattern [directory]\n\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, " -i : ignore case distinctions in pattern\n");
	fprintf(stderr, " -r : raw mode\n");
	fprintf(stderr, " -t type : look into files with specified extension\n");
	fprintf(stderr, " -I name : ignore file/dir with specified name\n");
	fprintf(stderr, " -e : pattern is a regular expression\n");
	fprintf(stderr, " -v : display version\n");
	exit(-1);
}

void ncurses_init(void)
{
	struct theme_t *theme;

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	start_color();
	use_default_colors();
	init_pair(1, -1, -1);
	theme = read_theme();
	apply_theme(theme);
	destroy_theme(theme);
	curs_set(0);
}

void ncurses_stop(void)
{
	endwin();
}

int parse_file(struct search_t *search, const char *file, const char *pattern)
{
	int f;
	char *pointer;
	char *start;
	char *end;
	char *endline;
	int first_occurrence;
	struct stat sb;
	int line_number;
	char * (*parser)(struct search_t *, const char *, const char*);
	errno = 0;

	f = open(file, O_RDONLY);
	if (f < 0)
		return -1;

	if (fstat(f, &sb) < 0) {
		close(f);
		return -1;
	}

	pointer = mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, f, 0);
	if (pointer == MAP_FAILED) {
		close(f);
		return -1;
	}

	close(f);

	parser = get_parser(search);

	first_occurrence = 1;
	line_number = 1;
	start = pointer;
	end = pointer + sb.st_size;
	while (1) {

		if (pointer == end)
			break;

		endline = strchr(pointer, '\n');
		if (endline == NULL)
			break;

		/* replace \n with \0 */
		*endline = '\0';

		if (parser(search, pointer, pattern) != NULL) {
			if (first_occurrence) {
				if (search->nbentry == 0)
					ncurses_init();
				ncurses_add_file(search, file);
				first_occurrence = 0;
			}
			if (pointer[strlen(pointer) - 2] == '\r')
				pointer[strlen(pointer) - 2] = '\0';
			ncurses_add_line(search, pointer, line_number);
		}

		/* switch back to \n */
		*endline = '\n';
		pointer = endline + 1;
		line_number++;
	}

	if (munmap(start, sb.st_size) < 0)
		return -1;

	return 0;
}

void lookup_file(struct search_t *search, const char *file, const char *pattern)
{
	errno = 0;
	pthread_mutex_t *mutex;

	if (is_ignored_file(search, file))
		return;

	if (search->raw_option) {
		synchronized(search->data_mutex)
			parse_file(search, file, pattern);
		return;
	}

	if (is_specific_file(search, file)) {
		synchronized(search->data_mutex)
			parse_file(search, file, pattern);
		return;
	}

	if (is_extension_good(search, file)) {
		synchronized(search->data_mutex)
			parse_file(search, file, pattern);
		return;
	}
}

void lookup_directory(struct search_t *search, const char *dir, const char *pattern)
{
	DIR *dp;

	dp = opendir(dir);
	if (!dp)
		return;

	if (is_ignored_file(search, dir)) {
		closedir(dp);
		return;
	}

	while (1) {
		struct dirent *ep;
		ep = readdir(dp);

		if (!ep)
			break;

		if (!(ep->d_type & DT_DIR)) {
			char file_path[PATH_MAX];
			snprintf(file_path, PATH_MAX, "%s/%s", dir, ep->d_name);

			if (!is_simlink(file_path)) {
				lookup_file(search, file_path, pattern);
			}
		}

		if (ep->d_type & DT_DIR && is_dir_good(ep->d_name)) {
			char path_dir[PATH_MAX] = "";
			snprintf(path_dir, PATH_MAX, "%s/%s", dir, ep->d_name);
			lookup_directory(search, path_dir, pattern);
		}
	}
	closedir(dp);
}

void display_entries(struct search_t *search, int *index, int *cursor)
{
	int i = 0;
	struct entry_t *ptr = search->start;

	for (i = 0; i < *index; i++)
		ptr = ptr->next;

	for (i = 0; i < LINES; i++) {
		if (ptr && *index + i < search->nbentry) {
			if (i == *cursor)
				display_entry_with_cursor(search, &i, ptr);
			 else
				display_entry(search, &i, ptr);

			if (ptr->next)
				ptr = ptr->next;
		}
	}
}

void ncurses_add_file(struct search_t *search, const char *file)
{
	search->entries = create_file(search, (char *)file);
}

void ncurses_add_line(struct search_t *search, const char *line, int line_number)
{
	search->entries = create_line(search, (char *)line, line_number);
	if (search->nbentry <= LINES)
		display_entries(search, &search->index, &search->cursor);
}

void resize(struct search_t *search, int *index, int *cursor)
{
	/* right now this is a bit trivial,
	 * but we may do more complex moving around
	 * when the window is resized */
	clear();
	display_entries(search, index, cursor);
	refresh();
}

void page_up(struct search_t *search, int *index, int *cursor)
{
	clear();
	refresh();
	if (*index == 0)
		*cursor = 1;
	else
		*cursor = LINES - 1;
	*index -= LINES;
	*index = (*index < 0 ? 0 : *index);

	if (!is_selectionable(search, *index + *cursor))
		*cursor -= 1;

	display_entries(search, index, cursor);
}

void page_down(struct search_t *search, int *index, int *cursor)
{
	int max_index;

	if (search->nbentry == 0)
		return;

	if (search->nbentry % LINES == 0)
		max_index = (search->nbentry - LINES);
	else
		max_index = (search->nbentry - (search->nbentry % LINES));

	if (*index == max_index)
		*cursor = (search->nbentry - 1) % LINES;
	else
		*cursor = 0;

	clear();
	refresh();
	*index += LINES;
	*index = (*index > max_index ? max_index : *index);

	if (!is_selectionable(search, *index + *cursor))
		*cursor += 1;
	display_entries(search, index, cursor);
}

void cursor_up(struct search_t *search, int *index, int *cursor)
{
	/* when cursor is on the first page and on the 2nd line,
	   do not move the cursor up */
	if (*index == 0 && *cursor == 1)
		return;

	if (*cursor == 0) {
		page_up(search, index, cursor);
		return;
	}

	if (*cursor > 0)
		*cursor = *cursor - 1;

	if (!is_selectionable(search, *index + *cursor))
		*cursor = *cursor - 1;

	if (*cursor < 0) {
		page_up(search, index, cursor);
		return;
	}

	display_entries(search, index, cursor);
}

void cursor_down(struct search_t *search, int *index, int *cursor)
{
	if (*cursor == (LINES - 1)) {
		page_down(search, index, cursor);
		return;
	}

	if (*cursor + *index < search->nbentry - 1)
		*cursor = *cursor + 1;

	if (!is_selectionable(search, *index + *cursor))
		*cursor = *cursor + 1;

	if (*cursor > (LINES - 1)) {
		page_down(search, index, cursor);
		return;
	}

	display_entries(search, index, cursor);
}

void open_entry(struct search_t *search, int index, const char *editor, const char *pattern)
{
	int i;
	struct entry_t *ptr;
	struct entry_t *file = search->start;

	char command[PATH_MAX];
	char filtered_file_name[PATH_MAX];
	pthread_mutex_t *mutex;

	for (i = 0, ptr = search->start; i < index; i++) {
		ptr = ptr->next;
		if (!is_entry_selectionable(ptr))
			file = ptr;
	}

	struct line_t *line = container_of(ptr, struct line_t, entry);

	synchronized(search->data_mutex) {
		remove_double(file->data, '/', filtered_file_name);
		snprintf(command, sizeof(command), editor,
			pattern,
			line->line,
			filtered_file_name);
	}

	if (system(command) < 0)
		return;

        line->opened = 1;
}

void clean_search(struct search_t *search)
{
	struct entry_t *ptr = search->start;
	struct entry_t *p;

	while (ptr) {
		p = ptr;
		ptr = ptr->next;
		free_entry(p);
	}

	free_list(&search->extension);
	free_list(&search->specific_file);
	free_list(&search->ignore);

	/* free pcre stuffs if needed */
	if (search->pcre_compiled)
		pcre_free((void *) search->pcre_compiled);

	if (search->pcre_extra)
		pcre_free((void *) search->pcre_extra);

}

void sig_handler(int signo)
{
	if (signo == SIGINT) {
		ncurses_stop();
		clean_search(global_search);
		exit(-1);
	}
}

void *lookup_thread(void *arg)
{
	DIR *dp;

	struct search_t *d = (struct search_t *) arg;
	dp = opendir(d->directory);

	if (!dp) {
		fprintf(stderr, "error: coult not open directory \"%s\"\n", d->directory);
		exit(-1);
	}

	lookup_directory(d, d->directory, d->pattern);
	d->status = 0;
	closedir(dp);
	return (void *) NULL;
}

void init_searchstruct(struct search_t *search)
{
	search->index = 0;
	search->cursor = 1;
	search->nbentry = 0;
	search->status = 1;
	search->raw_option = 0;
	search->entries = NULL;
	search->start = search->entries;
	strcpy(search->directory, "./");
}

void display_status(struct search_t *search)
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

	attron(COLOR_PAIR(COLOR_FILE));
	if (search->status)
		mvaddstr(0, COLS - 3, rollingwheel[++i%60]);
	else
		mvaddstr(0, COLS - 5, "");
}

void display_version(void)
{
	printf("version %s\n", NGP_VERSION);
}

void read_config(struct search_t *search)
{
	const char *specific_files;
	const char *extensions;
	const char *ignore;
        const char *buffer;
	char *ptr;
	char *buf = NULL;
        config_t cfg;

	configuration_init(&cfg);

	if (!config_lookup_string(&cfg, "editor", &buffer)) {
		fprintf(stderr, "ngprc: no editor string found!\n");
		exit(-1);
	}
        strncpy(search->editor, buffer, LINE_MAX);

	/* only if we don't provide extension as argument */
	if (!search->extension_option) {
		if (!config_lookup_string(&cfg, "files", &specific_files)) {
			fprintf(stderr, "ngprc: no files string found!\n");
			exit(-1);
		}

		search->specific_file = create_list();
		ptr = strtok_r((char *) specific_files, " ", &buf);
		while (ptr != NULL) {
			add_element(&search->specific_file, ptr);
			ptr = strtok_r(NULL, " ", &buf);
		}
	}

	if (!search->extension_option) {
		/* getting files extensions from configuration */
		if (!config_lookup_string(&cfg, "extensions", &extensions)) {
			fprintf(stderr, "ngprc: no extensions string found!\n");
			exit(-1);
		}

		search->extension = create_list();
		ptr = strtok_r((char *) extensions, " ", &buf);
		while (ptr != NULL) {
			add_element(&search->extension, ptr);
			ptr = strtok_r(NULL, " ", &buf);
		}
	}

	if (!search->ignore_option) {
		/* getting ignored files from configuration */
		if (!config_lookup_string(&cfg, "ignore", &ignore)) {
			fprintf(stderr, "ngprc: no ignore string found!\n");
			exit(-1);
		}

		search->ignore = create_list();
		ptr = strtok_r((char *) ignore, " ", &buf);
		while (ptr != NULL) {
			add_element(&search->ignore, ptr);
			ptr = strtok_r(NULL, " ", &buf);
		}
	}
	config_destroy(&cfg);
}

void parse_args(struct search_t *search, int argc, char *argv[])
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
			strcpy(search->options, "-i");
			break;
		case 't':
			if (!clear_extensions) {
				free_list(&search->extension);
				search->extension_option = 1;
				clear_extensions = 1;
			}
			add_element(&search->extension, optarg);
			break;
		case 'I':
			if (!clear_ignores) {
				free_list(&search->ignore);
				search->ignore_option = 1;
				clear_ignores = 1;
			}
			add_element(&search->ignore, optarg);
			break;
		case 'r':
			search->raw_option = 1;
			break;
		case 'e':
			search->regexp_option = 1;
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
			strcpy(search->pattern, argv[optind]);
			first = 1;
		} else {
			strcpy(search->directory, argv[optind]);
                        if (!opendir(search->directory)) {
                                fprintf(stderr, "error: could not open directory \"%s\"\n", search->directory);
                                exit(-1);
                        }
		}
	}
}

int main(int argc, char *argv[])
{
	int ch;
	pthread_mutex_t *mutex;
	static struct search_t mainsearch;
	struct search_t *current;
	pthread_t pid;

	current = &mainsearch;
        global_search = &mainsearch;
	init_searchstruct(current);
	pthread_mutex_init(&current->data_mutex, NULL);

	parse_args(current, argc, argv);
	read_config(current);

	signal(SIGINT, sig_handler);
	if (pthread_create(&pid, NULL, &lookup_thread, current)) {
		fprintf(stderr, "ngp: cannot create thread");
		clean_search(current);
		exit(-1);
	}

	synchronized(current->data_mutex)
		display_entries(current, &current->index, &current->cursor);

	while ((ch = getch())) {
		switch(ch) {
		case KEY_RESIZE:
			synchronized(current->data_mutex)
				resize(current, &current->index, &current->cursor);
			break;
		case CURSOR_DOWN:
		case KEY_DOWN:
			synchronized(current->data_mutex)
				cursor_down(current, &current->index, &current->cursor);
			break;
		case CURSOR_UP:
		case KEY_UP:
			synchronized(current->data_mutex)
				cursor_up(current, &current->index, &current->cursor);
			break;
		case KEY_PPAGE:
		case PAGE_UP:
			synchronized(current->data_mutex)
				page_up(current, &current->index, &current->cursor);
			break;
		case KEY_NPAGE:
		case PAGE_DOWN:
			synchronized(current->data_mutex)
				page_down(current, &current->index, &current->cursor);
			break;
		case ENTER:
		case '\n':
			if (current->nbentry == 0)
				break;
			ncurses_stop();
			open_entry(current, current->cursor + current->index,
				current->editor, current->pattern);
			ncurses_init();
			resize(current, &current->index, &current->cursor);
			break;
		case QUIT:
			goto quit;
		default:
			break;
		}

		usleep(10000);
		synchronized(current->data_mutex) {
			display_entries(current, &current->index, &current->cursor);
			display_status(current);
		}

		synchronized(current->data_mutex) {
			if (current->status == 0 && current->nbentry == 0) {
				goto quit;
			}
		}
	}

quit:
	ncurses_stop();
	clean_search(&mainsearch);
	return 0;
}
