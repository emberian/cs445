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

/* Super simple, crappy chained hash table. */
struct hash_table {
    // number of buckets
    size_t num_buckets;
    int64_t (*hash)(void *);
    bool (*comp)(void *, void *);
    void (*key_dtor)(void *);
    void (*val_dtor)(void *);
    struct list **buckets;
};

struct hash_table *hash_new(size_t, int64_t (*)(void *), bool (*)(void *, void *), void (*)(void *), void (*)(void *));
void *hash_lookup(struct hash_table *, void *);
void hash_insert(struct hash_table *, void *, void *);
void hash_free(struct hash_table *);

#endif
