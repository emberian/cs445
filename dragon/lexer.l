%{
#include <stdio.h>
#include "parser.tab.h"
#include "ast.h"
#include "symbol.h"

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
"//".*          {  }
[ \t\n]         {  }

"div"       { return DIV; }
"mod"       { return MOD; }
"and"       { return AND; }
"or"        { return OR; }
"not"       { return NOT; }
"function"  { return FUNCTION; }
"procedure" { return PROCEDURE; }
"begin"     { return TBEGIN; }
"end"       { return END; }
"integer"   { return INTEGER; }
"real"      { return REAL; }
"array"     { return ARRAY; }
"of"        { return OF; }
"program"   { return PROGRAM; }
"do"        { return DO; }
"else"      { return ELSE; }
"if"        { return IF; }
"then"      { return THEN; }
"var"       { return VAR; }
"while"     { return WHILE; }
"for"       { return FOR; }
"to"        { return TO; }

"="  { return EQ; }
"<>" { return NEQ; }
"<"  { return LT; }
"<=" { return LE; }
">"  { return GT; }
">=" { return GE; }

"+" { return PLUS; }
"-" { return MINUS; }
"*" { return STAR; }
"/" { return SLASH; }

"("  { return LPAREN; }
")"  { return RPAREN; }
"["  { return LBRACKET; }
"]"  { return RBRACKET; }
","  { return COMMA; }
".." { return DOTDOT; }
"."  { return DOT; }
":=" { return ASSIGNOP; }
":"  { return COLON; }
";"  { return SEMI; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval->node = intern(yytext);
    return ID;
}

[0-9]+(\.[0-9]+)?([eE](\+|\-)?[0-9]+)? {
    yylval->node = M(ast_node);
    yylval->node->type = AST_EXPR_LIT;
    yylval->node->lit_val = intern(yytext);
    return NUM;
}

%%

int yyerror(YYLTYPE *loc, struct ast_node **an, void *scanner, const char *s) {
    printf("parse error at line %d: %s\n", loc->first_line, s);
    return 0;
}
