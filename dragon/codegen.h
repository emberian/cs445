#ifndef _CODEGEN_H
#define _CODEGEN_H
#include "translate.h"
#include "analysis.h"

struct ccx {
    struct hash_table *alloc_offsets;
    int retp_offset;
};
void codegen(struct acx *);

#endif
