#include <stdlib.h>
#include <stdio.h>

#include "interpreter.h"
#include "parser_helpers.h"

#define declare_on_link_entry(struct_type_name, value_type) \
struct struct_type_name *on_##struct_type_name(value_type *value, struct struct_type_name *next) { \
    checked_calloc(struct struct_type_name, list_entry); \
    list_entry->value = value; \
    set_next(list_entry, next); \
    return list_entry; \
}

declare_on_link_entry(arg_entry, struct expr)
declare_on_link_entry(param_entry, struct ident)
declare_on_link_entry(definition_entry, struct func)

struct ident *on_ident(char *name) {
    checked_calloc(struct ident, ident);
    ident->name = name;

    return ident;
}

struct expr *on_literal(unsigned long value) {
    declare_expr_of_type(LITERAL);

    expr->literal = value;

    return expr;
}

struct expr *on_binop(enum binop_type type, struct expr *l, struct expr *r) {
    declare_expr_of_type(BINOP);

    expr->binop.type = type;
    expr->binop.l = l;
    expr->binop.r = r;

    return expr;
}

struct expr *on_puts(struct expr *body) {
    declare_expr_of_type(PUTS);

    expr->puts.body = body;

    return expr;
}

struct expr *on_var(struct ident *ident) {
    declare_expr_of_type(VAR);

    expr->var = ident;

    return expr;
}

struct expr *on_conditional(struct expr *cond, struct expr *on_true, struct expr *on_false) {
    declare_expr_of_type(CONDITIONAL);

    expr->conditional.cond = cond;
    expr->conditional.on_true = on_true;
    expr->conditional.on_false = on_false;

    return expr;
}

struct expr *on_func_call(struct ident *ident, struct arg_entry *arg_entries) {
    declare_expr_of_type(CALL);

    expr->call.args = arg_entries;
    expr->call.callee = ident;

    return expr;
}

struct func *on_func_def(struct ident *ident, struct param_entry *param_entries, struct expr *body) {
    checked_calloc(struct func, func);

    func->ident = ident;
    func->params = param_entries;
    func->body = body;

    return func;
}

struct program *on_program(struct definition_entry *definition_entries, struct expr *main_expr) {
    checked_calloc(struct program, program);

    program->funcs = definition_entries;
    program->expr = main_expr;

    return program;
}
