#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <menu.h>
#include <signal.h>

#define BOTTOM (LINES - 2)

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
        printf("Usage: dos regexp [directory]\n");
}

static void ncurses_init()
{
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
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
                                attron(COLOR_PAIR(1));
                                printl(y, entry[*index].line);
                                attroff(COLOR_PAIR(1));
                        } else {
                                printl(y, entry[*index].line);
                        }
                } else {
                        attron(COLOR_PAIR(2));
                        printl(y, entry[*index].file + 3);
                        attroff(COLOR_PAIR(2));
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

static int parse_file(const char *file, const char *pattern)
{
        FILE *f;
        char line[256];
        char command[256];
        int first;
        errno = 0;

        snprintf(command, sizeof(command), "grep -n %s %s", pattern,  file);
        f = popen(command, "r");
        if(f == NULL) {
                printf("popen : %d %s\n", errno, strerror(errno));
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

static void lookup_file(const char *file, const char *pattern)
{
        int i;
        regex_t preg;
        int nb_regex;
        errno = 0;

        nb_regex = sizeof(regex_langages) / sizeof(*regex_langages);
        for (i = 0;i < nb_regex; i++) {
                if(regcomp(&preg, regex_langages[i], REG_NOSUB|REG_EXTENDED)) {
                        printf("regcomp : %s\n", strerror(errno));
                }
                if(regexec(&preg, file, 0, NULL, 0) == 0) {
                        parse_file(file, pattern);
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

static void lookup_directory(const char *dir, const char *pattern)
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
                        lookup_file(file_path, pattern);
                        refresh();
                        
                }

                if(ep->d_type & DT_DIR) { 
                        if(strcmp(ep->d_name, "..") !=0 && \
                        strcmp(ep->d_name, ".") !=0 ) {
                                char path_dir[PATH_MAX]=""; 
                                snprintf(path_dir, PATH_MAX, "%s/%s", dir, ep->d_name);
                                lookup_directory(path_dir, pattern);
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

static void open_entry(int index)
{
        char command[256];
        snprintf(command, sizeof(command), "vim +%s %s/%s", 
                extract_line_number(entry[index].line), directory, 
                entry[index].file+3);
        system(command);              
}

void sig_handler(int signo)
{
	if(signo == SIGINT) {
		free(entry);
		ncurses_stop();
		exit(-1);
	}
}

void main(int argc, char *argv[])
{
        DIR *dp;
        int opt;
        struct dirent *ep;
        int ch;
        int cursor = 0;
        int index = 0;
        char command[128];

        if(argc < 2) {
                usage();
                exit(-1);
        }

        while ((opt = getopt(argc, argv, "h")) != -1) {
                switch (opt) {
                case 'h':
                        usage();
                        break;
                default:
                        break;
                }
        }

        if(argv[2] != NULL) {
                strncpy((char*)directory, (char*)argv[2], sizeof(directory)); 
        }

        signal(SIGINT, sig_handler);

        ncurses_init();

        entry = (entry_t *) calloc(size, sizeof(entry_t));

        lookup_directory(directory, argv[1]);
        display_entries(&index, &cursor);

        if(!nbentry) {
                goto quit;
        }
        
        while(ch = getch()) {
                switch(ch) {
                case 'j':
                        cursor_down(&index, &cursor);
                        break;
                case 'k': 
                        cursor_up(&index, &cursor);
                        break;
                case 'p':
                        open_entry(cursor + index);
                        goto quit;
                        break;
                default:
                        goto quit;
                        break;
                }
        }

quit:

        free(entry);
        ncurses_stop();
}
