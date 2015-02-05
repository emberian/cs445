#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "ast.h"
#include "symbol.h"

struct stab *analyze(struct ast_program *);

#endif
