#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "ast.h"
#include "symbol.h"

struct register_set {
    int overflow;
    // bitmask of used registers. 1 if unused, 0 if used (to make ffs nice)
    unsigned short regs_used;
};

struct acx {
    struct register_set rs;
    struct stab *st;
    int disp_offset;
    int ofd;
    bool toplevel;
    // per-function. should really be split into an fcx.
    enum subprogs current_func_type;
    int ret_assigned;
    char *current_func_name;
    int label;
};

struct acx analyze(struct ast_program *);

#endif
