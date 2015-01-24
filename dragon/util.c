#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "ast.h"

struct list *list_many(void (*dtor)(void*), ...) {
    va_list args;
    struct list *l = list_empty(dtor);
    void *elt;

    va_start(args, dtor);

    while ((elt = va_arg(args, void*))) {
        list_add(l, elt);
    }

    va_end(args);

    return l;
}

struct list *list_new(void *data, void (*dtor)(void*)) {
    struct list *l = list_empty(dtor);
    l->inner.elt = data;
    return l;
}

struct list *list_empty(void (*dtor)(void*)) {
    struct list *l = M(struct list);
    l->inner.next = NULL;
    l->inner.prev = NULL;
    l->inner.elt = NULL;
    l->dtor = dtor;
    return l;
}

void list_append(struct node *a, struct node *b) {
    assert(a->next == NULL);
    a->next = b;
    assert(b->prev == NULL);
    b->prev = a;
}

static void list_free_backward(struct node *a, void(*dtor)(void*)) {
    struct node *p = a->prev;
    if (!a) return;
    if (a->elt) dtor(a->elt);
    free(a);
    list_free_backward(p, dtor);
}

static void list_free_forward(struct node *a, void(*dtor)(void*)) {
    struct node *p = a->next;
    if (!a) return;
    if (a->elt) dtor(a->elt);
    free(a);
    list_free_forward(p, dtor);
}

void list_free(struct list *a) {
    if (!a) return;
    list_free_backward(a->inner.prev, a->dtor);
    list_free_forward(a->inner.next, a->dtor);
    a->dtor(a->inner.elt);
    free(a);
}

void list_add(struct list *a, void *elt) {
    struct node *new = M(struct node);
    new->elt = elt;
    list_append(&a->inner, new);
}

charvec *charvec_new() {
    charvec *temp = calloc(1, sizeof(charvec));
    if (temp == NULL) abort();
    char *data = calloc(8, 1);
    if (data == NULL) abort();
    temp->length = 0;
    temp->capacity = 8;
    temp->data = data;
    return temp;
}

static void charvec_reserve(charvec *vec, size_t len) {
    if (len > vec->capacity) return;
    if (vec->length < vec->capacity) {
        // pass
    } else {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity);
        if (vec->data == NULL) abort();
    }
}

void charvec_free(charvec *vec) {
    if (vec) {
        free(vec->data);
        free(vec);
    }
}

void charvec_concat(charvec *source, charvec *dest) {
    charvec_reserve(source, source->length + dest->length);
    memcpy(source->data + source->length, dest->data, dest->length);
}

void charvec_push_all(charvec *dest, const char *source, size_t len) {
    charvec_reserve(dest, dest->length + len);
    memcpy(dest->data + dest->length, source, len);
}

charvec *charvec_from_cstr(const char *str) {
    charvec *res = charvec_new();
    charvec_reserve(res, strlen(str));
    memcpy(res->data, str, strlen(str));
    return res;
}

struct hash_table *hash_new(size_t num_buckets, int64_t (*hash)(void *), bool (*comp)(void *, void *),
        void (*key_dtor)(void *), void (*val_dtor)(void *)) {
    struct hash_table *ret = M(struct hash_table);
    ret->num_buckets = num_buckets;
    ret->hash = hash;
    ret->comp = comp;
    ret->key_dtor = key_dtor;
    ret->val_dtor = val_dtor;
    ret->buckets = malloc(sizeof(struct list*) * num_buckets);
    for (int i = 0; i < num_buckets; i++) {
        ret->buckets[i] = list_empty(free);
    }
    return ret;
}

#define HASH tab->hash(key) % tab->num_buckets

struct bucket_entry {
    void *key;
    void *val;
};

void *hash_lookup(struct hash_table *tab, void *key) {
    struct list *bucket = tab->buckets[HASH];
    LFOREACH(struct bucket_entry *, ent, bucket, if (tab->comp(key, ent->key)) { return ent->val; });
    // sorry boss!
    return NULL;
}

void hash_insert(struct hash_table *tab, void *key, void *val) {
    void **elt = NULL;
    struct list *bucket = tab->buckets[HASH];
    // check if it's already in the table
    LFOREACH(struct bucket_entry *, ent, bucket, tab->comp(key, ent->key) ? elt = ent->val : NULL);
    if (elt != NULL) {
        tab->val_dtor(*elt);
        *elt = val;
        return;
    }
    // otherwise, insert a new bucket entry.
    struct bucket_entry *b = M(struct bucket_entry);
    b->key = key;
    b->val = val;
    list_add(bucket, b);
}

void hash_free(struct hash_table *tab) {
    for (int i = 0; i < tab->num_buckets; i++) {
        LFOREACH(struct bucket_entry *, ent, tab->buckets[i],
                tab->key_dtor(ent->key);
                tab->val_dtor(ent->val););
        list_free(tab->buckets[i]);
    }
}
