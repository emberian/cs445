#include "ast.h"
#include "symbol.h"
#include "util.h"

typedef struct acx {
    struct stab *st;
} acx;

void register_input(acx *acx, struct ast_program *prog) {
    struct ast_type *gl = ast_type(TYPE_FUNCTION, SUB_PROCEDURE, list_empty(CB free_decls), ast_type(TYPE_STRING));
    stab_add_func(acx->st, strdup("getline"), gl);
    free_type(gl);

    stab_add_magic_func(acx->st, MAGIC_SCANLINE);

    struct ast_type *gc = ast_type(TYPE_FUNCTION, SUB_FUNCTION, list_empty(CB free_decls), ast_type(TYPE_CHAR));
    stab_add_func(acx->st, strdup("getchar"), gc);
    free_type(gc);
}

void register_output(acx *acx, struct ast_program *prog) {
    struct list *args;

    args = list_new("line", CB dummy_free);
    struct ast_type *pl = ast_type(TYPE_FUNCTION, SUB_PROCEDURE, list_new(ast_decls(args, ast_type(TYPE_STRING)), CB free_decls), NULL);
    stab_add_func(acx->st, strdup("putline"), pl);
    free_type(pl);

    stab_add_magic_func(acx->st, MAGIC_PUTALL);

    args = list_new("ch", CB dummy_free);
    struct ast_type *pc = ast_type(TYPE_FUNCTION, SUB_PROCEDURE, list_new(ast_decls(args, ast_type(TYPE_CHAR)), CB free_decls), NULL);
    stab_add_func(acx->st, strdup("putchar"), pc);
    free_type(pc);
}

void do_imports(acx *acx, struct ast_program *prog) {
    LFOREACH(char *import, prog->args)
        if (strcmp(import, "input")) {
            register_input(acx, prog);
        } else if (strcmp(import, "output")) {
            register_output(acx, prog);
        } else {
            span_err("no such library: `%s`", NULL, import);
        }
    ENDLFOREACH;
}

// return the type of a path.
size_t type_of_path(acx *acx, struct ast_path *p) {
    // for the first component in the list, check for a variable with that
    // name. if its type is TYPE_RECORD, check its fields for that name. if it
    // isn't a record, error. if it doesn't have a field with that name,
    // error. otherwise, set the type to the record's field type and continue
    // traversing the list.
    struct stab *st = acx->st;
    struct list *c = p->components;
    size_t t;
    struct stab_resolved_type *ty;
    size_t idx = stab_resolve_var(acx->st, c->inner.elt);
    if (!stab_has_local_var(st, c->inner.elt)) {
        STAB_VAR(st, idx)->captured = true;
    }
    CHKRESV(idx, c->inner.elt);
    t = STAB_VAR(st, idx)->type;
    ty = &STAB_TYPE(st, t)->ty;
    bool first = false;

    LFOREACH(char *n, c)
        if (first) { first = false; continue; }
        if (ty->tag != TYPE_RECORD && temp->next) {
            span_err("tried to access field `%s` of non-record type", NULL, n);
        } else if (temp->next) {
            bool foundit = false;
            LFOREACH(struct stab_record_field *f, ty->record.fields)
                if (strcmp(f->name, n) == 0) {
                    idx = f->type;
                    ty = &STAB_TYPE(st, idx)->ty;
                    foundit = true;
                    break;
                }
            ENDLFOREACH;
            if (!foundit) {
                span_err("could not find field `%s` in record", NULL, n);
            }
        }
    ENDLFOREACH;

    return t;
}

size_t analyze_expr(acx *acx, struct ast_expr *e);

void analyze_magic(acx *acx, int which, struct list *args) {
    if (which == MAGIC_PUTALL) {
        // it's fine, putall can take anything.
    } else if (which == MAGIC_SCANLINE) {
        // scanline needs lvalues.
        LFOREACH(struct ast_expr *e, args)
            if (e->tag != EXPR_IDX && e->tag != EXPR_DEREF && e->tag != EXPR_PATH) {
                DIAG("scanline called with argument:\n");
                print_expr(e, INDSZ);
                span_err("but scanline must be called with lvalues", NULL);
            }
        ENDLFOREACH;
    } else {
        DIAG("bad magic %d!\n", which);
        abort();
    }
}

