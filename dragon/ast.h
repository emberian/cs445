#ifndef _AST_H
#define _AST_H

#include "util.h"
#include "parser.tab.h"

typedef enum ast_node_type {
    AST_EXPR_APPLY,
    AST_EXPR_BINOP,
    AST_EXPR_LIT,
    AST_EXPR_NAME,
    AST_EXPR_RELOP,
    AST_EXPR_UNARY,
    AST_FUNCTION,
    AST_NAME,
    AST_PROCEDURE,
    AST_PROGRAM,
    AST_STMT_ASSIGN,
    AST_STMT_IF_ELSE,
    AST_STMT_LIST,
    AST_STMT_PROCEDURE_STMT,
    AST_STMT_WHILE_DO,
    AST_SUBPROGRAM_DECL,
    AST_SUBPROGRAM_HEAD,
    AST_TYPE_ARRAY,
    AST_TYPE_INTEGER,
    AST_TYPE_REAL,
    AST_VARIABLE,
} ast_node_type;

typedef struct ast_node {
    ast_node_type type;
    union {
        struct {
            struct ast_node *prog_name;
            list *prog_args, *prog_decls, *prog_subprogs, *prog_stmts;
        };
        struct {
            struct ast_node *ty_type, *ty_num1, *ty_num2;
        };
        struct {
            enum yytokentype decl_type;
            list *decl_names;
        };
        struct {
            struct ast_node *ass_lvalue, *ass_rvalue;
        };
        struct {
            struct ast_node *if_cond, *if_if, *if_else;
        };
        struct {
            struct ast_node *wdo_expr, *wdo_stmt;
        };
        struct {
            struct ast_node *var_name;
            struct ast_node *var_expr;
        };
        struct {
            struct ast_node *ro_left, *ro_right;
            enum yytokentype ro_op;
        };
        struct {
            struct ast_node *bin_left, *bin_right;
            enum yytokentype bin_op;
        };
        struct {
            struct ast_node *un_expr;
            enum yytokentype un_sign;
        };
        struct {
            list *app_args;
            struct ast_node *app_name;
        };
        struct {
            struct ast_node *lit_val;
        };
        struct {
            ast_node_type head_type;
            struct ast_node *head_name;
            list *head_args;
            ast_node_type head_ret_ty;
        };
        struct {
            list *decl_decls;
            list *decl_body;
        };
        struct {
            const char *id_name;
        };
        struct {
            struct ast_node *procs_name;
            list *procs_args;
        };
        struct {
            list *stmts;
        };
        struct {
            struct ast_node *id_name_node;
        };
    };
} ast_node;

typedef struct ast_decls {
    ast_node *type;
    list *names;
} ast_decls;

ast_node *ast_subprogram_head_new(ast_node_type, ast_node*, list *, ast_node *);
ast_node *ast_subprogram_decl_new(ast_node *, list *, list *);
ast_node *ast_statement_new(ast_node_type);
ast_node *ast_variable_new(ast_node*, ast_node *);
ast_node *ast_new_empty(ast_node_type);
void ast_print(ast_node *);

#endif
