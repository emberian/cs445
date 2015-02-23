#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define M(ty) ((ty*)malloc(sizeof(ty)))
#define D(ptr) (free(ptr))
#define YOLO (void*)
#define CB (void (*)(void*))

// HAZARD: ## removing preceeding comma is a gcc ext
#ifndef NDEBUG
#define DEBUG(f, ...) fprintf(stderr, f, ##__VA_ARGS__)
#else
#define DEBUG(f, ...) ;
#endif

#define DIAG(f, ...) fprintf(stderr, f, ##__VA_ARGS__)
#define ERR(f, ...) fprintf(stderr, f, ##__VA_ARGS__); abort();

// number of spaces per indentation level
#define INDSZ 2

#define INDENT INDENTE(indent)
#define INDENTE(indent) do { for (int i = 0; i < indent; i++) putchar(' '); } while(0)

#define LFOREACH(decl, lname) do {\
    struct node *temp = &lname->inner;\
    for (int __ti = 0; __ti < lname->length; __ti++) {\
        decl = temp->elt;\

#define ENDLFOREACH temp = temp->next; } } while (0)

#define LFOREACH2(decl1, decl2, lname1, lname2) do {\
    if (lname1->length == 0 || lname2->length == 0) break;\
    for (struct node *temp1 = &lname1->inner, *temp2 = &lname2->inner; temp1 && temp2;\
            temp1 = temp1->next, temp2 = temp2->next) {\
        decl1 = temp1->elt;\
        decl2 = temp2->elt;\

#define ENDLFOREACH2 } } while(0)

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
    size_t length;
    FREE_FUNC dtor;
};

struct list *list_new(void *, void (*)(void*));
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
void *ptrvec_last(struct ptrvec *);
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

#define HFOREACH(decl, hm) do { \
    int __i; struct list *__bucket; \
    for (__i = 0, __bucket = hm->buckets[0]; __i < hm->num_buckets; __i++, __bucket = hm->buckets[__i]) {\
    LFOREACH(decl, __bucket)

#define ENDHFOREACH ENDLFOREACH; } } while (0)

struct hash_table *hash_new(size_t, HASH_FUNC, COMPARE_FUNC, FREE_FUNC, FREE_FUNC);
void *hash_lookup(struct hash_table *, void *);
void hash_insert(struct hash_table *, void *, void *);
void hash_free(struct hash_table *);
uint64_t hashpjw(char *, size_t);

/* compiler-specific stuff, remove if copying */

struct YYLTYPE;

void span_err(char *fmt, struct YYLTYPE *loc, ...);
void span_diag(char *fmt, struct YYLTYPE *loc, ...);
void dummy_free(void *);

#endif
