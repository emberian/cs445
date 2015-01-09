#include <stdio.h>

#include "ast.h"
#include "util.h"

ast_node *ast_subprogram_head_new(ast_node_type t, ast_node *name, list *args, ast_node *retty) {
    ast_node *n = M(ast_node);
    n->type = AST_SUBPROGRAM_HEAD;
    n->head_type = t;
    n->head_name = name;
    n->head_args = args;
    n->head_ret_ty = retty->type;
    return n;
}

ast_node *ast_subprogram_decl_new(ast_node *head, list *decls, list *body) {
    ast_node *n = M(ast_node);
    n->type = AST_SUBPROGRAM_DECL;
    n->decl_decls = decls;
    n->decl_body = body;
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

void ast_print(ast_node *prog) {
    printf("%lld", (long long)prog);
}
