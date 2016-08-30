#include <stdio.h>
#include <string.h>
#include "minunit.h"
#include "search.h"

int tests_run = 0;

static char * test_no_entry() {

    struct search_t *search;
    search = create_search();
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "third";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 0);
    free_search(search);
    return 0;
}

static char * test_one_entry_on_the_first_line() {

    struct search_t *search;
    search = create_search();
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "first";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 2);
    free_search(search);
    return 0;
}

static char * test_one_entry_on_the_second_line() {

    struct search_t *search;
    search = create_search();
    char text[] = "this is a the first line\nthis is the second line\n";
    const char *pattern = "second";
    parse_text(search, "fake_file", strlen(text), text, pattern);
    mu_assert("error in number of entry", search->nbentry == 2);
    free_search(search);
    return 0;
}

static char * test_one_entry_incase() {

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

static char * all_tests() {
    mu_run_test(test_no_entry);
    mu_run_test(test_one_entry_on_the_first_line);
    mu_run_test(test_one_entry_on_the_second_line);
    mu_run_test(test_one_entry_incase);
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
