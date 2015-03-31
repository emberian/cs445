#include <assert.h>
#include <ctype.h>

#include "ast.h"
#include "symbol.h"
#include "util.h"
#include "analysis.h"
#include "translate.h"

int size_of_type(struct acx *cx, size_t idx) {
    switch (STAB_TYPE(cx->st, idx)->ty.tag) {
        case TYPE_ARRAY:
            return 8;
        case TYPE_BOOLEAN:
            return 1;
        case TYPE_CHAR:
            return 1;
        case TYPE_FUNCTION:
            return 8;
        case TYPE_INTEGER:
            return 8;
        case TYPE_POINTER:
            return 8;
        case TYPE_REAL:
            return 8;
        case TYPE_RECORD:
            return 64;
        case TYPE_REF:
            span_err("What is a TYPE_REF doing in size_of_type?", NULL);
            return 0;
        case TYPE_STRING:
            return 8;
        case TYPE_VOID:
            return 1;
        default:
            fprintf(stderr, "WARN: size_of_type unknown type %d\n", STAB_TYPE(cx->st, idx)->ty.tag);
            return 0;
    }
}

struct resu {
    size_t type;
    struct insn *op;
};

void register_input(struct acx *acx, struct ast_program *prog) {
    struct ast_type *gl = ast_type(TYPE_FUNCTION, SUB_PROCEDURE, list_empty(CB free_decls), ast_type(TYPE_STRING));
    stab_add_func(acx->st, strdup("getline"), gl, true);
    free_type(gl);

    stab_add_magic_func(acx->st, MAGIC_SCANLINE);

    struct ast_type *gc = ast_type(TYPE_FUNCTION, SUB_FUNCTION, list_empty(CB free_decls), ast_type(TYPE_CHAR));
    stab_add_func(acx->st, strdup("getchar"), gc, true);
    free_type(gc);
}

void register_output(struct acx *acx, struct ast_program *prog) {
    struct list *args;

    args = list_new("line", CB dummy_free);
    struct ast_type *pl = ast_type(TYPE_FUNCTION, SUB_PROCEDURE, list_new(ast_decls(args, ast_type(TYPE_STRING)), CB free_decls), NULL);
    stab_add_func(acx->st, strdup("putline"), pl, true);
    free_type(pl);

    stab_add_magic_func(acx->st, MAGIC_PUTALL);

    args = list_new("ch", CB dummy_free);
    struct ast_type *pc = ast_type(TYPE_FUNCTION, SUB_PROCEDURE, list_new(ast_decls(args, ast_type(TYPE_CHAR)), CB free_decls), NULL);
    stab_add_func(acx->st, strdup("putchar"), pc, true);
    free_type(pc);
}

void do_imports(struct acx *acx, struct ast_program *prog) {
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

// return the type of a path, and the instruction computing its address.
struct resu type_of_path(struct acx *acx, struct ast_path *p) {
    // for the first component in the list, check for a variable with that
    // name. if its type is TYPE_RECORD, check its fields for that name. if it
    // isn't a record, error. if it doesn't have a field with that name,
    // error. otherwise, set the type to the record's field type and continue
    // traversing the list.
    //
    // "loc" tracks the address of the most-recently-analyzed field. a load
    // from loc will load the value of that subpath.
    struct stab *st = acx->st;
    struct list *c = p->components;
    struct resu res;
    size_t t;
    struct stab_resolved_type *ty;
    size_t idx = stab_resolve_var(acx->st, c->inner.elt);
    struct insn *loc;
    CHKRESV(idx, c->inner.elt);
    if (!stab_has_local_var(st, c->inner.elt)) {
        STAB_VAR(st, idx)->captured = true;
    }
    t = STAB_VAR(st, idx)->type;
    ty = &STAB_TYPE(st, t)->ty;
    loc = STAB_VAR(st, idx)->loc;
    // skip the first component of the path.
    bool first = true;

    LFOREACH(char *n, c)
        if (first) { first = false; continue; }
        if (ty->tag != TYPE_RECORD && temp->next) {
            span_err("tried to access field `%s` of non-record type", NULL, n);
        } else if (temp->next) {
            bool foundit = false;
            int offset = 0;
            LFOREACH(struct stab_record_field *f, ty->record.fields)
                if (strcmp(f->name, n) == 0) {
                    idx = f->type;
                    ty = &STAB_TYPE(st, idx)->ty;
                    INSN(ADD, loc, ILIT(offset));
                    foundit = true;
                    break;
                } else {
                    offset += size_of_type(acx, f->type);
                }
            ENDLFOREACH;
            if (!foundit) {
                span_err("could not find field `%s` in record", NULL, n);
            }
        }
    ENDLFOREACH;

    res.op = loc;
    res.type = t;
    return res;
}

struct resu analyze_expr(struct acx *, struct ast_expr *e);

void analyze_magic(struct acx *acx, int which, struct list *args) {
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
    } else if (which == MAGIC_BUILTIN) {
        /* don't do anything... yet */
    } else {
        DIAG("bad magic %d!\n", which);
        abort();
    }
}

