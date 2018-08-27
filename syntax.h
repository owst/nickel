#ifndef SYNTAX_H
#define SYNTAX_H

#include "helpers.h"

struct definition_entry { struct link link; struct func *value; };

struct ident { char *name; };
struct arg_entry { struct link link; struct expr *value; };

struct expr {
    enum expr_type { LITERAL, VAR, BINOP, CONDITIONAL, PUTS, CALL } type;
    union {
        unsigned long literal;
        struct ident *var;
        struct binop {
            enum binop_type { PLUS, MULT, LSHIFT, MINUS, LE, EQ } type;
            struct expr *l;
            struct expr *r;
        } binop;
        struct conditional { struct expr *cond; struct expr *on_true; struct expr *on_false; } conditional;
        struct puts { struct expr *body; } puts;
        struct call { struct ident *callee; struct arg_entry *args; } call;
    };
};

struct param_entry { struct link link;  struct ident *value; };
struct func { struct ident *ident; struct param_entry* params; struct expr *body; };
struct program { struct definition_entry *funcs; struct expr *expr; };

#endif /* SYNTAX_H */
