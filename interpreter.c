#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter.h"

long interpret_binop(struct environment *env, struct binop b) {
    long lval = interpret_expr(env, b.l);
    long rval = interpret_expr(env, b.r);

    switch (b.type) {
        case EQ:
            return lval == rval;
        case PLUS:
            return lval + rval;
        case LSHIFT:
            return lval << rval;
        case MULT:
            return lval * rval;
        case MINUS:
            return lval - rval;
        case LE:
            return lval <= rval;
    }
}

long interpret_conditional(struct environment *env, struct conditional c) {
    if (interpret_expr(env, c.cond)) {
        return interpret_expr(env, c.on_true);
    } else {
        return interpret_expr(env, c.on_false);
    }
}

long interpret_puts(struct environment *env, struct puts p) {
    long res = interpret_expr(env, p.body);

    printf("%ld\n", res);

    return res;
}

bool equal_ident(struct ident *l, struct ident *r) {
    return strcmp(l->name, r->name) == 0;
}

long lookup_var(struct environment *env, struct ident *search_ident) {
    struct stack_frame *stack_frame = env->stack_frame;
    struct var_binding *var_binding = stack_frame->var_binding;

    while (var_binding != NULL) {
        if (equal_ident(var_binding->var_name, search_ident)) {
            return var_binding->var_value;
        }

        var_binding = next_entry(var_binding, struct var_binding);
    }

    die("Could not find var %s in stack frame of %s!", search_ident->name, stack_frame->func_ident->name);
}

struct func *lookup_func(struct environment *env, struct ident *search_ident) {
    struct definition_entry *definition_entry = env->head_definition_entry;

    while (definition_entry != NULL) {
        if (equal_ident(definition_entry->value->ident, search_ident)) {
            return definition_entry->value;
        }

        definition_entry = next_entry(definition_entry, struct definition_entry);
    }

    die("Could not find func %s in env!", search_ident->name);
}

struct var_binding *create_var_binding(struct environment *env,
                                       struct ident *ident, struct expr *expr) {
    checked_calloc(struct var_binding, var_binding)

    var_binding->var_name = ident;
    var_binding->var_value = interpret_expr(env, expr);

    return var_binding;
}

long push_stack_frame(struct ident *func_ident,
                               struct environment *env,
                               struct param_entry *param_entry,
                               struct arg_entry *arg_entry) {
    struct var_binding *var_binding;
    unsigned long arg_count = 0;

    checked_calloc(struct stack_frame, stack_frame);
    stack_frame->func_ident = func_ident;

    while (param_entry != NULL && arg_entry != NULL) {
        var_binding = create_var_binding(env, param_entry->value, arg_entry->value);
        set_next(var_binding, stack_frame->var_binding);
        stack_frame->var_binding = var_binding;

        arg_entry = next_entry(arg_entry, struct arg_entry);
        param_entry = next_entry(param_entry, struct param_entry);
        arg_count++;
    }

    set_next(stack_frame, env->stack_frame);
    env->stack_frame = stack_frame;

    if (param_entry != NULL || arg_entry != NULL) {
        die("Unexpected arg count for %s, expected %lu, got %lu args",
            func_ident->name,
            arg_count + linked_list_count((struct link *)param_entry),
            arg_count + linked_list_count((struct link *)arg_entry));
    }

    return arg_count;
}

void pop_stack_frame(struct environment *env, unsigned long arg_count) {
    struct var_binding *binding;
    struct stack_frame *stack_frame = env->stack_frame;

    for (unsigned long a = 0; a < arg_count; a++) {
        binding = stack_frame->var_binding;
        stack_frame->var_binding = next_entry(binding, struct var_binding);

        free(binding);
    }

    env->stack_frame = next_entry(stack_frame, struct stack_frame);
}

long interpret_call(struct environment *env, struct call c) {
    struct func *f;
    long func_res;
    unsigned long arg_count;

    f = lookup_func(env, c.callee);

    arg_count = push_stack_frame(f->ident, env, f->params, c.args);

    func_res = interpret_expr(env, f->body);

    pop_stack_frame(env, arg_count);

    return func_res;
}

long interpret_expr(struct environment *env, struct expr *expr) {
    switch (expr->type) {
        case LITERAL:
            return expr->literal;
        case VAR:
            return lookup_var(env, expr->var);
        case BINOP:
            return interpret_binop(env, expr->binop);
        case CONDITIONAL:
            return interpret_conditional(env, expr->conditional);
        case PUTS:
            return interpret_puts(env, expr->puts);
        case CALL:
            return interpret_call(env, expr->call);
    }
}