size_t analyze_call(acx *acx, struct ast_path *p, struct list *args, void *what_is_this) {
    size_t pty = stab_resolve_func(acx->st, list_last(p->components));
    CHKRESF(pty, list_last(p->components));
    struct stab_type *pt = STAB_TYPE(acx->st, pty);

    if (pt->magic != 0) {
        analyze_magic(acx, pt->magic, args);
        return VOID_TYPE_IDX;
    }

    if (pt->ty.tag != TYPE_FUNCTION) {
        print_path(p, 0); fflush(stdout); DIAG(" has type ");
        stab_print_type(acx->st, pty, 0);
        ERR("which cannot be called.\n");
    }

    if (args->length != pt->ty.func.args->length) {
        DIAG("%s arguments passed when calling ", args->length < pt->ty.func.args->length ? "not enough" : "too many");
        stab_print_type(acx->st, pty, 0);
        span_err("wanted %ld, given %ld", NULL, pt->ty.func.args->length, args->length);
    }

    int i = 0;
    LFOREACH2(struct ast_expr *e, void *ft, args, pt->ty.func.args)
        size_t et = analyze_expr(acx, e);
        if (!stab_types_eq(acx->st, et, STAB_VAR(acx->st, (size_t) ft)->type)) {
            DIAG("in "); stab_print_type(acx->st, pty, 0);
            span_diag("type of argument %d doesn't match declaration;", NULL, i);
            DIAG("expected:\n");
            INDENTE(INDSZ); stab_print_type(acx->st, STAB_VAR(acx->st, (size_t) ft)->type, INDSZ); DIAG("\n");
            DIAG("found:\n");
            INDENTE(INDSZ); stab_print_type(acx->st, et, INDSZ);
        }
        i++;
    ENDLFOREACH2;

    return pt->ty.func.retty;
}

size_t analyze_expr(acx *acx, struct ast_expr *e) {
    size_t lty, rty, ty, pty, ety;
    struct stab_resolved_type t;
    struct stab_type *n, *pt, *st;
    switch (e->tag) {
        case EXPR_APP:
            return analyze_call(acx, e->apply.name, e->apply.args, NULL);
        case EXPR_BIN:
            lty = analyze_expr(acx, e->binary.left);
            rty = analyze_expr(acx, e->binary.right);
            if (lty != rty) {
                span_diag("left:", NULL);
                print_expr(e->binary.left, INDSZ);
                DIAG("has type: ");
                stab_print_type(acx->st, lty, INDSZ);

                span_diag("right:", NULL);
                print_expr(e->binary.right, INDSZ);
                DIAG("has type: ");
                stab_print_type(acx->st, rty, INDSZ);

                span_err("incompatible types for binary operation", NULL);
            }

            if (is_relop(e->binary.op)) {
                return BOOLEAN_TYPE_IDX;
            } else {
                return lty;
            }
        case EXPR_DEREF:
            ty = type_of_path(acx, e->deref);
            st = STAB_TYPE(acx->st, ty);
            if (st->ty.tag != TYPE_POINTER) {
                span_err("tried to dereference non-pointer", NULL);
            }
            return st->ty.pointer;
        case EXPR_IDX:
            pty = type_of_path(acx, e->idx.path);
            pt = STAB_TYPE(acx->st, pty);
            if (pt->ty.tag != TYPE_ARRAY) {
                span_err("tried to index non-array", NULL);
            }
            ety = analyze_expr(acx, e->idx.expr);
            // struct stab_type *et = STAB_TYPE(acx->st, ety);
            if (ety != INTEGER_TYPE_IDX) {
                span_err("tried to index array with non-integer", NULL);
            }
            return pt->ty.array.elt_type;
        case EXPR_LIT:
            // FIXME
            return INTEGER_TYPE_IDX;
        case EXPR_PATH:
            return type_of_path(acx, e->path);
        case EXPR_UN:
            ety = analyze_expr(acx, e->unary.expr);
            if (e->unary.op != NOT && (ety != INTEGER_TYPE_IDX || ety != REAL_TYPE_IDX)) {
                span_err("tried to apply unary +/- to a non-number", NULL);
            } else if (ety != BOOLEAN_TYPE_IDX) {
                span_err("tried to boolean-NOT a non-boolean", NULL);
            }
            return ety;
        case EXPR_ADDROF:
            ety = analyze_expr(acx, e->addrof);
            t.tag = TYPE_POINTER;
            t.pointer = ety;
            n = M(struct stab_type);
            n->ty = t;
            n->name = strdup(STAB_TYPE(acx->st, ety)->name); //astrcat("@", STAB_TYPE(acx->st, ety)->name);
            n->size = ABI_POINTER_SIZE; // XHAZARD
            n->align = ABI_POINTER_ALIGN; // XHAZARD
            n->defn = NULL;
            return ptrvec_push(acx->st->types, YOLO n); // XXX
        default:
            abort();
    }
}

