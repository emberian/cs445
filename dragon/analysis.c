#include <assert.h>
#include <ctype.h>

#include "ast.h"
#include "symbol.h"
#include "util.h"
#include "analysis.h"

// from the bithacks page
static const char LogTable256[256] =
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
#undef LT
};

static int log2i(unsigned v) {
    unsigned r;     // r will be lg(v)
    unsigned int t, tt; // temporaries

    if (tt = v >> 16) {
        r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
    } else {
        r = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
    }

    return r;
}

// yup.

#define NUM_REGS 16

static char *REGS[NUM_REGS] = { "rax", "rbp", "rcx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rdx" };

struct reg {
    char *name;
    unsigned short mask;
    unsigned short need_restore;
};

struct reg reg_gimme(struct acx *acx) {
    struct regset *rs = &acx->rs;
    struct reg ret;
    unsigned reg_to_use = log2i(rs->regs_used)
    if (rs->regs_used == -1) {
        ret.need_restore = rs->overflow++ % NUM_REGS;
        ret.name = REGS[ret.need_restore];
        ret.mask = 0;
    } else {
        int reg_bit = 2 << reg_to_use;
        rs->regs_used &= ~reg_bit;
        ret.need_restore = 0;
        ret.mask = reg_bit;
        ret.name = REGS[reg_to_use];
    }

    return ret;
}

void reg_takeitback(struct acx *acx, struct reg reg) {
    if (reg.need_restore == 0) {
        acx->rs.regs_used |= reg.mask;
    } else {
        acx->rs.overflow--;
        fprintf(acx->ofd, "pop %s\n", reg.name);
    }
}

struct resu {
    size_t type; // what is it
    struct reg reg; // where is it TODO: non-word sized types
};

