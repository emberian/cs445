#include "util.h"
#include <assert.h>

// in general, the tests in this file ensure that the behavior of the data
// structures in util.c is correct. valgrind/asan/ubsan will catch any memory
// corruption and other incorrectness.

void test_list_empty() {
    struct list *l = list_empty(free);
    list_free(l);
    l = list_new(M(int), free);
    list_free(l);
}

void test_list_append() {
    struct list *l = list_empty(free);
    for (int i = 0; i < 256; i++) {
        int *a = M(int);
        *a = i;
        list_add(l, a);
    }

    int j = 0;
    LFOREACH(int *e, l)
        assert(*e == j++);
    ENDLFOREACH;

    LFOREACHREV(int *e, l)
        assert(*e == --j);
    ENDLFOREACHREV;

    list_free(l);
}

void test_hash_empty() {
    struct hash_table *h = hash_new(256, NULL, NULL, NULL, NULL);
    hash_free(h);
}

bool compare_int(int *a, int *b) {
    return *a == *b;
}

uint64_t hash_int(int *a) {
    return hashpjw((char*) a, sizeof(int));
}

void test_hash_smoke() {
    struct hash_table *h = hash_new(2, (HASH_FUNC) hash_int, (COMPARE_FUNC) compare_int, free, free);

    for (int i = 0; i < 256; i++) {
        int *key = M(int), *val = M(int);
        *key = i;
        *val = i;
        hash_insert(h, key, val);
        assert(*(int*)hash_lookup(h, key) == i);
    }

    HFOREACH(ent, h)
        assert(*(int*)ent->key == *(int*)ent->val);
    ENDHFOREACH;

    hash_free(h);
}

void test_ptrvec_smoke() {
    int *a = M(int), *b = M(int), *c = M(int), *d = M(int);
    *a = 0; *b = 1; *c = 2; *d = 3;

    struct ptrvec *p = ptrvec_wcap(0, free);
    assert(ptrvec_push(p, a) == 0);
    assert(ptrvec_push(p, b) == 1);
    assert(ptrvec_push(p, c) == 2);
    assert(ptrvec_push(p, d) == 3);

    ptrvec_free(p);
}

int main(int argc, char **argv) {
    test_list_empty();
    test_list_append();
    test_hash_empty();
    test_hash_smoke();
}
