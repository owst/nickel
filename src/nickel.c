#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "y.tab.h"
#include "interpreter.h"
#include "jit.h"

void interpret(struct program *p) {
    struct environment env = { NULL, NULL };

    env.head_definition_entry = p->funcs;

    interpret_expr(&env, p->expr);
}

int main(int argc, char *argv[]) {
    bool use_jit = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--jit") == 0) {
            use_jit = true;
        }
    }

    struct program *p;

    int parse_result = yyparse(&p);

    if (parse_result == 0) {
        if (use_jit) {
            jit(p);
        } else {
            interpret(p);
        }
    }

    return parse_result;
}
