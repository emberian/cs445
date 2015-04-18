%{
#define YYERROR_VERBOSE
#include "ast.h"
#include "util.h"
#include "lexer.h"

extern int yyerror(YYLTYPE *, struct ast_program **, int options, void *scanner, const char *s);
%}

%code requires {
struct ast_program;
}

%expect 0

%token AND
%token ARRAY
%token BOOLEAN
%token CHAR
%token DIV
%token DO
%token ELSE
%token END
%token FOR
%token FUNCTION
%token ID
%token IF
%token INTEGER
%token MOD
%token NOT
%token NUM
%token OF
%token OR
%token PROCEDURE
%token PROGRAM
%token REAL
%token RECORD
%token STRING
%token TBEGIN
%token THEN
%token TO
%token TYPE
%token VAR
%token WHILE

%token NEQ
%token DOTDOT
%token ASSIGNOP
%token LE
%token GE

%union {
    struct list *nlist;
    struct ast_type *type;
    struct ast_stmt *stmt;
    struct ast_subdecl *subdecl;
    struct ast_expr *expr;
    struct ast_path *path;
    enum yytokentype tok;
    char *name;
}

%destructor { free_expr($$); } <expr>
%destructor { free($$); } <name>
%destructor { free_path($$); } <path>
%destructor { free_stmt($$); } <stmt>
%destructor { free_subprogram_decl($$); } <subdecl>
%destructor { free_type($$); } <type>
%destructor { list_free($$); } <nlist>
%destructor { abort(); } <*>

%type <expr> expression
%type <expr> factor
%type <expr> lvalue
%type <expr> simple_expression
%type <expr> term
%type <nlist> arguments
%type <nlist> declarations
%type <nlist> expression_list
%type <nlist> identifier_list
%type <nlist> optional_identifier_list
%type <nlist> type_declarations
%type <nlist> record_fields
%type <nlist> parameter_list
%type <nlist> statement_list
%type <nlist> subprogram_declarations
%type <path> path
%type <stmt> compound_statement
%type <stmt> optional_statements
%type <stmt> procedure_statement
%type <stmt> statement
%type <subdecl> subprogram_declaration
%type <tok> addop
%type <tok> mulop
%type <tok> relop
%type <tok> sign
%type <name> ID NUM
%type <type> standard_type
%type <type> type

%precedence THEN
%precedence ELSE

%pure-parser
%lex-param {void * scanner}
%parse-param {struct ast_program **res}
%parse-param {int options}
%parse-param {void * scanner}

%locations

%start program

%%

program : PROGRAM ID '(' optional_identifier_list ')' ';'
        type_declarations
        declarations
        subprogram_declarations
        compound_statement
        '.'
        { *res = ast_program($2, $4, $8, $7, $9, $10); } ;

type_declarations : type_declarations TYPE ID '=' type ';' { $$ = $1; list_add($$, ast_type_decl($3, $5)); }
                  | %empty                                 { $$ = list_empty(CB free_type_decl); }
                  ;

optional_identifier_list : identifier_list
                         | %empty { $$ = list_empty(free); }
                         ;

identifier_list : ID                       { $$ = list_new($1, free); }
                | identifier_list ',' ID { $$ = $1; list_add($$, $3); }
                ;

declarations : declarations VAR identifier_list ':' type ';' { $$ = $1; list_add($$, ast_decls($3, $5)); }
             | %empty                                        { $$ = list_empty(CB free_decls); }
             ;

type : standard_type
     | ARRAY '[' NUM DOTDOT NUM ']' OF standard_type { $$ = ast_type(TYPE_ARRAY, $3, $5, $8); }
     | '^' type { $$ = ast_type(TYPE_POINTER, $2); }
     | ID         { $$ = ast_type(TYPE_REF, $1); }
     | RECORD record_fields END { $$ = ast_type(TYPE_RECORD, $2); }
     ;

record_fields : record_fields ID ':' type ';' { $$ = $1; list_add($$, ast_record_field($2, $4)); }
              | %empty { $$ = list_empty(CB free_record_field); }
              ;

standard_type : INTEGER { $$ = ast_type(TYPE_INTEGER); }
              | REAL    { $$ = ast_type(TYPE_REAL); }
              | STRING  { $$ = ast_type(TYPE_STRING); }
              | BOOLEAN { $$ = ast_type(TYPE_BOOLEAN); }
              | CHAR    { $$ = ast_type(TYPE_CHAR); }
              ;

subprogram_declarations : subprogram_declarations subprogram_declaration ';' { $$ = $1; list_add($$, $2); }
                        | %empty                                             { $$ = list_empty(CB free_subprogram_decl); }
                        ;

