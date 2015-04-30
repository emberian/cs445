#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "ast.h"
#include "symbol.h"
#include <stdio.h>

#define NUM_REGS 14

struct register_set {
    int overflow;
    bool regs_used[NUM_REGS];
};

struct acx {
    struct register_set rs;
    struct stab *st;
    int disp_offset;
    FILE *ofd;
    bool toplevel;
    // per-function. should really be split into an fcx.
    enum subprogs current_func_type;
    int ret_assigned;
    char *current_func_name;
    int label;
};

struct acx analyze(struct ast_program *, FILE *);

#endif
