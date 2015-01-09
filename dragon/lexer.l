%{
#include "parser.tab.h"
#include <stdio.h>
#include "ast.h"
%}

%option reentrant
%option bison-bridge
%option noyywrap

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

[a-zA-Z][a-zA-Z0-9]* {
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

int yyerror(void *scanner, struct ast_node **an, const char *s) {
    printf("LEX ERROR: %s\n", s);
    return 0;
}
