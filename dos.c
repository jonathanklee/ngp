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

#define CURSOR_UP 	'k'
#define CURSOR_DOWN 	'j'
#define ENTER	 	'p'
#define QUIT	 	'q'

char *regex_langages[] = {
	"([/[:alnum:]]+\\.c$)",
	"([/[:alnum:]]+\\.h$)",
	"([/[:alnum:]]+\\.cpp)$",
	"([/[:alnum:]]+\\.py$)",
	"([/[:alnum:]]+\\.sh$)"
};

typedef struct s_entry_t {
	char file[PATH_MAX];
	char line[256];
} entry_t;


static int nbentry = 0;
static long size = 100;
static entry_t *entry; 
static char directory[64] = "./";

static void usage() 
{
	fprintf(stderr, "Usage: dos regexp [directory]\n");
	exit(-1);
}

static void ncurses_init()
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	start_color();
	curs_set(0);
}

static void ncurses_stop()
{
	endwin();
}

static void check_alloc()
{
	if(nbentry >= size) {
		size = size + 500;
		entry = (entry_t*) realloc(entry, size * sizeof(entry_t)); 
	}
}

static void printl(int *y, char *line) 
{
	int size;
	int crop = COLS;
	char cropped_line[PATH_MAX];

	size = strlen(line);
	strncpy(cropped_line, line, crop);
	cropped_line[COLS] = '\0';
	mvprintw(*y, 0, "%s", cropped_line);
}

static int display_entry(int *y, int *index, int color) 
{
	if(*index <= nbentry) {
		if(strcmp(entry[*index].line, "")) {
			if(color == 1) {
				attron(A_REVERSE);
				printl(y, entry[*index].line);
				attroff(A_REVERSE);
			} else {
				printl(y, entry[*index].line);
			}
		} else {
			attron(A_BOLD);
			strcmp(directory, "./") == 0 ? printl(y, entry[*index].file + 3) :
				printl(y, entry[*index].file);	
			attroff(A_BOLD);
		}
	}
}

static void ncurses_add_file(const char *file)
{
	check_alloc();
	strcpy(entry[nbentry].file, file);
	strcpy(entry[nbentry].line, "");
	nbentry++;
}

static void ncurses_add_line(const char *line, const char* file)
{
	check_alloc();
	strcpy(entry[nbentry].file,file);
	strcpy(entry[nbentry].line,line);
	nbentry++;
}

