%{
#include "parser.tab.h"
%}

%option nounput
%option noinput

%%

[ \t\n] ; // ignore all whitespace
[0-9]+  { yylval.ulong = atoi(yytext); return T_NUM; }
"+"     { return T_PLUS; }
"-"     { return T_MINUS; }
"*"     { return T_MULTIPLY; }
"<<"    { return T_LSHIFT; }
"<="    { return T_LE; }
"=="    { return T_EQ; }
"("     { return T_LPAREN; }
","     { return T_COMMA; }
")"     { return T_RPAREN; }
"def"   { return T_DEF; }
"if"    { return T_IF; }
"else"  { return T_ELSE; }
"then"  { return T_THEN; }
"end"   { return T_END; }
"puts"  { return T_PUTS; }
[a-z_]+ { yylval.str = strdup(yytext); return T_IDENT; }

%%
