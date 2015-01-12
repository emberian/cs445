%{
#define YYERROR_VERBOSE
#include "ast.h"
#include "symbol.h"
#include "util.h"
#include "lexer.h"

extern int yyerror(void *scanner, struct ast_node **, const char *s);

%}

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
    struct astlist *nlist;
    enum yytokentype token_type;
    struct ast_node *node;
}


%type <nlist> arguments
%type <nlist> declarations
%type <nlist> expression_list
%type <nlist> identifier_list
%type <nlist> optional_statements
%type <nlist> parameter_list
%type <nlist> statement_list
%type <nlist> subprogram_declarations
%type <nlist> compound_statement
%type <nlist> optional_identifier_list
%type <node> expression
%type <node> factor
%type <node> id
%type <node> num
%type <node> procedure_statement
%type <node> program
%type <node> simple_expression
%type <node> standard_type
%type <node> statement
%type <node> subprogram_declaration
%type <node> subprogram_head
%type <node> term
%type <node> type
%type <node> variable
%type <token_type> addop
%type <token_type> mulop
%type <token_type> relop
%type <token_type> sign

%pure-parser
%lex-param {void * scanner}
%parse-param {struct ast_node **res}
%parse-param {void * scanner}

%start program

%%

program : PROGRAM id LPAREN optional_identifier_list RPAREN SEMI
        declarations
        subprogram_declarations
        compound_statement
        DOT
        {
        $$ = M(struct ast_node);
        $$->type = AST_PROGRAM;
        $$->prog_name = $2;
        $$->prog_args = $4;
        $$->prog_decls = $7;
        $$->prog_subprogs = $8;
        $$->prog_stmts = $9;
        *res = $$;
        }
        ;

optional_identifier_list : identifier_list
                         | { $$ = list_empty(); }
                         ;

identifier_list : id { $$ = list_new($1); }
                | identifier_list COMMA id
                {
                $$ = $1;
                list_add($$, $3);
                }
                ;

declarations : declarations VAR identifier_list COLON type SEMI
             {
             $$ = $1;
             ast_node *t = ast_decls($3, $5);
             list_add($$, t);
             }
             | { $$ = list_empty(); }
             ;

type : standard_type
     | ARRAY LBRACKET num DOTDOT num RBRACKET OF standard_type
     {
     $$ = ast_new_empty(AST_TYPE_ARRAY);
     $$->ty_num1 = $3;
     $$->ty_num2 = $5;
     $$->ty_type = $8;
     }
     ;

standard_type : INTEGER { $$ = ast_new_empty(AST_TYPE_INTEGER); }
              | REAL { $$ = ast_new_empty(AST_TYPE_REAL); }
              ;

subprogram_declarations : subprogram_declarations subprogram_declaration SEMI
                        {
                        $$ = $1;
                        list_add($$, $2);
                        }
                        | { $$ = list_empty(); }
                        ;

subprogram_declaration : subprogram_head declarations compound_statement
                       {
                       $$ = ast_subprogram_decl_new($1, $2, $3);
                       }
                       ;

subprogram_head : FUNCTION id arguments COLON standard_type SEMI
                {
                $$ = ast_subprogram_head_new(ast_new_empty(AST_FUNCTION), $2, $3, $5);
                }
                | PROCEDURE id arguments
                {
                $$ = ast_subprogram_head_new(ast_new_empty(AST_PROCEDURE), $2, $3, NULL);
                }
                ;

arguments : LPAREN parameter_list RPAREN { $$ = $2; }
          | { $$ = list_empty(); }
          ;

parameter_list : identifier_list COLON type
               {
               ast_node *t = ast_decls($1, $3);
               $$ = list_new(t);
               }
               | parameter_list SEMI identifier_list COLON type
               {
               $$ = $1;
               ast_node *t = ast_decls($3, $5);
               list_add($$, t);
               }
               ;

compound_statement : TBEGIN optional_statements END { $$ = $2; }
                   ;

optional_statements : statement_list
                    {
                    $$ = $1;
                    }
                    | { $$ = list_empty(); }
                    ;

statement_list : statement { $$ = list_new($1); }
               | statement_list SEMI statement
               {
               $$ = $1;
               list_add($$, $3);
               }
               ;