void analyze_stmt(acx *acx, struct ast_stmt *s) {
    size_t rty, lty, sty, ety, cty;
    switch (s->tag) {
        case STMT_ASSIGN:
            rty = analyze_expr(acx, s->assign.rvalue);
            lty = analyze_expr(acx, s->assign.lvalue);
            if (!stab_types_eq(acx->st, rty, lty)) {
                span_err("cannot assign incompatible type", NULL);
            }
            break;
        case STMT_FOR:
            sty = analyze_expr(acx, s->foor.start);
            ety = analyze_expr(acx, s->foor.end);
            if (sty != INTEGER_TYPE_IDX) {
                span_err("type of start not integer", NULL);
            } else if (ety != INTEGER_TYPE_IDX) {
                span_err("type of end not integer", NULL);
            }
            /* enter scope for the induction variable */
            stab_enter(acx->st);
            stab_add_var(acx->st, strdup(s->foor.id), sty, NULL);
            analyze_stmt(acx, s->foor.body);
            stab_leave(acx->st);
            break;
        case STMT_ITE:
            cty = analyze_expr(acx, s->ite.cond);
            if (cty != BOOLEAN_TYPE_IDX) {
                span_err("type of if condition not boolean", NULL);
            }
            analyze_stmt(acx, s->ite.then);
            analyze_stmt(acx, s->ite.elze);
            break;
        case STMT_PROC:
            analyze_call(acx, s->apply.name, s->apply.args, NULL);
            break;
        case STMT_STMTS:
            LFOREACH(struct ast_stmt *s, s->stmts)
                analyze_stmt(acx, s);
            ENDLFOREACH;
            break;
        case STMT_WDO:
            cty = analyze_expr(acx, s->wdo.cond);
            if (cty != BOOLEAN_TYPE_IDX) {
                span_err("type of while condition not boolean", NULL);
            }
            analyze_stmt(acx, s->wdo.body);
            break;
        default:
            abort();
    }
}

void analyze_subprog(acx *acx, struct ast_subdecl *s) {
    // add a new scope
    stab_enter(acx->st);

    // add the types...
    LFOREACH(struct ast_type_decl *t, s->types)
        stab_add_type(acx->st, t->name, t->type);
    ENDLFOREACH;

    // add formal arguments...
    LFOREACH(struct ast_decls *d, s->head->func.args)
        stab_add_decls(acx->st, d);
    ENDLFOREACH;

    // add the variables...
    LFOREACH(struct ast_decls *d, s->decls)
        stab_add_decls(acx->st, d);
    ENDLFOREACH;

    // add the return slot...
    stab_add_var(acx->st, strdup(s->name), stab_resolve_type(acx->st, strdup("<retslot>"), s->head->func.retty), NULL);

    // analyze each subprogram, taking care that it is in its own scope...
    LFOREACH(struct ast_subdecl *d, s->subprogs)
        stab_add_func(acx->st, strdup(d->name), d->head);
        analyze_subprog(acx, d);
    ENDLFOREACH;

    // now analyze the subprogram body.
    analyze_stmt(acx, s->body);

    // leave the new scope
    stab_leave(acx->st);
}

struct stab *analyze(struct ast_program *prog) {
    acx acx;
    acx.st = stab_new();

    stab_enter(acx.st);

    // setup the global scope: import any names from libraries...
    do_imports(&acx, prog);

    // add the global types...
    LFOREACH(struct ast_type_decl *t, prog->types)
        stab_add_type(acx.st, t->name, t->type);
    ENDLFOREACH;

    // add the global variables...
    LFOREACH(struct ast_decls *d, prog->decls)
        stab_add_decls(acx.st, d);
    ENDLFOREACH;

    // analyze each subprogram, taking care that it is in its own scope...
    // note that these all become globals
    LFOREACH(struct ast_subdecl *d, prog->subprogs)
        stab_add_func(acx.st, strdup(d->name), d->head);
        analyze_subprog(&acx, d);
    ENDLFOREACH;

    // now analyze the program body.
    analyze_stmt(&acx, prog->body);

    // and we're done!
    return acx.st;
}
