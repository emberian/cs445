#ifndef _SYMBOL_H
#define _SYMBOL_H
#include "util.h"

extern charvec *NAME_INTERNER;
struct ast_node *intern(const char *);

#endif
