%{
#define YYERROR_VERBOSE
#include "ast.h"
#include "util.h"
#include "lexer.h"
#define CB (void (*)(void*))

extern int yyerror(YYLTYPE *, struct ast_program **, int options, void *scanner, const char *s);

%}

%code requires {
struct ast_program;
}

%expect 0

%token DIV
%token MOD
%token AND
%token OR
%token NOT
%token FUNCTION
%token PROCEDURE
%token TBEGIN
%token END
%token INTEGER
%token REAL
%token ARRAY
%token CARET
%token AT
%token OF
%token PROGRAM
%token EQ
%token NEQ
%token LT
%token LE
%token GT
%token GE
%token PLUS
%token MINUS
%token STAR
%token SLASH
%token LPAREN
%token RPAREN
%token LBRACKET
%token RBRACKET
%token COMMA
%token DOTDOT
%token DOT
%token ASSIGNOP
%token COLON
%token SEMI
%token ID
%token NUM
%token DO
%token ELSE
%token IF
%token THEN
%token VAR
%token WHILE
%token FOR
%token TO

%union {
    struct list *nlist;
    struct ast_type *type;
    struct ast_stmt *stmt;
    struct ast_subdecl *subdecl;
    struct ast_subhead *head;
    struct ast_expr *expr;
    struct ast_path *path;
    enum yytokentype tok;
    char *name;
}

%destructor { free_expr($$); } <expr>
%destructor { free_subprogram_head($$); } <head>
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
%type <head> subprogram_head
%type <name> id
%type <name> num
%type <nlist> arguments
%type <nlist> declarations
%type <nlist> expression_list
%type <nlist> identifier_list
%type <nlist> optional_identifier_list
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
%type <type> standard_type
%type <type> type

%pure-parser
%lex-param {void * scanner}
%parse-param {struct ast_program **res}
%parse-param {int options}
%parse-param {void * scanner}

%locations

%start program

%%

program : PROGRAM id LPAREN optional_identifier_list RPAREN SEMI
        declarations
        subprogram_declarations
        compound_statement
        DOT
        { *res = ast_program($2, $4, $7, $8, $9); } ;

optional_identifier_list : identifier_list
                         | { $$ = list_empty(free); }
                         ;

identifier_list : id                       { $$ = list_new($1, free); }
                | identifier_list COMMA id { $$ = $1; list_add($$, $3); }
                ;

declarations : declarations VAR identifier_list COLON type SEMI { $$ = $1; list_add($$, ast_decls($3, $5)); }
             | /* empty */                                      { $$ = list_empty(CB free_decls); }
             ;

type : standard_type
     | ARRAY LBRACKET num DOTDOT num RBRACKET OF standard_type { $$ = ast_type(TYPE_ARRAY, $3, $5, $8); }
     | CARET type { $$ = ast_type(TYPE_POINTER, $2); }
     ;

standard_type : INTEGER { $$ = ast_type(TYPE_INTEGER); }
              | REAL    { $$ = ast_type(TYPE_REAL); }
              ;

subprogram_declarations : subprogram_declarations subprogram_declaration SEMI { $$ = $1; list_add($$, $2); }
                        |                                                     { $$ = list_empty(CB free_subprogram_decl); }
                        ;

subprogram_declaration : subprogram_head subprogram_declarations declarations compound_statement
                       {
                       $$ = ast_subprogram_decl($1, $2, $3, $4);
                       }
                       ;

subprogram_head : FUNCTION id arguments COLON standard_type SEMI
                {
                $$ = ast_subprogram_head(SUB_FUNCTION, $2, $3, $5);
                }
                | PROCEDURE id arguments
                {
                $$ = ast_subprogram_head(SUB_PROCEDURE, $2, $3, NULL);
                }
                ;

arguments : LPAREN parameter_list RPAREN { $$ = $2; }
          | /* empty */                  { $$ = list_empty(free); }
          ;