struct resu analyze_call(struct acx *acx, struct ast_path *p, struct list *args, void *what_is_this) {
    assert(p->components->length == 1);
    size_t pty = stab_resolve_func(acx->st, list_last(p->components));
    CHKRESF(pty, list_last(p->components));
    struct stab_type *pt = STAB_TYPE(acx->st, pty);
    struct resu retv;
    struct ptrvec *irargs = ptrvec_wcap(args->length, free);

    if (pt->magic != 0) {
        analyze_magic(acx, pt->magic, args);
        retv.type = VOID_TYPE_IDX;
        ptrvec_free(irargs);
        return retv;
    }

    if (pt->ty.tag != TYPE_FUNCTION) {
        print_path(p, 0); fflush(stdout); DIAG(" has type ");
        stab_print_type(acx->st, pty, 0);
        ERR("which cannot be called.\n");
    }

    if (args->length != pt->ty.func.args->length) {
        DIAG("%s arguments passed when calling ", args->length < pt->ty.func.args->length ? "not enough" : "too many");
        stab_print_type(acx->st, pty, 0); fflush(stdout);
        span_err("wanted %ld, given %ld", NULL, pt->ty.func.args->length, args->length);
    }

    int i = 0;
    LFOREACH2(struct ast_expr *e, void *ft, args, pt->ty.func.args)
        struct resu *et = M(struct resu);
        *et = analyze_expr(acx, e);
        if (!stab_types_eq(acx->st, et->type, STAB_VAR(acx->st, (size_t) ft)->type)) {
            DIAG("in "); stab_print_type(acx->st, pty, 0); fflush(stdout);
            span_diag("type of argument %d doesn't match declaration;", NULL, i);
            DIAG("expected:\n");
            INDENTE(INDSZ); stab_print_type(acx->st, STAB_VAR(acx->st, (size_t) ft)->type, INDSZ); fflush(stdout); DIAG("\n");
            DIAG("found:\n");
            INDENTE(INDSZ); stab_print_type(acx->st, et->type, INDSZ); fflush(stdout);
        }
        ptrvec_push(irargs, YOLO et);
        i++;
    ENDLFOREACH2;

    retv.type = pt->ty.func.retty;
    retv.op = INSN(CALL, pt->cfunc, irargs);
    return retv;
}

struct resu analyze_expr(struct acx *acx, struct ast_expr *e) {
    struct resu lty, rty, ety, retv, pathty;
    struct stab_resolved_type t;
    struct stab_type *n, *pt, *st;

