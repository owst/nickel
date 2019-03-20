%{
#include <stdio.h>

#include "parser_helpers.h"

void reverse_linked_list(struct link **head) {
    struct link *previous, *current, *temp;

    current = *head;
    previous = NULL;

    while (current != NULL) {
        temp = current->next;
        current->next = previous;
        previous = current;
        current = temp;
    }

    *head = previous;
}

#define reverse(x) reverse_linked_list((struct link **)x)

extern int yylex();

int yywrap() {
    return 1;
}

void yyerror (__attribute__((unused)) struct program **_root_program, char const *s) {
    fprintf (stderr, "%s\n", s);
}

%}

%token T_LPAREN T_RPAREN T_COMMA T_DEF T_END T_IF T_THEN T_ELSE
%token <str> T_IDENT
%token <ulong> T_NUM
%nonassoc T_PUTS T_LE T_EQ
%left T_LSHIFT
%left T_PLUS T_MINUS
%left T_MULTIPLY

%union {
  char *str;
  struct expr *expr;
  unsigned long ulong;
  struct ident *ident;
  struct arg_entry *arg_entries;
  struct param_entry *param_entries;
  struct definition_entry *definition_entries;
  struct func *func;
  struct program *program;
}

%type <program> program;
%type <expr> binop_expression expression;
%type <ident> ident;
%type <func> definition;
%type <arg_entries> optional_arguments arguments;
%type <param_entries> optional_params params;
%type <definition_entries> definitions optional_definitions;

%parse-param { struct program **root_program }

%start program

%%

program : optional_definitions expression { *root_program = on_program($1, $2); }
        ;

optional_definitions : /* nothing */ { $$ = NULL; }
                     | definitions { reverse(&$1); $$ = $1; }
                     ;

definitions : definitions definition { $$ = on_definition_entry($2, $1); }
            | definition { $$ = on_definition_entry($1, NULL); }
            ;

definition : T_DEF ident T_LPAREN optional_params T_RPAREN expression T_END { $$ = on_func_def($2, $4, $6); }
           ;

ident : T_IDENT { $$ = on_ident($1); }
      ;

optional_params : /* nothing... */ { $$ = NULL; }
                | params { reverse(&$1); $$ = $1; }
                ;

params : params T_COMMA ident { $$ = on_param_entry($3, $1); }
       | ident { $$ = on_param_entry($1, NULL); }
       ;

binop_expression : expression T_PLUS expression { $$ = on_binop(PLUS, $1, $3); }
                 | expression T_LSHIFT expression { $$ = on_binop(LSHIFT, $1, $3); }
                 | expression T_MULTIPLY expression { $$ = on_binop(MULT, $1, $3); }
                 | expression T_MINUS expression { $$ = on_binop(MINUS, $1, $3); }
                 | expression T_LE expression { $$ = on_binop(LE, $1, $3); }
                 | expression T_EQ expression { $$ = on_binop(EQ, $1, $3); }
                 ;

optional_arguments : /* nothing... */ { $$ = NULL; }
                   | arguments { reverse(&$1); $$ = $1; }
                   ;

arguments : arguments T_COMMA expression { $$ = on_arg_entry($3, $1); }
          | expression { $$ = on_arg_entry($1, NULL); }
          ;

expression : binop_expression { $$ = $1; }
           | T_NUM { $$ = on_literal($1); }
           | ident { $$ = on_var($1); }
           | ident T_LPAREN optional_arguments T_RPAREN { $$ = on_func_call($1, $3); }
           | T_LPAREN expression T_RPAREN { $$ = $2; }
           | T_PUTS expression { $$ = on_puts($2); }
           | T_IF expression T_THEN expression T_ELSE expression T_END { $$ = on_conditional($2, $4, $6); }
           ;
%%
