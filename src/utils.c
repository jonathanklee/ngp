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

#define _GNU_SOURCE

#include "entry.h"
#include "utils.h"
#include "list.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CONFIG_DIR     "ngp"
#define CONFIG_FILE    "ngprc"
#define CONFIG_CONTENT "// extensions your want to look into\n"                                  \
                       "extensions = \".c .h .cpp .hpp .py .S .pl .qml .pro .pri .rb .java\";\n" \
                       "// files you want to look into\n"                                        \
                       "files = \"Makefile rules control\";\n"                                   \
                       "// files you want to ignore\n"                                           \
                       "ignore = \"\";\n"                                                        \
                       "/* editor command :\n"                                                   \
                       "arg #1 = pattern to search\n"                                            \
                       "arg #2 = line number\n"                                                  \
                       "arg #3 = file path */\n"                                                 \
                       "editor = \"vim -c 'set hls' -c 'silent /\%1$s' -c \%2$d \%3$s\"\n"       \
                       "//editor = \"emacs +\%2$d \%3$s &\";\n"                                  \
                       "//editor = \"subl \%3$s:\%2$d 1>/dev/null 2>&1\";\n"                     \
                       "// default parser: nat (native), ag or git\n"                            \
                       "default_parser = nat\n"                                                  \
                       "\n"                                                                      \
                       "/* external parser commands :\n"                                         \
                       "*     arg \%1$s = options\n"                                             \
                       "*     arg \%2$s = pattern to search\n"                                   \
                       "*     arg \%3$s = directory\n"                                           \
                       "*/\n"                                                                    \
                       "ag_cmd = \"ag  \%1$s \"\%2$s\" \%3$s\n"                                  \
                       "git_cmd = \"git grep   %1$s \"%2$s\" %3$s\n"                             \
                       "\n"                                                                      \
                       "/* themes\n"                                                             \
                       "   colors available: cyan, yellow, red, green,\n"                        \
                       "   black, white, blue, magenta */\n"                                     \
                       "line_color = \"white\";\n"                                               \
                       "line_number_color = \"yellow\";\n"                                       \
                       "highlight_color = \"cyan\";\n"                                           \
                       "file_color = \"green\";\n"                                               \
                       "opened_line_color = \"red\";\n"

int is_selectable(struct search_t *search, int index)
{
    int i;
    struct entry_t *ptr = search->result->start;

    for (i = 0; i < index; i++)
        ptr = ptr->next;

    return is_entry_selectable(ptr);
}

