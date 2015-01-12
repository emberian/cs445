#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include "symbol.h"
#include "ast.h"
#include "util.h"

charvec *NAME_INTERNER = NULL;

ast_node *intern(const char *str) {
    /*
    if (!NAME_INTERNER) {
        NAME_INTERNER = charvec_new();
    }
    const char *res = NAME_INTERNER->data + NAME_INTERNER->length;
    charvec_push_all(NAME_INTERNER, str, strlen(str) + 1);
    */
    ast_node *n = M(ast_node);
    n->type = AST_NAME;
    n->id_name = strndup(str, strlen(str));
    return n;
}