    switch (e->tag) {
        case EXPR_APP:
            return analyze_call(acx, e->apply.name, e->apply.args, NULL);
        case EXPR_BIN:
            lty = analyze_expr(acx, e->binary.left);
            rty = analyze_expr(acx, e->binary.right);
            if (lty.type != rty.type) {
                span_diag("left:", NULL);
                print_expr(e->binary.left, INDSZ);
                DIAG("has type: ");
                stab_print_type(acx->st, lty.type, INDSZ);

                span_diag("right:", NULL);
                print_expr(e->binary.right, INDSZ);
                DIAG("has type: ");
                stab_print_type(acx->st, rty.type, INDSZ);

                span_err("incompatible types for binary operation", NULL);
            }

            if (is_relop(e->binary.op)) {
                retv.type = BOOLEAN_TYPE_IDX;
            } else {
                retv.type = lty.type;
            }
            switch ((int) e->binary.op) {
                case AND:
                    retv.op = INSN(AND, lty.op, rty.op); break;
                case OR:
                    retv.op = INSN(OR, lty.op, rty.op); break;
                case NOT:
                    retv.op = INSN(NOT, lty.op, rty.op); break;
                case '=':
                    retv.op = INSN(EQ, lty.op, rty.op); break;
                case NEQ:
                    retv.op = INSN(NE, lty.op, rty.op); break;
                case '<':
                    retv.op = INSN(LT, lty.op, rty.op); break;
                case '>':
                    retv.op = INSN(GT, lty.op, rty.op); break;
                case LE:
                    retv.op = INSN(LE, lty.op, rty.op); break;
                case GE:
                    retv.op = INSN(GE, lty.op, rty.op); break;
                case DIV:
                case '/':
                    retv.op = INSN(DIV, lty.op, rty.op); break;
                case MOD:
                    retv.op = INSN(MOD, lty.op, rty.op); break;
                case '+':
                    retv.op = INSN(ADD, lty.op, rty.op); break;
                case '-':
                    retv.op = INSN(SUB, lty.op, rty.op); break;
                case '*':
                    retv.op = INSN(MUL, lty.op, rty.op); break;
                default:
                    span_err("unsupported binary operation token %d! (`%c`)", NULL, e->binary.op, isgraph(e->binary.op) ? e->binary.op : '_');
                    retv.op = NULL;
                    break;
            }
            return retv;
        case EXPR_DEREF:
            pathty = type_of_path(acx, e->deref->path);
            st = STAB_TYPE(acx->st, pathty.type);
            if (st->ty.tag != TYPE_POINTER) {
                span_err("tried to dereference non-pointer", NULL);
            }
            retv.type = st->ty.pointer;
            retv.op = INSN(LD, pathty.op, size_of_type(acx, pathty.type));
            return retv;
        case EXPR_IDX:
            pathty = type_of_path(acx, e->idx.path);
            pt = STAB_TYPE(acx->st, pathty.type);
            if (pt->ty.tag != TYPE_ARRAY) {
                DIAG("tried to index non-array `");
                print_path(e->idx.path, 0); fflush(stdout);
                DIAG("` which has type ");
                stab_print_type(acx->st, pathty.type, 0);
                span_err("", NULL);
            }
            ety = analyze_expr(acx, e->idx.expr);
            // struct stab_type *et = STAB_TYPE(acx->st, ety);
            if (ety.type != INTEGER_TYPE_IDX) {
                span_err("tried to index array with non-integer", NULL);
            }
            retv.type = pt->ty.array.elt_type;
            retv.op = INSN(ADD, pathty.op, INSN(MUL, ety, size_of_type(acx, pt->ty.array.elt_type)));
            return retv;
        case EXPR_LIT:
            retv.type = INTEGER_TYPE_IDX;
            retv.op = INSN(LIT, ILIT(atoi(e->lit)));
            return retv;
        case EXPR_PATH:
            // always an lvalue, compute address
            return type_of_path(acx, e->path);
        case EXPR_UN:
            ety = analyze_expr(acx, e->unary.expr);
            if (e->unary.op != NOT && (ety.type != INTEGER_TYPE_IDX || ety.type != REAL_TYPE_IDX)) {
                span_err("tried to apply unary +/- to a non-number", NULL);
            } else if (ety.type != BOOLEAN_TYPE_IDX) {
                span_err("tried to boolean-NOT a non-boolean", NULL);
            }
            retv.type = ety.type;
            return retv;
        case EXPR_ADDROF:
            ety = analyze_expr(acx, e->addrof);
            t.tag = TYPE_POINTER;
            t.pointer = ety.type;
            n = M(struct stab_type);
            n->ty = t;
            n->name = strdup(STAB_TYPE(acx->st, ety.type)->name); //astrcat("@", STAB_TYPE(struct acx->st, ety)->name);
            n->size = ABI_POINTER_SIZE; // XHAZARD
            n->align = ABI_POINTER_ALIGN; // XHAZARD
            n->defn = NULL;

            retv.type = ptrvec_push(acx->st->types, YOLO n);
            return retv;
        default:
            abort();
    }
}

struct ast_path *check_assignability(struct acx *acx, struct ast_expr *e) {
    // we're in the toplevel program, we're fine.
    if (!acx->current_func) { return NULL; }

