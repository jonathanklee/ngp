#include "circular_list.h"

struct circular_list *create_circular_list(int size) {
    struct circular_list *list = calloc(1, sizeof(struct circular_list));

    list->size = size;
    list->nb_element = 0;
    list->head = NULL;

    return list;
}

void add_circular_element(struct circular_list **list, char *element) {
    struct list_element *new_element;
    struct list_element *pointer_element;
    int len = strlen(element) + 1;

    new_element = calloc(1, sizeof(struct list_element) + len);
    strncpy(new_element->data, element, len);
    new_element->next = NULL;

    if ((*list)->nb_element == 0) {
        (*list)->head = new_element;
        (*list)->nb_element++;
        return;
    }

    pointer_element = (*list)->head;
    while (pointer_element->next) {
        pointer_element = pointer_element->next;
    }
    pointer_element->next = new_element;

    if ((*list)->nb_element < (*list)->size) {
        (*list)->nb_element++;
    } else {
        struct list_element *old_head = (*list)->head;
        (*list)->head = (*list)->head->next;
        free(old_head);
    }
}

void free_circular_list(struct circular_list **list) {
    struct list_element *element = (*list)->head;

    while (element->next) {
        struct list_element *old_element = element;
        element = element->next;
        free(old_element);
    }

    free(element);
}
