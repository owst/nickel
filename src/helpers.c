#include <stdlib.h>

#include "helpers.h"

unsigned long linked_list_count(struct link *head) {
    unsigned long count = 0;

    while (head != NULL) {
        head = head->next;
        count++;
    }

    return count;
}
