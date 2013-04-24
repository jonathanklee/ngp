#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
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

char *regex_langages[] = {
	"([/[:alnum:]]+\\.c$)",
	"([/[:alnum:]]+\\.h$)",
	"([/[:alnum:]]+\\.cpp)$",
	"([/[:alnum:]]+\\.py$)",
	"([/[:alnum:]]+\\.sh$)"
};

typedef struct s_entry_t {
	char file[PATH_MAX];
	char line[NAME_MAX];
} entry_t;

typedef struct s_data_t {
	int index;
	int cursor;
	int nbentry;
	int size;
	entry_t *entry;
	char directory[PATH_MAX];
	char pattern[NAME_MAX];
	char options[NAME_MAX];
	char file_type[4];
	pthread_mutex_t data_mutex;
	int status;
} data_t;

static data_t data;
static pid_t pid;
static int nb_of_files, nb_of_hits;

static void ncurses_add_file(const char *file);
static void ncurses_add_line(const char *line, const char* file);

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
	fprintf(stderr, "Usage: ngp [options]... pattern [directory]\n\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, " -i : Ignore case distinctions in pattern\n");
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
	curs_set(0);
}

static void ncurses_stop()
{
	endwin();
}

static void check_alloc()
{
	if (data.nbentry >= data.size) {
		data.size = data.size + 500;
		data.entry = (entry_t*) realloc(data.entry,
						data.size * sizeof(entry_t));
	}
}

static void printl(int *y, char *line) 
{
	int size;
	int crop = COLS;
	char cropped_line[PATH_MAX];
	char filtered_line[PATH_MAX];
	char *pos;
	int length=0;

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_CYAN, COLOR_BLACK);

	strncpy(cropped_line, line, crop);
	cropped_line[COLS] = '\0';

	if (isdigit(cropped_line[0])) {
		pos = strtok(cropped_line, ":");
		attron(COLOR_PAIR(2));
		mvprintw(*y, 0, "%s:", pos);
		length = strlen(pos) + 1;
		attron(COLOR_PAIR(1));
		mvprintw(*y, length, "%s", cropped_line + length);
	} else {
		attron(COLOR_PAIR(4));
		mvprintw(*y, 0, "%s", cropped_line,
		remove_double_appearance(cropped_line, '/', filtered_line));
		attron(COLOR_PAIR(1));
	}
}

static int display_entry(int *y, int *index, int color) 
{
	char filtered_line[PATH_MAX];
	
	if (*index <= data.nbentry) {
		if (strcmp(data.entry[*index].line, "")) {
			if (color == 1) {
				attron(A_REVERSE);
				printl(y, data.entry[*index].line);
				attroff(A_REVERSE);
			} else {
				printl(y, data.entry[*index].line);
			}
		} else {
			attron(A_BOLD);
			if (strcmp(data.directory, "./") == 0)
				printl(y, remove_double_appearance(
					data.entry[*index].file + 3, '/', 
					filtered_line));
			else
				printl(y, remove_double_appearance(
					data.entry[*index].file, '/',
					filtered_line));	
			attroff(A_BOLD);
		}
	}
}

static int parse_file(const char *file, const char *pattern, char *options)
{
	FILE *f;
	char line[256];
	char command[256];
	int first;
	errno = 0;

	snprintf(command, sizeof(command), "grep -n %s \'%s\' %s", options, 
							pattern,  file);
	f = popen(command, "r");
	if (f == NULL) {
		fprintf(stderr, "popen : %d %s\n", errno, strerror(errno));
		return -1;       
	}

	first = 1;
	while (fgets(line, sizeof(line), f)) {
		if (first) {
			ncurses_add_file(file);
			first = 0;
			nb_of_hits++;
		}

		/* cleanup bad files that have a \r at the end of lines */
		if (line[strlen(line) - 2] == '\r')
			line[strlen(line) - 2] = '\0';
		ncurses_add_line(line, file);
	}
	pclose(f);
	return 0;
}