subprogram_declaration : FUNCTION ID arguments ':' standard_type ';'  subprogram_declarations
                       type_declarations declarations compound_statement
                       {
                       $$ = ast_subprogram_decl(ast_type(TYPE_FUNCTION, SUB_FUNCTION, $3, $5), $2, $7, $8, $9, $10);
                       }
                       | PROCEDURE ID arguments ';' subprogram_declarations type_declarations declarations compound_statement
                       {
                       $$ = ast_subprogram_decl(ast_type(TYPE_FUNCTION, SUB_PROCEDURE, $3, NULL), $2, $5, $6, $7, $8);
                       }
                       ;

arguments : '(' parameter_list ')' { $$ = $2; }
          | %empty                 { $$ = list_empty(CB free_decls); }
          ;

parameter_list : identifier_list ':' type                    { $$ = list_new(ast_decls($1, $3), CB free_decls); }
               | parameter_list ';' identifier_list ':' type { $$ = $1; list_add($$, ast_decls($3, $5)); }
               ;

compound_statement : TBEGIN optional_statements END { $$ = $2; }
                   ;

optional_statements : statement_list { $$ = ast_stmt(STMT_STMTS, $1); }
                    | %empty         { $$ = NULL; }
                    ;

statement_list : statement                     { $$ = list_new($1, CB free_stmt); }
               | statement_list ';' statement  { $$ = $1; list_add($$, $3); }
               ;

statement : lvalue ASSIGNOP expression                                { $$ = ast_stmt(STMT_ASSIGN, $1, $3);      }
          | procedure_statement
          | compound_statement                                        { $$ = $1;                                 }
          | IF expression THEN statement                              { $$ = ast_stmt(STMT_ITE, $2, $4, NULL);   }
          | IF expression THEN statement ELSE statement               { $$ = ast_stmt(STMT_ITE, $2, $4, $6);     }
          | WHILE expression DO statement                             { $$ = ast_stmt(STMT_WDO, $2, $4);         }
          | FOR ID ASSIGNOP expression TO expression DO statement     { $$ = ast_stmt(STMT_FOR, $2, $4, $6, $8); }
          ;

path : path '.' ID { $$ = $1; ast_path_append($$, $3); }
     | ID          { $$ = ast_path($1);                }
     ;

lvalue : path                    { $$ = ast_expr(EXPR_PATH,  $1    ); }
       | path '[' expression ']' { $$ = ast_expr(EXPR_IDX,   $1, $3); }
       | path '^'                { $$ = ast_expr(EXPR_DEREF, ast_expr(EXPR_PATH, $1)); }
       ;

procedure_statement : path                         { $$ = ast_stmt(STMT_PROC, $1, list_empty(CB free)); }
                    | path '(' expression_list ')' { $$ = ast_stmt(STMT_PROC, $1, $3);   }
                    ;

expression_list : expression                     { $$ =     list_new($1, CB free_expr);     }
                | expression_list ',' expression { $$ = $1; list_add($$, $3); }
                | %empty                         { $$ = list_empty(CB free_expr); }
                ;

expression : simple_expression
           | simple_expression relop simple_expression { $$ = ast_expr(EXPR_BIN, $1, $2, $3); }
           ;

simple_expression : term
                  | sign term                    { $$ = ast_expr(EXPR_UN, $1, $2); }
                  | simple_expression addop term { $$ = ast_expr(EXPR_BIN, $1, $2, $3); }
                  ;

term : factor
     | term mulop factor { $$ = ast_expr(EXPR_BIN, $1, $2, $3); }
     ;

factor : path '(' expression_list ')' { $$ = ast_expr(EXPR_APP, $1, $3); }
       | NUM                          { $$ = ast_expr(EXPR_LIT, $1);     }
       | '(' expression ')'           { $$ = $2;                         }
       | NOT factor                   { $$ = ast_expr(EXPR_UN, NOT, $2); }
       | lvalue
       | '@' factor                   { $$ = ast_expr(EXPR_ADDROF, $2);  }
       ;

sign : '+' { $$ = '+'; }
     | '-' { $$ = '-'; }
     ;

relop : '=' { $$ = '='; }
      | NEQ { $$ = NEQ; }
      | '<' { $$ = '<'; }
      | '>' { $$ = '>'; }
      | LE  { $$ = LE; }
      | GE  { $$ = GE; }
      ;

addop : '+' { $$ = '+'; }
      | '-' { $$ = '-'; }
      | OR  { $$ = OR; }
      ;

mulop : '*' { $$ = '*'; }
      | '/' { $$ = '/'; }
      | DIV { $$ = DIV; }
      | MOD { $$ = MOD; }
      | AND { $$ = AND; }
      ;
