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
    l->last = &l->inner;
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
    if (!a) return;
    struct node *p = a->prev;
    if (a->elt) dtor(a->elt);
    D(a);
    list_free_backward(p, dtor);
}

static void list_free_forward(struct node *a, void(*dtor)(void*)) {
    if (!a) return;
    struct node *p = a->next;
    if (a->elt) dtor(a->elt);
    D(a);
    list_free_forward(p, dtor);
}

void list_free(struct list *a) {
    if (!a) return;
    list_free_backward(a->inner.prev, a->dtor);
    list_free_forward(a->inner.next, a->dtor);
    a->dtor(a->inner.elt);
    D(a);
}

void list_add(struct list *a, void *elt) {
    struct node *new = M(struct node);
    new->elt = elt;
    new->prev = NULL;
    new->next = NULL;
    list_append(a->last, new);
    a->last = new;
}

void *list_pop(struct list *a) {
    if (!a || !a->last) return NULL;
    void *last_elt = a->last->elt;
    struct node *last = a->last;
    a->last = last->prev;
    a->last->next = NULL;
    D(last);
    return last_elt;
}

void *list_last(struct list *a) {
    if (!a || !a->last) return NULL;
    return a->last->elt;
}

struct ptrvec *ptrvec_wcap(size_t cap, FREE_FUNC dtor) {
    struct ptrvec *temp = calloc(1, sizeof(struct ptrvec));
    if (!temp) abort();
    void *data = calloc(cap, sizeof(void *));
    if (!data) abort();
    temp->length = 0;
    temp->capacity = cap;
    temp->data = data;
    temp->dtor = dtor;
    return temp;
}

static void ptrvec_reserve(struct ptrvec *vec, size_t len) {
    // wasteful. oh well.
    if (len < vec->capacity) {
        return;
    } else {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(void *));
        if (!vec->data) abort();
    }
}

size_t ptrvec_push(struct ptrvec *vec, void *elem) {
    size_t idx = vec->length++;
    ptrvec_reserve(vec, vec->length);
    vec->data[idx] = elem;
    return idx;
}

void ptrvec_free(struct ptrvec *vec) {
    if (!vec) return;
    for (int i = 0; i < vec->length; i++) {
        vec->dtor(vec->data[i]);
    }
    D(vec->data);
    D(vec);
}

struct hash_table *hash_new(size_t num_buckets, HASH_FUNC hash,
        COMPARE_FUNC comp, FREE_FUNC key_dtor, FREE_FUNC val_dtor) {
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
    LFOREACH(struct bucket_entry *ent, bucket)
        if (tab->comp(key, ent->key)) { return ent->val; }
    ENDLFOREACH;
    // sorry boss!
    return (void *)-1;
}

void hash_insert(struct hash_table *tab, void *key, void *val) {
    void **elt = NULL;
    struct list *bucket = tab->buckets[HASH];
    // check if it's already in the table
    LFOREACH(struct bucket_entry *ent, bucket)
        if (tab->comp(key, ent->key)) {
            elt = ent->val;
        }
    ENDLFOREACH;

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
        LFOREACH(struct bucket_entry *ent, tab->buckets[i])
            tab->key_dtor(ent->key);
            tab->val_dtor(ent->val);
        ENDLFOREACH;
        list_free(tab->buckets[i]);
    }
    D(tab->buckets);
    D(tab);
}

uint64_t hashpjw(char *string, size_t size) {
    uint64_t h = 0, g;
    for (int i = 0; i < size; i++) {
        h = (h << 4) + (*string++);
        if ((g = h & 0xf000000000000000)) {
            h ^= g >> 48;
            h ^= g;
        }
    }
    return h;
}
