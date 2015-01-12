#include <stdio.h>
#include "token.h"
#include "util.h"
#include "ast.h"

// number of spaces per indentation level
#define INDSZ 2

ast_node *ast_subprogram_head_new(ast_node *t, ast_node *name, list *args, ast_node *retty) {
    ast_node *n = M(ast_node);
    n->type = AST_SUBPROGRAM_HEAD;
    n->head_type = t;
    n->head_name = name;
    n->head_args = args;
    n->head_ret_ty = retty;
    return n;
}

ast_node *ast_subprogram_decl_new(ast_node *head, list *decls, list *body) {
    ast_node *n = M(ast_node);
    n->type = AST_SUBPROGRAM_DECL;
    n->sub_decl_head = head;
    n->sub_decl_decls = decls;
    n->sub_decl_body = body;
    return n;
}

ast_node *ast_new_empty(ast_node_type type) {
    ast_node *n = M(ast_node);
    n->type = type;
    return n;
}

ast_node *ast_variable_new(ast_node *name, ast_node *expr) {
    ast_node *n = M(ast_node);
    n->type = AST_VARIABLE;
    n->var_name = name;
    n->var_expr = expr;
    return n;
}

ast_node *ast_decls(list *names, ast_node *type) {
    ast_node *n = M(ast_node);
    n->type = AST_DECL;
    n->decl_names = names;
    n->decl_type = type;
    return n;
}

static void ast_print_type(ast_node *node) {
    switch (node->type) {
        case AST_TYPE_INTEGER:
            printf("INTEGER");
            break;
        case AST_TYPE_REAL:
            printf("REAL");
            break;
        case AST_TYPE_ARRAY:
            printf("ARRAY[`%s` .. `%s`] OF", node->ty_num1->id_name,
                    node->ty_num2->id_name);
            ast_print_type(node->ty_type);
            break;
        default:
            printf("\nunknown type...\n");
            break;
    }
}

#define FOREACH(name, lname, exprs...) LFOREACH(ast_node*, name, lname, exprs)
#define INDENT do { for (int i = 0; i < indent; i++) putchar(' '); } while(0)
void ast_print(ast_node *node, int indent) {
    char *kind; // I hate not being able to declare in case arms...
    switch (node->type) {
        case AST_EXPR_APPLY:
            INDENT;
            printf("APPLY `%s` TO ARGS:\n", node->app_name->id_name);
            FOREACH(arg, node->app_args, ast_print(arg, indent+INDSZ));
            break;
        case AST_EXPR_BINOP:
            INDENT;
            printf("BINOP ");
            print_token(node->bin_op, NULL);
            indent += INDSZ;
            INDENT;
            printf("LEFT:\n");
            ast_print(node->bin_left, indent+INDSZ);
            INDENT;
            printf("RIGHT:\n");
            ast_print(node->bin_right, indent+INDSZ);
            indent -= INDSZ;
            break;
        case AST_EXPR_LIT:
            INDENT;
            printf("LIT `%s`\n", node->lit_val->id_name);
            break;
        case AST_EXPR_UNARY:
            INDENT;
            printf("UNARY ");
            print_token(node->un_sign, NULL);
            ast_print(node->un_expr, indent+INDSZ);
            break;
        case AST_FUNCTION:
            fprintf(stderr, "error: AST_FUNCTION appeared in ast_print...\n");
            abort();
            break;
        case AST_NAME:
            INDENT;
            printf("`%s`\n", node->id_name);
            break;
        case AST_PROCEDURE:
            fprintf(stderr, "error: AST_PROCEDURE appeared in ast_print...\n");
            abort();
            break;
        case AST_PROGRAM:
            INDENT;
            printf("PROGRAM `%s` WITH ARGS:\n", node->prog_name->id_name);
            FOREACH(arg, node->prog_args, ast_print(arg, indent+INDSZ));
            INDENT;
            puts("AND VARIABLES:");
            FOREACH(var, node->prog_decls, ast_print(var, indent+INDSZ));
            INDENT;
            puts("AND SUBPROGRAMS:");
            FOREACH(sub, node->prog_subprogs, ast_print(sub, indent+INDSZ));
            INDENT;
            puts("DOES:");
            FOREACH(stmt, node->prog_stmts, ast_print(stmt, indent+INDSZ));
            break;
        case AST_STMT_ASSIGN:
            INDENT;
            puts("ASSIGN");
            ast_print(node->ass_lvalue, indent+INDSZ);
            INDENT;
            puts("TO");
            ast_print(node->ass_rvalue, indent+INDSZ);
            break;
        case AST_STMT_IF_ELSE:
            INDENT; puts("IF FOLLOWING IS TRUE:");
            ast_print(node->if_cond, indent+INDSZ);
            INDENT; puts("THEN:");
            ast_print(node->if_if, indent+INDSZ);
            INDENT; puts("OTHERWISE:");
            ast_print(node->if_else, indent+INDSZ);
            break;
        case AST_STMT_LIST:
            FOREACH(stmt, node->stmts, ast_print(stmt, indent));
            break;
        case AST_STMT_PROCEDURE_STMT:
            INDENT;
            if (!node->procs_args) {
                printf("APPLY `%s`\n", node->procs_name->id_name);
            } else {
                printf("APPLY `%s` WITH:\n", node->procs_name->id_name);
                FOREACH(arg, node->procs_args, ast_print(arg, indent+INDSZ));
            }
            break;
        case AST_STMT_WHILE_DO:
            INDENT; puts("WHILE THE FOLLOWING IS TRUE:");
            ast_print(node->wdo_expr, indent+INDSZ);
            INDENT; puts("DO:");
            ast_print(node->wdo_stmt, indent+INDSZ);
            break;
        case AST_SUBPROGRAM_DECL:
            ast_print(node->sub_decl_head, indent);
            INDENT;
            puts("AND VARIABLES");
            FOREACH(decl, node->sub_decl_decls, ast_print(decl, indent+INDSZ));
            INDENT;
            puts("DOES:");
            FOREACH(stmt, node->sub_decl_body, ast_print(stmt, indent+INDSZ));
            break;
        case AST_SUBPROGRAM_HEAD:
            kind = node->head_type->type == AST_FUNCTION ?
                "FUNCTION" : "PROCEDURE";
            INDENT;
            printf("%s `%s` WITH ARGS:\n", kind, node->head_name->id_name);
            FOREACH(arg, node->head_args, ast_print(arg, indent+INDSZ));
            INDENT;
            printf("RETURNING ");
            ast_print_type(node->head_ret_ty);
            puts("");
            break;
        case AST_TYPE_ARRAY:
        case AST_TYPE_INTEGER:
        case AST_TYPE_REAL:
            INDENT;
            ast_print_type(node);
            puts("");
            break;
        case AST_VARIABLE:
            INDENT;
            if (!node->var_expr) {
                printf("VARIABLE `%s`\n", node->var_name->id_name);
            } else {
                printf("VARIABLE `%s` INDEXED BY:", node->var_name->id_name);
                ast_print(node->var_expr, indent+INDSZ);
            }
            break;
        case AST_DECL:
            INDENT;
            printf("DECLARATIONS WITH TYPE ");
            ast_print_type(node->decl_type);
            puts(":");
            FOREACH(dec, node->decl_names, ast_print(dec, indent+INDSZ));
            break;
        default:
            INDENT;
            printf("unrecognized ast node %d...\n", node->type);
            break;
    }
}