static void lookup_file(const char *file, const char *pattern, char *options)
{
	int i;
	regex_t preg;
	int nb_regex;
	errno = 0;
	pthread_mutex_t *mutex;

	nb_of_files++;
	nb_regex = sizeof(regex_langages) / sizeof(*regex_langages);
	for (i = 0; i < nb_regex; i++) {
		if (regcomp(&preg, regex_langages[i], REG_NOSUB|REG_EXTENDED)) {
			fprintf(stderr, "regcomp : %s\n", strerror(errno));
		}
		if (regexec(&preg, file, 0, NULL, 0) == 0) {
			synchronized(data.data_mutex) 
				parse_file(file, pattern, options);
		}
		regfree(&preg);
	}
}

static char * extract_line_number(char *line)
{
	char *token;
	char *buffer;
	token = strtok_r(line, " :", &buffer);
	return token;
}

static void lookup_directory(const char *dir, const char *pattern, 
	char *options, char *file_type)
{
	DIR *dp;
	struct stat filestat;

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

		if (!(ep->d_type & DT_DIR) && strcmp(ep->d_name, ".") != 0 &&
			strcmp(ep->d_name, "..") != 0) {
			char file_path[PATH_MAX];
			snprintf(file_path, PATH_MAX, "%s/%s", dir, 
				ep->d_name);

			lstat(file_path, &filestat);
			if (!S_ISLNK(filestat.st_mode)) {
				if (file_type != NULL) {
					if (!strcmp(file_type, ep->d_name + strlen(ep->d_name) - strlen(file_type) ))
						lookup_file(file_path, pattern, options);
				} else {
					lookup_file(file_path, pattern, options);
				}
			}
		}

		if (ep->d_type & DT_DIR) {
			if (strcmp(ep->d_name, "..") != 0 && 
			strcmp(ep->d_name, ".") != 0 && 
			strcmp(ep->d_name, ".git") != 0) {
				char path_dir[PATH_MAX] = ""; 
				snprintf(path_dir, PATH_MAX, "%s/%s", dir, 
					ep->d_name);
				lookup_directory(path_dir, pattern, options,
					file_type);
			}
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
	check_alloc();
	strcpy(data.entry[data.nbentry].file, file);
	strcpy(data.entry[data.nbentry].line, "");
	data.nbentry++;
}

static void ncurses_add_line(const char *line, const char* file)
{
	check_alloc();
	strcpy(data.entry[data.nbentry].file,file);
	strcpy(data.entry[data.nbentry].line,line);
	data.nbentry++;
	display_entries(&data.index, &data.cursor);
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
	if (!strcmp(data.entry[*index + *cursor].line, "") && *index != 0)
		*cursor -= 1;

	display_entries(index, cursor);
}

static void page_down(int *index, int *cursor)
{
	int max_index;
	if (data.nbentry % LINES == 0)
		max_index = (data.nbentry - LINES);
	else
		max_index = (data.nbentry - (data.nbentry % LINES));

	if (*index == max_index)
		*cursor = (data.nbentry - 1) % LINES;
	else
		*cursor = 0;

	clear();
	refresh();
	*index += LINES;
	*index = (*index > max_index ? max_index : *index);

	if (!strcmp(data.entry[*index + *cursor].line, ""))
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

	/* If line is a file, skip it*/
	if (!strcmp(data.entry[*cursor + *index].line, ""))
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

	if (*cursor + *index < data.nbentry - 1) {
		*cursor = *cursor + 1;
	}

	/* If line is a file, skip it*/
	if (!strcmp(data.entry[*cursor + *index].line, ""))
		*cursor = *cursor + 1;

	if (*cursor > (LINES - 1)) {
		page_down(index, cursor);
		return;
	}

	display_entries(index, cursor);
}

static void open_entry(int index, const char *editor, const char *pattern)
{
	char command[PATH_MAX];
	char filtered_file_name[PATH_MAX];
	char line_copy[PATH_MAX];
	pthread_mutex_t *mutex;

	synchronized(data.data_mutex) {
		strcpy(line_copy, data.entry[index].line);
		snprintf(command, sizeof(command), editor, 
			extract_line_number(line_copy),
			remove_double_appearance(data.entry[index].file, '/',
			filtered_file_name), pattern);
	}
	system(command);              
}

static void sig_handler(int signo)
{
	if (signo == SIGINT) {
		free(data.entry);
		ncurses_stop();
		exit(-1);
	}
}

static void configuration_init(config_t *cfg)
{
	config_init(cfg);
	if (!config_read_file(cfg, "/etc/ngprc")) {
		if (!config_read_file(cfg, "ngprc")) {
			fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg),
				config_error_line(cfg), config_error_text(cfg));
			fprintf(stderr, "Could be that the configuration file \
				has not been found in /etc nor in current dir\n");
			config_destroy(cfg);
			exit(1);
		}
	}
}

