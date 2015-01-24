#ifndef _DRIVER_H
#define _DRIVER_H

#include <stdlib.h>

#define DUMP_TOKENS (1 << 0)
#define DUMP_AST (1 << 1)
#define NO_PARSE (1 << 2)
#define NO_ANALYSIS (1 << 3)
#define NO_CODEGEN (1 << 4)

char *compile_input(char *, size_t, int);

#endif