static int size_of_type(struct acx *cx, size_t idx) {
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

static void register_input(struct acx *acx, struct ast_program *prog) {
    stab_add_magic_func(acx->st, MAGIC_READLN);
    stab_add_magic_func(acx->st, MAGIC_READ);
}

static void register_output(struct acx *acx, struct ast_program *prog) {
    stab_add_magic_func(acx->st, MAGIC_WRITELN);
    stab_add_magic_func(acx->st, MAGIC_WRITE);
}

static void do_imports(struct acx *acx, struct ast_program *prog) {
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

// return the type of a path, and either its address or value.
static struct resu type_of_path(struct acx *acx, struct ast_path *p, bool compute_rvalue) {
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
    CHKRESV(idx, c->inner.elt);
    struct reg reg = reg_gimme(acx);

    if (!stab_has_local_var(st, c->inner.elt)) {
        // load from the display.
        if (!STAB_VAR(st, idx)->captured) {
            STAB_VAR(st, idx)->captured = true;
            STAB_VAR(st, idx)->disp_offset = acx->disp_offset++;
        }
        fprintf(acx->ofd, "mov %s, [display@ + %d]\n", reg.name, STAB_VAR(st, idx)->disp_offset * ABI_POINTER_ALIGN);
    } else {
        fprintf(acx->ofd, "mov %s, rbp+%d\n", reg.name, STAB_VAR(st, idx)->stack_base_offset);
    }
    t = STAB_VAR(st, idx)->type;
    ty = &STAB_TYPE(st, t)->ty;

    // skip the first component of the path (we *just* found it; it deserves
    // special handling since it's always a local. the loop deals with struct
    // offsets).
    bool first = true;

    LFOREACH(char *n, c)
        if (first) { first = false; continue; }
        if (ty->tag != TYPE_RECORD && temp->next) {
            span_err("tried to access field `%s` of non-record type, which can't have fields", NULL, n);
        } else if (temp->next) {
            bool foundit = false;
            int offset = 0;
            LFOREACH(struct stab_record_field *f, ty->record.fields)
                if (strcmp(f->name, n) == 0) {
                    idx = f->type;
                    ty = &STAB_TYPE(st, idx)->ty;
                    if (ty->tag == TYPE_POINTER) {
                        ty = &STAB_TYPE(st, ty->pointer)->ty;
                        fprintf(acx->ofd, "mov %s, [%s]\n", reg.name, reg.name);
                    } else {
                        fprintf(acx->ofd, "add %s, %d\n", reg.name, offset);
                    }
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

    if (compute_rvalue) {
        fprintf(acx->ofd, "mov %s, [%s]\n", res.name, res.name);
    }
    res.reg = reg;
    res.type = t;
    return res;
}

static struct resu analyze_expr(struct acx *, struct ast_expr *e, bool compute_rvalue);

static void analyze_magic(struct acx *acx, int which, struct list *args) {
    if (which == MAGIC_WRITELN || which == MAGIC_WRITE) {
        LFOREACH(struct ast_expr *e, args)
            struct resu *r = M(struct resu);
            *r = analyze_expr(acx, e, false);
            char *callit;
            switch (r->type) {
                case INTEGER_TYPE_IDX:
                    callit = "write_integer@";
                    break;
                case REAL_TYPE_IDX:
                    callit = "write_real@";
                    break;
                case STRING_TYPE_IDX:
                    callit = "write_string@";
                    break;
                case BOOLEAN_TYPE_IDX:
                    callit = "write_bool@";
                    break;
                case CHAR_TYPE_IDX:
                    callit = "write_char@";
                    break;
                case VOID_TYPE_IDX:
                    callit = "write_void@";
                    break;
                default:
                    span_err("argument of unprintable type passed to write/ln", NULL);
                    abort();
                    break;
            }
            INSN(FCALL, strdup(callit), ptrvec_new(free, 1, r));
        ENDLFOREACH;
        // it's fine, putall can take anything.
        if (which == MAGIC_WRITELN) {
            INSN(FCALL, strdup("write_newline@"), NULL);
        }
    } else if (which == MAGIC_READ || which == MAGIC_READLN) {
        // needs lvalues.
        LFOREACH(struct ast_expr *e, args)
            if (e->tag != EXPR_IDX && e->tag != EXPR_DEREF && e->tag != EXPR_PATH) {
                DIAG("read/ln called with argument:\n");
                print_expr(e, INDSZ);
                span_err("but read/ln must be called with lvalues", NULL);
            }
            struct resu *r = M(struct resu);
            *r = analyze_expr(acx, e, false); // this will be either an ALLOC or an address computation.
            char *callit;
            switch (r->type) {
                case INTEGER_TYPE_IDX:
                    callit = "read_integer@";
                    break;
                case REAL_TYPE_IDX:
                    callit = "read_real@";
                    break;
                case STRING_TYPE_IDX:
                    callit = "read_string@";
                    break;
                case BOOLEAN_TYPE_IDX:
                    callit = "read_bool@";
                    break;
                case CHAR_TYPE_IDX:
                    callit = "read_char@";
                    break;
                case VOID_TYPE_IDX:
                    callit = "read_void@";
                    break;
                default:
                    span_err("argument of unprintable type passed to write/ln", NULL);
                    abort();
                    break;
            }
            INSN(FCALL, strdup(callit), ptrvec_new(free, 1, r));
            // the result should be a pointer to a value.
        ENDLFOREACH;
    } else {
        DIAG("bad magic %d!\n", which);
        abort();
    }
}

static struct resu analyze_call(struct acx *acx, struct ast_path *p, struct list *args) {
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
        *et = analyze_expr(acx, e, true);
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

static struct resu analyze_expr(struct acx *acx, struct ast_expr *e, bool compute_rvalue) {
    struct resu lty, rty, ety, retv, pathty;
    struct stab_resolved_type t;
    struct stab_type *n, *pt, *st;

    switch (e->tag) {
        case EXPR_APP:
            return analyze_call(acx, e->apply.name, e->apply.args);
        case EXPR_BIN:
            /* This is the *only* place we need to be concerned about gencode.
             * Everything else needs no extra registers.
             */
            lty = analyze_expr(acx, e->binary.left, true);
            rty = analyze_expr(acx, e->binary.right, true);
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
                    fprintf(acx->ofd, "and %s, %s\n", lty.reg.name, rty.reg.name); break;
                case OR:
                    fprintf(acx->ofd, "or %s, %s\n", lty.reg.name, rty.reg.name); break;
                case '=':
                case NEQ:
                case '<':
                case '>':
                case LE:
                case GE:
                    fprintf(acx->ofd, "cmp %s, %s\n", lty.reg.name, rty.reg.name); break;
                case DIV:
                case '/':
                    fprintf(acx->ofd, "push rdx\npush rax\nmov rax, %s\ncdq\nidiv %s\nmov %s, rax\npop rax\npop rdx\n",
                            lty.reg.name, rty.reg.name, lty.reg.name);
                    break;
                case MOD:
                    fprintf(acx->ofd, "push rdx\npush rax\nmov rax, %s\ncdq\nidiv %s\nmov %s, rdx\npop rax\npop rdx\n",
                            lty.reg.name, rty.reg.name, lty.reg.name);
                    break;
                case '+':
                    fprintf(acx->ofd, "add %s, %s\n", lty.reg.name, rty.reg.name); break;
                case '-':
                    fprintf(acx->ofd, "sub %s, %s\n", lty.reg.name, rty.reg.name); break;
                case '*':
                    fprintf(acx->ofd, "imul %s, %s\n", lty.reg.name, rty.reg.name); break;
                default:
                    span_err("unsupported binary operation token %d! (`%c`)", NULL, e->binary.op, isgraph(e->binary.op) ? e->binary.op : '_');
                    retv.reg = lty;
                    break;
            }
            retv.reg = lty;
            reg_takeitback(acx, rty.reg);
            return retv;
        case EXPR_DEREF:
            pathty = type_of_path(acx, e->deref->path, false);
            st = STAB_TYPE(acx->st, pathty.type);
            if (st->ty.tag != TYPE_POINTER) {
                span_err("tried to dereference non-pointer", NULL);
            }
            retv.type = st->ty.pointer;
            retv.reg = pathty.reg;
            fprintf(acx->ofd, "mov %s, [%s]\n", pathty.reg.name);
            return retv;
        case EXPR_IDX:
            pathty = type_of_path(acx, e->idx.path, false);
            pt = STAB_TYPE(acx->st, pathty.type);
            if (pt->ty.tag != TYPE_ARRAY) {
                DIAG("tried to index non-array `");
                print_path(e->idx.path, 0); fflush(stdout);
                DIAG("` which has type ");
                stab_print_type(acx->st, pathty.type, 0);
                span_err("", NULL);
            }
            ety = analyze_expr(acx, e->idx.expr, true);
            // struct stab_type *et = STAB_TYPE(acx->st, ety);
            if (ety.type != INTEGER_TYPE_IDX) {
                span_err("tried to index array with non-integer", NULL);
            }
            retv.type = pt->ty.array.elt_type;
            retv.reg = ety.reg;
            if (compute_rvalue) {
                fprintf(acx->ofd, "mov %s, [%s]\n", ety.reg.name, ety.reg.name);
            }
            return retv;
        case EXPR_LIT:
            retv.type = INTEGER_TYPE_IDX;
            retv.reg = reg_gimme(acx);
            printf(acx->ofd, "mov %s, %d\n", retv.reg.name, e->lit);
            return retv;
        case EXPR_PATH:
            return type_of_path(acx, e->path, compute_rvalue);
        case EXPR_UN:
            ety = analyze_expr(acx, e->unary.expr, true);
            if (e->unary.op == NOT && (ety.type != INTEGER_TYPE_IDX || ety.type != REAL_TYPE_IDX)) {
                span_err("tried to apply unary +/- to a non-number", NULL);
            } else if (ety.type != BOOLEAN_TYPE_IDX) {
                span_err("tried to boolean-NOT a non-boolean", NULL);
            }
            printf(acx->ofd, "not %s\n", ety.reg.name);
            return ety;
        case EXPR_ADDROF:
            fprintf(stderr, "EXPR_ADDROF not yet supported!\n"); abort();
            // this will require some careful thought about where the temporary can live.
            // alternatively, we could pull a C and only allow addr-of on locals.
            ety = analyze_expr(acx, e->addrof, false);
            t.tag = TYPE_POINTER;
            t.pointer = ety.type;
            n = M(struct stab_type);
            n->ty = t;
            n->name = strdup(STAB_TYPE(acx->st, ety.type)->name); // astrcat("@", STAB_TYPE(struct acx->st, ety)->name);
            n->size = ABI_POINTER_SIZE; // XHAZARD
            n->align = ABI_POINTER_ALIGN; // XHAZARD
            n->defn = NULL;

            retv.type = ptrvec_push(acx->st->types, YOLO n);
            return retv;
        default:
            abort();
    }
}

static struct ast_path *check_assignability(struct acx *acx, struct ast_expr *e) {
    // we're in the toplevel program, we're fine.
    if (acx->toplevel) { return NULL; }

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


    if (acx->current_func_type == SUB_FUNCTION) {
        if (!stab_has_local_var(acx->st, e->path->components->inner.elt)) {
            span_err("assigned to non-local in function", NULL);
        }
    }
    if (strcmp(acx->current_func_name, e->path->components->inner.elt) == 0) {
        acx->ret_assigned = true;
    }

    return root;
}

static void analyze_stmt(struct acx *acx, struct ast_stmt *s) {
    struct resu lty, rty, sty, ety, cty;
    int l0, l1, l2;

    if (!s) return;

    switch (s->tag) {
        case STMT_ASSIGN:
            lty = analyze_expr(acx, s->assign.lvalue, false);
            check_assignability(acx, s->assign.lvalue);

            rty = analyze_expr(acx, s->assign.rvalue, true);
            if (!stab_types_eq(acx->st, rty.type, lty.type)) {
                span_err("cannot assign incompatible type", NULL);
            }
            fprintf(acx->ofd, "mov [%s], %s\n", lty.reg.name, rty.reg.name);
            reg_takeitback(acx, rty.reg);
            reg_takeitback(acx, lty.reg);
            break;

        case STMT_FOR:
            sty = analyze_expr(acx, s->foor.start, true);
            ety = analyze_expr(acx, s->foor.end, true);
            if (sty.type != INTEGER_TYPE_IDX) {
                span_err("type of start not integer", NULL);
            } else if (ety.type != INTEGER_TYPE_IDX) {
                span_err("type of end not integer", NULL);
            }

            /* enter scope for the induction variable */
            stab_enter(acx->st);

            //stab_add_var(acx->st, strdup(s->foor.id), sty.type, NULL, true);

            fprintf(acx->ofd, "push %s\n", sty.reg.name);
            l0 = acx->label++;
            l1 = acx->label++;

            fprintf(acx->ofd, ".L%d:\n", l0);
            i3 = INSN(LD, i1, size_of_type(acx, INTEGER_TYPE_IDX));
            i4 = INSN(LT, i3, ety.op);
            fprintf(acx->ofd, "cmp %s, %s\nje .L%d\n", sty.reg.name, ety.reg.name, l1);

            analyze_stmt(acx, s->foor.body);

            fprintf(acx->ofd, "inc %s\nmov [rsp], %s\njmp .L%d\n.L%d:\n", sty.reg.name, sty.reg.name, l0, l1);
            reg_takeitback(acx, ety.reg);
            reg_takeitback(acx, sty.reg);

            stab_leave(acx->st);

            break;

        case STMT_ITE:
            cty = analyze_expr(acx, s->ite.cond, true);
            if (cty.type != BOOLEAN_TYPE_IDX) {
                span_err("type of if condition not boolean", NULL);
            }
            l0 = acx->label++;

            i1 = INSN(BR, cty.op, NULL, NULL); // patch with l0, l1
            fprintf(acx-ofd, "cmp %s, %s\njz .L%d\n", cty.reg.name, cty.reg.name, l0);
            reg_takeitback(acx, cty.reg);

            analyze_stmt(acx, s->ite.then);

            fprintf(acx->ofd, ".L%d:\n", l0);
            analyze_stmt(acx, s->ite.elze); // TODO: Handle this being NULL (I suspect the CFG will be FUBAR)

            break;

        case STMT_PROC:
            cty = analyze_call(acx, s->apply.name, s->apply.args);
            reg_takeitback(acx, cty.reg);
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

            l0 = acx->label++;
            l1 = acx->label++;

            fprintf(acx->ofd, ".L%d:\n", l0);

            cty = analyze_expr(acx, s->wdo.cond, true);
            fprintf(acx->ofd, "cmp %s, %s\njz .L%d\n", l1);
            reg_takeitback(acx, cty.reg);

            analyze_stmt(acx, s->wdo.body);

            fprintf(acx->ofd, ".L%d:\n", l1);

            break;

        default:
            abort();
    }
}

static void analyze_subprog(struct acx *acx, struct ast_subdecl *s) {
    char *old_func_name = acx->current_func_name;
    int old_label = acx->label;
    bool old_ret_assigned = acx->ret_assigned;
    enum subprogs old_cft = acx->current_func_type;

    acx->current_func_name = s->name;
    acx->ret_assigned = false;
    acx->current_func_type = s->head->func.type;
    acx->label = 0;

    int curr_var_offset = ABI_POINTER_SIZE; // skip over the return pointer

    // add a new scope
    stab_enter(acx->st);

    // add the types...
    LFOREACH(struct ast_type_decl *t, s->types)
        stab_add_type(acx->st, t->name, t->type);
    ENDLFOREACH;

    // add formal arguments...
    LFOREACH(struct ast_decls *d, s->head->func.args)
        stab_add_decls(acx->st, d, false);
    ENDLFOREACH;

    // add the variables...
    LFOREACH(struct ast_decls *d, s->decls)
        stab_add_decls(acx->st, d, &curr_var_offset, true);
    ENDLFOREACH;

    // add the return slot...
    size_t retslot = stab_add_var(acx->st, strdup(s->name), stab_resolve_type(acx->st, strdup("<retslot>"), s->head->func.retty), NULL, &curr_var_offset, true);

    // analyze each subprogram, taking care that it is in its own scope...
    LFOREACH(struct ast_subdecl *d, s->subprogs)
        stab_add_func(acx->st, strdup(d->name), d->head);
        analyze_subprog(acx, d);
    ENDLFOREACH;

    fprintf(acx->ofd, "sub rsp, %d\n", curr_var_offset - ABI_POINTER_SIZE); // don't reserve stack space for the retp, it's already accounted for!

    // Go over all our locals, and if they are captured:
    // 1. Save a copy of the old access link for that local
    // 2. Add the pointer to our version of the local to the display for that local.
    struct ptrvec *captured = ptrvec_wcap(1, dummy_free);
    HFOREACH(ent, ((struct stab_scope *)list_last(acx->st->chain))->vars)
        struct stab_var *v = STAB_VAR(acx->st, (size_t)ent->val);
        if (v->captured) {
            ptrvec_push(captured, v);
            fprintf(acx->ofd, "push [display@ + %d]", v->disp_offset * ABI_POINTER_SIZE);
            fprintf(acx->ofd, "mov [display@ + %d], rbp+%d\n", v->disp_offset * ABI_POINTER_SIZE, v->stack_base_offset);
        }
    ENDHFOREACH;

    // now analyze the subprogram body.
    analyze_stmt(acx, s->body);

    if (!acx->current_func->ty.func.ret_assigned && acx->current_func->ty.func.type == SUB_FUNCTION) {
        span_err("return value of %s not assigned", NULL, acx->current_func->name);
    }

    fprintf(acx->ofd, "push rax\n");

    // restore the display
    for (int i = captured->length - 1; i > 0; i--) {
        struct stab_var *v = captured->data[i];
        fprintf(acx->ofd, "pop rax\nmov [display@ + %d], rax\n", v->disp_offset * ABI_POINTER_SIZE);
    }
    ptrvec_free(captured);

    if (acx->current_func->ty.func.type == SUB_FUNCTION) {
        // HACK: copy retval to outslot
        fprintf(acx->ofd, "mov rax, [rbp + %d]\nmov [rbp-8], rax", STAB_VAR(acx->st, retslot)->stack_base_offset);
        fprintf(acx->ofd, "pop rax\nret\n");
    }

    // leave the new scope
    stab_leave(acx->st);

    acx->current_func_name = old_func_name;
    acx->label = old_label;
    acx->ret_assigned = old_ret_assigned;
    acx->current_func_type = old_cft;
}

struct acx analyze(struct ast_program *prog, int output_to) {
    struct acx acx_;
    acx_.disp_offset = 0;
    acx_.st = stab_new();
    acx_.ofd = output_to;
    acx_.rs = {.overflow = 0, .regs_used = ~0};
    acx_.toplevel = false;
    acx_.current_func_name = "@~unassignable~@";
    acx_.ret_assigned = false;
    acx_.current_func_type = SUB_PROCEDURE;
    acx_.label = 0;
    struct acx *acx = &acx_;

    int curr_var_offset = 0; // no ret pointer to skip
    stab_enter(acx->st);

    // setup the global scope: import any names from libraries...
    do_imports(acx, prog);

    // add the global types...
    LFOREACH(struct ast_type_decl *t, prog->types)
        stab_add_type(acx->st, t->name, t->type);
    ENDLFOREACH;

    // add the global variables...
    LFOREACH(struct ast_decls *d, prog->decls)
        stab_add_decls(acx->st, d, &curr_var_offset, true);
    ENDLFOREACH;

    // analyze each subprogram, taking care that it is in its own scope...
    // note that these all become globals
    LFOREACH(struct ast_subdecl *d, prog->subprogs)
        stab_add_func(acx->st, strdup(d->name), d->head);
        analyze_subprog(acx, d);
    ENDLFOREACH;

    fprintf(acx->ofd, "sub rsp, %d\n", curr_var_offset);

    HFOREACH(ent, ((struct stab_scope *)list_last(acx->st->chain))->vars)
        struct stab_var *v = STAB_VAR(acx->st, (size_t)ent->val);
        if (v->captured) {
            fprintf(acx->ofd, "mov [display@ + %d], rbp+%d\n", v->disp_offset * ABI_POINTER_SIZE, v->stack_base_offset);
        }
    ENDHFOREACH;

    acx_.toplevel = true;

    // now analyze the program body.
    analyze_stmt(acx, prog->body);

    acx->main = t.cfunc;

    // and we're done!
    return acx_;
}
