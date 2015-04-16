#include "symbol.h"
#include "util.h"
#include <assert.h>
#include <stdarg.h>

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
            func_free(t->cfunc);
            break;

        default:
            break;
    }
    D(t);
}

static struct stab_scope *stab_scope_new() {
    struct stab_scope *sc = M(struct stab_scope);
    sc->vars = hash_new(1 << 8, (HASH_FUNC) hash_string, (COMPARE_FUNC) streq,
            (FREE_FUNC) dummy_free, (FREE_FUNC) dummy_free);
    sc->funcs = hash_new(1 << 8, (HASH_FUNC) hash_string, (COMPARE_FUNC) streq,
            (FREE_FUNC) dummy_free, (FREE_FUNC) dummy_free);
    sc->types = hash_new(1 << 8, (HASH_FUNC) hash_string, (COMPARE_FUNC) streq,
            (FREE_FUNC) dummy_free, (FREE_FUNC) dummy_free);
    /*
    sc->expr_ty_cache = hash_new(1 << 8, (HASH_FUNC) hash_expr,
            (COMPARE_FUNC) ptreq, (FREE_FUNC) dummy_free,
            (FREE_FUNC) dummy_free);
    sc->path_ty_cache = hash_new(1 << 8, (HASH_FUNC) hash_path,
            (COMPARE_FUNC) patheq, (FREE_FUNC) dummy_free,
            (FREE_FUNC) dummy_free);
    */

    return sc;
}

struct stab *stab_new() {
    struct stab *s = M(struct stab);
    // the chain doesn't own the scopes, st->scopes does.
    s->chain = list_empty(CB dummy_free);
    s->vars = ptrvec_wcap(1 << 8, CB free_stab_var);
    s->types = ptrvec_wcap(1 << 8, CB free_stab_type);
    s->scopes = ptrvec_wcap(1 << 8, CB free_stab_scope);

    // int, real, str, bool, char, void
    struct stab_resolved_type rt;
    struct stab_type *t;

    rt.tag = TYPE_INTEGER;
    t = M(struct stab_type);
    // XHAZARD
    t->name = strdup("integer"); t->defn = NULL; t->size = 8; t->align = 8; t->ty = rt;
    ptrvec_push(s->types, t);

    rt.tag = TYPE_REAL;
    t = M(struct stab_type);
    // XHAZARD
    t->name = strdup("real"); t->defn = NULL; t->size = 8; t->align = 8; t->ty = rt;
    ptrvec_push(s->types, t);

    rt.tag = TYPE_STRING;
    t = M(struct stab_type);
    // XHAZARD
    t->name = strdup("string"); t->defn = NULL; t->size = ABI_POINTER_SIZE; t->align =ABI_POINTER_ALIGN; t->ty = rt;
    ptrvec_push(s->types, t);

    rt.tag = TYPE_BOOLEAN;
    t = M(struct stab_type);
    // XHAZARD
    t->name = strdup("boolean"); t->defn = NULL; t->size = 1; t->align = 1; t->ty = rt;
    ptrvec_push(s->types, t);

    rt.tag = TYPE_CHAR;
    t = M(struct stab_type);
    // XHAZARD
    t->name = strdup("char"); t->defn = NULL; t->size = 1; t->align = 1; t->ty = rt;
    ptrvec_push(s->types, t);

    rt.tag = TYPE_VOID;
    t = M(struct stab_type);
    // XHAZARD
    t->name = strdup("<void>"); t->defn = NULL; t->size = 4; t->align = 4; t->ty = rt;
    ptrvec_push(s->types, t);

    return s;
}

void stab_free(struct stab *st) {
    list_free(st->chain);
    ptrvec_free(st->scopes);
    ptrvec_free(st->vars);
    ptrvec_free(st->types);
    D(st);
}

void stab_enter(struct stab *st) {
    // create empty stab_scope, push onto the chain
    struct stab_scope *sc = stab_scope_new();
    list_add(st->chain, sc);
    ptrvec_push(st->scopes, YOLO sc);
    return;
}

