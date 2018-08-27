#ifndef PARSER_HELPERS_H
#define PARSER_HELPERS_H

#include <stdlib.h>
#include "interpreter.h"

#define declare_expr_of_type(expr_type) \
  checked_calloc(struct expr, expr); \
  expr->type = expr_type

struct ident *on_ident(char *name);

struct expr *on_literal(unsigned long value);
struct expr *on_binop(enum binop_type type, struct expr *l, struct expr *r);
struct expr *on_conditional(struct expr *cond, struct expr *on_true, struct expr *on_false);
struct expr *on_puts(struct expr *body);
struct expr *on_var(struct ident *ident);
struct expr *on_func_call(struct ident *ident, struct arg_entry *arg_entries);
struct func *on_func_def(struct ident *ident, struct param_entry *param_entries, struct expr *body);
struct program *on_program(struct definition_entry *definition_entries, struct expr *main_expr);

struct arg_entry *on_arg_entry(struct expr *value, struct arg_entry *next);
struct param_entry *on_param_entry(struct ident *value, struct param_entry *next);
struct definition_entry *on_definition_entry(struct func *value, struct definition_entry *next);

#endif /* PARSER_HELPERS_H */
