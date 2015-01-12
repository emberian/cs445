#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "ast.h"

list *list_many(int n, ...) {
    va_list args;
    list *l = list_empty();
    void *elt;

    va_start(args, n);

    while ((elt = va_arg(args, void*))) {
        list_add(l, elt);
    }

    va_end(args);

    return l;
}

list *list_new(void *data) {
    list *l = list_empty();
    l->ptr = data;
    return l;
}

list *list_empty() {
    list *l = M(list);
    l->next = NULL;
    l->prev = NULL;
    l->ptr = NULL;
    return l;
}

void list_append(list *a, list *b) {
    assert(a->next == NULL);
    a->next = b;
    assert(b->prev == NULL);
    b->prev = a;
}

static void list_free_backward(list *a) {
    if (!a) return;
    if (a->ptr) ast_free(a->ptr);
    list_free_backward(a->prev);
    free(a);
}

static void list_free_forward(list *a) {
    if (!a) return;
    if (a->ptr) ast_free(a->ptr);
    list_free_forward(a->next);
    free(a);
}

void list_free(list *a) {
    if (!a) return;
    list_free_backward(a->prev);
    list_free_forward(a->next);
    ast_free(a->ptr);
    free(a);
}

void list_add(list *a, void *elt) {
    list_append(a, list_new(elt));
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
