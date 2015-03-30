%{
#include <stdio.h>
#include <string.h>
#include "parser.tab.h"
#include "ast.h"

/* http://stackoverflow.com/a/22125500
   You wouldn't think this would be so annoying! */
static void update_loc(YYLTYPE *yylloc, char *yytxt){
  static int curr_line = 1;
  static int curr_col  = 1;

  yylloc->first_line   = curr_line;
  yylloc->first_column = curr_col;

  {char * s; for(s = yytxt; *s != '\0'; s++){
    if(*s == '\n'){
      curr_line++;
      curr_col = 1;
    }else{
      curr_col++;
    }
  }}

  yylloc->last_line   = curr_line;
  yylloc->last_column = curr_col-1;
}

#define YY_USER_ACTION update_loc(yylloc, yytext);

%}

%option reentrant bison-bridge bison-locations noyywrap yylineno
%option full

%%

"{"(.|"\n")*"}" {  }
"(*"(.|"\n")*"*)" {  }
"//".*          {  }
[ \t\n]         {  }

"and"       { return AND; }
"array"     { return ARRAY; }
"begin"     { return TBEGIN; }
"boolean"   { return BOOLEAN; }
"char"      { return CHAR; }
"div"       { return DIV; }
"do"        { return DO; }
"else"      { return ELSE; }
"end"       { return END; }
"for"       { return FOR; }
"function"  { return FUNCTION; }
"if"        { return IF; }
"integer"   { return INTEGER; }
"mod"       { return MOD; }
"not"       { return NOT; }
"of"        { return OF; }
"or"        { return OR; }
"procedure" { return PROCEDURE; }
"program"   { return PROGRAM; }
"real"      { return REAL; }
"record"    { return RECORD; }
"string"    { return STRING; }
"then"      { return THEN; }
"to"        { return TO; }
"type"      { return TYPE; }
"var"       { return VAR; }
"while"     { return WHILE; }

":="        { return ASSIGNOP; }
"<>"        { return NEQ; }
"<="        { return LE; }
">="        { return GE; }
".."        { return DOTDOT; }

[a-zA-Z_][a-zA-Z0-9_]* {
#define _POSIX_SOURCE 200809L
#include <string.h>
    yylval->name = strndup(yytext, yyleng);
    return ID;
}

[0-9]+(\.[0-9]+)?([eE](\+|\-)?[0-9]+)? {
#define _POSIX_SOURCE 200809L
#include <string.h>
    yylval->name = strndup(yytext, yyleng);
    return NUM;
}

. { return yytext[0]; }

%%

int yyerror(YYLTYPE *loc, struct ast_program **p, int options, void *scanner, const char *s) {
    printf("parse error at line %d: %s\n", loc->first_line, s);
    return 0;
}
