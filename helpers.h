#ifndef HELPERS_H
#define HELPERS_H

#define die(fmt, ...) do { \
    fprintf(stderr, fmt, ##__VA_ARGS__); /* Flawfinder: ignore */ \
    fprintf(stderr, "\n"); \
    exit(1); \
    __builtin_unreachable(); \
} while (0)

#define checked_calloc(type, name) \
    type *name = calloc(1, sizeof(type)); \
    if (name == NULL) { die("calloc failure"); }

struct link {
  struct link *next;
};

#define set_next(head, tail) ((struct link *)head)->next = (struct link *)tail
#define next_entry(name, type) (type *)((struct link *)name)->next

unsigned long linked_list_count(struct link *head);

#endif /* HELPERS_H */
