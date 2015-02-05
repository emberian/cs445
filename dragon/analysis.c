#include "ast.h"
#include "symbol.h"
#include "util.h"

struct acx {
    int cur_id;
    struct stab *st;
};

void do_imports(struct acx *acx, struct ast_program *prog) {
    LFOREACH(char *import, prog->args)
        if (strcmp(import, "input")) {

        } else if (strcmp(import, "output")) {

        } else {
            span_err("no such library: `%s`", NULL, import);
        }
    ENDLFOREACH;
}

void do_globals(struct acx *acx, struct ast_program *prog) {

}

struct stab *analyze(struct ast_program *prog) {
    struct acx acx;
    acx.cur_id = 0;
    acx.st = stab_new();
    stab_enter(acx.st, acx.cur_id++);
    // setup the global scope: import any names from libraries, define
    // globals, and (iteratively) introduce subprogs after analyzing their
    // bodies.
    do_imports(&acx, prog);
    do_globals(&acx, prog);
    return NULL;
}