parameter_list : identifier_list COLON type                     { $$ = list_new(ast_decls($1, $3), CB free_decls); }
               | parameter_list SEMI identifier_list COLON type { $$ = $1; list_add($$, ast_decls($3, $5)); }
               ;

compound_statement : TBEGIN optional_statements END { $$ = $2; }
                   ;

optional_statements : statement_list { $$ = ast_stmt(STMT_STMTS, $1); }
                    | { $$ = NULL; }
                    ;

statement_list : statement                     { $$ = list_new($1, CB free_stmt); }
               | statement_list SEMI statement { $$ = $1; list_add($$, $3); }
               ;

statement : lvalue ASSIGNOP expression                                 { $$ = ast_stmt(STMT_ASSIGN, $1, $3);      }
          | procedure_statement
          | compound_statement                                         { $$ = ast_stmt(STMT_STMTS, $1);           }
          | IF expression THEN statement ELSE statement                { $$ = ast_stmt(STMT_ITE, $2, $4, $6);     }
          | WHILE expression DO statement                              { $$ = ast_stmt(STMT_WDO, $2, $4);         }
          | FOR id ASSIGNOP expression TO expression DO statement SEMI { $$ = ast_stmt(STMT_FOR, $2, $4, $6, $8); }
          ;

path : path '.' id { $$ = $1; ast_path_append($$, $3); }
     | id          { $$ = ast_path($1);                }
     ;

lvalue :   path                              { $$ = ast_expr(EXPR_PATH,  $1    ); }
         | path LBRACKET expression RBRACKET { $$ = ast_expr(EXPR_IDX,   $1, $3); }
         | path CARET                        { $$ = ast_expr(EXPR_DEREF, $1    ); }
         ;

procedure_statement : path                               { $$ = ast_stmt(STMT_PROC, $1, NULL); }
                    | path LPAREN expression_list RPAREN { $$ = ast_stmt(STMT_PROC, $1, $3);   }
                    ;

expression_list : expression                       { $$ =     list_new($1, CB free_expr);     }
                | expression_list COMMA expression { $$ = $1; list_add($$, $3); }
                ;

expression : simple_expression
           | simple_expression relop simple_expression { $$ = ast_expr(EXPR_BIN, $1, $2, $3); }
           ;

simple_expression : term
                  | sign term { $$ = ast_expr(EXPR_UN, $1, $2); }
                  | simple_expression addop term { $$ = ast_expr(EXPR_BIN, $1, $2, $3); }
                  ;

term : factor
     | term mulop factor { $$ = ast_expr(EXPR_BIN, $1, $2, $3); }
     ;

factor : path LPAREN expression_list RPAREN { $$ = ast_expr(EXPR_APP, $1, $3); }
       | num                              { $$ = ast_expr(EXPR_LIT, $1);     }
       | LPAREN expression RPAREN         { $$ = $2;                         }
       | NOT factor                       { $$ = ast_expr(EXPR_UN, NOT, $2); }
       | lvalue
       | AT factor                        { $$ = ast_expr(EXPR_ADDROF, $2);  }
       ;

sign : PLUS  { $$ = PLUS; }
     | MINUS { $$ = MINUS; }
     ;

relop : EQ  { $$ = EQ; }
      | NEQ { $$ = NEQ; }
      | LT  { $$ = LT; }
      | GT  { $$ = GT; }
      | LE  { $$ = LE; }
      | GE  { $$ = GE; }
      ;

addop : PLUS  { $$ = PLUS; }
      | MINUS { $$ = MINUS; }
      | OR    { $$ = OR; }
      ;

mulop : STAR  { $$ = STAR; }
      | SLASH { $$ = SLASH; }
      | DIV   { $$ = DIV; }
      | MOD   { $$ = MOD; }
      | AND   { $$ = AND; }
      ;

id : ID   { $$ = yylval.name; } ;

num : NUM { $$ = yylval.name; } ;