void * lookup_thread(void *arg)
{
	data_t *d = (data_t *) arg;

	lookup_directory(d->directory, d->pattern, d->options, d->file_type);
	d->status = 0;
}

void main(int argc, char *argv[])
{
	DIR *dp;
	int opt;
	struct dirent *ep;
	int ch;
	int first = 0;
	char command[128];
	const char *editor;
	config_t cfg;
	int i = 0, time = 0;
	pthread_mutex_t *mutex;

	data.index = 0;
	data.cursor = 0;
	data.size = 100;
	data.nbentry = 0;
	data.status = 1;
	strcpy(data.directory, "./");

	while ((opt = getopt(argc, argv, "hit:")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			break;
		case 'i':
			strcpy(data.options, "-i");	
			break;
		case 't':
			strncpy(data.file_type, optarg, 3);
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
			strcpy(data.pattern, argv[optind]);	
			first = 1;
		} else {
			strcpy(data.directory, argv[optind]);	
		}
	}

	pthread_mutex_init(&data.data_mutex, NULL);

	configuration_init(&cfg);
	if (!config_lookup_string(&cfg, "editor", &editor)) {
		fprintf(stderr, "ngprc: no editor string found!\n");	
		exit(-1);
	}

	signal(SIGINT, sig_handler);

	data.entry = (entry_t *) calloc(data.size, sizeof(entry_t));

	if (pthread_create(&pid, NULL, &lookup_thread, &data)) {
		fprintf(stderr, "ngp: cannot create thread");		
		free(data.entry);
		exit(-1);
	}

	ncurses_init();

	synchronized(data.data_mutex)
		display_entries(&data.index, &data.cursor);

	while (ch = getch()) {
		switch(ch) {
		case KEY_RESIZE:
			synchronized(data.data_mutex)
				resize(&data.index, &data.cursor);
			break;
		case CURSOR_DOWN:
		case KEY_DOWN:
			synchronized(data.data_mutex)
				cursor_down(&data.index, &data.cursor);
			break;
		case CURSOR_UP: 
		case KEY_UP:
			synchronized(data.data_mutex)
				cursor_up(&data.index, &data.cursor);
			break;
		case KEY_PPAGE:
		case PAGE_UP:
			synchronized(data.data_mutex)
				page_up(&data.index, &data.cursor);
			break;
		case KEY_NPAGE:
		case PAGE_DOWN:
			synchronized(data.data_mutex)
				page_down(&data.index, &data.cursor);
			break;
		case ENTER:
			ncurses_stop();
			open_entry(data.cursor + data.index, editor, 
				data.pattern);
			ncurses_init();
			resize(&data.index, &data.cursor);
			break;
		case '\n':
			synchronized(data.data_mutex)
				open_entry(data.cursor + data.index, editor, 
					data.pattern);
			goto quit;
		case QUIT:
			goto quit;
		default:
			break;
		}
		if (time == 0) {
			attron(A_BOLD);
			mvaddch(0, COLS - 10, (data.status == 1) ?
				ACS_LTEE+i++%4 : ACS_BULLET);

			if (!data.status)
				mvprintw(1, COLS - 10, "%d/%d",
					nb_of_hits, nb_of_files);

			attroff(A_BOLD);
		}
		
		usleep(10000);
		time += 1;
		time %=10;
		
		refresh();

		synchronized(data.data_mutex) {
			if (data.status == 0 && data.nbentry == 0) {
				goto quit;
			}
		}
	}

quit:
	free(data.entry);
	ncurses_stop();
}
