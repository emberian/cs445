#ifndef _CODEGEN_H
#define _CODEGEN_H
#include "anal.h"
#include "ast.h"

char *codegen(struct ast_program *, analysis *);

#endif
