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

#include <stdio.h>
#include <string.h>

#include "configuration.h"
#include "display.h"
#include "list.h"
#include "minunit.h"
#include "ngp_search.h"
#include "search.h"

int tests_run = 0;
char *command_line_arg_tests();

#include "../src/ngp_search.c"

static char *test_no_entry() {
    char *argv[] = {"ngp", "third"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    char text[] = "this is a the first line\nthis is the second line\n";
    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    mu_assert("error in number of entry", search->result->nbentry == 0);
    free_search(search);
    return 0;
}

static char *test_one_entry_on_the_first_line() {
    char *argv[] = {"ngp", "first"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    char text[] = "this is a the first line\nthis is the second line\n";
    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    mu_assert("error in number of entry", search->result->nbentry == 2);
    free_search(search);
    return 0;
}

static char *test_one_entry_on_the_second_line() {
    char *argv[] = {"ngp", "second"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    char text[] = "this is a the first line\nthis is the second line\n";
    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    mu_assert("error in number of entry", search->result->nbentry == 2);
    free_search(search);
    return 0;
}

static char *test_one_entry_incase() {
    char *argv[] = {"ngp", "First"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    options->incase_option = 1;
    struct search_t *search = create_search(options);
    char text[] = "this is a the first line\nthis is the second line\n";
    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    mu_assert("error in number of entry", search->result->nbentry == 2);
    free_search(search);
    return 0;
}

static char *test_two_entries() {
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    char text[] = "this is a the first line\nthis is the second line\n";
    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    mu_assert("error in number of entry", search->result->nbentry == 3);
    free_search(search);
    return 0;
}

static char *test_regexp_start_of_line() {
    char *argv[] = {"ngp", "^this is"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    options->regexp_option = 1;
    struct search_t *search = create_search(options);
    char text[] = "this is a the first line\nthis is the second line\n";
    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    mu_assert("error in number of entry", search->result->nbentry == 3);
    free_search(search);
    return 0;
}

static char *test_wrong_regexp() {
    char *argv[] = {"ngp", "the.*file"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    options->regexp_option = 1;
    struct search_t *search = create_search(options);
    char text[] = "this is a the first line\nthis is the second line\n";
    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    mu_assert("error in number of entry", search->result->nbentry == 0);
    free_search(search);
    return 0;
}

static char *test_is_specific_file_ok() {
    char *argv[] = {"ngp", "pattern"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    add_element(&options->specific_file, "Makefile");
    struct search_t *search = create_search(options);
    mu_assert("test_is_specific_file_ok failed",
              is_specific_file(search->options, "Makefile") == 1);
    free_search(search);
    return 0;
}

static char *test_is_specific_file_ko() {
    char *argv[] = {"ngp", "pattern"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    add_element(&options->specific_file, "Makefile");
    struct search_t *search = create_search(options);
    mu_assert("test_is_specific_file_ko failed",
              is_specific_file(search->options, "makefile") == 0);
    free_search(search);
    return 0;
}

static char *test_is_ignored_file_ok() {
    char *argv[] = {"ngp", "pattern"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    add_element(&options->ignore, "rules");
    struct search_t *search = create_search(options);
    mu_assert("test_is_ignored_file_ok failed",
              is_ignored_file(search->options, "rules") == 1);
    free_search(search);
    return 0;
}

static char *test_is_ignored_file_ko() {
    char *argv[] = {"ngp", "pattern"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    add_element(&options->ignore, "rules");
    struct search_t *search = create_search(options);
    mu_assert("test_is_ignored_file_ko failed",
              is_ignored_file(search->options, "Rules") == 0);
    free_search(search);
    return 0;
}

static char *test_is_extension_good_ok() {
    char *argv[] = {"ngp", "pattern"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    add_element(&options->extension, ".cpp");
    struct search_t *search = create_search(options);
    mu_assert("test_is_extension_good_ok failed",
              is_ignored_file(search->options, "file.cpp") == 0);
    free_search(search);
    return 0;
}

static char *test_is_extension_good_ko() {
    char *argv[] = {"ngp", "pattern"};
    int argc = sizeof(argv) / sizeof(*argv);
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    add_element(&options->extension, ".cpp");
    struct search_t *search = create_search(options);
    mu_assert("test_is_extension_good_ko failed",
              is_ignored_file(search->options, "file.c") == 0);
    free_search(search);
    return 0;
}

static char *test_cursor_down() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] = "this is a the first line\nthis is the second line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down failed", display->cursor == 2);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_down_end_of_entries() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] = "this is a the first line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_end_of_entries failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_down_skip_file() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] = "this is a the first line\n";
    char text2[] = "this is a the first line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    parse_text(search, "fake_file2", strlen(text2), text, options->pattern);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_skip_file failed", display->cursor == 3);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_down_end_of_page() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] =
            "this is the first line\n this is the second line\n this is the "
            "third line\n this is the fourth line \n ";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 3;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_end_of_page failed", display->cursor == 0);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_down_end_of_page_skip_file() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] = "this is the first line\n this is the second line\n";
    char text2[] = "this is the first line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 3;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    parse_text(search, "fake_file2", strlen(text2), text2, options->pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_end_of_page_skip_file failed",
              display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_up() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] = "this is the first line\nthis the second line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    display->cursor = 2;
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_up_top_first_page() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] = "this is the first line\nthis the second line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_top_first_page failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_up_skip_file() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] = "this is the first line\n";
    char text2[] = "this is the first line\nthis the second line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    parse_text(search, "fake_file2", strlen(text2), text2, options->pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_skip_file failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_cursor_up_page_up() {
    struct display_t *display;
    int terminal_line_nb;
    char text[] =
            "this is the first line\nthis is the second line\nthis is the "
            "third line\nthis is the fourth line\n";
    char *argv[] = {"ngp", "line"};
    int argc = sizeof(argv) / sizeof(*argv);

    display = create_display();
    struct configuration_t *config = NULL;
    struct options_t *options = create_options(config, argc, argv);
    struct search_t *search = create_search(options);
    terminal_line_nb = 3;

    parse_text(search, "fake_file", strlen(text), text, options->pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_skip_file failed", display->cursor == 0);
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_skip_file failed", display->cursor == 2);
    free_search(search);
    free_display(display);

    return 0;
}

static char *test_get_file_name_simple() {
    char file_name[FILENAME_MAX];
    char *result = get_file_name("/file1", file_name);
    mu_assert("test_bla failed", strcmp(result, "file1") == 0);

    return 0;
}

static char *test_get_file_name_multiple() {
    char file_name[FILENAME_MAX];
    char *result = get_file_name("/dir1/file1", file_name);
    mu_assert("test_get_file_name failed", strcmp(result, "file1") == 0);

    return 0;
}

static char *test_get_file_name_current_dir() {
    char file_name[FILENAME_MAX];
    char *result = get_file_name(".", file_name);
    mu_assert("test_get_file_name_current_dir failed",
              strcmp(result, ".") == 0);

    return 0;
}

static char *test_get_file_name_ending_with_slash() {
    char file_name[FILENAME_MAX];
    char *result = get_file_name("/dir1/dir2/", file_name);
    mu_assert("test_get_file_name failed_ending_with_slash",
              strcmp(result, "dir2") == 0);

    return 0;
}

static char *all_tests() {
    char *message = command_line_arg_tests();
    if (message) return message;

    mu_run_test(test_no_entry);
    mu_run_test(test_one_entry_on_the_first_line);
    mu_run_test(test_one_entry_on_the_second_line);
    mu_run_test(test_one_entry_incase);
    mu_run_test(test_two_entries);
    mu_run_test(test_regexp_start_of_line);
    mu_run_test(test_wrong_regexp);
    mu_run_test(test_is_specific_file_ok);
    mu_run_test(test_is_specific_file_ko);
    mu_run_test(test_is_ignored_file_ok);
    mu_run_test(test_is_ignored_file_ko);
    mu_run_test(test_is_extension_good_ok);
    mu_run_test(test_is_extension_good_ko);
    mu_run_test(test_cursor_down);
    mu_run_test(test_cursor_down_end_of_entries);
    mu_run_test(test_cursor_down_skip_file);
    mu_run_test(test_cursor_down_end_of_page);
    mu_run_test(test_cursor_down_end_of_page_skip_file);
    mu_run_test(test_cursor_up);
    mu_run_test(test_cursor_up_top_first_page);
    mu_run_test(test_cursor_up_skip_file);
    mu_run_test(test_cursor_up_page_up);
    mu_run_test(test_get_file_name_simple);
    mu_run_test(test_get_file_name_multiple);
    mu_run_test(test_get_file_name_current_dir);
    mu_run_test(test_get_file_name_ending_with_slash);
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
