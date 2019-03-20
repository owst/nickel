#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "helpers.h"
#include "syntax.h"

struct stack_frame {
    struct link link;
    struct ident *func_ident;
    struct var_binding {
        struct link link;
        struct ident *var_name;
        unsigned long var_value;
    } *var_binding;
};

struct environment {
    struct stack_frame *stack_frame;
    struct definition_entry *head_definition_entry;
};

long interpret_expr(struct environment *env, struct expr *expr);

#endif /* INTERPRETER_H */
