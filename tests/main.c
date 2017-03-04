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
#include "minunit.h"
#include "search.h"
#include "list.h"
#include "display.h"

int tests_run = 0;

static char * test_no_entry()
{
    struct search_t *search;
    search = create_search();
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "third";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 0);
    free_search(search);
    return 0;
}

static char * test_one_entry_on_the_first_line()
{
    struct search_t *search;
    search = create_search();
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "first";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 2);
    free_search(search);
    return 0;
}

static char * test_one_entry_on_the_second_line()
{
    struct search_t *search;
    search = create_search();
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "second";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 2);
    free_search(search);
    return 0;
}

static char * test_one_entry_incase()
{
    struct search_t *search;
    search = create_search();
    search->incase_option = 1;
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "First";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 2);
    free_search(search);
    return 0;
}

static char * test_two_entries()
{
    struct search_t *search;
    search = create_search();
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "line";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 3);
    free_search(search);
    return 0;
}

static char * test_regexp_start_of_line()
{
    struct search_t *search;
    search = create_search();
    search->regexp_option = 1;
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "^this is";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 3);
    free_search(search);
    return 0;
}

static char * test_wrong_regexp()
{
    struct search_t *search;
    search = create_search();
    search->regexp_option = 1;
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "the.*file";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 0);
    free_search(search);
    return 0;
}

static char * test_is_specific_file_ok()
{
    struct search_t *search;
    search = create_search();
    add_element(&search->specific_file, "Makefile");
    mu_assert("test_is_specific_file_ok failed", is_specific_file(search, "Makefile") == 0);
    free_search(search);
    return 0;
}

static char * test_is_specific_file_ko()
{
    struct search_t *search;
    search = create_search();
    add_element(&search->specific_file, "Makefile");
    mu_assert("test_is_specific_file_ko failed", is_specific_file(search, "makefile") == 0);
    free_search(search);
    return 0;
}

static char * test_is_ignored_file_ok()
{
    struct search_t *search;
    search = create_search();
    add_element(&search->ignore, "rules");
    mu_assert("test_is_ignored_file_ok failed", is_ignored_file(search, "rules") == 0);
    free_search(search);
    return 0;
}

static char * test_is_ignored_file_ko()
{
    struct search_t *search;
    search = create_search();
    add_element(&search->ignore, "rules");
    mu_assert("test_is_ignored_file_ko failed", is_ignored_file(search, "Rules") == 0);
    free_search(search);
    return 0;
}

static char * test_is_extension_good_ok()
{
    struct search_t *search;
    search = create_search();
    add_element(&search->extension, ".cpp");
    mu_assert("test_is_extension_good_ok failed", is_ignored_file(search, "file.cpp") == 0);
    free_search(search);
    return 0;
}

static char * test_is_extension_good_ko()
{
    struct search_t *search;
    search = create_search();
    add_element(&search->extension, ".cpp");
    mu_assert("test_is_extension_good_ko failed", is_ignored_file(search, "file.c") == 0);
    free_search(search);
    return 0;
}

static char * test_cursor_down()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down failed", display->cursor == 2);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_down_end_of_entries()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is a the first line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_end_of_entries failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_down_skip_file()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is a the first line\n";
    char text2[] = "this is a the first line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    parse_text(search, "fake_file2", strlen(text2), text, pattern);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_skip_file failed", display->cursor == 3);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_down_end_of_page()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is the first line\n this is the second line\n this is the third line\n this is the fourth line \n ";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 3;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_end_of_page failed", display->cursor == 0);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_down_end_of_page_skip_file()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is the first line\n this is the second line\n";
    char text2[] = "this is the first line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 3;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    parse_text(search, "fake_file2", strlen(text2), text2, pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_down_end_of_page_skip_file failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_up()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is the first line\nthis the second line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    display->cursor = 2;
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_up_top_first_page()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is the first line\nthis the second line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_top_first_page failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_up_skip_file()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is the first line\n";
    char text2[] = "this is the first line\nthis the second line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 10;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    parse_text(search, "fake_file2", strlen(text2), text2, pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_skip_file failed", display->cursor == 1);
    free_search(search);
    free_display(display);

    return 0;
}

static char * test_cursor_up_page_up()
{
    struct display_t *display;
    struct search_t *search;
    int terminal_line_nb;
    char text[] = "this is the first line\nthis is the second line\nthis is the third line\nthis is the fourth line\n";
    const char *pattern = "line";

    display = create_display();
    search = create_search();
    terminal_line_nb = 3;

    parse_text(search, "fake_file", strlen(text), text, pattern);
    move_cursor_down(display, search, terminal_line_nb);
    move_cursor_down(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_skip_file failed", display->cursor == 0);
    move_cursor_up(display, search, terminal_line_nb);
    mu_assert("test_cursor_up_skip_file failed", display->cursor == 2);
    free_search(search);
    free_display(display);

    return 0;
}

static char * all_tests() {
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
    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
