#include "entry.h"
#include "string.h"
#include "utils.h"
#include "ngp.h"
#include "themes.h"

struct entry_vtable file_vtable = {
	display_file,
	display_file_with_cursor,
	is_file_selectionable,
	free_file
};

struct entry_vtable line_vtable = {
	display_line,
	display_line_with_cursor,
	is_line_selectionable,
	free_line
};


struct entry_t *create_file(struct search_t *search, char *file)
{
	int len = strlen(file);
	struct file_t *new;

	new = calloc(1, sizeof(struct file_t));
	strncpy(new->entry.data, file, len + 1);
	new->entry.vtable = &file_vtable;
	search->nbentry++;

	if (search->entries) {
		search->entries->next = &new->entry;
	} else {
		search->start = &new->entry;
	}

	return &new->entry;
}

struct entry_t *create_line(struct search_t *search, char *line, int line_number)
{
	int len = strlen(line);
	struct line_t *new;

	new = calloc(1, sizeof(struct line_t));
	strncpy(new->entry.data, line, len + 1);
	new->opened = 0;
	new->mark = 0;
	new->line = line_number;
	search->nbentry++;

	new->entry.vtable = &line_vtable;

	if (search->entries) {
		search->entries->next = &new->entry;
	} else {
		search->start = &new->entry;
	}

	return &new->entry;
}

void display_entry(struct search_t *search, int *y, struct entry_t *entry)
{
	entry->vtable->display(search, y, entry);
}

void display_entry_with_cursor(struct search_t *search, int *y, struct entry_t *entry)
{
	entry->vtable->display_with_cursor(search, y, entry);
}

void free_entry(struct entry_t *entry)
{
	entry->vtable->free(entry);
}

bool is_entry_selectionable(struct entry_t *entry)
{
	return entry->vtable->is_selectionable(entry);
}

void display_file(struct search_t *search, int *y, struct entry_t *entry)
{
        char filtered_line[PATH_MAX];
        char cropped_line[PATH_MAX] = "";
        int crop = COLS;

        /* first clear line */
        move(*y, 0);
        clrtoeol();

        attron(A_BOLD);

        if (strcmp(search->directory, "./") == 0)
                remove_double(entry->data + 3, '/', filtered_line);
        else
                remove_double(entry->data , '/', filtered_line);

        strncpy(cropped_line, filtered_line, crop);
        attron(COLOR_PAIR(5));
        remove_double(cropped_line, '/', filtered_line);
        mvprintw(*y, 0, "%s", cropped_line);

        attroff(A_BOLD);
}

void display_file_with_cursor(struct search_t *search, int *y, struct entry_t *entry) { }

bool is_file_selectionable(struct entry_t *entry)
{
    return false;
}

void free_file(struct entry_t *entry)
{
	struct file_t *ptr = container_of(entry, struct file_t, entry);
	free(ptr);
}

void display_line(struct search_t *search, int *y, struct entry_t *entry)
{
	char *pattern = NULL;
	char *ptr;
	char *regexp_matched_string = NULL;
	char * (*parser)(struct search_t *, const char *, const char*);
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
	attron(COLOR_PAIR(2));
	struct line_t *container = container_of(entry, struct line_t, entry);
	mvprintw(*y, 0, "%d:", container->line);

	/* display rest of line */
	char line_begin[16];
	sprintf(line_begin, "%d", container->line);
	length = strlen(line_begin) + 1;
	attron(COLOR_PAIR(1));
	mvprintw(*y, length, "%s", cropped_line);

	/* highlight pattern */
	if (search->regexp_option) {
		regexp_matched_string = regex(search, cropped_line, search->pattern);
		if (!regexp_matched_string)
			return;

		pattern = strstr(cropped_line, regexp_matched_string);
		goto start_printing;
	}

	/* TODO why the fuck we are passing search twice ? */
	parser = get_parser(search, search->options);
	pattern = parser(search, cropped_line, search->pattern);

start_printing:

	if (!pattern)
		return;

	ptr = cropped_line;
	move(*y, length);
	while (ptr != pattern) {
		addch(*ptr);
		ptr++;
	}

	/* switch color to cyan */
	attron(A_REVERSE);
	attron(COLOR_PAIR(4));
	if (search->regexp_option) {
		length = strlen(regexp_matched_string);
		pcre_free_substring(regexp_matched_string);
	} else {
		length = strlen(search->pattern);
	}

	for (counter = 0; counter < length; counter++, ptr++)
		addch(*ptr);

	attroff(A_REVERSE);
}

void display_line_with_cursor(struct search_t *search, int *y, struct entry_t *entry)
{
	attron(A_REVERSE);
	display_line(search, y, entry);
	attroff(A_REVERSE);
}

bool is_line_selectionable(struct entry_t *entry)
{
	return true;
}

void free_line(struct entry_t *entry)
{
	struct line_t *ptr = container_of(entry, struct line_t, entry);
	free(ptr);
}


