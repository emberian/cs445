#ifndef _ANAL_H
#define _ANAL_H
#include "ast.h"

typedef struct analysis {

} analysis;

analysis *anal_new();
void anal_resolve(struct ast_program *, analysis *);
void anal_check(struct ast_program *, analysis *);

#endif