void stab_leave(struct stab *st) {
    list_pop(st->chain);
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

size_t stab_resolve_func(struct stab *st, char *name) {
    LFOREACHREV(struct stab_scope *sc, st->chain)
        size_t id = (size_t) hash_lookup(sc->funcs, YOLO name);
        if (id == -1) {
            continue;
        } else {
            return id;
        }
    ENDLFOREACHREV;

    // didn't find anything
    return RESOLVE_FAILURE;
}

size_t stab_resolve_var(struct stab *st, char *name) {
    LFOREACHREV(struct stab_scope *sc, st->chain)
        size_t id = (size_t) hash_lookup(sc->vars, YOLO name);
        if (id == -1) {
            continue;
        } else {
            return id;
        }
    ENDLFOREACHREV;

    // didn't find anything
    return RESOLVE_FAILURE;
}

size_t stab_add_var(struct stab *st, char *name, size_t type, YYLTYPE *span, int nestdepth, bool add_to_locals) {
    struct stab_scope *sc = list_last(st->chain);
    struct stab_var *v = M(struct stab_var);
    v->type = type;
    v->defn = span;
    v->name = name;
    v->offset_into_stack_frame = sc->stack_frame_length;
    v->nestdepth = nestdepth;
    sc->stack_frame_length += STAB_TYPE(st, type)->size;

    size_t id = ptrvec_push(st->vars, YOLO v);
    v->loc = insn_new(IALLOC, STAB_TYPE(st, type)->size);
    hash_insert(sc->vars, YOLO name, YOLO id);

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
    t->magic = 0;

    switch (ty->tag) {
        case TYPE_POINTER:
            t->ty.pointer = stab_resolve_type(st, strdup(name), ty->pointer);
            t->size = ABI_POINTER_SIZE; // XHAZARD
            t->align = ABI_POINTER_ALIGN; // XHAZARD
            break;

        case TYPE_RECORD:
            t->ty.record.fields = list_empty(CB free_stab_record_field);

            LFOREACH(struct ast_record_field *field, ty->record)
                // todo: check that field name is unique
                list_add(t->ty.record.fields, YOLO stab_record_field(field->name, stab_resolve_type(st, strdup(field->name), field->type)));
            ENDLFOREACH;

            struct rec_layout *layout = compute_rec_layout(st, t->ty.record.fields);
            t->size = layout->overall.size;
            t->align = layout->overall.align;
            t->ty.record.layout = layout;

            break;

        case TYPE_ARRAY:
            t->ty.array.lower = atoi(ty->array.lower);
            t->ty.array.upper = atoi(ty->array.upper);
            t->ty.array.elt_type = stab_resolve_type(st, strdup("<array elts>"), ty->array.elt_type);
            t->size = STAB_TYPE(st, t->ty.array.elt_type)->size * (t->ty.array.upper - t->ty.array.lower);
            break;

        case TYPE_FUNCTION:
            t->ty.func.type = ty->func.type;
            t->ty.func.retty = stab_resolve_type(st, strdup("<func ret>"), ty->func.retty);
            t->ty.func.args = list_empty(CB dummy_free);
            t->ty.func.ret_assigned = false;
            t->magic = 0;

            LFOREACH(struct ast_decls *decl, ty->func.args)
                LFOREACH(char *name, decl->names)
                    size_t id = stab_resolve_type(st, strdup(name), decl->type);
                    list_add(t->ty.func.args, YOLO stab_add_var(st, strdup(name), id, NULL, 0, false));
                ENDLFOREACH;
            ENDLFOREACH;

            t->cfunc = cfunc_new(t->ty.func.args);

            t->size = ABI_CLOSURE_SIZE; // XHAZARD
            t->align = ABI_CLOSURE_ALIGN; // XHAZARD
            break;

        default:
            DIAG("resolve_complex_type given simple type!\n");
            abort();
            return -1;
    }

    return ptrvec_push(st->types, YOLO t);
}

size_t stab_resolve_type(struct stab *st, char *name, struct ast_type *ty) {
    if (ty == NULL) {
        free(name);
        return VOID_TYPE_IDX;
    }

    switch (ty->tag) {
        case TYPE_REF:
            free(name);
            return stab_resolve_type_name(st, ty->ref);
        case TYPE_INTEGER:
            free(name);
            return INTEGER_TYPE_IDX;
        case TYPE_REAL:
            free(name);
            return REAL_TYPE_IDX;
        case TYPE_STRING:
            free(name);
            return STRING_TYPE_IDX;
        case TYPE_BOOLEAN:
            free(name);
            return BOOLEAN_TYPE_IDX;
        case TYPE_CHAR:
            free(name);
            return CHAR_TYPE_IDX;
        case TYPE_VOID:
            free(name);
            return VOID_TYPE_IDX;

        case TYPE_POINTER:
        case TYPE_RECORD:
        case TYPE_ARRAY:
        case TYPE_FUNCTION:
            return stab_resolve_complex_type(st, name, ty);

        default:
            DIAG("bad type to resolve!\n");
            abort();
    }
}

void stab_add_decls(struct stab *st, struct ast_decls *decls, int nestdepth, bool arguments) {
    // unconditionally add these to the local scope if they're not defined
    // locally. shadow upper names.
    size_t type = stab_resolve_type(st, strdup("<decls>"), decls->type);
    LFOREACH(char *var, decls->names)
        if (stab_has_local_var(st, (char *)var)) {
            span_err("%s is already defined", NULL, var);
        } else {
            stab_add_var(st, strdup(var), type, NULL, nestdepth, !arguments);
        }
    ENDLFOREACH;
    //? stab_abort(st);
    return;
}

void stab_add_func(struct stab *st, char *name, struct ast_type *sig) {
    assert(sig->tag == TYPE_FUNCTION);
    if (stab_has_local_func(st, name)) {
        span_err("%s is already defined", NULL, name);
    } else {
        size_t type = stab_resolve_complex_type(st, name, sig);
        hash_insert(((struct stab_scope *)list_last(st->chain))->funcs, YOLO name, YOLO type);
    }
    //? stab_abort(st);
    return;
}

void stab_add_magic_func(struct stab *st, int which) {
    char *name;
    switch (which) {
        case MAGIC_READLN: name = strdup("readln"); break;
        case MAGIC_READ: name = strdup("read"); break;
        case MAGIC_WRITELN: name = strdup("writeln"); break;
        case MAGIC_WRITE: name = strdup("write"); break;
        default: abort(); break;
    }
    struct stab_type *t = M(struct stab_type);
    t->defn = NULL;
    t->name = name;
    t->ty.tag = TYPE_FUNCTION;
    t->magic = which;
    t->cfunc = cfunc_new(NULL);
    t->ty.func.args = NULL;

    size_t type = ptrvec_push(st->types, t);
    hash_insert(((struct stab_scope *)list_last(st->chain))->funcs, YOLO name, YOLO type);

    return;
}

void stab_add_type(struct stab *st, char *name, struct ast_type *ty) {
    if (stab_has_local_type(st, name)) {
        span_err("%s is already defined", NULL, name);
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

bool stab_types_eq(struct stab *st, size_t a, size_t b) {
    // todo: when cache in place, just check a == b
    struct stab_type *at = STAB_TYPE(st, a), *bt = STAB_TYPE(st, b);
    if (at->ty.tag != bt->ty.tag) {
        return false;
    } else {
        switch (at->ty.tag) {
            case TYPE_BOOLEAN:
            case TYPE_CHAR:
            case TYPE_INTEGER:
            case TYPE_REAL:
            case TYPE_STRING:
                return true;
            case TYPE_ARRAY:
                return (at->ty.array.upper == bt->ty.array.upper
                    &&  at->ty.array.lower == bt->ty.array.lower
                    &&  at->ty.array.elt_type == bt->ty.array.elt_type);
            case TYPE_FUNCTION:
            case TYPE_RECORD:
                return a == b;
            case TYPE_POINTER:
                return stab_types_eq(st, at->ty.pointer, bt->ty.pointer);

            case TYPE_REF:
            default:
                DIAG("bad type for comparison!\n");
                abort();
        }
    }
}

void stab_print_type(struct stab *st, size_t t, int indent) {
    struct stab_type *ty = STAB_TYPE(st, t);
    INDENT; DEBUG("<id %ld> ", t);
    INDENT;
    switch (ty->ty.tag) {
        case TYPE_BOOLEAN:
            DIAG("boolean\n");
            break;
        case TYPE_CHAR:
            DIAG("char\n");
            break;
        case TYPE_INTEGER:
            DIAG("integer\n");
            break;
        case TYPE_REAL:
            DIAG("real\n");
            break;
        case TYPE_STRING:
            DIAG("string\n");
            break;
        case TYPE_ARRAY:
            DIAG("array `%s`\n", ty->name);
            break;
        case TYPE_FUNCTION:
            DIAG("%s %s\n", ty->ty.func.type == SUB_PROCEDURE ? "procedure" : "function", ty->name);
            break;
        case TYPE_RECORD:
            DIAG("record `%s`\n", ty->name);
            break;
        case TYPE_POINTER:
            DIAG("pointer to `%s`\n", ty->name);
            break;
        case TYPE_VOID:
            DIAG("void\n");
            break;
        case TYPE_REF:
        default:
            abort();
    }
}
