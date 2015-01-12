#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>
#include <stdint.h>

#define M(ty) ((ty*)malloc(sizeof(ty)))
#define D(ptr) (free(ptr))

#define L(xs...) list_many(0, xs, NULL)
#define LFOREACH(ty, name, lname, exprs...) do {\
    for ( list *temp = lname; temp; temp = temp->next) {\
        if (!temp->ptr) continue;\
        ty name = temp->ptr;\
        exprs;\
    }\
} while(0)

typedef struct astlist {
    struct astlist *next;
    struct astlist *prev;
    struct ast_node *ptr;
} list;

list *list_new(void *);
list *list_many(int, ...);
list *list_empty();
void list_append(list *, list *);
void list_add(list *, void *);
void list_free(list *);

typedef struct {
    size_t length;
    size_t capacity;
    char *data;
} charvec;

void charvec_concat(charvec *, charvec *);
void charvec_free(charvec *);
void charvec_push(charvec *, char);
void charvec_push_all(charvec *, const char *, size_t);
charvec *charvec_new();
charvec *charvec_from_cstr(const char *);

#endif
