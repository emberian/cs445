#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define M(ty) ((ty*)malloc(sizeof(ty)))
#define D(ptr) (free(ptr))
#define YOLO (void*)
#define CB (void (*)(void*))

#define L(dtor, xs...) list_many(dtor, xs, NULL)
#define LFOREACH(decl, lname) do {\
    for (struct node *temp = &lname->inner; temp; temp = temp->next) {\
        if (!temp->elt) continue;\
        decl = temp->elt;\

#define ENDLFOREACH } } while (0)

#define LFOREACHREV(decl, lname) do {\
    for (struct node *temp = lname->last; temp; temp = temp->prev) {\
        if (!temp->elt) continue;\
        decl = temp->elt;\

#define ENDLFOREACHREV } } while(0)

typedef uint64_t (*HASH_FUNC)(void *);
typedef bool (*COMPARE_FUNC)(void *, void *);
typedef void (*FREE_FUNC)(void *);

struct node {
    void *elt;
    struct node *prev, *next;
};

struct list {
    struct node inner;
    struct node *last;
    FREE_FUNC dtor;
};

struct list *list_new(void *, void (*)(void*));
struct list *list_many(void (*)(void*), ...);
struct list *list_empty(void (*)(void*));
void list_append(struct node *, struct node *);
void list_add(struct list *, void *);
void list_free(struct list *);
void *list_pop(struct list *);
void *list_last(struct list *);

struct ptrvec {
    size_t length;
    size_t capacity;
    void **data;
    FREE_FUNC dtor;
};

struct ptrvec *ptrvec_wcap(size_t, FREE_FUNC);
size_t ptrvec_push(struct ptrvec *, void *);
void ptrvec_free(struct ptrvec *);

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

/* compiler-specific stuff, remove if copying */

struct YYLTYPE;

void span_err(char *fmt, struct YYLTYPE *loc, ...);

#endif