    struct ast_path *root;
    struct ast_expr temp;
    switch (e->tag) {
        case EXPR_PATH:
            root = e->path;
            break;
        case EXPR_IDX:
            temp.tag = EXPR_PATH;
            temp.path = e->path;
            root = check_assignability(acx, &temp);
            break;
        case EXPR_DEREF:
            root = check_assignability(acx, e->deref);
            break;
        default:
            DIAG("tried to check_assignability of a bogon\n");
            print_expr(e, 0);
            abort();
    }


    if (acx->current_func->ty.func.type == SUB_FUNCTION) {
        if (!stab_has_local_var(acx->st, e->path->components->inner.elt)) {
            span_err("assigned to non-local in function", NULL);
        }
    }
    if (strcmp(acx->current_func->name, e->path->components->inner.elt) == 0) {
        acx->current_func->ty.func.ret_assigned = true;
    }

    return root;
}

void analyze_stmt(struct acx *acx, struct ast_stmt *s) {
    struct resu lty, rty, sty, ety, cty;
    struct insn *i1, *i2, *i3, *i4, *i5;
    struct cir_bb *saved, *l0, *l1;

    if (!s) return;

    switch (s->tag) {
        case STMT_ASSIGN:
            lty = analyze_expr(acx, s->assign.lvalue);
            check_assignability(acx, s->assign.lvalue);

            rty = analyze_expr(acx, s->assign.rvalue);
            if (!stab_types_eq(acx->st, rty.type, lty.type)) {
                span_err("cannot assign incompatible type", NULL);
            }
            INSN(ST, lty.op, rty.op, size_of_type(acx, rty.type));
            break;

        case STMT_FOR:
            sty = analyze_expr(acx, s->foor.start);
            ety = analyze_expr(acx, s->foor.end);
            if (sty.type != INTEGER_TYPE_IDX) {
                span_err("type of start not integer", NULL);
            } else if (ety.type != INTEGER_TYPE_IDX) {
                span_err("type of end not integer", NULL);
            }
            /* enter scope for the induction variable */
            stab_enter(acx->st);

            //stab_add_var(acx->st, strdup(s->foor.id), sty.type, NULL, true);

            /*
             * FOR A := s TO e DO ... END
             *
             * %1 = ALLOC sizeof(A)
             * ST %1, s
             * BR true, .L0
             *
             * .L0:
             * %2 = LD %1
             * %3 = LT %2, e
             * BR %3 .L1, .L2
             *
             * .L1:
             * <body>
             * %4 = ADD %2, 1
             * BR true, .L0
             *
             * .L2:
             * ...
             */

            i1 = INSN(ALLOC, STAB_TYPE(acx->st, INTEGER_TYPE_IDX)->size);
            INSN(ST, i1, sty.op, size_of_type(acx, INTEGER_TYPE_IDX));
            i2 = INSN(BR, INSN_TRUE, NULL); // patch with l0

            saved = acx->current_bb;
            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);
            l0 = acx->current_bb;

            i3 = INSN(LD, i1, size_of_type(acx, INTEGER_TYPE_IDX));
            i4 = INSN(LT, i3, ety.op);
            INSN(BR, i4, NULL, NULL); // patch with l1, l2

            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);
            l1 = acx->current_bb;

            analyze_stmt(acx, s->foor.body);

            i5 = INSN(ADD, i3, ILIT(1));
            INSN(ST, i1, i5, size_of_type(acx, INTEGER_TYPE_IDX));
            INSN(BR, INSN_TRUE, l0);

            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);

            i2->b = oper_new(OPER_LABEL, l0);
            i4->b = oper_new(OPER_LABEL, l1);
            i4->c = oper_new(OPER_LABEL, acx->current_bb);

            stab_leave(acx->st);

            break;

        case STMT_ITE:
            cty = analyze_expr(acx, s->ite.cond);
            if (cty.type != BOOLEAN_TYPE_IDX) {
                span_err("type of if condition not boolean", NULL);
            }

            /*
             * IF c THEN t ELSE e
             *
             * BR c, .L0, .L1
             *
             * .L0:
             * t
             * BR true, .L2
             *
             * .L1:
             * e
             * BR true, .L2
             *
             * .L2:
             * ...
             */

            i1 = INSN(BR, cty.op, NULL, NULL); // patch with l0, l1

            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);
            l0 = acx->current_bb;
            analyze_stmt(acx, s->ite.then);
            i2 = INSN(BR, INSN_TRUE, NULL); // patch with l2

            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);
            l1 = acx->current_bb;
            analyze_stmt(acx, s->ite.elze); // TODO: Handle this being NULL (I suspect the CFG will be FUBAR)
            i3 = INSN(BR, INSN_TRUE, NULL); // patch with l2

            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);

            i1->b = oper_new(OPER_LABEL, l0);
            i1->c = oper_new(OPER_LABEL, l1);
            i2->b = oper_new(OPER_LABEL, acx->current_bb);
            i3->b = oper_new(OPER_LABEL, acx->current_bb);

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
            if (cty.type != BOOLEAN_TYPE_IDX) {
                span_err("type of while condition not boolean", NULL);
            }
            /*
             * WHILE c DO w END
             *
             * .L0:
             * %1 = c
             * BR %1, .L1, .L2
             *
             * .L1:
             * w
             * BR true, .L0
             *
             * .L2:
             * ...
             */
            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);
            l0 = acx->current_bb;

            cty = analyze_expr(acx, s->wdo.cond);

            i1 = INSN(BR, cty.op, NULL, NULL); // patch with l1, l2

            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);
            l1 = acx->current_bb;
            analyze_stmt(acx, s->wdo.body);
            INSN(BR, INSN_TRUE, oper_new(OPER_LABEL, l0));

            acx->current_bb = cir_bb();
            ptrvec_push(acx->current_func->cfunc->bbs, acx->current_bb);
            i1->b = oper_new(OPER_LABEL, l1);
            i1->c = oper_new(OPER_LABEL, acx->current_bb);

            break;
        default:
            abort();
    }
}

