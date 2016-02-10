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
#include "entry.h"
#include "utils.h"

int is_selectionable(struct search_t *search, int index)
{
	int i;
	struct entry_t *ptr = search->start;

	for (i = 0; i < index; i++)
		ptr = ptr->next;

	return is_entry_selectionable(ptr);
}

int is_dir_good(char *dir)
{
	return  *dir != '.' &&
		strcmp(dir, "..") != 0 &&
		strcmp(dir, ".git") != 0 ? 1 : 0;
}

char *get_file_name(const char * absolute_path)
{
	char *ret;

	if (strrchr(absolute_path + 3, '/') != NULL)
		ret = strrchr(absolute_path + 3, '/') + 1;
	else
		ret = (char *) absolute_path + 3;

	return ret;
}

int is_specific_file(struct search_t *search, const char *name)
{
	char *name_begins;
	struct list *pointer = search->specific_file;

	while (pointer) {
		name_begins = get_file_name(name);
		if (!strcmp(name_begins, pointer->data))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

int is_ignored_file(struct search_t *search, const char *name)
{
	char *name_begins;
	struct list *pointer = search->ignore;

	while (pointer) {
		name_begins = get_file_name(name);
		if (!strcmp(name_begins, pointer->data))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

int is_extension_good(struct search_t *search, const char *file) {

	struct list *pointer;

	pointer = search->extension;
	while (pointer) {
		if (!strcmp(pointer->data, file + strlen(file) -
			strlen(pointer->data)))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

char *remove_double(char *initial, char c, char *final)
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

char *extract_line_number(char *line)
{
	char *token;
	char *buffer;
	token = strtok_r(line, " :", &buffer);
	return token;
}

int is_simlink(char *file_path)
{
	struct stat filestat;

	lstat(file_path, &filestat);
	return S_ISLNK(filestat.st_mode);
}

void configuration_init(config_t *cfg)
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

char *regex(struct search_t *search, const char *line, const char *pattern)
{
	int ret;
	const char *pcre_error;
	int pcre_error_offset;
	int substring_vector[30];
	const char *matched_string;

	/* check if regexp has already been compiled */
	if (!search->pcre_compiled) {
		search->pcre_compiled = pcre_compile(pattern, 0, &pcre_error,
			&pcre_error_offset, NULL);
		if (!search->pcre_compiled)
			return NULL;

		search->pcre_extra =
			pcre_study(search->pcre_compiled, 0, &pcre_error);
		if (!search->pcre_extra)
			return NULL;
	}

	ret = pcre_exec(search->pcre_compiled, search->pcre_extra, line,
		strlen(line), 0, 0, substring_vector, 30);

	if (ret < 0)
		return NULL;

	pcre_get_substring(line, substring_vector, ret, 0, &matched_string);

	return (char *) matched_string;
}

void *get_parser(struct search_t *search)
{
	char * (*parser)(struct search_t *, const char *, const char*);

	if (strstr(search->options, "-i") == NULL)
		parser = strstr_wrapper;
	else
		parser = strcasestr_wrapper;

	if (search->regexp_option)
		parser = regex;

	return parser;
}

char *strstr_wrapper(struct search_t *search, const char *line, const char *pattern)
{
	return strstr(line, pattern);
}

char *strcasestr_wrapper(struct search_t *search, const char *line, const char *pattern)
{
	return strcasestr(line, pattern);
}

