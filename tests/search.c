#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "search.h"
#include "list.h"

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
