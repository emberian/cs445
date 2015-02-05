#include "util.h"
#include "symbol.h"
#include "translate.h"
#include "ast.h"

static struct rec_layout only_rec = { 8, 8 };

void free_ir(struct ir *i) { }

struct ir *translate(struct ast_program *prog, struct stab *st) {
    // we walk the AST. as we do so, we do name resolution and type checking.
    // menwhile, we are emiting an IR
    return NULL;
}

void free_rec_layout(struct rec_layout *r) { }

struct rec_layout *compute_rec_layout(struct stab *st, struct list *fields) { return &only_rec; }
