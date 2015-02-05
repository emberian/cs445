#include "symbol.h"
#include "util.h"
#include <assert.h>
#include <stdarg.h>

#define INTEGER_TYPE_IDX 0
#define REAL_TYPE_IDX 1

static void stab_err(struct stab *st, char *fmt, YYLTYPE *loc, ...) {
    va_list args;
    va_start(args, loc);
    vfprintf(stderr, fmt, args);
}

static uint64_t hash_stab_scope(struct stab_scope *sc) {
    return hashpjw((char *)sc, sizeof(sc));
}

static bool ptr_equality(struct stab_scope *s1, struct stab_scope *s2) {
    return s1 == s2;
}

static void dummy_free(void *unused) {
    return;
}

static bool streq(char *a, char *b) {
    return strcmp(a, b) == 0;
}

static int64_t hash_string(char *s) {
    return hashpjw(s, strlen(s));
}

static void free_stab_scope(struct stab_scope *sc) {
    hash_free(sc->vars);
    hash_free(sc->funcs);
    hash_free(sc->types);
    D(sc);
};

static void free_stab_var(struct stab_var *v) {
    D(v->name);
    D(v->defn);
    D(v);
}

static void free_stab_type(struct stab_type *t) {
    D(t->name);
    D(t->defn);
    switch (t->ty.tag) {
        case TYPE_RECORD:
            free_rec_layout(t->ty.record.layout);
            list_free(t->ty.record.fields);
            break;

        case TYPE_FUNCTION:
            list_free(t->ty.func.args);
            break;

        default:
            break;
    }
    D(t);
}

static void (*free_stab_func)(struct stab_var *v) = free_stab_var;

static struct stab_scope *stab_scope_new() {
    struct stab_scope *sc = M(struct stab_scope);
    sc->vars = hash_new(1 << 8, (HASH_FUNC) hash_string, (COMPARE_FUNC) streq,
            (FREE_FUNC) free, (FREE_FUNC) dummy_free);
    sc->funcs = hash_new(1 << 8, (HASH_FUNC) hash_string, (COMPARE_FUNC) streq,
            (FREE_FUNC) free, (FREE_FUNC) dummy_free);
    sc->types = hash_new(1 << 8, (HASH_FUNC) hash_string, (COMPARE_FUNC) streq,
            (FREE_FUNC) free, (FREE_FUNC) dummy_free);

    return sc;
}

struct stab *stab_new() {
    struct stab *s = M(struct stab);
    // the chain doesn't own the scopes, st->scopes does.
    s->chain = list_empty(CB dummy_free);
    s->vars = ptrvec_wcap(1 << 8, CB free_stab_var);
    s->funcs = ptrvec_wcap(1 << 8, CB free_stab_func);
    s->types = ptrvec_wcap(1 << 8, CB free_stab_type);
    s->scopes = hash_new(1 << 8, (HASH_FUNC) hash_stab_scope,
                         (COMPARE_FUNC) ptr_equality,
                         (FREE_FUNC) dummy_free,
                         (FREE_FUNC) free_stab_scope);
    return s;
}

void stab_free(struct stab *st) {
    ptrvec_free(st->vars);
    ptrvec_free(st->funcs);
    ptrvec_free(st->types);
    hash_free(st->scopes);
    list_free(st->chain);
    D(st);
}

void stab_enter(struct stab *st, intptr_t cur_id) {
    // create empty stab_scope, push onto the chain
    struct stab_scope *sc = stab_scope_new();
    list_add(st->chain, sc);
    hash_insert(st->scopes, YOLO cur_id, YOLO sc);
    return;
}

void stab_leave(struct stab *st) {
    free_stab_scope((struct stab_scope *)list_pop(st->chain));
    return;
}

size_t stab_resolve_type_name(struct stab *st, char *name) {
    LFOREACHREV(struct stab_scope *sc, st->chain)
        size_t id = (size_t) hash_lookup(sc->types, YOLO name);
        if (id == -1) {
            continue;
        } else {
            return id;
        }
    ENDLFOREACHREV;

    // didn't find anything
    return RESOLVE_FAILURE;
}

static size_t stab_add_var(struct stab *st, char *name, size_t type, YYLTYPE *span) {
    struct stab_var *v = M(struct stab_var);
    v->type = type;
    v->address_taken = false;
    v->defn = span;
    v->name = name;

    size_t id = ptrvec_push(st->vars, YOLO v);
    hash_insert(((struct stab_scope*)list_last(st->chain))->vars, YOLO name, YOLO id);

    return id;
}

static struct stab_record_field *stab_record_field(char *name, size_t type) {
    struct stab_record_field *f = M(struct stab_record_field);
    f->name = strdup(name);
    f->type = type;
    return f;
}

static void free_stab_record_field(struct stab_record_field *f) {
    D(f->name);
    D(f);
}