void analyze_subprog(struct acx *acx, struct ast_subdecl *s) {
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
    stab_add_var(acx->st, strdup(s->name), stab_resolve_type(acx->st, strdup("<retslot>"), s->head->func.retty), NULL, true);

    HFOREACH(ent, ((struct stab_scope *)list_last(acx->st->chain))->vars)
        ptrvec_push(acx->current_bb->insns, STAB_VAR(acx->st, (size_t)ent->val)->loc);
    ENDHFOREACH;

    // analyze each subprogram, taking care that it is in its own scope...
    LFOREACH(struct ast_subdecl *d, s->subprogs)
        stab_add_func(acx->st, strdup(d->name), d->head, false);
        analyze_subprog(acx, d);
    ENDLFOREACH;

    struct stab_type *saved = acx->current_func;
    acx->current_func = STAB_FUNC(acx->st, stab_resolve_func(acx->st, s->name));
    acx->current_func->cfunc->args = s->head->func.args;
    acx->current_bb = acx->current_func->cfunc->entry;

    // now analyze the subprogram body.
    analyze_stmt(acx, s->body);
    if (!acx->current_func->ty.func.ret_assigned && acx->current_func->ty.func.type == SUB_FUNCTION) {
        span_err("return value of %s not assigned", NULL, acx->current_func->name);
    }

    acx->current_func = saved;

    // leave the new scope
    stab_leave(acx->st);
}

struct acx analyze(struct ast_program *prog) {
    struct acx acx;
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

    struct stab_type t;
    t.cfunc = cfunc_new(NULL);
    t.name = "~!@__unassignable__@!~";
    acx.current_func = &t;
    acx.current_bb = t.cfunc->entry;

    // analyze each subprogram, taking care that it is in its own scope...
    // note that these all become globals
    LFOREACH(struct ast_subdecl *d, prog->subprogs)
        stab_add_func(acx.st, strdup(d->name), d->head, false);
        analyze_subprog(&acx, d);
    ENDLFOREACH;

    HFOREACH(ent, ((struct stab_scope *)list_last(acx.st->chain))->vars)
        ptrvec_push(acx.current_bb->insns, STAB_VAR(acx.st, (size_t)ent->val)->loc);
    ENDHFOREACH;

    // now analyze the program body.
    analyze_stmt(&acx, prog->body);

    acx.main = t.cfunc;

    // and we're done!
    return acx;
}