void ast_free(ast_node *node) {
    if (!node) return;

    switch (node->type) {
        case AST_EXPR_APPLY:
            list_free(node->app_args);
            ast_free(node->app_name);
            break;
        case AST_EXPR_BINOP:
            ast_free(node->bin_left);
            ast_free(node->bin_right);
            break;
        case AST_EXPR_LIT:
            ast_free(node->lit_val);
            break;
        case AST_EXPR_UNARY:
            ast_free(node->un_expr);
            break;
        case AST_FUNCTION:
            break;
        case AST_NAME:
            free(node->id_name);
            break;
        case AST_PROCEDURE:
            break;
        case AST_PROGRAM:
            ast_free(node->prog_name);
            list_free(node->prog_args);
            list_free(node->prog_decls);
            list_free(node->prog_subprogs);
            list_free(node->prog_stmts);
            break;
        case AST_STMT_ASSIGN:
            ast_free(node->ass_lvalue);
            ast_free(node->ass_rvalue);
            break;
        case AST_STMT_IF_ELSE:
            ast_free(node->if_if);
            ast_free(node->if_else);
            ast_free(node->if_cond);
            break;
        case AST_STMT_LIST:
            list_free(node->stmts);
            break;
        case AST_STMT_PROCEDURE_STMT:
            ast_free(node->procs_name);
            list_free(node->procs_args);
            break;
        case AST_STMT_WHILE_DO:
            ast_free(node->wdo_expr);
            ast_free(node->wdo_stmt);
            break;
        case AST_SUBPROGRAM_DECL:
            ast_free(node->sub_decl_head);
            list_free(node->sub_decl_decls);
            list_free(node->sub_decl_body);
            break;
        case AST_SUBPROGRAM_HEAD:
            ast_free(node->head_type);
            ast_free(node->head_name);
            ast_free(node->head_ret_ty);
            list_free(node->head_args);
            break;
        case AST_TYPE_ARRAY:
            ast_free(node->ty_type);
            ast_free(node->ty_num1);
            ast_free(node->ty_num2);
            break;
        case AST_TYPE_INTEGER:
        case AST_TYPE_REAL:
            break;
        case AST_VARIABLE:
            ast_free(node->var_name);
            ast_free(node->var_expr);
            break;
        case AST_DECL:
            ast_free(node->decl_type);
            list_free(node->decl_names);
            break;
        default:
            printf("unrecognized free of ast node %d...\n", node->type);
            break;
    }

    free(node);
}
