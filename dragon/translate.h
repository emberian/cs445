#ifndef _TRANSLATE_H
#define _TRANSLATE_H

#include "ast.h"

struct ir { };

struct ir *translate(struct ast_program *);
void free_ir(struct ir *);

#endif