void configuration_init(config_t *cfg)
{
    config_init(cfg);

    char user_ngprc[PATH_MAX];
#ifdef __linux__
    char *xdg_config_home = getenv("XDG_CONFIG_HOME");

    if (xdg_config_home != NULL) {
        snprintf(user_ngprc, PATH_MAX, "%s/%s/%s", xdg_config_home,
                 CONFIG_DIR, CONFIG_FILE);

        if (config_read_file(cfg, user_ngprc))
            return;
    }


    char *xdg_config_dirs = getenv("XDG_CONFIG_DIRS");

    if (xdg_config_dirs != NULL) {
        // strtok overwrites delimiters with NULL bytes, so we need to operate
        // on a copy here. Allow for five entries in XDG_CONFIG_DIRS here, that
        // should probably be enough.
        char xdg_config_dirs_cpy[PATH_MAX * 5];
        strncpy(xdg_config_dirs_cpy, xdg_config_dirs, strlen(xdg_config_dirs));

        char *token = strtok(xdg_config_dirs_cpy, ":");

        // Iterate through all directories within XDG_CONFIG_DIRS
        while (token != NULL) {
            char sys_ngprc[PATH_MAX];
            snprintf(sys_ngprc, PATH_MAX, "%s/%s/%s", token, CONFIG_DIR,
                     CONFIG_FILE);

            if (config_read_file(cfg, sys_ngprc))
                return;

            token = strtok(NULL, ":");
        }
    }

    // We've found the config file neither in the user's config dir nor globally,
    // let's try to create one.
    if (xdg_config_home != NULL) {
#elif __APPLE__
    // At first, try the user specific config file
    char *home = getenv("HOME");

    if (home != NULL) {
        snprintf(user_ngprc, PATH_MAX, "%s/Library/Preferences/%s/%s", home,
                 CONFIG_DIR, CONFIG_FILE);

        if (config_read_file(cfg, user_ngprc))
            return;
    }

    // No user specific config file found or $HOME not set. Trying the system wide.
    char sys_ngprc[PATH_MAX];
    snprintf(sys_ngprc, PATH_MAX, "/etc/%s/%s", CONFIG_DIR, CONFIG_FILE);

    if (config_read_file(cfg, sys_ngprc))
        return;

    // Neither a system wide nor a user specific config file found, but $HOME is
    // set. Try to create a per-user file.
    if (home != NULL) {
#endif /* __linux__ / __APPLE__ */
        fprintf(stderr, "No configuration file found. Trying to create a minimal "
                        "default one in %s. You should adopt it to your needs.\n",
                        user_ngprc);

        // The file shouldn't exist (we've tested before). Use O_EXCL though, just
        // to be sure we don't delete anything.
        int fd = open(user_ngprc, O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL,
                                                                  S_IRUSR | S_IWUSR);
        if (0 > fd) {
           fprintf(stderr, "Failed to open default configuration file in %s (%s).\n",
                           user_ngprc, strerror(errno));
           exit(EXIT_FAILURE);
        }

        const char* buf = CONFIG_CONTENT;
        ssize_t written = 0u;
        ssize_t ret = 0u;

        do {
            ret = write(fd, (void*)(buf + written), (size_t)(strlen(buf) - written));

            if (-1 == ret) {
                fprintf(stderr, "Failed to write to default configuration file in %s"
                                " (%s).\n", user_ngprc, strerror(errno));
                exit(EXIT_FAILURE);
            }

            written += ret;
        } while (strlen(buf) > written);

        close(fd);

        if (config_read_file(cfg, user_ngprc))
            return;

        fprintf(stderr, "Failed to create default configuration file in %s.\n",
                        user_ngprc);
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "No configuration file found and unable to create a default"
#ifdef __linux__
                        " one (XDG_CONFIG_HOME not set). Aborting.\n");
#elif __APPLE__
                        " one (HOME not set). Aborting.\n");
#endif /* __linux__ / __APPLE__ */
        config_destroy(cfg);
        exit(EXIT_FAILURE);
    }
}

char *regex(struct options_t *options, const char *line, const char *pattern)
{
    int ret;
    const char *pcre_error;
    int pcre_error_offset;
    int substring_vector[30];
    const char *matched_string;

    /* check if regexp has already been compiled */
    if (!options->pcre_compiled) {
        options->pcre_compiled = pcre_compile(pattern, 0, &pcre_error,
            &pcre_error_offset, NULL);
        if (!options->pcre_compiled)
            return NULL;

        options->pcre_extra =
            pcre_study(options->pcre_compiled, 0, &pcre_error);
        if (pcre_error)
            return NULL;
    }

    ret = pcre_exec(options->pcre_compiled, options->pcre_extra, line,
        strlen(line), 0, 0, substring_vector, 30);

    if (ret < 0)
        return NULL;

    pcre_get_substring(line, substring_vector, ret, 0, &matched_string);

    return (char *) matched_string;
}

void *get_parser(struct options_t *options)
{
    char * (*parser)(struct options_t *, const char *, const char*);

    if (!options->incase_option)
        parser = strstr_wrapper;
    else
        parser = strcasestr_wrapper;

    if (options->regexp_option)
        parser = regex;

    return parser;
}

char *strstr_wrapper(struct options_t *options, const char *line, const char *pattern)
{
    return strstr(line, pattern);
}

char *strcasestr_wrapper(struct options_t *options, const char *line, const char *pattern)
{
    return strcasestr(line, pattern);
}
