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

#ifndef UTILS_H
#define UTILS_H

#include <libconfig.h>

int is_file(int index);
int is_dir_good(char *dir);
int is_specific_file(const char *name);
int is_ignored_file(const char *name);
int is_extension_good(const char *file);
int is_simlink(char *file_path);
char * get_file_name(const char * absolute_path);
char * remove_double_appearance(char *initial, char c, char *final);
char * extract_line_number(char *line);
void configuration_init(config_t *cfg);

#endif /* UTILS_H */
