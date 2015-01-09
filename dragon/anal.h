#ifndef _ANAL_H
#define _ANAL_H

typedef struct analysis {
    int x;
} analysis;

analysis *anal_new();
void anal_resolve(ast_node *, analysis *);
void anal_check(ast_node *, analysis *);

#endif