static int parse_file(const char *file, const char *pattern, char *options)
{
	FILE *f;
	char line[256];
	char command[256];
	int first;
	errno = 0;

	snprintf(command, sizeof(command), "grep -n %s %s %s", options, 
							pattern,  file);
	f = popen(command, "r");
	if(f == NULL) {
		fprintf(stderr, "popen : %d %s\n", errno, strerror(errno));
		return -1;       
	}

	first = 1;
	while(fgets(line, sizeof(line), f)) {
		if(first) {
			ncurses_add_file(file);
			first = 0;
		}
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

	nb_regex = sizeof(regex_langages) / sizeof(*regex_langages);
	for (i = 0;i < nb_regex; i++) {
		if(regcomp(&preg, regex_langages[i], REG_NOSUB|REG_EXTENDED)) {
			fprintf(stderr, "regcomp : %s\n", strerror(errno));
		}
		if(regexec(&preg, file, 0, NULL, 0) == 0) {
			parse_file(file, pattern, options);
		}
		regfree(&preg);
	}
}

static char * extract_line_number(char *line)
{
	char *token;
	token = strtok(line, " :");
	return token;
}

static void lookup_directory(const char *dir, const char *pattern, char *options)
{
	DIR *dp;

	dp = opendir(dir);
	if (!dp) {
		return;
	}

	while(1) {
		struct dirent *ep;
		ep = readdir(dp);

		if(!ep) {
			break;
		}

		if(strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0) {
			char file_path[PATH_MAX];
			snprintf(file_path, PATH_MAX, "%s/%s", dir,ep->d_name); 
			lookup_file(file_path, pattern, options);
			refresh();
			
		}

		if(ep->d_type & DT_DIR) { 
			if(strcmp(ep->d_name, "..") !=0 && \
			strcmp(ep->d_name, ".") !=0 ) {
				char path_dir[PATH_MAX]=""; 
				snprintf(path_dir, PATH_MAX, "%s/%s", dir, ep->d_name);
				lookup_directory(path_dir, pattern, options);
			}
		} 
	}
	closedir(dp);
}

static void display_entries(int *index, int *cursor)
{
	int i = 0;
	int ptr = 0;
	int how_deep = 0;
	int ptr_deep = 0;

	for(i = 0; i < LINES; i++) {
		ptr = *index + i;
		if(i == *cursor) {
			display_entry(&i, &ptr, 1);
		} else {
			display_entry(&i, &ptr, 0);
		}
	}
}

static void cursor_up(int *index, int *cursor)
{
	if(*cursor == 0 && *index > 0) {
		clear();
		refresh();
		*cursor = LINES;
		*index = *index - LINES;
		display_entries(index, cursor);
		return;
	}

	if(*cursor > 0) {
		*cursor = *cursor - 1;
	}

	display_entries(index, cursor);
}

static void cursor_down(int *index, int *cursor)
{
	if(*cursor == (LINES - 1)) {
		clear();
		refresh();
		*cursor = 0;
		*index = *index + LINES;
		display_entries(index, cursor);
		return;
	} 

	if(*cursor + *index < nbentry - 1) {
		*cursor = *cursor + 1;
	}

	display_entries(index, cursor);
}

static void open_entry(int index, const char *editor)
{
	char command[256];
	snprintf(command, sizeof(command), editor, 
		extract_line_number(entry[index].line),
		entry[index].file);
	system(command);              
}

static void sig_handler(int signo)
{
	if(signo == SIGINT) {
		free(entry);
		ncurses_stop();
		exit(-1);
	}
}

static void configuration_init(config_t *cfg)
{
	config_init(cfg);
	if(!config_read_file(cfg, "/etc/dosrc")) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg),
			config_error_line(cfg), config_error_text(cfg));
		config_destroy(cfg);
		exit(1);
	}
}

void main(int argc, char *argv[])
{
	DIR *dp;
	int opt;
	struct dirent *ep;
	int ch;
	int first = 0;
	int cursor = 0;
	int index = 0;
	char command[128];
	char pattern[128] = "";
	char options[128] = "";
	const char *editor;
	config_t cfg;

	while ((opt = getopt(argc, argv, "hi")) != -1) {
		switch (opt) {
		case 'h':
			usage();
			break;
		case 'i':
			strcpy(options, "-i");	
			break;
		default:
			exit(-1);
			break;
		}
	}

	for(; optind < argc; optind++) {
		if(!first) {
			strcpy(pattern, argv[optind]);	
			first = 1;
		} else {
			strcpy(directory, argv[optind]);	
		}
	}

	configuration_init(&cfg);
	if(!config_lookup_string(&cfg, "editor", &editor)) {
		fprintf(stderr, "dosrc: no editor string found!\n");	
		exit(-1);
	}

	signal(SIGINT, sig_handler);

	ncurses_init();

	entry = (entry_t *) calloc(size, sizeof(entry_t));

	lookup_directory(directory, pattern, options);
	display_entries(&index, &cursor);

	if(!nbentry) {
		goto quit;
	}

	while(ch = getch()) {
		switch(ch) {
		case CURSOR_DOWN:
			cursor_down(&index, &cursor);
			break;
		case CURSOR_UP: 
			cursor_up(&index, &cursor);
			break;
		case ENTER:
		case '\n':
			open_entry(cursor + index, editor);
			goto quit;
		case QUIT:
			goto quit;
		default:
			break;
		}
	}

quit:
	free(entry);
	ncurses_stop();
}
