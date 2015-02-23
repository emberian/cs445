#include "util.h"
#include "symbol.h"
#include "translate.h"
#include "ast.h"

/*
static struct rec_layout only_rec = { 8, 8 };
*/

void free_ir(struct cir_prog *i) { }

struct cir_prog *translate(struct ast_program *prog, struct stab *st) {
    /*
    struct cir_prog *p = cir_prog();
    struct cir_func *m = cir_func();

    struct cir_bb *b = cir_bb();

    // bb for the global vars.
    LFOREACH(struct ast_decls *d, prog->decls)
        LFOREACH(char *var, d->names)
            struct stab_var *sv = STAB_VAR(st, stab_resolve_var(st, var));
            struct insn *slot = insn(IALLOC, size_of_type(st, sv));
            sv->where = slot;
            ptrvec_push(b->insns, slot);
        ENDLFOREACH;
    ENDLFOREACH;
    */


    // we walk the AST. as we do so, we do name resolution and type checking.
    // menwhile, we are emiting an IR
    return NULL;
}

void free_rec_layout(struct rec_layout *r) { }

struct rec_layout *compute_rec_layout(struct stab *st, struct list *fields) { return NULL; }
