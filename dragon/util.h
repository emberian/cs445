#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define M(ty) ((ty*)malloc(sizeof(ty)))
#define D(ptr) (free(ptr))

#define L(dtor, xs...) list_many(dtor, xs, NULL)
#define LFOREACH(ty, name, lname, exprs...) do {\
    for (struct node *temp = &lname->inner; temp; temp = temp->next) {\
        if (!temp->elt) continue;\
        ty name = temp->elt;\
        exprs;\
    }\
} while(0)


struct node {
    void *elt;
    struct node *prev, *next;
};

struct list {
    struct node inner;
    struct node *last;
    void (*dtor)(void*);
};

struct list *list_new(void *, void (*)(void*));
struct list *list_many(void (*)(void*), ...);
struct list *list_empty(void (*)(void*));
void list_append(struct node *, struct node *);
void list_add(struct list *, void *);
void list_free(struct list *);

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

typedef uint64_t (*HASH_FUNC)(void *);
typedef bool (*COMPARE_FUNC)(void *, void *);
typedef void (*FREE_FUNC)(void *);

/* Super simple, crappy chained hash table. */
struct hash_table {
    // number of buckets
    size_t num_buckets;
    HASH_FUNC hash;
    COMPARE_FUNC comp;
    FREE_FUNC key_dtor, val_dtor;
    struct list **buckets;
};

struct hash_table *hash_new(size_t, HASH_FUNC, COMPARE_FUNC, FREE_FUNC, FREE_FUNC);
void *hash_lookup(struct hash_table *, void *);
void hash_insert(struct hash_table *, void *, void *);
void hash_free(struct hash_table *);
uint64_t hashpjw(char *, size_t);

#endif
