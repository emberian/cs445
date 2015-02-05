#ifndef _TRANSLATE_H
#define _TRANSLATE_H

#include "ast.h"
#include "symbol.h"

// XHAZARD
#define ABI_POINTER_SIZE 8
#define ABI_POINTER_ALIGN 8

#define ABI_CLOSURE_SIZE (ABI_POINTER_SIZE * 2)
#define ABI_CLOSURE_ALIGN (ABI_POINTER_ALIGN * 2)

struct stab;

struct ir { };
struct rec_layout {
    int64_t size, align;
};

struct ir *translate(struct ast_program *);
void free_ir(struct ir *);

void free_rec_layout(struct rec_layout *);
struct rec_layout *compute_rec_layout(struct stab *, struct list *);

#endif
