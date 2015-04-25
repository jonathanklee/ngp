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

extern search_t	*current;

int is_file(int index)
{
	int i;
	entry_t *ptr = current->start;

	for (i = 0; i < index; i++)
		ptr = ptr->next;

	return ptr->isfile;
}

int is_dir_good(char *dir)
{
	return  *dir != '.' &&
		strcmp(dir, "..") != 0 &&
		strcmp(dir, ".git") != 0 ? 1 : 0;
}

char * get_file_name(const char * absolute_path)
{
	char *ret;

	if (strrchr(absolute_path + 3, '/') != NULL)
		ret = strrchr(absolute_path + 3, '/') + 1;
	else
		ret = (char *) absolute_path + 3;

	return ret;
}

int is_specific_file(const char *name)
{
	char *name_begins;
	struct list *pointer = current->specific_file;

	while (pointer) {
		name_begins = get_file_name(name);
		if (!strcmp(name_begins, pointer->data))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

int is_ignored_file(const char *name)
{
	char *name_begins;
	struct list *pointer = current->ignore;

	while (pointer) {
		name_begins = get_file_name(name);
		if (!strcmp(name_begins, pointer->data))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

int is_extension_good(const char *file) {

	struct list *pointer;

	pointer = current->extension;
	while (pointer) {
		if (!strcmp(pointer->data, file + strlen(file) -
			strlen(pointer->data)))
			return 1;
		pointer = pointer->next;
	}
	return 0;
}

char * remove_double_appearance(char *initial, char c, char *final)
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

char * extract_line_number(char *line)
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