statement : variable ASSIGNOP expression
          {
          $$ = ast_new_empty(AST_STMT_ASSIGN);
          $$->ass_lvalue = $1;
          $$->ass_rvalue = $3;
          }
          | procedure_statement { $$ = $1; }
          | compound_statement
          {
          $$ = ast_new_empty(AST_STMT_LIST);
          $$->stmts = $1;
          }
          | IF expression THEN statement ELSE statement
          {
          $$ = ast_new_empty(AST_STMT_IF_ELSE);
          $$->if_cond = $2;
          $$->if_if = $4;
          $$->if_else = $6;
          }
          | WHILE expression DO statement
          {
          $$ = ast_new_empty(AST_STMT_WHILE_DO);
          $$->wdo_expr = $2;
          $$->wdo_stmt = $4;
          }
          | FOR id ASSIGNOP expression TO expression DO statement SEMI
          {
          $$ = ast_new_empty(AST_STMT_FOR);
          $$->for_name = $2;
          $$->for_start = $4;
          $$->for_end = $6;
          $$->for_body = $8;
          }
          ;

variable : id { $$ = ast_variable_new($1, NULL); }
         | id LBRACKET expression RBRACKET
         { $$ = ast_variable_new($1, $3); }
         ;

procedure_statement : id
                    {
                    $$ = ast_new_empty(AST_STMT_PROCEDURE_STMT);
                    $$->procs_name = $1;
                    $$->procs_args = NULL;
                    }
                    | id LPAREN expression_list RPAREN
                    {
                    $$ = ast_new_empty(AST_STMT_PROCEDURE_STMT);
                    $$->procs_name = $1;
                    $$->procs_args = $3;
                    }
                    ;

expression_list : expression { $$ = list_new($1); }
                | expression_list COMMA expression
                {
                $$ = $1;
                list_add($$, $3);
                }
                ;

expression : simple_expression
           | simple_expression relop simple_expression
           {
           $$ = ast_new_empty(AST_EXPR_BINOP);
           $$->bin_left = $1;
           $$->bin_op = $2;
           $$->bin_right = $3;
           }
           ;

simple_expression : term
                  | sign term
                  {
                  $$ = ast_new_empty(AST_EXPR_UNARY);
                  $$->un_sign = $1;
                  $$->un_expr = $2;
                  }
                  | simple_expression addop term
                  {
                  $$ = ast_new_empty(AST_EXPR_BINOP);
                  $$->bin_left = $1;
                  $$->bin_op = $2;
                  $$->bin_right = $3;
                  }
                  ;

term : factor
     | term mulop factor
     {
     $$ = ast_new_empty(AST_EXPR_BINOP);
     $$->bin_left = $1;
     $$->bin_op = $2;
     $$->bin_right = $3;
     }
     ;

factor : id { $$ = $1; }
       | id LPAREN expression_list RPAREN
       {
       $$ = ast_new_empty(AST_EXPR_APPLY);
       $$->app_name = $1;
       $$->app_args = $3;
       }
       | num { $$ = $1; }
       | LPAREN expression RPAREN { $$ = $2; }
       | NOT factor
       {
       $$ = ast_new_empty(AST_EXPR_UNARY);
       $$->un_sign = NOT;
       $$->un_expr = $2;
       }
       ;

sign : PLUS { $$ = PLUS; }
     | MINUS { $$ = MINUS; }
     ;

relop : EQ { $$ = EQ; }
      | NEQ { $$ = NEQ; }
      | LT { $$ = LT; }
      | GT { $$ = GT; }
      | LE { $$ = LE; }
      | GE { $$ = GE; }
      ;

addop : PLUS { $$ = PLUS; }
      | MINUS { $$ = MINUS; }
      | OR { $$ = OR; }
      ;

mulop : STAR { $$ = STAR; }
      | SLASH { $$ = SLASH; }
      | DIV { $$ = DIV; }
      | MOD { $$ = MOD; }
      | AND { $$ = AND; }
      ;

id : ID { $$ = yylval.node; }
   ;

num : NUM { $$ = yylval.node; }
    ;
