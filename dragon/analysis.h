#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "ast.h"
#include "symbol.h"

struct acx {
    struct stab *st;
    struct cir_func *main;
    struct stab_type *current_func;
    struct cir_bb *current_bb;
    struct ptrvec *funcs;
    int disp_offset;
};

struct acx analyze(struct ast_program *);

#endif
