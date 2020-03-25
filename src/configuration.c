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

#include "configuration.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "entry.h"
#include "list.h"
#include "utils.h"

#define CONFIG_DIR "ngp"
#define CONFIG_FILE "ngprc"

struct configuration_t {
    config_t config;
};

struct configuration_t *create_configuration() {
    struct configuration_t *new = calloc(1, sizeof(struct configuration_t));
    return new;
}

config_t get_config(struct configuration_t *config) { return config->config; }

static void create_user_ngprc(const char *file_path) {
    // The file shouldn't exist (we've tested before). Use O_EXCL though, just
    // to be sure we don't delete anything.
    int fd = open(file_path, O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL,
                  S_IRUSR | S_IWUSR);
    if (0 > fd) {
        fprintf(stderr,
                "Failed to open default configuration file in %s (%s).\n",
                file_path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    const char *buf = CONFIG_CONTENT;
    ssize_t written = 0u;
    ssize_t ret = 0u;

    do {
        ret = write(fd, (void *)(buf + written),
                    (size_t)(strlen(buf) - written));

        if (-1 == ret) {
            fprintf(stderr,
                    "Failed to write to default configuration file in %s"
                    " (%s).\n",
                    file_path, strerror(errno));
            exit(EXIT_FAILURE);
        }

        written += ret;
    } while (strlen(buf) > written);

    close(fd);
}

void load_configuration(struct configuration_t *config) {
    config_init(&config->config);

    char user_ngprc[PATH_MAX];
#ifdef __linux__
    char *xdg_config_home = getenv("XDG_CONFIG_HOME");

    if (xdg_config_home != NULL) {
        snprintf(user_ngprc, PATH_MAX, "%s/%s/%s", xdg_config_home, CONFIG_DIR,
                 CONFIG_FILE);

        if (config_read_file(&config->config, user_ngprc)) return;
    }

    char *xdg_config_dirs = getenv("XDG_CONFIG_DIRS");

    if (xdg_config_dirs != NULL) {
        // strtok overwrites delimiters with NULL bytes, so we need to operate
        // on a copy here. Allow for five entries in XDG_CONFIG_DIRS here, that
        // should probably be enough.
        char xdg_config_dirs_cpy[PATH_MAX * 5];
        strncpy(xdg_config_dirs_cpy, xdg_config_dirs,
                strlen(xdg_config_dirs) - 1);

        char *token = strtok(xdg_config_dirs_cpy, ":");

        // Iterate through all directories within XDG_CONFIG_DIRS
        while (token != NULL) {
            char sys_ngprc[PATH_MAX];
            snprintf(sys_ngprc, PATH_MAX, "%s/%s/%s", token, CONFIG_DIR,
                     CONFIG_FILE);

            if (config_read_file(&config->config, sys_ngprc)) return;

            token = strtok(NULL, ":");
        }
    }

#elif __APPLE__
    // At first, try the user specific config file
    char *home = getenv("HOME");

    if (home != NULL) {
        snprintf(user_ngprc, PATH_MAX, "%s/Library/Preferences/%s/%s", home,
                 CONFIG_DIR, CONFIG_FILE);

        if (config_read_file(&config->config, user_ngprc)) return;
    }

#endif /* __linux__ / __APPLE__ */

    // If file does not exist, create a default ngprc and open it
    char dir[PATH_MAX - strlen(CONFIG_FILE)];
    snprintf(dir, sizeof(dir), "%s/.config/%s", getenv("HOME"), CONFIG_DIR);
    mkdir(dir, 0777);
    snprintf(user_ngprc, sizeof(dir), "%s/%s", dir, CONFIG_FILE);

    if (access(user_ngprc, R_OK) < 0) {
        create_user_ngprc(user_ngprc);
    }

    if (config_read_file(&config->config, user_ngprc)) return;

    fprintf(stderr, "Failed to create default configuration file in %s.\n",
            user_ngprc);
    exit(EXIT_FAILURE);
}

void destroy_configuration(struct configuration_t *config) { free(config); }