static size_t stab_resolve_complex_type(struct stab *st, char *name, struct ast_type *ty) {
    struct stab_type *t = M(struct stab_type);
    t->defn = NULL;
    t->name = name;
    t->ty.tag = ty->tag;

    switch (ty->tag) {
        case TYPE_POINTER:
            t->ty.pointer = stab_resolve_type(st, strdup(name), ty->pointer);
            t->size = ABI_POINTER_SIZE; // XHAZARD
            t->align = ABI_POINTER_ALIGN; // XHAZARD
            break;

        case TYPE_RECORD:
            t->ty.record.fields = list_empty(CB free_stab_record_field);

            LFOREACH(struct ast_record_field *field, ty->record)
                list_add(t->ty.record.fields, YOLO stab_record_field(field->name, stab_resolve_type(st, strdup(field->name), field->type)));
            ENDLFOREACH;

            struct rec_layout *layout = compute_rec_layout(st, t->ty.record.fields);
            t->size = layout->size;
            t->align = layout->align;
            t->ty.record.layout = layout;

            break;

        case TYPE_ARRAY:
            t->ty.array.lower = atoi(ty->array.lower);
            t->ty.array.upper = atoi(ty->array.upper);
            t->ty.array.elt_type = stab_resolve_type(st, strdup("<array elts>"), ty->array.elt_type);
            break;

        case TYPE_FUNCTION:
            t->ty.func.type = ty->func.type;
            t->ty.func.retty = stab_resolve_type(st, strdup("<func ret>"), ty->func.retty);

            LFOREACH(struct ast_decls *decl, ty->func.args)
                LFOREACH(char *name, decl->names)
                    size_t id = stab_resolve_type(st, strdup("<func arg>"), decl->type);
                    list_add(t->ty.func.args, YOLO stab_add_var(st, strdup(name), id, NULL));
                ENDLFOREACH;
            ENDLFOREACH;

            t->size = ABI_CLOSURE_SIZE; // XHAZARD
            t->align = ABI_CLOSURE_ALIGN; // XHAZARD
            break;

        default:
            fprintf(stderr, "resolve_complex_type given simple type!\n");
            abort();
            return -1;
    }

    return ptrvec_push(st->types, YOLO t);
}

size_t stab_resolve_type(struct stab *st, char *name, struct ast_type *ty) {
    switch (ty->tag) {
        case TYPE_REF:
            return stab_resolve_type_name(st, ty->ref);

        case TYPE_INTEGER:
            return INTEGER_TYPE_IDX;
        case TYPE_REAL:
            return REAL_TYPE_IDX;

        case TYPE_POINTER:
        case TYPE_RECORD:
        case TYPE_ARRAY:
        case TYPE_FUNCTION:
            return stab_resolve_complex_type(st, name, ty);

        default:
            abort();
            return RESOLVE_FAILURE;
    }
}

void stab_add_decls(struct stab *st, struct ast_decls *decls) {
    // unconditionally add these to the local scope if they're not defined
    // locally. shadow upper names.
    size_t type = stab_resolve_type(st, strdup("<decls>"), decls->type);
    LFOREACH(char *var, decls->names)
        if (stab_has_local_var(st, (char *)var)) {
            stab_err(st, "%s is already defined", NULL, var);
        } else {
            stab_add_var(st, var, type, NULL);
        }
    ENDLFOREACH;
    //? stab_abort(st);
    return;
}

void stab_add_func(struct stab *st, char *name, struct ast_type *sig) {
    assert(sig->tag == TYPE_FUNCTION);
    if (stab_has_local_func(st, name)) {
        stab_err(st, "%s is already defined", NULL, name);
    } else {
        size_t type = stab_resolve_type(st, name, sig);
        hash_insert(((struct stab_scope *)list_last(st->chain))->funcs, YOLO name, YOLO type);
    }
    //? stab_abort(st);
    return;
}

void stab_add_type(struct stab *st, char *name, struct ast_type *ty) {
    if (stab_has_local_type(st, name)) {
        stab_err(st, "%s is already defined", NULL, name);
    } else {
        size_t type = stab_resolve_type(st, name, ty);
        hash_insert(((struct stab_scope *)list_last(st->chain))->types, YOLO name, YOLO type);
    }
    //? stab_abort(st);
    return;
}


bool stab_has_local_type(struct stab *st, char *name) {
    size_t id = (size_t) hash_lookup(((struct stab_scope *)list_last(st->chain))->types, YOLO name);
    return id != RESOLVE_FAILURE;
}

bool stab_has_local_var(struct stab *st, char *name) {
    size_t id = (size_t) hash_lookup(((struct stab_scope *)list_last(st->chain))->vars, YOLO name);
    return id != RESOLVE_FAILURE;
}

bool stab_has_local_func(struct stab *st, char *name) {
    size_t id = (size_t) hash_lookup(((struct stab_scope *)list_last(st->chain))->funcs, YOLO name);
    return id != RESOLVE_FAILURE;
}
