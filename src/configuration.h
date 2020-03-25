/* Copyright (c) 2013 Jonathan Klee

This file is part of ngp.

ngp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ngp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ngp.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <libconfig.h>

#define CONFIG_CONTENT                                                         \
    "// extensions your want to look into\n"                                   \
    "extensions = \".c .h .cpp .hpp .py .S .pl .qml .pro .pri .rb .java\"\n\n" \
    "// files you want to look into besides the ones matching the "            \
    "extensions\n"                                                             \
    "files = \"Makefile rules control\"\n\n"                                   \
    "// files or directories you want to ignore\n"                             \
    "ignore = \"\"\n\n"                                                        \
    "/* editor command :\n"                                                    \
    "arg #1 = pattern to search\n"                                             \
    "arg #2 = line number\n"                                                   \
    "arg #3 = file path */\n\n"                                                \
    "editor = \"vim -c 'set hls' -c 'silent /\%1$s' -c \%2$d \%3$s\"\n"        \
    "//editor = \"emacs +\%2$d \%3$s &\"\n"                                    \
    "//editor = \"subl \%3$s:\%2$d 1>/dev/null 2>&1\"\n\n"                     \
    "// default parser: nat (native), ag or git\n"                             \
    "default_parser = \"nat\"\n\n"                                             \
    "/* external parser commands :\n"                                          \
    "*     arg \%1$s = options\n"                                              \
    "*     arg \%2$s = pattern to search\n"                                    \
    "*     arg \%3$s = directory\n"                                            \
    "*/\n"                                                                     \
    "ag_cmd = \"ag \%1$s \\\"\%2$s\\\" \%3$s\"\n"                              \
    "git_cmd = \"git grep \%1$s \\\"\%2$s\\\" \%3$s\"\n\n"                     \
    "/* themes\n"                                                              \
    "   colors available: cyan, yellow, red, green,\n"                         \
    "   black, white, blue, magenta */\n\n"                                    \
    "line_color = \"white\"\n"                                                 \
    "line_number_color = \"yellow\"\n"                                         \
    "highlight_color = \"cyan\"\n"                                             \
    "file_color = \"green\"\n"                                                 \
    "opened_line_color = \"red\"\n"

struct configuration_t *create_configuration();
config_t get_config(struct configuration_t *config);
void load_configuration(struct configuration_t *cfg);
void destroy_configuration(struct configuration_t *cfg);

#endif
